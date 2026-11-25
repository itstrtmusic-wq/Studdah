#pragma once
#include "TempoClock.h"
#include <array>
#include <cmath>

namespace studdah
{
enum PatternLane : int { stutterLane = 1, reverseLane = 2, swellLane = 4 };

struct PatternSettings
{
    bool enabled = false;
    int denominator = 16;
    int length = 16;
    std::array<int, 16> steps{};
};

struct PatternStep
{
    int index = -1;
    int mask = 0;
    bool stutter() const noexcept { return (mask & (stutterLane | reverseLane)) != 0; }
    bool reverse() const noexcept { return (mask & reverseLane) != 0; }
    bool swell() const noexcept { return (mask & swellLane) != 0; }
};

class PatternSequencer
{
public:
    void beginBlock(const TempoClock& clock, const PatternSettings& settings) noexcept
    {
        config = settings;
        config.length = std::clamp(config.length, 1, 16);
        division = TempoClock::divisionInQuarterNotes(config.denominator, NoteModifier::straight);
        phase = clock.ppqAtBlockStart();
        step = clock.quarterNotesPerSample();
        running = config.enabled && clock.isPlaying() && division > 0.0;
    }

    PatternStep at(int offset) const noexcept
    {
        if (!running) return {};
        const double grid = std::floor((phase + offset * step) / division + 1.0e-9);
        double wrapped = std::fmod(grid, static_cast<double>(config.length));
        if (wrapped < 0.0) wrapped += config.length;
        const int index = static_cast<int>(wrapped);
        return {index, config.steps[static_cast<std::size_t>(index)] & 7};
    }

private:
    PatternSettings config;
    double division = 0.25, phase = 0.0, step = 0.0;
    bool running = false;
};
}
