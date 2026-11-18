#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

RelinkSmartSidechainAudioProcessor::RelinkSmartSidechainAudioProcessor()
    : AudioProcessor(BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)
          .withInput("Kick Sidechain", juce::AudioChannelSet::stereo(), false)),
      parameters(*this, nullptr, "SMART_SIDECHAIN", makeParameters()) {}

juce::AudioProcessorValueTreeState::ParameterLayout RelinkSmartSidechainAudioProcessor::makeParameters()
{
    using P = juce::AudioParameterFloat;
    using B = juce::AudioParameterBool;
    using C = juce::AudioParameterChoice;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    p.push_back(std::make_unique<P>(juce::ParameterID{"amount", 1}, "Amount (%)", juce::NormalisableRange<float>(0, 100, 1), 65));
    p.push_back(std::make_unique<P>(juce::ParameterID{"depth", 1}, "Maximum Reduction (dB)", juce::NormalisableRange<float>(0, 24, 0.5f), 12));
    p.push_back(std::make_unique<P>(juce::ParameterID{"release", 1}, "Manual Duration (ms)", juce::NormalisableRange<float>(40, 700, 1), 180));
    p.push_back(std::make_unique<P>(juce::ParameterID{"offset", 1}, "Timing Offset (ms)", juce::NormalisableRange<float>(0, 30, 0.5f), 0));
    p.push_back(std::make_unique<P>(juce::ParameterID{"threshold", 1}, "Kick Detection (dB)", juce::NormalisableRange<float>(-48, -6, 1), -28));
    p.push_back(std::make_unique<B>(juce::ParameterID{"auto", 1}, "Auto Timing", true));
    p.push_back(std::make_unique<B>(juce::ParameterID{"autodepth", 1}, "Auto Depth", true));
    p.push_back(std::make_unique<C>(juce::ParameterID{"mode", 1}, "Ducking Mode", juce::StringArray { "Volume", "Spectral" }, 0));
    p.push_back(std::make_unique<P>(juce::ParameterID{"frequency", 1}, "Spectral Cutoff (Hz)", juce::NormalisableRange<float>(50, 300, 1), 130));
    p.push_back(std::make_unique<B>(juce::ParameterID{"autofrequency", 1}, "Auto Frequency", true));
    return { p.begin(), p.end() };
}

bool RelinkSmartSidechainAudioProcessor::isBusesLayoutSupported(const BusesLayout& l) const
{
    if (l.getMainInputChannelSet() != juce::AudioChannelSet::stereo()
        || l.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()) return false;
    const auto sc = l.inputBuses.size() > 1 ? l.getChannelSet(true, 1) : juce::AudioChannelSet::disabled();
    return sc.isDisabled() || sc == juce::AudioChannelSet::mono() || sc == juce::AudioChannelSet::stereo();
}

void RelinkSmartSidechainAudioProcessor::prepareToPlay(double sr, int)
{
    fs = sr;
    detectorEnvelope = previousEnvelope = 0;
    gainEnvelope = 1;
    samplesSinceTrigger = 1000000;
    observedPulseSamples = 0;
    insidePulse = false;
    durationEstimateMs = 180;
    lowEndEnvelope = lowEndPeak = 0.0f;
    estimatedBeatMs = 485.0f;
    measuringLowEnd = false;
    lowEndSamples = 0;
    adaptiveDepthDb = 6.0f;
    bassLowEnergy = kickLowEnergy = kickHighEnergy = 0.0f;
    kickLowState = kickMidState = 0.0f;
    bassLowState[0] = bassLowState[1] = 0.0f;
    bassAnalysisLowState = 0.0f;
    analysisSamples = analysisSamplesWithKick = 0;
    analysisKickSum = analysisBassSum = 0.0;
    analysisRunning.store(false);
    analysisReady.store(false);
    analysisProgress.store(0);
}

void RelinkSmartSidechainAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const auto n = buffer.getNumSamples();
    auto mainInput = getBusBuffer(buffer, true, 0);
    const bool hasSC = getBusCount(true) > 1 && getBus(true, 1)->isEnabled();
    juce::AudioBuffer<float> scBuffer;
    if (hasSC) scBuffer = getBusBuffer(buffer, true, 1);
    const auto amount = parameters.getRawParameterValue("amount")->load() * 0.01f;
    const auto maxDepth = parameters.getRawParameterValue("depth")->load();
    const auto thresholdDb = parameters.getRawParameterValue("threshold")->load();
    const auto offsetMs = parameters.getRawParameterValue("offset")->load();
    const auto autoMode = parameters.getRawParameterValue("auto")->load() > 0.5f;
    const auto spectralMode = parameters.getRawParameterValue("mode")->load() > 0.5f;
    const auto autoDepth = parameters.getRawParameterValue("autodepth")->load() > 0.5f;
    const auto autoFrequency = parameters.getRawParameterValue("autofrequency")->load() > 0.5f;
    const float fixedFrequency = parameters.getRawParameterValue("frequency")->load();
    const float threshold = juce::Decibels::decibelsToGain(thresholdDb);
    const float detAttack = std::exp(-1.0f / (float(fs) * 0.0015f));
    const float detRelease = std::exp(-1.0f / (float(fs) * 0.022f));
    const float rmsWeight = 1.0f - std::exp(-1.0f / (float(fs) * 0.075f));
    const float low80Weight = 1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi * 80.0f / float(fs));
    const float low220Weight = 1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi * 220.0f / float(fs));
    const float bassAnalysisWeight = 1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi * 130.0f / float(fs));
    const float lowAttack = std::exp(-1.0f / (float(fs) * 0.004f));
    const float lowRelease = std::exp(-1.0f / (float(fs) * 0.016f));
    if (requestAnalysis.exchange(false))
    {
        analysisSamples = analysisSamplesWithKick = 0;
        analysisKickSum = analysisBassSum = 0.0;
        analysisReady.store(false);
        analysisRunning.store(true);
        analysisProgress.store(0);
    }
    float peakKick = 0;
    for (int i = 0; i < n; ++i)
    {
        float kick = 0;
        if (hasSC && scBuffer.getNumChannels() > 0)
            for (int ch = 0; ch < scBuffer.getNumChannels(); ++ch)
                kick = juce::jmax(kick, std::abs(scBuffer.getSample(ch, i)));
        peakKick = juce::jmax(peakKick, kick);
        // Slow amplitude/spectrum statistics; the bass is measured before gain reduction.
        const float kickSignal = hasSC && scBuffer.getNumChannels() > 0 ? scBuffer.getSample(0, i) : 0.0f;
        kickLowState += low80Weight * (kickSignal - kickLowState);
        const float lowMagnitude = std::abs(kickLowState);
        lowEndEnvelope = lowMagnitude + (lowEndEnvelope - lowMagnitude) *
            (lowMagnitude > lowEndEnvelope ? lowAttack : lowRelease);
        kickMidState += low220Weight * (kickSignal - kickMidState);
        kickLowEnergy += rmsWeight * (kickLowState * kickLowState - kickLowEnergy);
        const float upperLow = kickMidState - kickLowState;
        kickHighEnergy += rmsWeight * (upperLow * upperLow - kickHighEnergy);
        const float bassInput = 0.5f * (mainInput.getSample(0, i) + mainInput.getSample(1, i));
        bassAnalysisLowState += bassAnalysisWeight * (bassInput - bassAnalysisLowState);
        bassLowEnergy += rmsWeight * (bassAnalysisLowState * bassAnalysisLowState - bassLowEnergy);
        detectorEnvelope = kick + (detectorEnvelope - kick) * (kick > detectorEnvelope ? detAttack : detRelease);
        const bool above = detectorEnvelope > threshold;
        const int refractory = int(fs * 0.090);
        if (above && !insidePulse && samplesSinceTrigger > refractory)
        {
            const float beat = samplesSinceTrigger * 1000.0f / float(fs);
            if (beat >= 220.0f && beat <= 1100.0f)
                estimatedBeatMs = 0.75f * estimatedBeatMs + 0.25f * beat;
            samplesSinceTrigger = 0;
            observedPulseSamples = 0;
            insidePulse = true;
            lowEndPeak = juce::jmax(lowEndEnvelope, 0.00001f);
            lowEndSamples = 0;
            measuringLowEnd = true;
        }
        if (measuringLowEnd)
        {
            ++lowEndSamples;
            lowEndPeak = juce::jmax(lowEndPeak, lowEndEnvelope);
            const float elapsedMs = lowEndSamples * 1000.0f / float(fs);
            // The relevant bass/kick overlap usually ends before the absolute
            // silence threshold used by the original envelope detector.
            if ((elapsedMs > 45.0f && lowEndEnvelope < lowEndPeak * 0.30f)
                || elapsedMs > 450.0f)
            {
                const float rhythmicCeiling = juce::jlimit(100.0f, 320.0f, 0.44f * estimatedBeatMs);
                const float observed = juce::jlimit(65.0f, rhythmicCeiling, elapsedMs * 0.85f + 12.0f);
                durationEstimateMs = 0.70f * durationEstimateMs + 0.30f * observed;
                measuringLowEnd = false;
            }
        }
        if (insidePulse)
        {
            ++observedPulseSamples;
            if (!above || observedPulseSamples > int(fs * 0.45))
            {
                insidePulse = false;
            }
        }
        ++samplesSinceTrigger;
        if (analysisRunning.load())
        {
            // Accumulate overlapping low-band power over four seconds of playback.
            if (hasSC && samplesSinceTrigger < int(0.22 * fs))
            {
                analysisKickSum += double(kickLowState) * double(kickLowState);
                analysisBassSum += double(bassAnalysisLowState) * double(bassAnalysisLowState);
                ++analysisSamplesWithKick;
            }
            ++analysisSamples;
            if (analysisSamples >= int(fs * 4.0))
            {
                if (analysisSamplesWithKick > int(fs * 0.08) && analysisKickSum > 1.0e-8)
                {
                    const float k = float(std::sqrt(analysisKickSum / analysisSamplesWithKick));
                    const float b = float(std::sqrt(analysisBassSum / analysisSamplesWithKick));
                    // An audible overlapping bass needs space even when its RMS is
                    // lower than the kick's. The recommendation is independent of
                    // Max Reduction and is intentionally a conservative heuristic.
                    const float presence = juce::jlimit(0.0f, 1.0f, b / 0.012f);
                    const float levelRatio = juce::jlimit(0.0f, 1.5f, b / juce::jmax(0.001f, k));
                    frozenDepthDb = b < 0.0015f ? 0.0f
                        : juce::jlimit(0.0f, 16.0f, (5.0f + 5.0f * presence + 2.0f * levelRatio) * presence);
                    frozenDurationMs = durationEstimateMs;
                    recommendedDepthDb.store(frozenDepthDb);
                    analysisReady.store(true);
                }
                analysisRunning.store(false);
            }
        }
        const float durationMs = autoMode ? (analysisReady.load() ? frozenDurationMs : durationEstimateMs)
                                          : parameters.getRawParameterValue("release")->load();
        const float t = (samplesSinceTrigger - int(offsetMs * float(fs) / 1000.0f)) / juce::jmax(1.0f, durationMs * float(fs) / 1000.0f);
        // Smoothstep recovery: the duck begins at a detected kick transient.
        const float u = juce::jlimit(0.0f, 1.0f, t);
        const float recovery = u * u * (3.0f - 2.0f * u);
        const float reduction = (samplesSinceTrigger < int(offsetMs * fs / 1000.0)) ? 0.0f : (1.0f - recovery);
        // Independent live recommendation (no longer pegged to Max Reduction).
        const float bassRms = std::sqrt(juce::jmax(0.0f, bassLowEnergy));
        const float kickRms = std::sqrt(juce::jmax(0.0f, kickLowEnergy));
        const float presence = juce::jlimit(0.0f, 1.0f, bassRms / 0.012f);
        const float ratio = juce::jlimit(0.0f, 1.5f, bassRms / juce::jmax(0.001f, kickRms));
        const float suggestedDepth = bassRms < 0.0015f || kickRms < 0.0015f ? 0.0f
            : juce::jlimit(0.0f, 16.0f, (5.0f + 5.0f * presence + 2.0f * ratio) * presence);
        adaptiveDepthDb += 0.0005f * (suggestedDepth - adaptiveDepthDb);
        const float recommended = analysisReady.load() ? frozenDepthDb : adaptiveDepthDb;
        const float depth = autoDepth ? juce::jmin(maxDepth, recommended) : maxDepth;
        const float targetGain = juce::Decibels::decibelsToGain(-depth * amount * reduction);
        const float slew = targetGain < gainEnvelope ? 0.34f : 0.08f;
        gainEnvelope += slew * (targetGain - gainEnvelope);
        const float suggestedCutoff = kickLowEnergy > kickHighEnergy * 1.8f ? 105.0f : 185.0f;
        const float cutoff = autoFrequency ? suggestedCutoff : fixedFrequency;
        const float lowWeight = 1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi * cutoff / float(fs));
        for (int ch = 0; ch < mainInput.getNumChannels(); ++ch)
        {
            const float input = mainInput.getSample(ch, i);
            bassLowState[ch] += lowWeight * (input - bassLowState[ch]);
            // Complementary low-band ducking: exact unity when no reduction is active.
            const float output = spectralMode ? input + (gainEnvelope - 1.0f) * bassLowState[ch]
                                              : input * gainEnvelope;
            mainInput.setSample(ch, i, output);
        }
    }
    visibleReductionDb.store(-juce::Decibels::gainToDecibels(juce::jmax(0.00001f, gainEnvelope)));
    learnedDurationMs.store(analysisReady.load() ? frozenDurationMs : durationEstimateMs);
    kickLevelDb.store(juce::Decibels::gainToDecibels(juce::jmax(1.0e-5f, peakKick)));
    sidechainPresent.store(hasSC && peakKick > 1.0e-5f);
    autoDepthDb.store(adaptiveDepthDb);
    if (analysisRunning.load()) analysisProgress.store(juce::jlimit(0.0f, 1.0f, float(analysisSamples / (fs * 4.0))));
    appliedDepthDb.store(parameters.getRawParameterValue("autodepth")->load() > 0.5f
        ? juce::jmin(maxDepth, analysisReady.load() ? frozenDepthDb : adaptiveDepthDb) : maxDepth);
    crossoverHz.store(parameters.getRawParameterValue("autofrequency")->load() > 0.5f
        ? (kickLowEnergy > kickHighEnergy * 1.8f ? 105.0f : 185.0f) : fixedFrequency);
}

void RelinkSmartSidechainAudioProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    if (auto xml = parameters.copyState().createXml()) copyXmlToBinary(*xml, dest);
}

void RelinkSmartSidechainAudioProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new RelinkSmartSidechainAudioProcessor(); }
