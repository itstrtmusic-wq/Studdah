#include "PluginProcessor.h"
#include "UI/TransportTestEditor.h"

StuddahProcessor::StuddahProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "StuddahState", createLayout())
{
    inputGain = parameters.getRawParameterValue("inputGain");
    outputGain = parameters.getRawParameterValue("outputGain");
    mix = parameters.getRawParameterValue("mix");
    fallbackBpm = parameters.getRawParameterValue("fallbackBpm");
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
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"fallbackBpm", 2}, "Fallback BPM",
        juce::NormalisableRange<float>{40.0f, 300.0f, 0.1f}, 120.0f, "BPM"));
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
    tempoClock.prepare(sampleRate);
    history.prepare(getTotalNumInputChannels(), juce::jmax(1, juce::roundToInt(sampleRate * 16.0)));
    gainStage.prepare(sampleRate, inputGain->load(), outputGain->load(), mix->load() * 0.01f);
}

void StuddahProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear(ch, 0, buffer.getNumSamples());
    studdah::HostTransport transport;
    if (auto* playHead = getPlayHead())
        if (auto position = playHead->getPosition())
        {
            transport.playing = position->getIsPlaying();
            if (auto value = position->getBpm()) transport.bpm = *value;
            if (auto value = position->getPpqPosition()) transport.ppq = *value;
            if (auto value = position->getTimeInSamples()) transport.samplePosition = *value;
        }
    tempoClock.beginBlock(transport, fallbackBpm->load(), buffer.getNumSamples());
    displayBpm.store(static_cast<float>(tempoClock.bpm()));
    displayFlags.store((tempoClock.hasHostTempo() ? 1 : 0)
        | (tempoClock.hasHostPosition() ? 2 : 0) | (tempoClock.isPlaying() ? 4 : 0));
    history.push(buffer.getArrayOfReadPointers(), getTotalNumInputChannels(), buffer.getNumSamples());
    gainStage.process(buffer, inputGain->load(), outputGain->load(), mix->load() * 0.01f);
}

juce::AudioProcessorEditor* StuddahProcessor::createEditor()
{
    return new TransportTestEditor(*this);
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
