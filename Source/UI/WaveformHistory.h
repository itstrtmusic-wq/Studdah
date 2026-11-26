#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>

namespace studdah
{
// Single audio producer, any number of UI readers. No audio-thread allocations.
class WaveformHistory
{
public:
    static constexpr int size = 512;
    struct Range { float low = 0.0f, high = 0.0f; };
    using Snapshot = std::array<Range, size>;

    void prepare(double sampleRate) noexcept
    {
        samplesPerBin = std::max(1, static_cast<int>(sampleRate / 200.0));
        count = 0;
        low = high = 0.0f;
        for (auto& bin : bins) bin.store(0, std::memory_order_relaxed);
        written.store(0, std::memory_order_release);
    }

    void push(float left, float right) noexcept
    {
        low = std::min({low, left, right});
        high = std::max({high, left, right});
        if (++count < samplesPerBin) return;
        const auto index = written.load(std::memory_order_relaxed);
        bins[index % size].store(encode(low, high), std::memory_order_relaxed);
        written.store(index + 1, std::memory_order_release);
        count = 0;
        low = high = 0.0f;
    }

    Snapshot snapshot() const noexcept
    {
        Snapshot result{};
        const auto end = written.load(std::memory_order_acquire);
        const auto available = std::min<std::uint64_t>(end, size);
        for (std::uint64_t i = 0; i < available; ++i)
        {
            const auto raw = bins[(end - available + i) % size].load(std::memory_order_relaxed);
            result[size - available + i] = { -static_cast<float>(raw >> 16) / 32767.0f,
                                              static_cast<float>(raw & 0xffffu) / 32767.0f };
        }
        return result;
    }
private:
    static std::uint32_t encode(float minimum, float maximum) noexcept
    {
        auto magnitude = [](float value) {
            return static_cast<std::uint32_t>(std::clamp(value, 0.0f, 1.0f) * 32767.0f);
        };
        return (magnitude(-minimum) << 16) | magnitude(maximum);
    }
    std::array<std::atomic<std::uint32_t>, size> bins{};
    std::atomic<std::uint64_t> written{0};
    int samplesPerBin = 220, count = 0;
    float low = 0.0f, high = 0.0f;
};
}
