#include "PluginEditor.h"

RelinkSmartSidechainAudioProcessorEditor::RelinkSmartSidechainAudioProcessorEditor(RelinkSmartSidechainAudioProcessor& p)
    : AudioProcessorEditor(p), processor(p)
{
    setupSlider(amount, amountLabel, "DUCKING AMOUNT (%)");
    setupSlider(depth, depthLabel, "MAX REDUCTION (dB)");
    setupSlider(release, releaseLabel, "MANUAL DURATION (ms)");
    setupSlider(offset, offsetLabel, "TIMING OFFSET (ms)");
    setupSlider(threshold, thresholdLabel, "KICK DETECTION (dB)");
    setupSlider(frequency, frequencyLabel, "SPECTRAL CUTOFF (Hz)");
    amount.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    amount.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 100, 22);
    addAndMakeVisible(autoTiming);
    addAndMakeVisible(autoDepth);
    addAndMakeVisible(autoFrequency);
    addAndMakeVisible(mode);
    addAndMakeVisible(analyzeButton);
    analyzeButton.onClick = [this] { processor.requestAnalysis.store(true); };
    analyzeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff198a80));
    addAndMakeVisible(modeLabel);
    modeLabel.setText("DUCKING MODE", juce::dontSendNotification);
    modeLabel.setColour(juce::Label::textColourId, juce::Colour(0xffbdc9d5));
    mode.addItem("Volume", 1);
    mode.addItem("Spectral", 2);
    autoTiming.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xff34dec9));
    amountA = std::make_unique<SliderAttachment>(p.parameters, "amount", amount);
    depthA = std::make_unique<SliderAttachment>(p.parameters, "depth", depth);
    releaseA = std::make_unique<SliderAttachment>(p.parameters, "release", release);
    offsetA = std::make_unique<SliderAttachment>(p.parameters, "offset", offset);
    thresholdA = std::make_unique<SliderAttachment>(p.parameters, "threshold", threshold);
    frequencyA = std::make_unique<SliderAttachment>(p.parameters, "frequency", frequency);
    autoA = std::make_unique<ButtonAttachment>(p.parameters, "auto", autoTiming);
    autoDepthA = std::make_unique<ButtonAttachment>(p.parameters, "autodepth", autoDepth);
    autoFrequencyA = std::make_unique<ButtonAttachment>(p.parameters, "autofrequency", autoFrequency);
    modeA = std::make_unique<ChoiceAttachment>(p.parameters, "mode", mode);
    setSize(690, 550);
    startTimerHz(20);
}

void RelinkSmartSidechainAudioProcessorEditor::setupSlider(juce::Slider& s, juce::Label& l, const juce::String& name)
{
    s.setSliderStyle(juce::Slider::LinearHorizontal);
    s.setTextBoxStyle(juce::Slider::TextBoxRight, false, 75, 22);
    s.setColour(juce::Slider::thumbColourId, juce::Colour(0xff34dec9));
    s.setColour(juce::Slider::trackColourId, juce::Colour(0xff34dec9));
    l.setText(name, juce::dontSendNotification);
    l.setColour(juce::Label::textColourId, juce::Colour(0xffbdc9d5));
    l.setFont(juce::FontOptions(11.5f, juce::Font::bold));
    addAndMakeVisible(s);
    addAndMakeVisible(l);
}

void RelinkSmartSidechainAudioProcessorEditor::resized()
{
    amountLabel.setBounds(22, 69, 195, 20);
    amount.setBounds(40, 95, 160, 190);
    autoTiming.setBounds(35, 289, 155, 28);
    autoDepth.setBounds(35, 318, 155, 28);
    autoFrequency.setBounds(35, 347, 155, 28);
    modeLabel.setBounds(33, 379, 170, 22);
    mode.setBounds(33, 404, 165, 27);
    analyzeButton.setBounds(35, 450, 250, 36);
    auto place = [&](juce::Label& l, juce::Slider& s, int y) {
        l.setBounds(248, y, 350, 19);
        s.setBounds(245, y + 18, 370, 27);
    };
    place(depthLabel, depth, 80);
    place(releaseLabel, release, 143);
    place(offsetLabel, offset, 206);
    place(thresholdLabel, threshold, 269);
    place(frequencyLabel, frequency, 332);
}

void RelinkSmartSidechainAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff101820));
    g.setColour(juce::Colour(0xff34dec9));
    g.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    g.drawText("RELINK", 22, 16, 150, 35, juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xffd8e3eb));
    g.setFont(juce::FontOptions(16.0f));
    g.drawText("SMART SIDECHAIN   v0.3", 145, 18, 300, 31, juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xff253540));
    g.drawLine(228, 75, 228, 435, 1.5f);
    g.setColour(processor.sidechainPresent.load() ? juce::Colour(0xff34dec9) : juce::Colour(0xfff2aa5c));
    g.setFont(juce::FontOptions(12.0f));
    g.drawText(processor.sidechainPresent.load() ? "KICK DETECTED" : "WAITING FOR SIDECHAIN KICK", 310, 455, 340, 22, juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xffafbac4));
    const auto status = processor.analysisRunning.load() ? "ANALYZING: " + juce::String(int(processor.analysisProgress.load() * 100)) + "%"
                     : processor.analysisReady.load() ? "ANALYSIS READY" : "PRESS ANALYZE TO CALIBRATE";
    g.drawText(status, 25, 510, 220, 21, juce::Justification::centredLeft);
    g.drawText("Time: " + juce::String(processor.learnedDurationMs.load(), 0) + " ms", 244, 510, 112, 21, juce::Justification::centredLeft);
    g.drawText("Suggested: " + juce::String(processor.analysisReady.load() ? processor.recommendedDepthDb.load() : processor.autoDepthDb.load(), 1) + " dB", 360, 510, 145, 21, juce::Justification::centredLeft);
    g.drawText("Applied: " + juce::String(processor.appliedDepthDb.load(), 1) + " dB", 509, 510, 125, 21, juce::Justification::centredLeft);
}

juce::AudioProcessorEditor* RelinkSmartSidechainAudioProcessor::createEditor()
{
    return new RelinkSmartSidechainAudioProcessorEditor(*this);
}
