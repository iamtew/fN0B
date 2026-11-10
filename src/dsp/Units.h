#pragma once

#include <cmath>

// The three DSP units fN0B's presets are built from. JUCE-free on purpose so
// tests/fn0b_check.cpp can exercise them with plain cl. Nothing here allocates,
// locks, or calls into the system: safe to run on the audio thread.

namespace fn0b
{

// Soft clip with a drive gain in dB. tanh saturates to +-1, so the output can
// never exceed full scale no matter how hard the knob pushes it.
inline float softClip (float x, float driveDb)
{
    const float g = std::pow (10.f, driveDb / 20.f);
    return std::tanh (x * g);
}

// Sample-rate reducer plus bit-depth quantiser, the "digital grit" of the crush
// preset. Hold each input sample for `div` samples, then quantise to `bits` bits.
// One instance is one channel; presets 3 runs a pair.
struct BitCrusher
{
    void reset()
    {
        counter = 0;
        held = 0.f;
    }

    float process (float x, float div, float bits)
    {
        const int d = div < 1.f ? 1 : (int) div;

        if (counter <= 0)
        {
            // Quantise: 16 bits is inaudible, 4 bits is a fuzz pedal.
            const float levels = std::pow (2.f, bits < 1.f ? 1.f : bits);
            const float step = 2.f / levels;
            held = step * std::floor (x / step + 0.5f);
            counter = d;
        }

        --counter;
        return held;
    }

    int counter = 0;
    float held = 0.f;
};

} // namespace fn0b
