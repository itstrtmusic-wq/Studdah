#pragma once
#include "../PluginProcessor.h"

class TransportTestEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit TransportTestEditor(StuddahProcessor& owner)
        : AudioProcessorEditor(owner), processor(owner)
    {
        addAndMakeVisible(status);
        status.setJustificationType(juce::Justification::centredLeft);
        addToggle("patternEnabled", "Sequencer");
        addChoice("patternRate", "Step Rate", {"1/4", "1/8", "1/16", "1/32"});
        addSlider("patternLength", "Steps");
        addSlider("inputGain", "Input Gain");
        addSlider("outputGain", "Output Gain");
        addSlider("mix", "Mix");
        addSlider("fallbackBpm", "Fallback BPM");
        addToggle("stutterEnabled", "Stutter");
        addChoice("stutterRate", "Rate", {"1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/64", "1/128"});
        addChoice("stutterModifier", "Rhythm", {"Straight", "Dotted", "Triplet"});
        addSlider("stutterSlice", "Slice Length");
        addSlider("stutterRepeats", "Repeats");
        addSlider("stutterFade", "Edge Fade");
        addChoice("stutterDirection", "Direction", {"Forward", "Reverse", "Alternating"});
        addToggle("reverbEnabled", "Reverse Reverb");
        addChoice("reverbLength", "Swell Length", {"1/1", "1/2", "1/4", "1/8", "1/16"});
        addSlider("reverbDecay", "Decay");
        addSlider("reverbDamping", "Damping");
        addSlider("reverbMix", "Swell Amount");
        addSlider("reverbCurve", "Swell Curve");

        constexpr std::array<const char*, 3> names{"Stutter", "Reverse", "Swell"};
        for (int lane = 0; lane < 3; ++lane)
        {
            auto& label = laneLabels[static_cast<std::size_t>(lane)];
            label.setText(names[static_cast<std::size_t>(lane)], juce::dontSendNotification);
            addAndMakeVisible(label);
            for (int step = 0; step < 16; ++step)
            {
                auto& button = cells[static_cast<std::size_t>(lane * 16 + step)];
                button.setButtonText(juce::String(step + 1));
                button.setTooltip(juce::String(names[static_cast<std::size_t>(lane)]) + " step " + juce::String(step + 1));
                button.onClick = [this, lane, step]
                {
                    const auto id = "patternStep" + juce::String(step + 1);
                    if (auto* parameter = processor.parameters.getParameter(id))
                    {
                        const int oldMask = juce::roundToInt(parameter->convertFrom0to1(parameter->getValue()));
                        parameter->beginChangeGesture();
                        parameter->setValueNotifyingHost(parameter->convertTo0to1(static_cast<float>(oldMask ^ (1 << lane))));
                        parameter->endChangeGesture();
                    }
                    timerCallback();
                };
                addAndMakeVisible(button);
            }
        }
        setSize(880, 540);
        timerCallback();
        startTimerHz(30);
    }
    ~TransportTestEditor() override { stopTimer(); }

    void paint(juce::Graphics& g) override { g.fillAll(juce::Colour(0xff181b20)); }
    void resized() override
    {
        auto area = getLocalBounds().reduced(12);
        status.setBounds(area.removeFromTop(30));
        for (int lane = 0; lane < 3; ++lane)
        {
            auto row = area.removeFromTop(34);
            laneLabels[static_cast<std::size_t>(lane)].setBounds(row.removeFromLeft(75));
            for (int step = 0; step < 16; ++step)
            {
                const int width = row.getWidth() / (16 - step);
                cells[static_cast<std::size_t>(lane * 16 + step)].setBounds(row.removeFromLeft(width).reduced(2));
            }
        }
        area.removeFromTop(10);
        for (std::size_t start = 0; start < controls.size(); start += 5)
        {
            auto row = area.removeFromTop(94);
            for (std::size_t col = 0; col < 5 && start + col < controls.size(); ++col)
            {
                auto bounds = row.removeFromLeft(row.getWidth() / static_cast<int>(5 - col)).reduced(5);
                controls[start + col]->label.setBounds(bounds.removeFromTop(22));
                controls[start + col]->editor->setBounds(bounds.removeFromTop(42));
            }
        }
    }
private:
    using State = juce::AudioProcessorValueTreeState;
    struct Control
    {
        juce::Label label;
        std::unique_ptr<juce::Component> editor;
        std::unique_ptr<State::SliderAttachment> sliderAttachment;
        std::unique_ptr<State::ComboBoxAttachment> comboAttachment;
        std::unique_ptr<State::ButtonAttachment> buttonAttachment;
    };
    Control& addControl(const juce::String& name)
    {
        auto control = std::make_unique<Control>();
        control->label.setText(name, juce::dontSendNotification);
        addAndMakeVisible(control->label);
        controls.push_back(std::move(control));
        return *controls.back();
    }
    void addSlider(const juce::String& id, const juce::String& name)
    {
        auto& control = addControl(name);
        auto slider = std::make_unique<juce::Slider>();
        slider->setSliderStyle(juce::Slider::LinearHorizontal);
        slider->setTextBoxStyle(juce::Slider::TextBoxRight, false, 58, 22);
        control.sliderAttachment = std::make_unique<State::SliderAttachment>(processor.parameters, id, *slider);
        addAndMakeVisible(*slider);
        control.editor = std::move(slider);
    }
    void addChoice(const juce::String& id, const juce::String& name, const juce::StringArray& choices)
    {
        auto& control = addControl(name);
        auto box = std::make_unique<juce::ComboBox>();
        box->addItemList(choices, 1);
        control.comboAttachment = std::make_unique<State::ComboBoxAttachment>(processor.parameters, id, *box);
        addAndMakeVisible(*box);
        control.editor = std::move(box);
    }
    void addToggle(const juce::String& id, const juce::String& name)
    {
        auto& control = addControl(name);
        auto button = std::make_unique<juce::ToggleButton>("Enabled");
        control.buttonAttachment = std::make_unique<State::ButtonAttachment>(processor.parameters, id, *button);
        addAndMakeVisible(*button);
        control.editor = std::move(button);
    }
    void timerCallback() override
    {
        const int flags = processor.displayedTransportFlags();
        const int active = processor.displayedPatternStep();
        status.setText(juce::String((flags & 1) ? "Host BPM: " : "Fallback BPM: ")
            + juce::String(processor.displayedBpm(), 1) + "  |  "
            + ((flags & 4) ? "Playing" : "Stopped") + "  |  "
            + (active >= 0 ? "Step " + juce::String(active + 1) : juce::String("Manual mode")),
            juce::dontSendNotification);
        const int length = juce::roundToInt(processor.parameters.getRawParameterValue("patternLength")->load());
        for (int step = 0; step < 16; ++step)
        {
            const int mask = juce::roundToInt(processor.parameters.getRawParameterValue("patternStep" + juce::String(step + 1))->load());
            for (int lane = 0; lane < 3; ++lane)
            {
                auto& button = cells[static_cast<std::size_t>(lane * 16 + step)];
                button.setEnabled(step < length);
                button.setToggleState((mask & (1 << lane)) != 0, juce::dontSendNotification);
                button.setColour(juce::TextButton::buttonColourId, juce::Colour(step == active ? 0xff566278 : 0xff303640));
                button.setColour(juce::TextButton::buttonOnColourId, juce::Colour(step == active ? 0xfff0a64b : 0xffa46d37));
            }
        }
    }
    StuddahProcessor& processor;
    juce::Label status;
    std::array<juce::Label, 3> laneLabels;
    std::array<juce::TextButton, 48> cells;
    std::vector<std::unique_ptr<Control>> controls;
};
