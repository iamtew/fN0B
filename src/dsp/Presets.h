#pragma once

// The whole musical brain of fN0B: which preset, and how the one knob maps onto
// that preset's parameters. Deliberately free of JUCE so tests/fn0b_check.cpp can
// compile it with plain cl, the same way the sibling checks do.

namespace fn0b
{

inline constexpr int kPresetCount = 3;

inline const char* const kPresetNames[kPresetCount] = {
    "Warm Drive", "Filter Funk", "Bitcrush Destruct"
};

// One resolved set of unit parameters for the current knob position.
// Unused fields stay at their neutral value, so a preset only fills what it uses.
struct Voice
{
    float driveDb  = 0.f;      // soft-clip input gain
    float cutoffHz = 20000.f;  // low-pass cutoff
    float res      = 0.707f;   // low-pass resonance (SVF: 1/sqrt(2) is flat)
    float crushDiv = 1.f;      // sample-rate divisor, 1 = clean
    float bits     = 16.f;     // bit depth
};

inline float lerp (float a, float b, float t)
{
    return a + (b - a) * t;
}

// knob 0..1 to unit parameters. Monotonic in knob for every preset, which is what
// the check asserts: turning the knob up always moves the sound one direction.
inline Voice voiceFor (int preset, float knob)
{
    const float k = knob < 0.f ? 0.f : (knob > 1.f ? 1.f : knob);
    Voice v;

    switch (preset)
    {
        case 0:  // Warm Drive: clean boost into soft clip, then darker as it warms.
            v.driveDb  = lerp (0.f, 18.f, k);
            v.cutoffHz = lerp (20000.f, 4000.f, k);
            break;

        case 1:  // Filter Funk: resonant sweep, drive rises with the cutoff.
            v.cutoffHz = lerp (200.f, 8000.f, k);
            v.res      = lerp (0.2f, 0.9f, k);
            v.driveDb  = lerp (0.f, 12.f, k);
            break;

        case 2:  // Bitcrush Destruct: sample rate, then bits, then clip.
            v.crushDiv = lerp (1.f, 16.f, k);
            v.bits     = lerp (16.f, 4.f, k);
            v.driveDb  = lerp (0.f, 12.f, k);
            break;

        default:
            break;
    }

    return v;
}

} // namespace fn0b
