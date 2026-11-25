#include "PluginProcessor.h"

StuddahProcessor::StuddahProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "StuddahState", createLayout())
{
    inputGain = parameters.getRawParameterValue("inputGain");
    outputGain = parameters.getRawParameterValue("outputGain");
    mix = parameters.getRawParameterValue("mix");
}

juce::AudioProcessorValueTreeState::ParameterLayout StuddahProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"inputGain", 1}, "Input Gain",
        juce::NormalisableRange<float>{-24.0f, 12.0f, 0.1f}, 0.0f, "dB"));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"outputGain", 1}, "Output Gain",
        juce::NormalisableRange<float>{-24.0f, 12.0f, 0.1f}, 0.0f, "dB"));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"mix", 1}, "Mix",
        juce::NormalisableRange<float>{0.0f, 100.0f, 0.1f}, 100.0f, "%"));
    return layout;
}

bool StuddahProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo())
        && layouts.getMainInputChannelSet() == out;
}

void StuddahProcessor::prepareToPlay(double sampleRate, int)
{
    gainStage.prepare(sampleRate, inputGain->load(), outputGain->load(), mix->load() * 0.01f);
}

void StuddahProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear(ch, 0, buffer.getNumSamples());
    gainStage.process(buffer, inputGain->load(), outputGain->load(), mix->load() * 0.01f);
}

juce::AudioProcessorEditor* StuddahProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor(*this);
}

void StuddahProcessor::getStateInformation(juce::MemoryBlock& destination)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary(*xml, destination);
}

void StuddahProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new StuddahProcessor();
}
