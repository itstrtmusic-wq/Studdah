#include "../Source/Core/Transport/TempoClock.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace studdah;
static void require(bool value, const char* message)
{
    if (!value) throw std::runtime_error(message);
}
static bool near(double a, double b) { return std::abs(a-b) < 1.0e-6; }
int main()
{
    TempoClock clock;
    clock.prepare(48000);
    HostTransport host;
    host.bpm = 120;
    host.ppq = 0;
    host.samplePosition = 0;
    clock.beginBlock(host, 90, 240);
    require(clock.hasHostTempo() && clock.hasHostPosition(), "host values");
    require(near(clock.samplesForDivision(4), 24000), "quarter");
    require(near(clock.samplesForDivision(16), 6000), "sixteenth");
    require(near(clock.samplesForDivision(16, NoteModifier::dotted), 9000), "dotted");
    require(near(clock.samplesForDivision(16, NoteModifier::triplet), 4000), "triplet");
    require(clock.nextGridOffset(0.25, 0, 240) == 0, "on grid");
    require(!clock.nextGridOffset(0.25, 1, 240), "outside block");
    host.ppq = 0.01; host.samplePosition = 240;
    clock.beginBlock(host, 90, 240);
    require(!clock.hasDiscontinuity(), "continuous transport");
    host.ppq = 0.24; host.samplePosition = 5760;
    clock.beginBlock(host, 90, 512);
    require(clock.hasDiscontinuity() && clock.nextGridOffset(0.25, 0, 512) == 240, "seek and crossing");
    host.playing = false;
    clock.beginBlock(host, 90, 512);
    require(!clock.nextGridOffset(0.25, 0, 512), "stopped");
    host.playing = true; host.bpm = 60;
    clock.beginBlock(host, 90, 512);
    require(clock.hasDiscontinuity() && near(clock.samplesForDivision(4), 48000), "restart and tempo change");
    clock.prepare(48000);
    clock.beginBlock({}, 90, 480);
    require(!clock.hasHostTempo() && near(clock.bpm(), 90), "fallback tempo");
    clock.beginBlock({}, 90, 480);
    require(near(clock.ppqAtBlockStart(), 0.015), "fallback phase");
    std::cout << "TempoClock tests passed\n";
}
