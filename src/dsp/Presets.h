#pragma once

// The whole musical brain of fN0B: which preset, and how the one knob maps onto
// that preset's parameters. Deliberately free of JUCE so tests/fn0b_check.cpp can
// compile it with plain cl, the same way the sibling checks do.

namespace fn0b
{

inline constexpr int kPresetCount = 6;

inline const char* const kPresetNames[kPresetCount] = {
    "Warm Drive", "Filter Funk", "Bitcrush Destruct", "Tape Echo", "Shimmer Verb", "Phaser Sweep"
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
    float delayMs  = 0.f;      // delay-line length in milliseconds
    float feedback = 0.f;      // delay/phaser regeneration, 0..1
    float mix      = 0.f;      // wet amount, 0..1 (0 = full dry, effects-only chain)
    float reverbSize = 0.f;    // juce Reverb roomSize, 0..1
    float shimmer    = 0.f;    // octave-up feedback level, 0..1
    float phaserRateHz = 0.f;  // 0 = phaser off
    float phaserDepth  = 0.f;  // 0..1
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

        case 3:  // Tape Echo: longer, denser, sadder repeats as the knob turns.
            v.delayMs  = lerp (60.f, 450.f, k);
            v.feedback = lerp (0.15f, 0.6f, k);
            v.driveDb  = lerp (0.f, 6.f, k);
            v.cutoffHz = lerp (12000.f, 2500.f, k);
            break;

        case 4:  // Shimmer Verb: room grows, then the octave bloom rises.
            v.reverbSize = lerp (0.15f, 1.f, k);
            v.mix        = lerp (0.1f, 0.45f, k);
            v.shimmer    = lerp (0.f, 0.5f, k);
            v.cutoffHz   = lerp (20000.f, 6000.f, k);
            break;

        case 5:  // Phaser Sweep: faster and deeper, the jet plane takes off.
            v.phaserRateHz  = lerp (0.12f, 3.5f, k);
            v.phaserDepth   = lerp (0.2f, 1.f, k);
            v.feedback      = lerp (0.1f, 0.7f, k);
            break;

        default:
            break;
    }

    return v;
}

} // namespace fn0b
