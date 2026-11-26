#include "../Source/UI/WaveformHistory.h"
#include <iostream>
#include <stdexcept>
using studdah::WaveformHistory;
static void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
int main()
{
    WaveformHistory history;
    history.prepare(1000);
    for (int i = 0; i < 5; ++i) history.push(-0.5f, 0.75f);
    auto snapshot = history.snapshot();
    require(snapshot.back().low < -0.49f && snapshot.back().high > 0.74f, "peak range");
    require(snapshot.front().low == 0.0f, "startup padding");
    for (int i = 0; i < 512 * 5; ++i) history.push(-2.0f, 2.0f);
    snapshot = history.snapshot();
    require(snapshot.front().low == -1.0f && snapshot.back().high == 1.0f, "wrap and clipping");
    history.prepare(48000);
    require(history.snapshot().back().high == 0.0f, "prepare clears");
    std::cout << "WaveformHistory tests passed\n";
}
