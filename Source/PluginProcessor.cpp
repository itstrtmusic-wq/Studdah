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
    stutterEnabled = parameters.getRawParameterValue("stutterEnabled");
    stutterRate = parameters.getRawParameterValue("stutterRate");
    stutterModifier = parameters.getRawParameterValue("stutterModifier");
    stutterSlice = parameters.getRawParameterValue("stutterSlice");
    stutterRepeats = parameters.getRawParameterValue("stutterRepeats");
    stutterFade = parameters.getRawParameterValue("stutterFade");
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
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"stutterEnabled", 3}, "Stutter", false));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"stutterRate", 3}, "Rate",
        juce::StringArray{"1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/64", "1/128"}, 4));
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"stutterModifier", 3}, "Rhythm",
        juce::StringArray{"Straight", "Dotted", "Triplet"}, 0));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"stutterSlice", 3}, "Slice Length",
        juce::NormalisableRange<float>{10.0f, 100.0f, 1.0f}, 100.0f, "%"));
    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{"stutterRepeats", 3}, "Repeats", 1, 16, 4));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"stutterFade", 3}, "Edge Fade",
        juce::NormalisableRange<float>{1.0f, 30.0f, 0.1f}, 5.0f, "ms"));
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
    stutter.prepare(sampleRate);
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
    if (tempoClock.hasDiscontinuity())
        history.reset();

    studdah::StutterSettings settings;
    settings.enabled = stutterEnabled->load() >= 0.5f;
    settings.denominator = 1 << juce::jlimit(0, 7, juce::roundToInt(stutterRate->load()));
    settings.modifier = static_cast<studdah::NoteModifier>(juce::jlimit(0, 2, juce::roundToInt(stutterModifier->load())));
    settings.slice = stutterSlice->load() * 0.01f;
    settings.repeats = juce::roundToInt(stutterRepeats->load());
    settings.fadeMs = stutterFade->load();

    stutter.beginBlock(tempoClock, settings);
    gainStage.beginBlock(inputGain->load(), outputGain->load(), mix->load() * 0.01f);
    const int channels = juce::jmin(2, getTotalNumInputChannels());
    auto* const* data = buffer.getArrayOfWritePointers();
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        const studdah::StutterEngine::Frame dry{
            channels > 0 ? data[0][i] : 0.0f,
            channels > 1 ? data[1][i] : 0.0f};
        const auto effected = stutter.processFrame(history, dry, i);
        const float* source[]{ &dry[0], &dry[1] };
        history.push(source, channels, 1);
        const auto output = gainStage.processFrame(dry, effected);
        for (int ch = 0; ch < channels; ++ch)
            data[ch][i] = output[static_cast<std::size_t>(ch)];
    }
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
