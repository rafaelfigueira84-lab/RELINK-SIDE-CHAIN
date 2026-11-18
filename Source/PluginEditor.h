#pragma once
#include "PluginProcessor.h"

class RelinkSmartSidechainAudioProcessorEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit RelinkSmartSidechainAudioProcessorEditor(RelinkSmartSidechainAudioProcessor&);
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override { repaint(); }
    RelinkSmartSidechainAudioProcessor& processor;
    juce::Slider amount, depth, release, offset, threshold, frequency;
    juce::ToggleButton autoTiming { "AUTO TIMING" };
    juce::ToggleButton autoDepth { "AUTO DEPTH" }, autoFrequency { "AUTO FREQ" };
    juce::TextButton analyzeButton { "ANALYZE KICK + BASS (4s)" };
    juce::ComboBox mode;
    juce::Label amountLabel, depthLabel, releaseLabel, offsetLabel, thresholdLabel, frequencyLabel, modeLabel;
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ChoiceAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<SliderAttachment> amountA, depthA, releaseA, offsetA, thresholdA, frequencyA;
    std::unique_ptr<ButtonAttachment> autoA, autoDepthA, autoFrequencyA;
    std::unique_ptr<ChoiceAttachment> modeA;
    void setupSlider(juce::Slider&, juce::Label&, const juce::String&);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RelinkSmartSidechainAudioProcessorEditor)
};
