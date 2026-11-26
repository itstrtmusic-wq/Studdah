#pragma once
#include "../PluginProcessor.h"
#include "ChromeTheme.h"

class StuddahEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit StuddahEditor(StuddahProcessor& owner)
        : AudioProcessorEditor(owner), processor(owner)
    {
        setLookAndFeel(&theme);
        addAndMakeVisible(status);
        status.setColour(juce::Label::textColourId, juce::Colour(0xff17191b));
        status.setFont(juce::Font(13.0f, juce::Font::bold));
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
            label.setColour(juce::Label::textColourId, juce::Colour(0xffd9dbdc));
            label.setFont(juce::Font(12.0f, juce::Font::bold));
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
        setSize(1100, 660);
        timerCallback();
        startTimerHz(30);
    }
    ~StuddahEditor() override { stopTimer(); setLookAndFeel(nullptr); }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colour(0xff151719));
        metalPanel(g, {0, 0, 1100, 60});
        g.setColour(juce::Colour(0xff16191b));
        g.setFont(juce::Font(30.0f, juce::Font::bold));
        g.drawText("STUDDAH", 18, 0, 220, 60, juce::Justification::centredLeft);
        g.setColour(juce::Colour(0xffca2933));
        g.fillRect(212, 15, 3, 30);
        g.setFont(juce::Font(11.0f, juce::Font::bold));
        g.setColour(juce::Colour(0xff373b3e));
        g.drawText("VOCAL MANIPULATOR  /  v0.8", 226, 7, 250, 20, juce::Justification::centredLeft);
        drawWaveform(g);

        darkPanel(g, {12, 236, 1076, 164});
        g.setColour(juce::Colour(0xffe1e3e4));
        g.setFont(juce::Font(14.0f, juce::Font::bold));
        g.drawText("PATTERN", 24, 243, 115, 30, juce::Justification::centredLeft);
        for (int step = 0; step < 16; ++step)
        {
            const auto cell = cells[static_cast<std::size_t>(step)].getBounds();
            g.setColour(juce::Colour(0xffb4b8bb));
            g.setFont(juce::Font(10.0f));
            g.drawText(juce::String(step + 1), cell.getX(), 280, cell.getWidth(), 14, juce::Justification::centred);
        }

        metalPanel(g, {12, 412, 430, 236});
        metalPanel(g, {452, 412, 410, 236});
        metalPanel(g, {872, 412, 216, 236});
        sectionTitle(g, "STUTTER", 24, 420, 220);
        sectionTitle(g, "REVERSE REVERB", 464, 420, 220);
        sectionTitle(g, "MASTER", 884, 420, 190);
    }
    void resized() override
    {
        status.setBounds(226, 29, 610, 23);
        place(6, {866, 4, 214, 50});

        place(0, {138, 241, 130, 38});
        place(1, {280, 241, 125, 38});
        place(2, {418, 241, 160, 38});
        for (int lane = 0; lane < 3; ++lane)
        {
            juce::Rectangle<int> row(24, 296 + lane * 32, 1052, 30);
            laneLabels[static_cast<std::size_t>(lane)].setBounds(row.removeFromLeft(83));
            for (int step = 0; step < 16; ++step)
                cells[static_cast<std::size_t>(lane * 16 + step)].setBounds(
                    row.removeFromLeft(row.getWidth() / (16 - step)).reduced(2));
        }
        place(7, {310, 416, 120, 42});
        place(8, {24, 461, 126, 58});
        place(9, {154, 461, 126, 58});
        place(13, {284, 461, 144, 58});
        place(10, {28, 526, 126, 114});
        place(11, {156, 526, 126, 114});
        place(12, {284, 526, 126, 114});

        place(14, {720, 416, 132, 42});
        place(15, {464, 461, 120, 58});
        place(16, {590, 461, 124, 87});
        place(17, {718, 461, 124, 87});
        place(18, {512, 550, 124, 90});
        place(19, {676, 550, 124, 90});

        place(3, {884, 457, 94, 94});
        place(4, {982, 457, 94, 94});
        place(5, {930, 551, 100, 90});
    }
private:
    static void metalPanel(juce::Graphics& g, juce::Rectangle<int> rect)
    {
        auto r = rect.toFloat();
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xfff0f1f2), r.getTopLeft(),
            juce::Colour(0xffa8adb0), r.getBottomLeft(), false));
        g.fillRoundedRectangle(r, 5.0f);
        // Subtle directional brushing, drawn procedurally rather than using a bitmap.
        juce::Graphics::ScopedSaveState saved(g);
        juce::Path clip;
        clip.addRoundedRectangle(r, 5.0f);
        g.reduceClipRegion(clip);
        for (int y = rect.getY() + 1; y < rect.getBottom(); y += 3)
        {
            g.setColour(juce::Colours::white.withAlpha(y % 2 == 0 ? 0.12f : 0.06f));
            g.drawHorizontalLine(y, r.getX(), r.getRight());
        }
        g.setColour(juce::Colour(0xff777d81));
        g.drawRoundedRectangle(r.reduced(0.5f), 5.0f, 1.0f);
    }
    static void darkPanel(juce::Graphics& g, juce::Rectangle<int> rect)
    {
        const auto r = rect.toFloat();
        g.setColour(juce::Colour(0xff0c0e10));
        g.fillRoundedRectangle(r, 5.0f);
        g.setColour(juce::Colour(0xff45494d));
        g.drawRoundedRectangle(r.reduced(0.5f), 5.0f, 1.0f);
    }
    static void sectionTitle(juce::Graphics& g, const juce::String& title, int x, int y, int width)
    {
        g.setColour(juce::Colour(0xffca2933));
        g.fillRect(x, y + 7, 4, 18);
        g.setColour(juce::Colour(0xff141618));
        g.setFont(juce::Font(17.0f, juce::Font::bold));
        g.drawText(title, x + 12, y, width - 12, 32, juce::Justification::centredLeft);
    }
    void drawWaveform(juce::Graphics& g)
    {
        darkPanel(g, {12, 72, 1076, 152});
        g.setColour(juce::Colour(0xff9da3a8));
        g.setFont(juce::Font(11.0f, juce::Font::bold));
        g.drawText("INPUT  /  LIVE", 24, 79, 180, 16, juce::Justification::centredLeft);
        const juce::Rectangle<float> plot(24.0f, 99.0f, 1052.0f, 113.0f);
        g.setColour(juce::Colour(0xff22272a));
        for (int i = 0; i <= 16; ++i)
            g.drawVerticalLine(static_cast<int>(plot.getX() + plot.getWidth() * i / 16.0f), plot.getY(), plot.getBottom());
        const auto mid = plot.getCentreY();
        g.drawHorizontalLine(static_cast<int>(mid), plot.getX(), plot.getRight());
        juce::Path peaks;
        for (int i = 0; i < studdah::WaveformHistory::size; ++i)
        {
            const auto x = plot.getX() + plot.getWidth() * i / static_cast<float>(studdah::WaveformHistory::size - 1);
            const auto& p = wave[static_cast<std::size_t>(i)];
            peaks.startNewSubPath(x, mid - p.high * plot.getHeight() * 0.47f);
            peaks.lineTo(x, mid - p.low * plot.getHeight() * 0.47f);
        }
        g.setColour(juce::Colour(0xffe0e4e6));
        g.strokePath(peaks, juce::PathStrokeType(1.3f));
        g.setColour(juce::Colour(0xffd92b35));
        g.drawVerticalLine(static_cast<int>(plot.getRight()), plot.getY(), plot.getBottom());
    }
    void place(std::size_t index, juce::Rectangle<int> bounds)
    {
        auto& c = *controls[index];
        c.label.setBounds(bounds.removeFromTop(16));
        c.editor->setBounds(bounds.reduced(2));
    }

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
        control->label.setText(name.toUpperCase(), juce::dontSendNotification);
        control->label.setJustificationType(juce::Justification::centred);
        control->label.setFont(juce::Font(11.0f, juce::Font::bold));
        control->label.setColour(juce::Label::textColourId, juce::Colour(0xff23272a));
        if (controls.size() < 3) control->label.setColour(juce::Label::textColourId, juce::Colour(0xffd9dbdc));
        addAndMakeVisible(control->label);
        controls.push_back(std::move(control));
        return *controls.back();
    }
    void addSlider(const juce::String& id, const juce::String& name)
    {
        auto& control = addControl(name);
        auto slider = std::make_unique<juce::Slider>();
        const bool compact = id == "patternLength" || id == "fallbackBpm";
        slider->setSliderStyle(compact ? juce::Slider::LinearHorizontal : juce::Slider::RotaryHorizontalVerticalDrag);
        slider->setTextBoxStyle(compact ? juce::Slider::TextBoxRight : juce::Slider::TextBoxBelow, false, 64, 18);
        slider->setColour(juce::Slider::trackColourId, juce::Colour(0xffc72d37));
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
        auto button = std::make_unique<juce::TextButton>("ENABLE");
        button->setClickingTogglesState(true);
        button->setColour(juce::TextButton::buttonColourId, juce::Colour(0xff353a3e));
        button->setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffc62832));
        control.buttonAttachment = std::make_unique<State::ButtonAttachment>(processor.parameters, id, *button);
        addAndMakeVisible(*button);
        control.editor = std::move(button);
    }
    void timerCallback() override
    {
        wave = processor.waveformSnapshot();
        repaint(12, 72, 1076, 152);
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
                button.setColour(juce::TextButton::buttonColourId, juce::Colour(step == active ? 0xff676b6e : 0xff24282b));
                button.setColour(juce::TextButton::buttonOnColourId, juce::Colour(step == active ? 0xffff5055 : (lane == 2 ? 0xff70777c : 0xffb82530)));
            }
        }
    }
    studdah::ChromeTheme theme;
    StuddahProcessor& processor;
    studdah::WaveformHistory::Snapshot wave{};
    juce::Label status;
    std::array<juce::Label, 3> laneLabels;
    std::array<juce::TextButton, 48> cells;
    std::vector<std::unique_ptr<Control>> controls;
};
