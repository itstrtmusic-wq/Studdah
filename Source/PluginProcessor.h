#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP/GainStage.h"
#include "DSP/CircularAudioBuffer.h"
#include "DSP/StutterEngine.h"
#include "DSP/ReverseReverb.h"
#include "Core/Transport/TempoClock.h"
#include "Core/Transport/PatternSequencer.h"
#include "UI/WaveformHistory.h"

class StuddahProcessor final : public juce::AudioProcessor
{
public:
    StuddahProcessor();
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Studdah"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 16.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    juce::AudioProcessorValueTreeState parameters;
    float displayedBpm() const noexcept { return displayBpm.load(); }
    int displayedTransportFlags() const noexcept { return displayFlags.load(); }
    int displayedPatternStep() const noexcept { return displayPatternStep.load(); }
    studdah::WaveformHistory::Snapshot waveformSnapshot() const noexcept { return waveform.snapshot(); }
private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    studdah::GainStage gainStage;
    studdah::CircularAudioBuffer history;
    studdah::TempoClock tempoClock;
    studdah::StutterEngine stutter;
    studdah::ReverseReverb reverseReverb;
    studdah::PatternSequencer sequencer;
    studdah::WaveformHistory waveform;
    std::atomic<int> displayPatternStep{-1};
    std::atomic<float>* patternEnabled = nullptr;
    std::atomic<float>* patternRate = nullptr;
    std::atomic<float>* patternLength = nullptr;
    std::array<std::atomic<float>*, 16> patternSteps{};
    std::atomic<float>* reverbEnabled = nullptr;
    std::atomic<float>* reverbLength = nullptr;
    std::atomic<float>* reverbDecay = nullptr;
    std::atomic<float>* reverbDamping = nullptr;
    std::atomic<float>* reverbMix = nullptr;
    std::atomic<float>* reverbCurve = nullptr;
    std::atomic<float>* stutterEnabled = nullptr;
    std::atomic<float>* stutterRate = nullptr;
    std::atomic<float>* stutterModifier = nullptr;
    std::atomic<float>* stutterSlice = nullptr;
    std::atomic<float>* stutterRepeats = nullptr;
    std::atomic<float>* stutterFade = nullptr;
    std::atomic<float>* stutterDirection = nullptr;
    std::atomic<float> displayBpm{120.0f};
    std::atomic<int> displayFlags{0};
    std::atomic<float>* fallbackBpm = nullptr;
    std::atomic<float>* inputGain = nullptr;
    std::atomic<float>* outputGain = nullptr;
    std::atomic<float>* mix = nullptr;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StuddahProcessor)
};
