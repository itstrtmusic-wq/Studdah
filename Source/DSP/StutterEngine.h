#pragma once
#include "CircularAudioBuffer.h"
#include "../Core/Transport/TempoClock.h"
#include <array>
#include <cmath>

namespace studdah
{
struct StutterSettings
{
    bool enabled = false;
    int denominator = 16;
    NoteModifier modifier = NoteModifier::straight;
    float slice = 1.0f;
    int repeats = 4;
    float fadeMs = 5.0f;
};

class StutterEngine
{
public:
    using Frame = std::array<float, 2>;

    void prepare(double rate) noexcept
    {
        sampleRate = rate;
        reset();
    }

    void reset() noexcept
    {
        length = index = repeatsLeft = 0;
        sliceStart = 0;
        fade = 1;
        activity = 0.0f;
        lastGrid.reset();
    }

    void beginBlock(const TempoClock& clock, StutterSettings settings) noexcept
    {
        if (clock.hasDiscontinuity())
            reset();
        const double nextDivision = TempoClock::divisionInQuarterNotes(settings.denominator, settings.modifier);
        if (nextDivision != division)
        {
            repeatsLeft = 0;
            lastGrid.reset();
        }
        division = nextDivision;
        phase = clock.ppqAtBlockStart();
        step = clock.quarterNotesPerSample();
        running = settings.enabled && clock.isPlaying() && division > 0.0;
        if (!running)
        {
            lastGrid.reset();
            repeatsLeft = 0;
        }
        const double desired = std::floor(clock.samplesForDivision(settings.denominator, settings.modifier)
            * std::clamp(settings.slice, 0.01f, 1.0f));
        requestedLength = static_cast<int>(std::clamp(desired, 1.0, sampleRate * 16.0));
        requestedRepeats = std::clamp(settings.repeats, 1, 16);
        requestedFade = std::max(1, static_cast<int>(sampleRate * std::clamp(settings.fadeMs, 1.0f, 30.0f) * 0.001));
        activityStep = 1.0f / static_cast<float>(requestedFade);
    }

    // Call before writing the current input frame into history.
    Frame processFrame(const CircularAudioBuffer& history, const Frame& dry, int offset) noexcept
    {
        if (running)
        {
            const double at = phase + offset * step;
            const double grid = std::floor(at / division + 1.0e-9);
            const double previous = std::floor((at - step) / division + 1.0e-9);
            if (grid > previous && (!lastGrid || grid != *lastGrid))
            {
                lastGrid = grid;
                if (length == 0 || repeatsLeft <= 0 || sliceStart < history.beginPosition())
                {
                    length = std::min(requestedLength, history.available());
                    sliceStart = history.endPosition() - static_cast<CircularAudioBuffer::Position>(length);
                    repeatsLeft = requestedRepeats;
                }
                --repeatsLeft;
                index = 0;
                fade = std::max(1, std::min(requestedFade, length / 2));
            }
        }

        activity = std::clamp(activity + (running ? activityStep : -activityStep), 0.0f, 1.0f);
        Frame result = dry;
        if (index < length && sliceStart + static_cast<CircularAudioBuffer::Position>(index) >= history.beginPosition())
        {
            const auto envelope = std::min({1.0f,
                static_cast<float>(index) / static_cast<float>(fade),
                static_cast<float>(length - 1 - index) / static_cast<float>(fade)}) * activity;
            for (int ch = 0; ch < 2; ++ch)
            {
                const auto past = history.read(ch, sliceStart + static_cast<CircularAudioBuffer::Position>(index));
                result[static_cast<std::size_t>(ch)] += envelope * (past - dry[static_cast<std::size_t>(ch)]);
            }
            ++index;
        }
        if (!running && activity == 0.0f)
            length = index = 0;
        return result;
    }

private:
    double sampleRate = 44100.0, phase = 0.0, step = 0.0, division = 0.0;
    bool running = false;
    int length = 0, index = 0, repeatsLeft = 0, fade = 1;
    int requestedLength = 1, requestedRepeats = 4, requestedFade = 1;
    float activity = 0.0f, activityStep = 1.0f;
    CircularAudioBuffer::Position sliceStart = 0;
    std::optional<double> lastGrid;
};
}
