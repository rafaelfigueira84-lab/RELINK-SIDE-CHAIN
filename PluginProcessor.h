#pragma once
#include <JuceHeader.h>
#include <atomic>

class RelinkSmartSidechainAudioProcessor final : public juce::AudioProcessor
{
public:
    RelinkSmartSidechainAudioProcessor();
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "RELINK Smart Sidechain"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::AudioProcessorValueTreeState parameters;
    std::atomic<float> visibleReductionDb { 0.0f };
    std::atomic<float> learnedDurationMs { 180.0f };
    std::atomic<float> kickLevelDb { -90.0f };
    std::atomic<bool> sidechainPresent { false };
private:
    static juce::AudioProcessorValueTreeState::ParameterLayout makeParameters();
    double fs = 44100.0;
    float detectorEnvelope = 0.0f, previousEnvelope = 0.0f;
    float gainEnvelope = 1.0f;
    int samplesSinceTrigger = 1000000;
    int observedPulseSamples = 0;
    bool insidePulse = false;
    float durationEstimateMs = 180.0f;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RelinkSmartSidechainAudioProcessor)
};
