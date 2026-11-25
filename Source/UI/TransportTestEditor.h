#pragma once
#include "../PluginProcessor.h"

class TransportTestEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit TransportTestEditor(StuddahProcessor& owner)
        : AudioProcessorEditor(owner), processor(owner), generic(owner)
    {
        addAndMakeVisible(status);
        addAndMakeVisible(generic);
        status.setJustificationType(juce::Justification::centredLeft);
        setSize(560, generic.getHeight() + 40);
        timerCallback();
        startTimerHz(10);
    }
    ~TransportTestEditor() override { stopTimer(); }
    void resized() override
    {
        auto area = getLocalBounds();
        status.setBounds(area.removeFromTop(40).reduced(10, 0));
        generic.setBounds(area);
    }
private:
    void timerCallback() override
    {
        const int flags = processor.displayedTransportFlags();
        const auto tempoSource = (flags & 1) ? "Host BPM" : "Fallback BPM";
        const auto phaseSource = (flags & 2) ? "Host phase" : "Internal phase";
        status.setText(juce::String(tempoSource) + ": "
            + juce::String(processor.displayedBpm(), 1) + "  |  " + phaseSource
            + "  |  " + ((flags & 4) ? "Playing" : "Stopped"), juce::dontSendNotification);
    }
    StuddahProcessor& processor;
    juce::GenericAudioProcessorEditor generic;
    juce::Label status;
};
