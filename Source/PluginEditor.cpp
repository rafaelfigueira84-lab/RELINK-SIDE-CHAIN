#include "PluginEditor.h"
#include <cmath>

namespace {
constexpr auto bg = 0xff0d1720;
constexpr auto cyan = 0xff38dcc6;
constexpr auto amber = 0xffffb568;
constexpr auto muted = 0xff8fa6b7;
float raw(juce::AudioProcessorValueTreeState& s, const char* id) { return s.getRawParameterValue(id)->load(); }
}

void RelinkSmartSidechainAudioProcessorEditor::EnvelopeGraph::writeParameter(const char* id, float value)
{
    if (auto* param = processor.parameters.getParameter(id))
        param->setValueNotifyingHost(param->convertTo0to1(value));
}

void RelinkSmartSidechainAudioProcessorEditor::EnvelopeGraph::paint(juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced(2.0f);
    g.setColour(juce::Colour(0xff142633)); g.fillRoundedRectangle(r, 10.0f);
    auto plot = r.reduced(20.0f, 30.0f);
    const float x0 = plot.getX(), y0 = plot.getY(), w = plot.getWidth(), h = plot.getHeight();
    g.setColour(juce::Colour(0xff263d4b));
    for (int k = 0; k <= 8; ++k) g.drawVerticalLine(int(x0 + w * k / 8), y0, y0+h);
    for (int k = 0; k <= 4; ++k) g.drawHorizontalLine(int(y0 + h * k / 4), x0, x0+w);
    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    g.setColour(juce::Colours::white.withAlpha(0.8f));
    g.drawText("KICK / BASS INPUT", int(x0), int(r.getY()+5), 200, 20, juce::Justification::centredLeft);
    g.setColour(juce::Colour(cyan)); g.drawText("KICK", int(x0+w-120), int(r.getY()+5), 50, 20, juce::Justification::centredLeft);
    g.setColour(juce::Colour(amber)); g.drawText("BASS", int(x0+w-64), int(r.getY()+5), 55, 20, juce::Justification::centredLeft);
    // Waveforms are peaks taken from a real 600 ms window following a detected kick.
    juce::Path kick, bass;
    for (int k = 0; k < RelinkSmartSidechainAudioProcessor::waveformPoints; ++k)
    {
        const float x = x0 + w * k / float(RelinkSmartSidechainAudioProcessor::waveformPoints-1);
        const float kv = juce::jlimit(0.0f, 1.0f, processor.kickWave[k].load(std::memory_order_relaxed) * 3.0f);
        const float bv = juce::jlimit(0.0f, 1.0f, processor.bassWave[k].load(std::memory_order_relaxed) * 3.0f);
        const float ky = y0 + h*0.35f - kv*h*0.28f;
        const float by = y0 + h*0.68f - bv*h*0.23f;
        if (k==0) { kick.startNewSubPath(x,ky); bass.startNewSubPath(x,by); }
        else { kick.lineTo(x,ky); bass.lineTo(x,by); }
    }
    g.setColour(juce::Colour(cyan).withAlpha(0.82f)); g.strokePath(kick, juce::PathStrokeType(1.4f));
    g.setColour(juce::Colour(amber).withAlpha(0.82f)); g.strokePath(bass, juce::PathStrokeType(1.3f));
    // The graph is normalized to the effective ducking duration, not to an arbitrary quarter note.
    const float duration = raw(processor.parameters, "auto") > 0.5f
        ? processor.learnedDurationMs.load() : raw(processor.parameters, "release");
    const float attack = juce::jlimit(0.001f, 0.7f, raw(processor.parameters,"attack") / juce::jmax(1.0f,duration));
    const float holdEnd = juce::jmin(0.9f, attack + raw(processor.parameters,"hold")*0.01f);
    const float shape = raw(processor.parameters,"shape");
    const float offset = raw(processor.parameters,"offset")/juce::jmax(1.0f, duration);
    const float top = y0 + 16.0f, bottom = y0+h-12.0f;
    juce::Path envelope;
    for (int k=0;k<=256;++k)
    {
        const float t = float(k)/256.0f;
        const float u = t - offset;
        float reduction = 0.0f;
        if (u >= 0.0f && u <= 1.0f)
        {
            if (u < attack)
            { const float a = juce::jlimit(0.0f,1.0f,u/attack); reduction = a*a*(3.0f-2.0f*a); }
            else if (u < holdEnd) reduction = 1.0f;
            else
            { const float v=juce::jlimit(0.0f,1.0f,(u-holdEnd)/juce::jmax(0.0001f,1.0f-holdEnd));
              const float z=std::pow(v,shape); reduction=1.0f-z*z*(3.0f-2.0f*z); }
        }
        float x=x0+w*t, y=top+(bottom-top)*reduction;
        if(k==0) envelope.startNewSubPath(x,y); else envelope.lineTo(x,y);
    }
    g.setColour(juce::Colours::black.withAlpha(0.36f)); g.strokePath(envelope, juce::PathStrokeType(6.0f));
    g.setColour(juce::Colour(0xffffdb65)); g.strokePath(envelope, juce::PathStrokeType(3.0f));
    auto handlePoint = [&](float frac, float reduction, int idx)
    {
        const float px=x0+w*juce::jlimit(0.0f,1.0f,frac+offset);
        const float py=top+(bottom-top)*reduction;
        g.setColour(idx==handle ? juce::Colours::white : juce::Colour(0xffffdb65));
        g.fillEllipse(px-5,py-5,10,10);
    };
    handlePoint(attack,1.0f,0); handlePoint(holdEnd,1.0f,1);
    const float mid=0.5f*(holdEnd+1.0f);
    const float z=std::pow(0.5f,shape);
    handlePoint(mid,1.0f-z*z*(3.0f-2.0f*z),2);
    g.setColour(juce::Colour(muted));
    g.drawText("0 ms", int(x0), int(y0+h+5), 65, 17, juce::Justification::centredLeft);
    g.drawText(juce::String(duration,0)+" ms = duck end", int(x0+w-172), int(y0+h+5), 172, 17, juce::Justification::centredRight);
    g.drawText("Drag yellow points: attack / hold / release shape", int(x0+190), int(r.getY()+5), 340, 20, juce::Justification::centredLeft);
}

void RelinkSmartSidechainAudioProcessorEditor::EnvelopeGraph::mouseDown(const juce::MouseEvent& e)
{
    const auto plot=getLocalBounds().toFloat().reduced(2.0f).reduced(20.0f,30.0f);
    const float x=juce::jlimit(0.0f,1.0f,(e.position.x-plot.getX())/plot.getWidth());
    const float duration=raw(processor.parameters,"auto")>0.5f ? processor.learnedDurationMs.load() : raw(processor.parameters,"release");
    const float a=raw(processor.parameters,"attack")/juce::jmax(1.0f,duration);
    const float h=a+raw(processor.parameters,"hold")*0.01f;
    const float mid=(h+1.0f)*0.5f;
    handle = std::abs(x-a)<0.05f ? 0 : std::abs(x-h)<0.05f ? 1 : std::abs(x-mid)<0.1f ? 2 : -1;
    if(handle>=0)
    {
        const char* id = handle==0 ? "attack" : handle==1 ? "hold" : "shape";
        if(auto* p=processor.parameters.getParameter(id)) p->beginChangeGesture();
        updateHandle(e);
    }
}
void RelinkSmartSidechainAudioProcessorEditor::EnvelopeGraph::mouseDrag(const juce::MouseEvent& e) { if(handle>=0) updateHandle(e); }
void RelinkSmartSidechainAudioProcessorEditor::EnvelopeGraph::mouseUp(const juce::MouseEvent&)
{
    if(handle>=0)
    {
        const char* id=handle==0 ? "attack" : handle==1 ? "hold" : "shape";
        if(auto* p=processor.parameters.getParameter(id)) p->endChangeGesture();
    }
    handle=-1;
}
void RelinkSmartSidechainAudioProcessorEditor::EnvelopeGraph::updateHandle(const juce::MouseEvent& e)
{
    const auto plot=getLocalBounds().toFloat().reduced(2.0f).reduced(20.0f,30.0f);
    const float x=juce::jlimit(0.0f,1.0f,(e.position.x-plot.getX())/plot.getWidth());
    const float y=juce::jlimit(0.0f,1.0f,(e.position.y-(plot.getY()+16))/(plot.getHeight()-28));
    const float duration=raw(processor.parameters,"auto")>0.5f ? processor.learnedDurationMs.load() : raw(processor.parameters,"release");
    const float a=raw(processor.parameters,"attack")/juce::jmax(1.0f,duration);
    if(handle==0) writeParameter("attack",juce::jlimit(0.5f,40.0f,x*duration));
    if(handle==1) writeParameter("hold",juce::jlimit(0.0f,75.0f,(x-a)*100.0f));
    if(handle==2)
    {
        // At the recovery midpoint, a vertically dragged node maps to curvature.
        const float v=juce::jlimit(0.001f,0.999f,y);
        // Smoothstep inverse (approximately): numerical bisection for z in [0,1].
        float lo=0.0f, hi=1.0f;
        for(int k=0;k<18;++k) {const float m=(lo+hi)*0.5f; if(m*m*(3-2*m)<1.0f-v) lo=m; else hi=m;}
        const float z=(lo+hi)*0.5f;
        writeParameter("shape",juce::jlimit(0.3f,3.0f,std::log(z)/std::log(0.5f)));
    }
    repaint();
}

RelinkSmartSidechainAudioProcessorEditor::RelinkSmartSidechainAudioProcessorEditor(RelinkSmartSidechainAudioProcessor& p)
    : AudioProcessorEditor(p), processor(p), graph(p)
{
    setupSlider(amount,amountLabel,"DUCKING AMOUNT (%)");
    setupSlider(depth,depthLabel,"MAX REDUCTION (dB)");
    setupSlider(release,releaseLabel,"MANUAL DURATION (ms)");
    setupSlider(offset,offsetLabel,"TIMING OFFSET (ms)");
    setupSlider(threshold,thresholdLabel,"KICK DETECTION (dB)");
    setupSlider(frequency,frequencyLabel,"SPECTRAL CUTOFF (Hz)");
    setupSlider(attack,attackLabel,"CURVE ATTACK (ms)");
    setupSlider(hold,holdLabel,"CURVE HOLD (%)");
    setupSlider(shape,shapeLabel,"RECOVERY SHAPE");
    amount.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    amount.setTextBoxStyle(juce::Slider::TextBoxBelow,false,94,22);
    for(auto* b : {&autoTiming,&autoDepth,&autoFrequency}) addAndMakeVisible(*b);
    addAndMakeVisible(mode); addAndMakeVisible(analyzeButton); addAndMakeVisible(graph);
    analyzeButton.onClick=[this]{processor.requestAnalysis.store(true);};
    analyzeButton.setColour(juce::TextButton::buttonColourId,juce::Colour(0xff198a80));
    addAndMakeVisible(modeLabel);
    modeLabel.setText("DUCKING MODE",juce::dontSendNotification);
    modeLabel.setColour(juce::Label::textColourId,juce::Colour(muted));
    mode.addItem("Volume",1); mode.addItem("Spectral",2);
    amountA=std::make_unique<SliderAttachment>(p.parameters,"amount",amount);
    depthA=std::make_unique<SliderAttachment>(p.parameters,"depth",depth);
    releaseA=std::make_unique<SliderAttachment>(p.parameters,"release",release);
    offsetA=std::make_unique<SliderAttachment>(p.parameters,"offset",offset);
    thresholdA=std::make_unique<SliderAttachment>(p.parameters,"threshold",threshold);
    frequencyA=std::make_unique<SliderAttachment>(p.parameters,"frequency",frequency);
    attackA=std::make_unique<SliderAttachment>(p.parameters,"attack",attack);
    holdA=std::make_unique<SliderAttachment>(p.parameters,"hold",hold);
    shapeA=std::make_unique<SliderAttachment>(p.parameters,"shape",shape);
    autoA=std::make_unique<ButtonAttachment>(p.parameters,"auto",autoTiming);
    autoDepthA=std::make_unique<ButtonAttachment>(p.parameters,"autodepth",autoDepth);
    autoFrequencyA=std::make_unique<ButtonAttachment>(p.parameters,"autofrequency",autoFrequency);
    modeA=std::make_unique<ChoiceAttachment>(p.parameters,"mode",mode);
    setSize(940,790); startTimerHz(20);
}
void RelinkSmartSidechainAudioProcessorEditor::setupSlider(juce::Slider& s,juce::Label& l,const juce::String& name)
{
    s.setSliderStyle(juce::Slider::LinearHorizontal);
    s.setTextBoxStyle(juce::Slider::TextBoxRight,false,78,22);
    s.setColour(juce::Slider::thumbColourId,juce::Colour(cyan));
    s.setColour(juce::Slider::trackColourId,juce::Colour(cyan));
    l.setText(name,juce::dontSendNotification);
    l.setColour(juce::Label::textColourId,juce::Colour(0xffbdc9d5));
    l.setFont(juce::FontOptions(11.5f,juce::Font::bold));
    addAndMakeVisible(s); addAndMakeVisible(l);
}
void RelinkSmartSidechainAudioProcessorEditor::resized()
{
    graph.setBounds(22,76,getWidth()-44,292);
    amountLabel.setBounds(35,391,190,20); amount.setBounds(45,413,160,165);
    autoTiming.setBounds(38,579,160,25); autoDepth.setBounds(38,608,160,25);
    autoFrequency.setBounds(38,637,160,25);
    modeLabel.setBounds(230,628,160,20);mode.setBounds(230,652,150,28);
    analyzeButton.setBounds(35,696,280,36);
    auto place=[&](juce::Label& l,juce::Slider& s,int x,int y){l.setBounds(x,y,320,18);s.setBounds(x,y+19,320,26);};
    place(depthLabel,depth,225,391);place(releaseLabel,release,225,460);
    place(offsetLabel,offset,225,529);place(thresholdLabel,threshold,225,598);
    place(frequencyLabel,frequency,585,391);place(attackLabel,attack,585,460);
    place(holdLabel,hold,585,529);place(shapeLabel,shape,585,598);
}
void RelinkSmartSidechainAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(bg));
    g.setColour(juce::Colour(cyan)); g.setFont(juce::FontOptions(26.0f,juce::Font::bold));
    g.drawText("RELINK",22,14,150,36,juce::Justification::centredLeft);
    g.setColour(juce::Colours::white);g.setFont(juce::FontOptions(17.0f));
    g.drawText("SMART SIDECHAIN   v0.6  /  CURVE EDITION",153,17,550,32,juce::Justification::centredLeft);
    g.setColour(processor.sidechainPresent.load()?juce::Colour(cyan):juce::Colour(amber));
    g.setFont(juce::FontOptions(12.0f));
    g.drawText(processor.sidechainPresent.load()?"KICK DETECTED":"WAITING FOR KICK SIDECHAIN",710,25,200,19,juce::Justification::centredRight);
    g.setColour(juce::Colour(0xff9eafbd));
    const juce::String status=processor.analysisRunning.load()?"ANALYZING " + juce::String(int(processor.analysisProgress.load()*100.0f))+"%"
                      :processor.analysisReady.load()?"ANALYSIS READY":"PRESS ANALYZE";
    g.drawText(status,35,744,200,21,juce::Justification::centredLeft);
    g.drawText("TIME " + juce::String(processor.learnedDurationMs.load(),0)+" ms",240,744,150,21,juce::Justification::centredLeft);
    g.drawText("SUGGESTED " + juce::String(processor.recommendedDepthDb.load(),1)+" dB",420,744,195,21,juce::Justification::centredLeft);
    g.drawText("APPLIED " + juce::String(processor.appliedDepthDb.load(),1)+" dB",630,744,190,21,juce::Justification::centredLeft);
}
juce::AudioProcessorEditor* RelinkSmartSidechainAudioProcessor::createEditor()
{
    return new RelinkSmartSidechainAudioProcessorEditor(*this);
}
