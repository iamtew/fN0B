#pragma once

#include <array>
#include <cmath>
#include <cstdint>

// The DSP units fN0B's presets are built from. JUCE-free on purpose so
// tests/fn0b_check.cpp can exercise them with plain cl. Nothing here allocates,
// locks, or calls into the system: safe to run on the audio thread.
// Sound first: these exist because they sound better (or more tunable) than
// the stock JUCE boxes for this plugin.

namespace fn0b
{

inline constexpr float kPi    = 3.14159265f;
inline constexpr float kTwoPi = 6.2831853f;

inline float clamp01 (float x)
{
    return x < 0.f ? 0.f : (x > 1.f ? 1.f : x);
}

inline float dbToGain (float db)
{
    return std::pow (10.f, db / 20.f);
}

// Soft clip with a drive gain in dB. tanh saturates to +-1, so the output can
// never exceed full scale no matter how hard the knob pushes it.
inline float softClip (float x, float driveDb)
{
    return std::tanh (x * dbToGain (driveDb));
}

inline float hardClip (float x)
{
    return x < -1.f ? -1.f : (x > 1.f ? 1.f : x);
}

// Mild fold: drive scales how many times the wave wraps. Loop is bounded.
inline float waveFold (float x, float amt)
{
    float y = x * (1.f + clamp01 (amt) * 8.f);
    for (int n = 0; n < 8; ++n)
    {
        if (y > 1.f)
            y = 2.f - y;
        else if (y < -1.f)
            y = -2.f - y;
        else
            break;
    }
    return y;
}

inline uint32_t rngNext (uint32_t& s)
{
    s = s * 1664525u + 1013904223u;
    return s;
}

inline float noiseUni (uint32_t& s)
{
    return (float) rngNext (s) * (1.f / 2147483648.f) - 1.f;
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

    float read() const
    {
        float pos = (float) write - readOffset;
        while (pos < 0.f)
            pos += (float) kMaxSamples;
        const int i0 = (int) pos % kMaxSamples;
        const int i1 = (i0 + 1) % kMaxSamples;
        const float t = pos - std::floor (pos);
        return buffer[(size_t) i0] * (1.f - t) + buffer[(size_t) i1] * t;
    }

    void writeSample (float x)
    {
        buffer[(size_t) write] = x;
        write = (write + 1) % kMaxSamples;
    }

    std::array<float, kMaxSamples> buffer {};
    int write = 0;
    float zDamp = 0.f;
};

// Eight-line Householder FDN. Denser and less metallic than juce::dsp::Reverb
// (Freeverb). Size stretches the taps and the decay; damp is a one-pole in the
// feedback; shimmer injects an octave-up copy into the tank, loop-gain capped
// so the bloom dies. Mix 0 is dry. Fixed buffers, never allocated on the
// audio thread. capN is set from sample rate (prepare).
struct FdReverb
{
    static constexpr int kN = 8;
    static constexpr int kMaxLine = 48000;   // 0.25 s @ 192 kHz
    static constexpr int kShMax   = 16384;
    static constexpr float kMs[kN] = { 29.f, 37.f, 43.f, 53.f, 61.f, 73.f, 83.f, 97.f };

    void setSampleRate (double sampleRate)
    {
        sr = sampleRate > 1.0 ? sampleRate : 44100.0;
        capN = (int) (sr * 0.22) + 4;
        if (capN > kMaxLine)
            capN = kMaxLine;
        if (capN < 8)
            capN = 8;
        shCap = (int) (sr * 0.09) + 8;
        if (shCap > kShMax)
            shCap = kShMax;
        if (shCap < 8)
            shCap = 8;
        reset();
    }

    void reset()
    {
        for (int i = 0; i < kN; ++i)
        {
            buf[i].fill (0.f);
            w[i] = 0;
            lp[i] = 0.f;
        }
        for (int c = 0; c < 2; ++c)
        {
            shBuf[c].fill (0.f);
            shW[c] = 0;
            shOff[c] = (float) (shCap / 2);
            zSh[c] = 0.f;
        }
    }

    // decay01 extra: 0 keeps size-linked rt60, 1 stretches toward ~20 s (Event Horizon).
    void process (float* L, float* R, int n, float size, float damp, float mix,
                  float shimmer = 0.f, float decay01 = 0.f, float freeze = 0.f)
    {
        if (n <= 0 || capN <= 2)
            return;
        size = clamp01 (size);
        damp = clamp01 (damp);
        mix = clamp01 (mix);
        const float shAmt = clamp01 (shimmer) * 0.45f;
        const float frz = clamp01 (freeze);
        const float scale = 0.38f + 0.62f * size;
        const float rt60 = (0.4f + 7.6f * size) * (1.f + 1.5f * clamp01 (decay01))
                         + 12.f * frz;
        const float lpCoef = 0.08f + 0.84f * (1.f - damp);
        const float dryG = 1.f - 0.5f * mix;
        const float maxD = (float) capN - 2.f;
        const float inGain = 1.f - 0.85f * frz; // freeze: stop feeding, hold the tank

        for (int s = 0; s < n; ++s)
        {
            float y[kN];
            float sum = 0.f;
            for (int i = 0; i < kN; ++i)
            {
                const float dly = kMs[i] * 0.001f * scale * (float) sr;
                y[i] = read (i, dly < 1.f ? 1.f : (dly > maxD ? maxD : dly));
                sum += y[i];
            }
            const float house = sum * (2.f / (float) kN);
            const float inL = L[s], inR = R[s];
            float outL = 0.f, outR = 0.f;
            for (int i = 0; i < kN; ++i)
            {
                const float delaySec = kMs[i] * 0.001f * scale;
                const float g = std::pow (0.001f, delaySec / rt60);
                const float inj = ((i & 1) != 0 ? inR : inL) * 0.25f * inGain
                                + ((i & 1) != 0 ? zSh[1] : zSh[0]) * shAmt;
                lp[i] += lpCoef * ((y[i] - house) * g - lp[i]);
                write (i, inj + lp[i]);
                if ((i & 1) != 0)
                    outR += y[i];
                else
                    outL += y[i];
            }
            const float wetL = outL * 0.25f, wetR = outR * 0.25f;
            L[s] = dryG * inL + mix * wetL;
            R[s] = dryG * inR + mix * wetR;
            if (shAmt > 0.f && shCap > 4)
            {
                const float shLp = 0.15f + 0.7f * (1.f - damp);
                zSh[0] = shLp * zSh[0] + (1.f - shLp) * octaveUp (0, wetL);
                zSh[1] = shLp * zSh[1] + (1.f - shLp) * octaveUp (1, wetR);
            }
        }
    }

private:
    float read (int i, float delaySamp) const
    {
        float rp = (float) w[i] - delaySamp;
        while (rp < 0.f)
            rp += (float) capN;
        const int i0 = (int) rp;
        const int i1 = i0 + 1 < capN ? i0 + 1 : 0;
        const float t = rp - (float) i0;
        return buf[i][(size_t) i0] * (1.f - t) + buf[i][(size_t) i1] * t;
    }

    void write (int i, float x)
    {
        buf[i][(size_t) w[i]] = x;
        if (++w[i] >= capN)
            w[i] = 0;
    }

    float octaveUp (int c, float x)
    {
        shBuf[c][(size_t) shW[c]] = x;
        if (++shW[c] >= shCap)
            shW[c] = 0;
        shOff[c] -= 1.f;
        if (shOff[c] < 1.f)
            shOff[c] += (float) shCap;
        float rp = (float) shW[c] - shOff[c];
        while (rp < 0.f)
            rp += (float) shCap;
        const int i0 = (int) rp;
        const int i1 = i0 + 1 < shCap ? i0 + 1 : 0;
        const float t = rp - (float) i0;
        return shBuf[c][(size_t) i0] * (1.f - t) + shBuf[c][(size_t) i1] * t;
    }

    std::array<std::array<float, kMaxLine>, kN> buf {};
    int w[kN] {};
    float lp[kN] {};
    int capN = 8;
    std::array<std::array<float, kShMax>, 2> shBuf {};
    int shW[2] {};
    float shOff[2] {};
    float zSh[2] {};
    int shCap = 8;
    double sr = 44100.0;
};

// Six first-order allpasses, LFO around centreHz, stereo phase offset.
struct Phaser
{
    static constexpr int kStages = 6;

    void setSampleRate (double sampleRate)
    {
        sr = sampleRate > 1.0 ? sampleRate : 44100.0;
        reset();
    }

    void reset()
    {
        phase = 0.f;
        for (int c = 0; c < 2; ++c)
        {
            fbZ[c] = 0.f;
            for (int s = 0; s < kStages; ++s)
                z[c][s] = 0.f;
        }
    }

    void process (float* L, float* R, int n, float rateHz, float depth, float feedback,
                  float centreHz = 800.f)
    {
        depth = clamp01 (depth);
        rateHz = rateHz < 0.01f ? 0.01f : rateHz;
        const float dph = kTwoPi * rateHz / (float) sr;
        const float ny = (float) sr * 0.45f;
        const float fb = feedback < 0.f ? 0.f : (feedback > 0.85f ? 0.85f : feedback);
        float* ch[2] = { L, R };
        for (int i = 0; i < n; ++i)
        {
            phase += dph;
            if (phase > kTwoPi)
                phase -= kTwoPi;
            for (int c = 0; c < 2; ++c)
            {
                const float lfo = 0.5f + 0.5f * std::sin (phase + (c != 0 ? 1.5707963f : 0.f));
                float fc = centreHz * std::exp2 ((lfo - 0.5f) * depth * 3.f);
                if (fc < 40.f) fc = 40.f;
                if (fc > ny) fc = ny;
                const float t = std::tan (fc * (kPi / (float) sr));
                const float a = (1.f - t) / (1.f + t);
                float x = ch[c][i] + fbZ[c] * fb;
                for (int s = 0; s < kStages; ++s)
                {
                    const float y = a * x + z[c][s];
                    z[c][s] = x - a * y;
                    x = y;
                }
                fbZ[c] = x;
                ch[c][i] = 0.5f * (ch[c][i] + x);
            }
        }
    }

    float z[2][kStages] {};
    float fbZ[2] {};
    float phase = 0.f;
    double sr = 44100.0;
};

// Peak compressor with instant-ish attack and auto makeup so Velvet Damage
// gets denser, not just louder.
struct PeakComp
{
    void setSampleRate (double sampleRate)
    {
        const float s = (float) (sampleRate > 1.0 ? sampleRate : 44100.0);
        atk = std::exp (-1.f / (0.003f * s));
        rel = std::exp (-1.f / (0.08f * s));
        env = 0.f;
    }

    void reset() { env = 0.f; }

    float process (float x, float thresh, float ratio)
    {
        const float p = x < 0.f ? -x : x;
        env = p > env ? atk * env + (1.f - atk) * p : rel * env + (1.f - rel) * p;
        const float r = ratio < 1.f ? 1.f : ratio;
        float g = 1.f;
        if (env > thresh && thresh > 1e-6f)
            g = (thresh + (env - thresh) / r) / env;
        const float makeup = std::sqrt (r) * 0.35f + 0.65f;
        return x * g * makeup;
    }

    float env = 0.f;
    float atk = 0.f;
    float rel = 0.f;
};

// One-pole used for tilt / shelf / wow smoothing.
struct OnePole
{
    void reset() { z = 0.f; }
    float process (float x, float a)
    {
        z += a * (x - z);
        return z;
    }
    float z = 0.f;
};

// Three parallel bandpasses that morph across A E I O U. Cheap talking filter.
struct Formant
{
    void reset()
    {
        for (int i = 0; i < 3; ++i)
            bp[i] = 0.f, z1[i] = 0.f, z2[i] = 0.f;
    }

    float process (float x, float morph, float q, double sr)
    {
        morph = clamp01 (morph);
        const float t = morph * 4.f;
        const int i0 = (int) t;
        const int i1 = i0 >= 4 ? 4 : i0 + 1;
        const float f = t - (float) i0;
        float y = 0.f;
        const float ny = (float) sr * 0.45f;
        for (int b = 0; b < 3; ++b)
        {
            float hz = kVowel[i0][b] + (kVowel[i1][b] - kVowel[i0][b]) * f;
            if (hz > ny) hz = ny;
            if (hz < 80.f) hz = 80.f;
            const float w = kTwoPi * hz / (float) sr;
            const float r = 1.f - kPi * hz / ((q < 1.f ? 1.f : q) * (float) sr);
            const float a = (1.f - r) * std::sin (w);
            const float o = a * x + 2.f * r * std::cos (w) * z1[b] - r * r * z2[b];
            z2[b] = z1[b];
            z1[b] = o;
            y += o;
        }
        return y * 0.45f;
    }

    // A E I O U, F1 F2 F3
    static constexpr float kVowel[5][3] = {
        { 800.f, 1150.f, 2800.f },
        { 400.f, 2000.f, 2550.f },
        { 350.f, 2000.f, 2800.f },
        { 450.f,  800.f, 2830.f },
        { 325.f,  700.f, 2530.f }
    };
    float bp[3] {}, z1[3] {}, z2[3] {};
};

// Circular freeze/scramble buffer. Writes until freeze; reads a window whose
// start jumps with scramble. Grain is the character, not a workstation.
struct Grain
{
    static constexpr int kMax = 48000;

    void reset()
    {
        buf.fill (0.f);
        w = 0;
        frozen = false;
    }

    void write (float x, bool freeze)
    {
        if (! freeze)
        {
            buf[(size_t) w] = x;
            w = (w + 1) % kMax;
        }
        frozen = freeze;
    }

    float read (float size01, float scramble, uint32_t& rng) const
    {
        const int n = 64 + (int) (clamp01 (size01) * (float) (kMax / 4));
        int origin = w - n;
        while (origin < 0)
            origin += kMax;
        if (scramble > 0.001f && (rngNext (rng) & 0xff) < (uint32_t) (clamp01 (scramble) * 40.f))
        {
            origin = (int) (rngNext (rng) % (uint32_t) kMax);
        }
        const int idx = origin % kMax;
        return buf[(size_t) idx];
    }

    // Sequential play through the window, used when scramble is low.
    float tap (int offset) const
    {
        int i = w - 1 - (offset % kMax);
        while (i < 0)
            i += kMax;
        return buf[(size_t) i];
    }

    std::array<float, kMax> buf {};
    int w = 0;
    bool frozen = false;
};

// Ping-pong reverse window: record into one half, play the previous half backwards.
struct Reverse
{
    static constexpr int kMax = 48000;

    void reset()
    {
        a.fill (0.f);
        b.fill (0.f);
        w = 0;
        recA = true;
        nWin = 2048;
    }

    float process (float x, int win)
    {
        if (win < 64) win = 64;
        if (win > kMax) win = kMax;
        nWin = win;
        float* rec = recA ? a.data() : b.data();
        float* ply = recA ? b.data() : a.data();
        rec[(size_t) w] = x;
        const float y = ply[(size_t) (nWin - 1 - w)];
        if (++w >= nWin)
        {
            w = 0;
            recA = ! recA;
        }
        return y;
    }

    std::array<float, kMax> a {}, b {};
    int w = 0;
    int nWin = 2048;
    bool recA = true;
};

} // namespace fn0b
