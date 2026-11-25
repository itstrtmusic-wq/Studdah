#include "../Source/DSP/CircularAudioBuffer.h"
#include <array>
#include <iostream>
#include <stdexcept>

static void require(bool condition, const char* description)
{
    if (!condition) throw std::runtime_error(description);
}

int main()
{
    studdah::CircularAudioBuffer history;
    history.prepare(2, 4);
    std::array<float, 6> left{1, 2, 3, 4, 5, 6}, right{11, 12, 13, 14, 15, 16};
    const float* input[]{left.data(), right.data()};
    std::array<float, 4> result{};
    history.push(input, 2, 2);
    history.copyRecent(0, result.data(), 4);
    require(result == std::array<float, 4>{0, 0, 1, 2}, "startup padding");
    history.copyRecent(0, result.data(), 4, true);
    require(result == std::array<float, 4>{2, 1, 0, 0}, "reverse startup");
    const float* rest[]{left.data() + 2, right.data() + 2};
    history.push(rest, 2, 4);
    require(history.beginPosition() == 2 && history.endPosition() == 6, "wrap positions");
    require(history.read(0, 1) == 0 && history.read(0, 6) == 0, "invalid positions");
    history.copyRecent(0, result.data(), 4);
    require(result == std::array<float, 4>{3, 4, 5, 6}, "forward wrap");
    history.copyRecent(1, result.data(), 4, true);
    require(result == std::array<float, 4>{16, 15, 14, 13}, "stereo reverse wrap");
    history.push(input, 2, 6);
    history.copyRecent(0, result.data(), 4);
    require(result == std::array<float, 4>{3, 4, 5, 6}, "oversized block");
    history.reset();
    require(history.available() == 0 && history.read(0, 0) == 0, "reset");
    history.prepare(1, 2);
    history.push(input, 1, 2);
    require(history.read(0, 1) == 2 && history.read(1, 1) == 0, "mono reprepare");
    std::cout << "CircularAudioBuffer tests passed\n";
}
