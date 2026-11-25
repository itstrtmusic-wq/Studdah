#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

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

    void process(juce::AudioBuffer<float>& buffer, float inputDb, float outputDb, float mix)
    {
        input.setTargetValue(juce::Decibels::decibelsToGain(inputDb));
        output.setTargetValue(juce::Decibels::decibelsToGain(outputDb));
        wet.setTargetValue(mix);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const auto in = input.getNextValue();
            const auto out = output.getNextValue();
            const auto blend = wet.getNextValue();
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                const auto dry = buffer.getSample(ch, i);
                buffer.setSample(ch, i, (dry + blend * (dry * in - dry)) * out);
            }
        }
    }
private:
    juce::SmoothedValue<float> input, output, wet;
};
}
