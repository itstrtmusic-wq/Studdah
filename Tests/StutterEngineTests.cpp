#include "../Source/DSP/StutterEngine.h"
#include <iostream>
#include <stdexcept>

using namespace studdah;
static void require(bool value, const char* description)
{
    if (!value) throw std::runtime_error(description);
}
int main()
{
    CircularAudioBuffer history;
    history.prepare(2, 16000);
    TempoClock clock;
    clock.prepare(1000);
    StutterEngine engine;
    engine.prepare(1000);
    StutterSettings settings{true, 16, NoteModifier::straight, 1.0f, 2, 1.0f};
    // At 120 BPM one sixteenth is 125 samples. Warm history with a ramp.
    HostTransport host;
    host.bpm = 120;
    host.ppq = 0;
    clock.beginBlock(host, 120, 500);
    engine.beginBlock(clock, settings);
    for (int i = 0; i < 500; ++i)
    {
        const StutterEngine::Frame dry{static_cast<float>(i), static_cast<float>(i + 1000)};
        const auto output = engine.processFrame(history, dry, i);
        if (i == 126 || i == 251) require(output[0] == 1 && output[1] == 1001, "repeat stereo slice");
        if (i == 375 + 1) require(output[0] == 251, "recapture after repeats");
        if (i == 125 || i == 249) require(output == dry, "edge fade");
        const float* source[]{&dry[0], &dry[1]};
        history.push(source, 2, 1);
    }
    settings.enabled = false;
    host.ppq = 1;
    clock.beginBlock(host, 120, 8);
    engine.beginBlock(clock, settings);
    for (int i = 0; i < 8; ++i)
    {
        const StutterEngine::Frame dry{0.25f, -0.25f};
        require(engine.processFrame(history, dry, i) == dry, "disabled passthrough");
    }
    // Verify a fractional grid crossing is identical across different callback sizes.
    auto render = [](int blockSize)
    {
        CircularAudioBuffer ring;
        ring.prepare(1, 16000);
        TempoClock time;
        time.prepare(1000);
        StutterEngine effect;
        effect.prepare(1000);
        StutterSettings config{true, 16, NoteModifier::triplet, 0.6f, 3, 2.0f};
        std::vector<float> output;
        for (int start = 0; start < 1000; start += blockSize)
        {
            const int count = std::min(blockSize, 1000 - start);
            HostTransport position;
            position.bpm = 137;
            position.ppq = start * 137.0 / 60000.0;
            position.samplePosition = start;
            time.beginBlock(position, 120, count);
            effect.beginBlock(time, config);
            for (int i = 0; i < count; ++i)
            {
                const StutterEngine::Frame dry{static_cast<float>(start + i) / 1000.0f, 0.0f};
                output.push_back(effect.processFrame(ring, dry, i)[0]);
                const float* source[]{&dry[0]};
                ring.push(source, 1, 1);
            }
        }
        return output;
    };
    require(render(37) == render(256), "block size independence");
    std::cout << "StutterEngine tests passed\n";
}
