#pragma once
#include "CircularAudioBuffer.h"
#include "../Core/Transport/TempoClock.h"
#include <array>
#include <cmath>

namespace studdah
{
enum class PlaybackDirection { forward, reverse, alternating };

struct StutterSettings
{
    bool enabled = false;
    int denominator = 16;
    NoteModifier modifier = NoteModifier::straight;
    float slice = 1.0f;
    int repeats = 4;
    float fadeMs = 5.0f;
    PlaybackDirection direction = PlaybackDirection::forward;
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
        repeatOrdinal = 0;
        reversePulse = false;
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
        if (settings.direction != requestedDirection)
            repeatsLeft = 0;
        requestedDirection = settings.direction;
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
                // Reverse reading moves toward older samples while recording continues.
                // Refresh before the active slice can be overwritten during a reverse pulse.
                const bool reverseCapable = requestedDirection != PlaybackDirection::forward;
                const bool expiresDuringPulse = reverseCapable && length > 0
                    && history.endPosition() - sliceStart + static_cast<CircularAudioBuffer::Position>(length)
                        > static_cast<CircularAudioBuffer::Position>(history.capacitySamples());
                if (length == 0 || repeatsLeft <= 0 || sliceStart < history.beginPosition() || expiresDuringPulse)
                {
                    const int limit = reverseCapable ? history.capacitySamples() / 2 : history.capacitySamples();
                    length = std::min({requestedLength, history.available(), limit});
                    sliceStart = history.endPosition() - static_cast<CircularAudioBuffer::Position>(length);
                    repeatsLeft = requestedRepeats;
                    repeatOrdinal = 0;
                }
                reversePulse = requestedDirection == PlaybackDirection::reverse
                    || (requestedDirection == PlaybackDirection::alternating && repeatOrdinal % 2 != 0);
                ++repeatOrdinal;
                --repeatsLeft;
                index = 0;
                fade = std::max(1, std::min(requestedFade, length / 2));
            }
        }

        activity = std::clamp(activity + (running ? activityStep : -activityStep), 0.0f, 1.0f);
        Frame result = dry;
        if (index < length)
        {
            const int readIndex = reversePulse ? length - 1 - index : index;
            const auto position = sliceStart + static_cast<CircularAudioBuffer::Position>(readIndex);
            const auto envelope = std::min({1.0f,
                static_cast<float>(index) / static_cast<float>(fade),
                static_cast<float>(length - 1 - index) / static_cast<float>(fade)}) * activity;
            for (int ch = 0; ch < 2; ++ch)
            {
                if (position >= history.beginPosition() && position < history.endPosition())
                {
                    const auto past = history.read(ch, position);
                    result[static_cast<std::size_t>(ch)] += envelope * (past - dry[static_cast<std::size_t>(ch)]);
                }
            }
            ++index;
        }
        if (!running && activity == 0.0f)
            length = index = 0;
        return result;
    }

private:
    double sampleRate = 44100.0, phase = 0.0, step = 0.0, division = 0.0;
    bool running = false, reversePulse = false;
    int repeatOrdinal = 0;
    PlaybackDirection requestedDirection = PlaybackDirection::forward;
    int length = 0, index = 0, repeatsLeft = 0, fade = 1;
    int requestedLength = 1, requestedRepeats = 4, requestedFade = 1;
    float activity = 0.0f, activityStep = 1.0f;
    CircularAudioBuffer::Position sliceStart = 0;
    std::optional<double> lastGrid;
};
}
