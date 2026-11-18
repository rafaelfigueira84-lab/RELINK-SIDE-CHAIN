#include "PluginEditor.h"

RelinkSmartSidechainAudioProcessorEditor::RelinkSmartSidechainAudioProcessorEditor(RelinkSmartSidechainAudioProcessor& p)
    : AudioProcessorEditor(p), processor(p)
{
    setupSlider(amount, amountLabel, "DUCKING AMOUNT (%)");
    setupSlider(depth, depthLabel, "MAX REDUCTION (dB)");
    setupSlider(release, releaseLabel, "MANUAL DURATION (ms)");
    setupSlider(offset, offsetLabel, "TIMING OFFSET (ms)");
    setupSlider(threshold, thresholdLabel, "KICK DETECTION (dB)");
    amount.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    amount.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 100, 22);
    addAndMakeVisible(autoTiming);
    autoTiming.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xff34dec9));
    amountA = std::make_unique<SliderAttachment>(p.parameters, "amount", amount);
    depthA = std::make_unique<SliderAttachment>(p.parameters, "depth", depth);
    releaseA = std::make_unique<SliderAttachment>(p.parameters, "release", release);
    offsetA = std::make_unique<SliderAttachment>(p.parameters, "offset", offset);
    thresholdA = std::make_unique<SliderAttachment>(p.parameters, "threshold", threshold);
    autoA = std::make_unique<ButtonAttachment>(p.parameters, "auto", autoTiming);
    setSize(620, 390);
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
    autoTiming.setBounds(58, 299, 140, 30);
    auto place = [&](juce::Label& l, juce::Slider& s, int y) {
        l.setBounds(248, y, 325, 19);
        s.setBounds(245, y + 18, 335, 27);
    };
    place(depthLabel, depth, 80);
    place(releaseLabel, release, 143);
    place(offsetLabel, offset, 206);
    place(thresholdLabel, threshold, 269);
}

void RelinkSmartSidechainAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff101820));
    g.setColour(juce::Colour(0xff34dec9));
    g.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    g.drawText("RELINK", 22, 16, 150, 35, juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xffd8e3eb));
    g.setFont(juce::FontOptions(16.0f));
    g.drawText("SMART SIDECHAIN   v0.1", 145, 18, 280, 31, juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xff253540));
    g.drawLine(228, 75, 228, 330, 1.5f);
    g.setColour(processor.sidechainPresent.load() ? juce::Colour(0xff34dec9) : juce::Colour(0xfff2aa5c));
    g.setFont(juce::FontOptions(12.0f));
    g.drawText(processor.sidechainPresent.load() ? "KICK DETECTED" : "WAITING FOR SIDECHAIN KICK", 22, 344, 270, 22, juce::Justification::centredLeft);
    g.setColour(juce::Colour(0xffafbac4));
    g.drawText("Learned: " + juce::String(processor.learnedDurationMs.load(), 0) + " ms", 300, 344, 156, 22, juce::Justification::centredLeft);
    g.drawText("Gain reduction: " + juce::String(processor.visibleReductionDb.load(), 1) + " dB", 450, 344, 160, 22, juce::Justification::centredLeft);
}

juce::AudioProcessorEditor* RelinkSmartSidechainAudioProcessor::createEditor()
{
    return new RelinkSmartSidechainAudioProcessorEditor(*this);
}
