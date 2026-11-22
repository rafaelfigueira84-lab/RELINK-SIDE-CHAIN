#pragma once
#include "PluginProcessor.h"

class RelinkSmartSidechainAudioProcessorEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit RelinkSmartSidechainAudioProcessorEditor(RelinkSmartSidechainAudioProcessor&);
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    class EnvelopeGraph final : public juce::Component
    {
    public:
        explicit EnvelopeGraph(RelinkSmartSidechainAudioProcessor& p) : processor(p) {}
        void paint(juce::Graphics&) override;
        void mouseDown(const juce::MouseEvent&) override;
        void mouseDrag(const juce::MouseEvent&) override;
        void mouseUp(const juce::MouseEvent&) override;
    private:
        RelinkSmartSidechainAudioProcessor& processor;
        int handle = -1;
        void updateHandle(const juce::MouseEvent&);
        void writeParameter(const char* id, float value);
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvelopeGraph)
    };
    void timerCallback() override { graph.repaint(); repaint(); }
    RelinkSmartSidechainAudioProcessor& processor;
    EnvelopeGraph graph;
    juce::Slider amount, depth, release, offset, threshold, frequency, attack, hold, shape;
    juce::ToggleButton autoTiming { "AUTO TIMING" }, autoDepth { "AUTO DEPTH" }, autoFrequency { "AUTO FREQ" };
    juce::TextButton analyzeButton { "ANALYZE KICK + BASS (4s)" };
    juce::ComboBox mode;
    juce::Label amountLabel, depthLabel, releaseLabel, offsetLabel, thresholdLabel, frequencyLabel;
    juce::Label attackLabel, holdLabel, shapeLabel, modeLabel;
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ChoiceAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<SliderAttachment> amountA, depthA, releaseA, offsetA, thresholdA, frequencyA;
    std::unique_ptr<SliderAttachment> attackA, holdA, shapeA;
    std::unique_ptr<ButtonAttachment> autoA, autoDepthA, autoFrequencyA;
    std::unique_ptr<ChoiceAttachment> modeA;
    void setupSlider(juce::Slider&, juce::Label&, const juce::String&);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RelinkSmartSidechainAudioProcessorEditor)
};
