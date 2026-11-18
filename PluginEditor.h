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
    juce::Slider amount, depth, release, offset, threshold;
    juce::ToggleButton autoTiming { "AUTO TIMING" };
    juce::Label amountLabel, depthLabel, releaseLabel, offsetLabel, thresholdLabel;
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<SliderAttachment> amountA, depthA, releaseA, offsetA, thresholdA;
    std::unique_ptr<ButtonAttachment> autoA;
    void setupSlider(juce::Slider&, juce::Label&, const juce::String&);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RelinkSmartSidechainAudioProcessorEditor)
};
