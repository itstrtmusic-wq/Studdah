#pragma once
#include "CircularAudioBuffer.h"
#include "../Core/Transport/TempoClock.h"
#include <array>
#include <cmath>
#include <optional>
#include <vector>

namespace studdah
{
struct ReverseReverbSettings
{
    bool enabled = false;
    int denominator = 4;
    float decaySeconds = 2.5f;
    float damping = 0.45f;
    float mix = 0.35f;
    float curve = 2.0f;
    bool patternControlled = false;
};

// A causal reverse swell: create a diffuse forward tail, record it, then
// replay the previous grid slice backwards with a rising envelope.
class ReverseReverb
{
public:
    using Frame = std::array<float, 2>;

    void prepare(double rate, int numChannels)
    {
        sampleRate = std::max(1.0, rate);
        channels = std::clamp(numChannels, 1, 2);
        history.prepare(channels, std::max(2, static_cast<int>(sampleRate * 8.0)));
        constexpr std::array<double, 4> combMs{29.7, 37.1, 41.1, 43.7};
        constexpr std::array<double, 2> allpassMs{5.0, 1.7};
        for (int ch = 0; ch < channels; ++ch)
        {
            auto& voice = voices[static_cast<std::size_t>(ch)];
            for (std::size_t i = 0; i < combMs.size(); ++i)
                voice.combs[i].prepare(samplesFromMs(combMs[i] + ch * 1.3));
            for (std::size_t i = 0; i < allpassMs.size(); ++i)
                voice.diffusers[i].prepare(samplesFromMs(allpassMs[i] + ch * 0.31));
        }
        reset();
    }

    void reset() noexcept
    {
        history.reset();
        for (auto& voice : voices)
        {
            for (auto& comb : voice.combs) comb.reset();
            for (auto& diffuser : voice.diffusers) diffuser.reset();
        }
        length = index = 0;
        start = 0;
        amount = 0.0f;
        lastGrid.reset();
    }

    void beginBlock(const TempoClock& clock, const ReverseReverbSettings& settings) noexcept
    {
        if (clock.hasDiscontinuity()) reset();
        const auto nextDivision = TempoClock::divisionInQuarterNotes(settings.denominator, NoteModifier::straight);
        if (nextDivision != division) lastGrid.reset();
        division = nextDivision;
        phase = clock.ppqAtBlockStart();
        step = clock.quarterNotesPerSample();
        transportRunning = clock.isPlaying() && division > 0.0;
        requestedAmount = std::clamp(settings.mix, 0.0f, 1.0f);
        if (!settings.patternControlled)
            setPatternGate(settings.enabled);
        else if (!transportRunning)
            setPatternGate(false);
        amountStep = 1.0f / static_cast<float>(samplesFromMs(10.0));
        desiredLength = static_cast<int>(std::clamp(
            std::floor(clock.samplesForDivision(settings.denominator)), 1.0,
            static_cast<double>(std::max(1, history.capacitySamples() / 2))));
        requestedCurve = std::clamp(settings.curve, 0.5f, 4.0f);
        const float decay = std::clamp(settings.decaySeconds, 0.2f, 8.0f);
        damping = std::clamp(settings.damping, 0.0f, 0.95f);
        for (int ch = 0; ch < channels; ++ch)
            for (auto& comb : voices[static_cast<std::size_t>(ch)].combs)
                comb.feedback = static_cast<float>(std::pow(10.0, -3.0 * static_cast<double>(comb.data.size()) / (sampleRate * decay)));
    }

    void setPatternGate(bool enabled) noexcept
    {
        running = enabled && transportRunning;
        if (!running) lastGrid.reset();
        targetAmount = running ? requestedAmount : 0.0f;
    }

    Frame processFrame(const Frame& input, int offset) noexcept
    {
        if (running)
        {
            const double at = phase + offset * step;
            const double grid = std::floor(at / division + 1.0e-9);
            const double before = std::floor((at - step) / division + 1.0e-9);
            if (grid > before && (!lastGrid || *lastGrid != grid))
            {
                lastGrid = grid;
                // Wait for a full grid of past audio rather than playing uninitialised samples.
                length = history.available() >= desiredLength ? desiredLength : 0;
                start = history.endPosition() - static_cast<CircularAudioBuffer::Position>(length);
                index = 0;
                fadeLength = std::max(1, std::min(samplesFromMs(5.0), length / 4));
                curve = requestedCurve;
            }
        }

        amount += std::clamp(targetAmount - amount, -amountStep, amountStep);
        Frame output = input;
        if (index < length)
        {
            const auto pos = start + static_cast<CircularAudioBuffer::Position>(length - 1 - index);
            const float progress = static_cast<float>(index) / static_cast<float>(std::max(1, length - 1));
            const float edge = std::min(1.0f, static_cast<float>(length - 1 - index) / static_cast<float>(fadeLength));
            const float envelope = std::pow(progress, curve) * edge;
            for (int ch = 0; ch < channels; ++ch)
            {
                const auto c = static_cast<std::size_t>(ch);
                const float wet = history.read(ch, pos) * envelope;
                // Parallel swell preserves the current vocal at all mix settings.
                output[c] += wet * amount;
            }
            ++index;
        }

        Frame tail{};
        for (int ch = 0; ch < channels; ++ch)
        {
            const auto c = static_cast<std::size_t>(ch);
            auto& voice = voices[c];
            const float excitation = input[c] * 0.8f + (channels == 2 ? input[1u-c] * 0.2f : input[c] * 0.2f);
            float diffuse = 0.0f;
            for (auto& comb : voice.combs) diffuse += comb.processComb(excitation, damping) * 0.25f;
            for (auto& diffuser : voice.diffusers) diffuse = diffuser.processAllpass(diffuse);
            tail[c] = diffuse;
        }
        const float* pointers[]{&tail[0], &tail[1]};
        history.push(pointers, channels, 1);
        return output;
    }

private:
    struct Delay
    {
        std::vector<float> data;
        std::size_t cursor = 0;
        float filtered = 0.0f, feedback = 0.0f;
        void prepare(int count) { data.assign(static_cast<std::size_t>(count), 0.0f); reset(); }
        void reset() noexcept { std::fill(data.begin(), data.end(), 0.0f); cursor = 0; filtered = 0.0f; }
        void advance() noexcept { if (++cursor == data.size()) cursor = 0; }
        float processComb(float x, float damp) noexcept
        {
            const float y = data[cursor];
            filtered = y * (1.0f - damp) + filtered * damp;
            data[cursor] = x + filtered * feedback;
            advance();
            return y;
        }
        float processAllpass(float x) noexcept
        {
            constexpr float g = 0.5f;
            const float delayed = data[cursor];
            const float y = delayed - g * x;
            data[cursor] = x + g * y;
            advance();
            return y;
        }
    };
    struct Voice { std::array<Delay, 4> combs; std::array<Delay, 2> diffusers; };
    int samplesFromMs(double ms) const noexcept { return std::max(1, static_cast<int>(std::round(sampleRate * ms * 0.001))); }
    double sampleRate = 44100.0, division = 0.0, phase = 0.0, step = 0.0;
    int channels = 1, desiredLength = 1, length = 0, index = 0, fadeLength = 1;
    float damping = 0.45f, curve = 2.0f, requestedCurve = 2.0f;
    float amount = 0.0f, targetAmount = 0.0f, amountStep = 1.0f;
    bool running = false, transportRunning = false;
    float requestedAmount = 0.0f;
    CircularAudioBuffer::Position start = 0;
    std::optional<double> lastGrid;
    CircularAudioBuffer history;
    std::array<Voice, 2> voices;
};
}
