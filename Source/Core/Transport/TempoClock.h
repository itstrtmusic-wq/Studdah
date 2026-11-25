#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>

namespace studdah
{
enum class NoteModifier { straight, dotted, triplet };

// All positions are in quarter-note units (PPQ), independent of time signature.
struct HostTransport
{
    bool playing = true;
    std::optional<double> bpm;
    std::optional<double> ppq;
    std::optional<std::int64_t> samplePosition;
};

class TempoClock
{
public:
    void prepare(double rate) noexcept
    {
        sampleRate = std::isfinite(rate) && rate > 0.0 ? rate : 44100.0;
        nextPpq = blockPpq = 0.0;
        tempo = 120.0;
        playing = true;
        hostTempo = hostPosition = discontinuity = false;
        expectedSample.reset();
        previousPlaying = false;
        seenBlock = false;
    }

    void beginBlock(const HostTransport& host, double fallbackBpm, int samples) noexcept
    {
        hostTempo = host.bpm && validTempo(*host.bpm);
        tempo = hostTempo ? *host.bpm : (validTempo(fallbackBpm) ? fallbackBpm : 120.0);
        hostPosition = host.ppq && std::isfinite(*host.ppq);
        playing = host.playing;
        blockPpq = hostPosition ? *host.ppq : nextPpq;
        const bool restarted = seenBlock && playing && !previousPlaying;
        const bool jumpedSamples = expectedSample && host.samplePosition
            && *expectedSample != *host.samplePosition;
        // Tolerate sub-sample rounding and tiny host PPQ variations.
        const bool jumpedPpq = hostPosition && std::abs(blockPpq - nextPpq)
            > std::max(1.0e-6, 2.0 * quarterNotesPerSample());
        discontinuity = playing && (restarted || (seenBlock && previousPlaying
            && (jumpedSamples || jumpedPpq)));
        nextPpq = blockPpq + (playing ? std::max(0, samples) * quarterNotesPerSample() : 0.0);
        expectedSample.reset();
        if (playing && host.samplePosition
            && *host.samplePosition <= std::numeric_limits<std::int64_t>::max() - std::max(0, samples))
            expectedSample = *host.samplePosition + std::max(0, samples);
        previousPlaying = playing;
        seenBlock = true;
    }

    double bpm() const noexcept { return tempo; }
    double ppqAtBlockStart() const noexcept { return blockPpq; }
    bool isPlaying() const noexcept { return playing; }
    bool hasHostTempo() const noexcept { return hostTempo; }
    bool hasHostPosition() const noexcept { return hostPosition; }
    bool hasDiscontinuity() const noexcept { return discontinuity; }
    double quarterNotesPerSample() const noexcept { return tempo / (60.0 * sampleRate); }

    static double divisionInQuarterNotes(int denominator, NoteModifier modifier) noexcept
    {
        if (denominator <= 0) return 0.0;
        const double beats = 4.0 / denominator;
        switch (modifier)
        {
            case NoteModifier::dotted: return beats * 1.5;
            case NoteModifier::triplet: return beats * (2.0 / 3.0);
            default: return beats;
        }
    }

    double samplesForDivision(int denominator, NoteModifier modifier = NoteModifier::straight) const noexcept
    {
        return divisionInQuarterNotes(denominator, modifier) / quarterNotesPerSample();
    }

    // First grid crossing on or after the given offset; no result if outside the block.
    std::optional<int> nextGridOffset(double divisionPpq, int from, int blockSamples) const noexcept
    {
        if (!playing || !std::isfinite(divisionPpq) || divisionPpq <= 0.0
            || from < 0 || from >= blockSamples) return {};
        const double step = quarterNotesPerSample();
        const double at = blockPpq + from * step;
        const double grid = std::ceil(at / divisionPpq - 1.0e-9) * divisionPpq;
        const double offset = std::ceil((grid - blockPpq) / step - 1.0e-7);
        if (!std::isfinite(offset) || offset >= blockSamples) return {};
        return offset <= from ? from : static_cast<int>(offset);
    }

private:
    static bool validTempo(double value) noexcept
    {
        return std::isfinite(value) && value >= 1.0 && value <= 1000.0;
    }
    double sampleRate = 44100.0, tempo = 120.0, blockPpq = 0.0, nextPpq = 0.0;
    bool playing = true, hostTempo = false, hostPosition = false, discontinuity = false;
    bool previousPlaying = false, seenBlock = false;
    std::optional<std::int64_t> expectedSample;
};
}
