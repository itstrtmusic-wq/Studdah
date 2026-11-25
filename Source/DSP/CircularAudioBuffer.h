#pragma once
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace studdah
{
// Audio-thread-owned history. Allocate only in prepare(), never during recording.
class CircularAudioBuffer
{
public:
    using Position = std::uint64_t;

    void prepare(int channels, int capacitySamples)
    {
        capacity = std::max(1, capacitySamples);
        channelCount = std::clamp(channels, 1, 2);
        for (int ch = 0; ch < 2; ++ch)
            samples[static_cast<std::size_t>(ch)].assign(ch < channelCount
                ? static_cast<std::size_t>(capacity) : 0u, 0.0f);
        reset();
    }

    void reset() noexcept { nextPosition = 0; }
    Position endPosition() const noexcept { return nextPosition; }
    Position beginPosition() const noexcept
    {
        const auto count = static_cast<Position>(capacity);
        return nextPosition > count ? nextPosition - count : 0;
    }
    int available() const noexcept { return static_cast<int>(endPosition() - beginPosition()); }
    int capacitySamples() const noexcept { return capacity; }

    // The caller supplies an array of channel pointers, each with count samples.
    void push(const float* const* source, int channels, int count) noexcept
    {
        if (capacity == 0 || source == nullptr || count <= 0)
            return;
        int offset = 0;
        while (offset < count)
        {
            const auto index = static_cast<int>(nextPosition % static_cast<Position>(capacity));
            const auto length = std::min(count - offset, capacity - index);
            for (int ch = 0; ch < channelCount; ++ch)
            {
                auto* destination = samples[static_cast<std::size_t>(ch)].data() + index;
                if (ch < channels && source[ch] != nullptr)
                    std::copy_n(source[ch] + offset, length, destination);
                else
                    std::fill_n(destination, length, 0.0f);
            }
            nextPosition += static_cast<Position>(length);
            offset += length;
        }
    }

    // Absolute positions are valid only while they remain in the history.
    float read(int channel, Position position) const noexcept
    {
        if (channel < 0 || channel >= channelCount || capacity == 0
            || position < beginPosition() || position >= endPosition())
            return 0.0f;
        return samples[static_cast<std::size_t>(channel)]
                      [static_cast<std::size_t>(position % static_cast<Position>(capacity))];
    }

    // Copy the latest requested samples into caller-owned storage in playback order.
    // Insufficient history is padded with silence; reverse also reverses that padding.
    void copyRecent(int channel, float* destination, int count, bool reverse = false) const noexcept
    {
        if (destination == nullptr || count <= 0)
            return;
        const int valid = std::min(count, available());
        const int padding = count - valid;
        const auto start = endPosition() - static_cast<Position>(valid);
        for (int i = 0; i < count; ++i)
        {
            const int forwardIndex = reverse ? count - 1 - i : i;
            destination[i] = forwardIndex < padding ? 0.0f
                : read(channel, start + static_cast<Position>(forwardIndex - padding));
        }
    }

private:
    std::array<std::vector<float>, 2> samples;
    int capacity = 0;
    int channelCount = 0;
    Position nextPosition = 0;
};
}
