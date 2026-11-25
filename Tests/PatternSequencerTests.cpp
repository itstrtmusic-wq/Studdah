#include "../Source/Core/Transport/PatternSequencer.h"
#include "../Source/DSP/StutterEngine.h"
#include "../Source/DSP/ReverseReverb.h"
#include <iostream>
#include <stdexcept>
using namespace studdah;
static void require(bool ok, const char* message)
{
    if (!ok) throw std::runtime_error(message);
}
int main()
{
    TempoClock clock;
    clock.prepare(1000);
    PatternSequencer sequence;
    PatternSettings settings;
    settings.enabled = true;
    settings.length = 4;
    settings.steps = {1, 2, 4, 7};
    HostTransport host;
    host.bpm = 120;
    host.ppq = 0;
    clock.beginBlock(host, 120, 625);
    sequence.beginBlock(clock, settings);
    require(sequence.at(0).stutter() && !sequence.at(0).reverse(), "first stutter");
    require(sequence.at(124).index == 0 && sequence.at(125).index == 1, "precise boundary");
    require(sequence.at(125).reverse() && sequence.at(125).stutter(), "reverse lane implies slice");
    require(sequence.at(250).swell() && !sequence.at(250).stutter(), "swell alone");
    require(sequence.at(375).mask == 7 && sequence.at(500).index == 0, "loop");
    host.ppq = -0.25;
    clock.beginBlock(host, 120, 125);
    sequence.beginBlock(clock, settings);
    require(sequence.at(0).index == 3, "negative PPQ");
    host.playing = false;
    clock.beginBlock(host, 120, 125);
    sequence.beginBlock(clock, settings);
    require(sequence.at(0).index == -1 && sequence.at(0).mask == 0, "stopped");
    host.playing = true;
    settings.enabled = false;
    clock.beginBlock(host, 120, 125);
    sequence.beginBlock(clock, settings);
    require(sequence.at(0).index == -1, "disabled");
    // Exercise the actual per-sample routing across host block boundaries.
    auto render = [](int blockSize)
    {
        CircularAudioBuffer history;
        history.prepare(1, 16000);
        TempoClock time;
        time.prepare(1000);
        PatternSequencer pattern;
        StutterEngine stutter;
        stutter.prepare(1000);
        ReverseReverb reverb;
        reverb.prepare(1000, 1);
        PatternSettings config;
        config.enabled = true;
        config.length = 4;
        config.steps = {0, reverseLane, 0, swellLane};
        StutterSettings chop{false, 16, NoteModifier::straight, 1.0f, 2, 1.0f};
        chop.patternControlled = true;
        ReverseReverbSettings swell{false, 16, 2.5f, 0.45f, 1.0f, 2.0f};
        swell.patternControlled = true;
        std::vector<float> out;
        for (int start = 0; start < 500; start += blockSize)
        {
            const int count = std::min(blockSize, 500 - start);
            HostTransport hostPosition;
            hostPosition.bpm = 120;
            hostPosition.ppq = start * 0.002;
            hostPosition.samplePosition = start;
            time.beginBlock(hostPosition, 120, count);
            pattern.beginBlock(time, config);
            stutter.beginBlock(time, chop);
            reverb.beginBlock(time, swell);
            for (int i = 0; i < count; ++i)
            {
                const auto cell = pattern.at(i);
                stutter.setPatternGate(cell.stutter(), cell.reverse() ? PlaybackDirection::reverse : PlaybackDirection::forward);
                reverb.setPatternGate(cell.swell());
                const StutterEngine::Frame input{static_cast<float>(start + i) / 1000.0f, 0.0f};
                const auto processed = stutter.processFrame(history, input, i);
                out.push_back(reverb.processFrame(processed, i)[0]);
                const float* source[]{&input[0]};
                history.push(source, 1, 1);
            }
        }
        return out;
    };
    const auto audio = render(37);
    require(audio == render(256), "effect routing independent of callback size");
    require(audio[100] == 0.1f && audio[300] == 0.3f, "empty steps passthrough");
    require(std::abs(audio[126] - 0.123f) < 1.0e-6f, "reverse step audible");
    require(std::abs(audio[450] - 0.45f) > 1.0e-5f, "swell step audible");
    std::cout << "PatternSequencer tests passed\n";
}
