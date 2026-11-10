#pragma once

#include <array>
#include <cmath>

// The DSP units fN0B's presets are built from. JUCE-free on purpose so
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
// One instance is one channel; Bitcrush Destruct runs a pair.
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

// One-second circular delay line, one per channel, living inside the Chain so
// the buffer is allocated at prepare time and never on the audio thread.
// Fractional delay with linear interpolation, so knob sweeps bend like tape.
struct Delay
{
    // ~1 s @ 96 kHz, so a 450 ms tape echo still fits at 192 kHz.
    static constexpr int kMaxSamples = 96000;

    void reset()
    {
        buffer.fill (0.f);
        write = 0;
        zDamp = 0.f;
    }

    // Reads the delayed sample, then writes x. For a plain tap, or when the
    // caller builds its own feedback (the shimmer loop does).
    float process (float x)
    {
        const float y = read();
        writeSample (x);
        return y;
    }

    // Tape loop: what goes onto the loop is saturated as a whole (dry input
    // plus damped, regenerated echo), so even the first repeat is colored,
    // like real tape. Returns the echo; the caller mixes dry + echo.
    // `damp` is a one-pole coefficient, 0..1.
    float processLooped (float x, float fb, float driveDb, float damp)
    {
        const float y = read();
        const float damped = zDamp + damp * (y - zDamp);
        zDamp = damped;
        writeSample (softClip (x + damped * fb, driveDb));
        return y;
    }

    // Read offset in samples; the Chain smooths it so turns bend the pitch.
    float readOffset = 0.f;

private:
    float read() const
    {
        // Fractional position is (write - readOffset) modulo the capacity.
        float pos = (float) write - readOffset;
        while (pos < 0.f)
            pos += (float) kMaxSamples;
        const int i0 = (int) pos;
        const int i1 = (i0 + 1) % kMaxSamples;
        const float t = pos - (float) i0;
        return buffer[(size_t) i0] * (1.f - t) + buffer[(size_t) i1] * t;
    }

    void writeSample (float x)
    {
        buffer[write] = x;
        write = (write + 1) % kMaxSamples;
    }

    std::array<float, kMaxSamples> buffer {};
    int write = 0;
    float zDamp = 0.f;
};

} // namespace fn0b
