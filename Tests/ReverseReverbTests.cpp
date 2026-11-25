#include "../Source/DSP/ReverseReverb.h"
#include <iostream>
#include <stdexcept>

using namespace studdah;
static void require(bool ok, const char* message)
{
    if (!ok) throw std::runtime_error(message);
}
static std::vector<ReverseReverb::Frame> render(int blockSize, bool enabled, int channels)
{
    ReverseReverb effect;
    effect.prepare(1000, channels);
    TempoClock clock;
    clock.prepare(1000);
    ReverseReverbSettings config{enabled, 4, 2.5f, 0.45f, 1.0f, 2.0f};
    std::vector<ReverseReverb::Frame> result;
    for (int start = 0; start < 2500; start += blockSize)
    {
        const int count = std::min(blockSize, 2500 - start);
        HostTransport host;
        host.bpm = 120;
        host.ppq = start * 0.002;
        host.samplePosition = start;
        clock.beginBlock(host, 120, count);
        effect.beginBlock(clock, config);
        for (int i = 0; i < count; ++i)
        {
            const ReverseReverb::Frame input{start + i == 0 ? 1.0f : 0.0f, 0.0f};
            result.push_back(effect.processFrame(input, i));
        }
    }
    return result;
}
int main()
{
    const auto bypass = render(64, false, 2);
    require(bypass[0][0] == 1.0f, "dry impulse");
    for (std::size_t i = 1; i < bypass.size(); ++i)
        require(bypass[i] == ReverseReverb::Frame{}, "disabled exact bypass");
    const auto wet = render(64, true, 2);
    require(wet[0][0] == 1.0f, "parallel dry unchanged");
    bool heard = false, stereo = false;
    for (std::size_t i = 1; i < wet.size(); ++i)
    {
        require(std::isfinite(wet[i][0]) && std::isfinite(wet[i][1]), "finite output");
        if (i < 500) require(wet[i] == ReverseReverb::Frame{}, "causal first grid");
        else if (std::abs(wet[i][0]) > 1.0e-5f) heard = true;
        if (std::abs(wet[i][1]) > 1.0e-5f) stereo = true;
    }
    require(heard && stereo, "audible stereo reversed tail");
    require(wet == render(137, true, 2), "block size independence");
    const auto mono = render(127, true, 1);
    for (const auto& frame : mono) require(frame[1] == 0.0f, "mono isolation");
    std::cout << "ReverseReverb tests passed\n";
}
