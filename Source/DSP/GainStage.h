#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>

namespace studdah
{
class GainStage
{
public:
    void prepare(double sampleRate, float inputDb, float outputDb, float mix)
    {
        input.reset(sampleRate, 0.02);
        output.reset(sampleRate, 0.02);
        wet.reset(sampleRate, 0.02);
        input.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(inputDb));
        output.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(outputDb));
        wet.setCurrentAndTargetValue(mix);
    }

    void beginBlock(float inputDb, float outputDb, float mix) noexcept
    {
        input.setTargetValue(juce::Decibels::decibelsToGain(inputDb));
        output.setTargetValue(juce::Decibels::decibelsToGain(outputDb));
        wet.setTargetValue(mix);
    }

    std::array<float, 2> processFrame(const std::array<float, 2>& dry,
                                      const std::array<float, 2>& effected) noexcept
    {
        const auto in = input.getNextValue();
        const auto out = output.getNextValue();
        const auto blend = wet.getNextValue();
        return { (dry[0] + blend * (effected[0] * in - dry[0])) * out,
                 (dry[1] + blend * (effected[1] * in - dry[1])) * out };
    }
private:
    juce::SmoothedValue<float> input, output, wet;
};
}
