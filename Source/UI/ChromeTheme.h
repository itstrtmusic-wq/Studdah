#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>

namespace studdah
{
class ChromeTheme final : public juce::LookAndFeel_V4
{
public:
    ChromeTheme()
    {
        setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xff15181b));
        setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xffd0d2d3));
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff92979b));
        setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff25282b));
        setColour(juce::ComboBox::textColourId, juce::Colours::white);
        setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff777b7e));
        setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff25282b));
        setColour(juce::PopupMenu::textColourId, juce::Colours::white);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xffb51e29));
        setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
        setColour(juce::TextButton::textColourOffId, juce::Colour(0xffe5e5e5));
        setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float position, float startAngle, float endAngle, juce::Slider&) override
    {
        const auto bounds = juce::Rectangle<float>(static_cast<float>(x), static_cast<float>(y),
            static_cast<float>(width), static_cast<float>(height)).reduced(5.0f);
        const float radius = std::min(bounds.getWidth(), bounds.getHeight()) * 0.45f;
        if (radius <= 4.0f) return;
        const auto centre = bounds.getCentre();
        const float angle = startAngle + position * (endAngle - startAngle);
        const juce::Rectangle<float> outer(centre.x - radius, centre.y - radius, radius * 2, radius * 2);
        g.setColour(juce::Colour(0xff727679));
        g.fillEllipse(outer);
        const auto ring = outer.reduced(2.0f);
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff3c4043), ring.getTopLeft(),
            juce::Colour(0xff08090a), ring.getBottomRight(), false));
        g.fillEllipse(ring);
        g.setColour(juce::Colour(0xff050607));
        g.drawEllipse(ring, 1.0f);
        juce::Path track;
        track.addCentredArc(centre.x, centre.y, radius - 3.0f, radius - 3.0f,
            0.0f, startAngle, endAngle, true);
        g.setColour(juce::Colour(0xff62676b));
        g.strokePath(track, juce::PathStrokeType(2.0f));
        juce::Path active;
        active.addCentredArc(centre.x, centre.y, radius - 3.0f, radius - 3.0f,
            0.0f, startAngle, angle, true);
        g.setColour(juce::Colour(0xffe3343e));
        g.strokePath(active, juce::PathStrokeType(2.8f));
        const auto face = outer.reduced(radius * 0.23f);
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff51565a), face.getTopLeft(),
            juce::Colour(0xff151719), face.getBottomRight(), false));
        g.fillEllipse(face);
        g.setColour(juce::Colour(0xff777c80));
        g.drawEllipse(face, 0.7f);
        const float r = radius * 0.58f;
        const juce::Point<float> tip(centre.x + std::sin(angle) * r, centre.y - std::cos(angle) * r);
        const juce::Point<float> inner(centre.x + std::sin(angle) * radius * 0.20f,
                                      centre.y - std::cos(angle) * radius * 0.20f);
        g.setColour(juce::Colour(0xfff13b42));
        g.drawLine(juce::Line<float>(inner, tip), 2.5f);
    }

    void drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour& colour,
                              bool highlighted, bool down) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
        auto base = colour;
        if (highlighted) base = base.brighter(0.15f);
        if (down) base = base.darker(0.2f);
        if (!button.isEnabled()) base = base.withMultipliedAlpha(0.33f);
        g.setGradientFill(juce::ColourGradient(base.brighter(0.24f), bounds.getTopLeft(),
            base.darker(0.32f), bounds.getBottomLeft(), false));
        g.fillRoundedRectangle(bounds, 3.0f);
        g.setColour(juce::Colour(0xff777b7e).withAlpha(0.65f));
        g.drawRoundedRectangle(bounds, 3.0f, 0.7f);
    }
};
}
