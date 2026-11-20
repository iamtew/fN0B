// JUCE-free check for the parts of fN0B that carry the musical decisions:
// the knob-to-parameter mapping and the two custom DSP units. Compiled with plain
// cl, no JUCE include paths, the same way the sibling checks work.
#include "dsp/Presets.h"
#include "dsp/Units.h"

#include <cassert>
#include <algorithm>
#include <cmath>
#include <cstdio>

int main()
{
    // Every preset's mapping stays in range and never goes NaN across the sweep.
    for (int p = 0; p < fn0b::kPresetCount; ++p)
    {
        float prevDrive = -1.f, prevCut = -1.f;
        float prevDelay = -1.f, prevFb = -1.f, prevMix = -1.f, prevSize = -1.f;
        float prevShim = -1.f, prevRate = -1.f, prevDepth = -1.f;
        for (int i = 0; i <= 100; ++i)
        {
            const auto v = fn0b::voiceFor (p, (float) i / 100.f);
            assert (std::isfinite (v.driveDb) && std::isfinite (v.cutoffHz)
                    && std::isfinite (v.res) && std::isfinite (v.crushDiv) && std::isfinite (v.bits)
                    && std::isfinite (v.delayMs) && std::isfinite (v.delayMs2)
                    && std::isfinite (v.feedback) && std::isfinite (v.mix)
                    && std::isfinite (v.reverbSize) && std::isfinite (v.shimmer)
                    && std::isfinite (v.phaserRateHz) && std::isfinite (v.phaserDepth)
                    && std::isfinite (v.chorusRateHz) && std::isfinite (v.hpHz)
                    && std::isfinite (v.pitch) && std::isfinite (v.freeze)
                    && std::isfinite (v.driveHi) && std::isfinite (v.ringHz));
            if (p < fn0b::kFirstBipolar)
                assert (v.driveDb >= 0.f && v.driveDb <= 24.f);
            else
                assert (v.driveDb >= -1.f && v.driveDb <= 24.f);
            assert (v.cutoffHz >= 20.f && v.cutoffHz <= 20000.f);
            assert (v.res > 0.f && v.res <= 1.f);
            assert (v.crushDiv >= 1.f && v.bits >= 4.f && v.bits <= 16.f);
            assert (v.delayMs >= 0.f && v.delayMs <= 500.f);
            if (p < fn0b::kFirstBipolar)
                assert (v.feedback >= 0.f && v.feedback <= 0.9f);
            else
                assert (v.feedback >= -0.9f && v.feedback <= 0.9f);
            assert (v.mix >= 0.f && v.mix <= 1.f);
            assert (v.reverbSize >= 0.f && v.reverbSize <= 1.f);
            assert (v.shimmer >= 0.f && v.shimmer <= 0.5f);
            assert (v.phaserRateHz >= 0.f && v.phaserRateHz <= 3.5f);
            assert (v.phaserDepth >= 0.f && v.phaserDepth <= 1.f);
            assert (v.freeze >= 0.f && v.freeze <= 1.f);

            // Drive is non-decreasing in the knob for unipolar presets.
            if (p < fn0b::kFirstBipolar)
                assert (v.driveDb + 1e-4f >= prevDrive);
            prevDrive = v.driveDb;

            // Cutoff moves one way only. Closers darken, 1 and 19 open.
            if (p == 0 || p == 3 || p == 4 || p == 11 || p == 13 || p == 18 || p == 28)
                assert (v.cutoffHz <= prevCut + 1e-3f || prevCut < 0.f);
            else if (p == 1 || p == 19)
                assert (v.cutoffHz + 1e-3f >= prevCut);
            prevCut = v.cutoffHz;

            // Preset 3: delay grows, feedback rises.
            if (p == 3)
            {
                assert (v.delayMs + 1e-3f >= prevDelay);
                assert (v.feedback + 1e-4f >= prevFb);
            }
            // Preset 4: room grows, mix and shimmer rise.
            if (p == 4)
            {
                assert (v.reverbSize + 1e-4f >= prevSize);
                assert (v.mix + 1e-4f >= prevMix);
                assert (v.shimmer + 1e-4f >= prevShim);
            }
            // Preset 5: the sweep speeds up and deepens.
            if (p == 5)
            {
                assert (v.phaserRateHz + 1e-4f >= prevRate);
                assert (v.phaserDepth + 1e-4f >= prevDepth);
                assert (v.feedback + 1e-4f >= prevFb);
            }
            prevDelay = v.delayMs;
            prevFb = v.feedback;
            prevMix = v.mix;
            prevSize = v.reverbSize;
            prevShim = v.shimmer;
            prevRate = v.phaserRateHz;
            prevDepth = v.phaserDepth;
        }
    }

    // Endpoints match the preset tables.
    assert (std::abs (fn0b::voiceFor (0, 0.f).driveDb - 0.f) < 1e-4f);
    assert (std::abs (fn0b::voiceFor (0, 1.f).driveDb - 18.f) < 1e-4f);
    assert (std::abs (fn0b::voiceFor (0, 0.f).cutoffHz - 20000.f) < 1e-1f);
    assert (std::abs (fn0b::voiceFor (0, 1.f).cutoffHz - 4000.f) < 1e-1f);
    assert (std::abs (fn0b::voiceFor (1, 0.f).cutoffHz - 200.f) < 1e-1f);
    assert (std::abs (fn0b::voiceFor (1, 1.f).cutoffHz - 8000.f) < 1e-1f);
    assert (std::abs (fn0b::voiceFor (2, 0.f).cutoffHz - 20000.f) < 1e-1f);
    assert (std::abs (fn0b::voiceFor (2, 0.f).crushDiv - 1.f) < 1e-4f);
    assert (std::abs (fn0b::voiceFor (2, 1.f).crushDiv - 16.f) < 1e-3f);
    assert (std::abs (fn0b::voiceFor (3, 0.f).delayMs - 60.f) < 1e-2f);
    assert (std::abs (fn0b::voiceFor (3, 1.f).delayMs - 450.f) < 1e-2f);
    assert (std::abs (fn0b::voiceFor (3, 0.f).feedback - 0.15f) < 1e-4f);
    assert (std::abs (fn0b::voiceFor (3, 1.f).feedback - 0.6f) < 1e-4f);
    assert (std::abs (fn0b::voiceFor (3, 1.f).cutoffHz - 2500.f) < 1e-1f);
    assert (std::abs (fn0b::voiceFor (4, 0.f).reverbSize - 0.15f) < 1e-4f);
    assert (std::abs (fn0b::voiceFor (4, 1.f).reverbSize - 1.f) < 1e-4f);
    assert (std::abs (fn0b::voiceFor (4, 1.f).mix - 0.45f) < 1e-4f);
    assert (std::abs (fn0b::voiceFor (4, 1.f).shimmer - 0.5f) < 1e-4f);
    assert (std::abs (fn0b::voiceFor (5, 0.f).phaserRateHz - 0.12f) < 1e-3f);
    assert (std::abs (fn0b::voiceFor (5, 1.f).phaserRateHz - 3.5f) < 1e-3f);
    assert (std::abs (fn0b::voiceFor (5, 1.f).phaserDepth - 1.f) < 1e-4f);
    assert (std::abs (fn0b::voiceFor (6, 1.f).width - 1.6f) < 1e-4f);
    assert (std::abs (fn0b::voiceFor (10, 1.f).driveDb - 18.f) < 1e-4f);
    assert (std::abs (fn0b::voiceFor (11, 1.f).chorusDepth - 0.7f) < 1e-4f);
    assert (std::abs (fn0b::voiceFor (12, 1.f).combMs - 45.f) < 1e-3f);
    assert (std::abs (fn0b::voiceFor (17, 1.f).ratio - 8.f) < 1e-4f);
    assert (std::abs (fn0b::voiceFor (19, 1.f).driveHi - 30.f) < 1e-4f);
    assert (std::abs (fn0b::voiceFor (20, 1.f).width - 1.8f) < 1e-4f);
    assert (fn0b::voiceFor (21, 1.f).pitch < -10.f);
    assert (fn0b::voiceFor (30, 1.f).freeze > 0.7f);
    assert (fn0b::kPresetCount == 41);
    assert (fn0b::kPresetNames[30][0] == 'S');
    assert (fn0b::kPresetNames[40][0] == 'S');
    assert (! fn0b::isBipolar (30));
    assert (fn0b::isBipolar (31));
    assert (std::abs (fn0b::signedThrow (0.5f)) < 1e-6f);
    assert (std::abs (fn0b::signedThrow (1.f) - 1.f) < 1e-6f);
    assert (std::abs (fn0b::mag (-0.4f) - 0.4f) < 1e-6f);

    // Bipolar noon is rest (Halo Rail keeps a light chorus).
    assert (std::abs (fn0b::voiceFor (31, 0.5f).cutoffHz - 20000.f) < 1.f);
    assert (std::abs (fn0b::voiceFor (31, 0.5f).hpHz - 20.f) < 0.1f);
    assert (std::abs (fn0b::voiceFor (31, 0.f).cutoffHz - 200.f) < 1.f);
    assert (std::abs (fn0b::voiceFor (31, 1.f).hpHz - 2500.f) < 1.f);
    assert (std::abs (fn0b::voiceFor (32, 0.5f).tilt) < 1e-4f);
    assert (std::abs (fn0b::voiceFor (32, 0.f).tilt + 8.f) < 1e-4f);
    assert (std::abs (fn0b::voiceFor (32, 1.f).tilt - 8.f) < 1e-4f);
    assert (std::abs (fn0b::voiceFor (33, 0.5f).width - 1.f) < 1e-4f);
    assert (std::abs (fn0b::voiceFor (33, 0.f).width) < 1e-4f);
    assert (std::abs (fn0b::voiceFor (33, 1.f).width - 1.8f) < 1e-4f);
    assert (std::abs (fn0b::voiceFor (34, 0.5f).mix) < 1e-4f);
    assert (fn0b::voiceFor (34, 0.f).pitch < -11.f);
    assert (fn0b::voiceFor (34, 1.f).pitch > 11.f);
    assert (std::abs (fn0b::voiceFor (35, 0.5f).mix) < 1e-4f);
    assert (std::abs (fn0b::voiceFor (36, 0.5f).delayMs - 12.f) < 1e-3f);
    assert (std::abs (fn0b::voiceFor (36, 0.5f).mix - 0.2f) < 1e-4f);
    assert (std::abs (fn0b::voiceFor (36, 0.5f).chorusDepth - 0.12f) < 1e-4f);
    assert (std::abs (fn0b::voiceFor (37, 0.5f).feedback) < 1e-4f);
    assert (std::abs (fn0b::voiceFor (37, 0.f).feedback + 0.7f) < 1e-4f);
    assert (std::abs (fn0b::voiceFor (37, 1.f).feedback - 0.7f) < 1e-4f);
    assert (std::abs (fn0b::voiceFor (38, 0.5f).bits - 16.f) < 1e-4f);
    assert (std::abs (fn0b::voiceFor (38, 0.f).bits - 6.f) < 1e-4f);
    assert (std::abs (fn0b::voiceFor (38, 1.f).excite - 0.55f) < 1e-4f);
    assert (std::abs (fn0b::voiceFor (39, 0.5f).ratio - 1.f) < 1e-4f);
    assert (std::abs (fn0b::voiceFor (39, 0.5f).mix) < 1e-4f);
    assert (std::abs (fn0b::voiceFor (39, 0.f).ratio - 8.f) < 1e-4f);
    assert (std::abs (fn0b::voiceFor (39, 1.f).mix - 0.45f) < 1e-4f);
    assert (std::abs (fn0b::voiceFor (40, 0.5f).driveDb) < 1e-4f);
    assert (std::abs (fn0b::voiceFor (40, 0.5f).shape) < 1e-4f);
    assert (std::abs (fn0b::voiceFor (40, 0.f).driveDb - 18.f) < 1e-4f);
    assert (std::abs (fn0b::voiceFor (40, 1.f).shape - 0.8f) < 1e-4f);

    // The knob is clamped, so out-of-range input cannot produce a wild parameter.
    assert (std::abs (fn0b::voiceFor (1, -5.f).cutoffHz - 200.f) < 1e-3f);
    assert (std::abs (fn0b::voiceFor (1, 9.f).cutoffHz - 8000.f) < 1e-1f);

    // Soft clip: bounded, odd, and monotone in drive.
    {
        assert (std::abs (fn0b::softClip (0.f, 12.f)) < 1e-6f);
        for (float x = -4.f; x <= 4.f; x += 0.25f)
        {
            const float y = fn0b::softClip (x, 18.f);
            assert (y >= -1.f && y <= 1.f);
            assert (std::abs (fn0b::softClip (-x, 6.f) + fn0b::softClip (x, 6.f)) < 1e-5f);
        }
        assert (fn0b::softClip (0.5f, 12.f) > fn0b::softClip (0.5f, 0.f));
    }

    // Bit crusher: 1x / 16 bits is effectively transparent, hard crush is not.
    {
        fn0b::BitCrusher clean, crushed;
        clean.reset();
        crushed.reset();

        float worstClean = 0.f, worstCrushed = 0.f;
        for (int i = 0; i < 256; ++i)
        {
            const float x = 0.4f * std::sin ((float) i * 0.1f);
            worstClean = std::max (worstClean, std::abs (clean.process (x, 1.f, 16.f) - x));
            worstCrushed = std::max (worstCrushed, std::abs (crushed.process (x, 16.f, 4.f) - x));
        }
        assert (worstClean < 1e-3f);
        assert (worstCrushed > worstClean * 10.f);
    }

    // Delay line: silence stays silent; an impulse comes back as decaying
    // echoes at the right tap, never louder than the loop gain allows.
    // Static: 96 k-sample buffers do not fit on the check's stack.
    {
        static fn0b::Delay d;
        d.reset();
        d.readOffset = 1000.f;

        float echo1 = 0.f, echo2 = 0.f;
        for (int i = 0; i < 2200; ++i)
        {
            const float x = i == 0 ? 1.f : 0.f;
            const float y = d.processLooped (x, 0.5f, 0.f, 1.f);
            if (i == 1000) echo1 = y;
            if (i == 2000) echo2 = y;
        }
        // The loop stores the saturated dry+regen sum, so the first repeat is
        // the softened impulse (tanh(1) with drive 0 dB), not fb times it, and
        // the regen gain shows up in the decay: each later repeat is smaller.
        assert (echo1 > 0.5f && echo1 < 1.f);
        assert (echo2 > 0.f && echo2 < echo1);

        // Damping darkens the loop: the second echo is quieter still.
        static fn0b::Delay dark;
        dark.reset();
        dark.readOffset = 1000.f;
        float dark2 = 0.f;
        for (int i = 0; i < 2200; ++i)
        {
            const float x = i == 0 ? 1.f : 0.f;
            const float y = dark.processLooped (x, 0.6f, 0.f, 0.4f);
            if (i == 2000) dark2 = y;
        }
        assert (dark2 < echo2);

        // Sweeping the read offset (what the knob does) never explodes.
        for (int i = 0; i < 5000; ++i)
        {
            d.readOffset = 10.f + 400.f * (1.f + std::sin ((float) i * 0.001f));
            const float y = d.processLooped (0.1f * std::sin ((float) i * 0.05f),
                                             0.6f, 3.f, 0.8f);
            assert (std::isfinite (y) && std::abs (y) < 4.f);
        }
    }

    // FDN: silence stays silent; an impulse comes back wet and finite.
    {
        static fn0b::FdReverb rv;
        rv.setSampleRate (44100.0);
        float L[64] {}, R[64] {};
        rv.process (L, R, 64, 0.5f, 0.4f, 0.4f, 0.2f);
        for (int i = 0; i < 64; ++i)
            assert (std::isfinite (L[i]) && std::abs (L[i]) < 1e-5f);

        L[0] = 1.f;
        R[0] = 1.f;
        rv.reset();
        rv.process (L, R, 64, 0.8f, 0.3f, 0.6f, 0.4f);
        float peak = 0.f;
        for (int i = 0; i < 64; ++i)
        {
            assert (std::isfinite (L[i]) && std::isfinite (R[i]));
            peak = std::max (peak, std::abs (L[i]));
        }
        assert (peak > 0.01f && peak < 8.f);
    }

    // Phaser: sine stays finite; fold and reverse stay bounded.
    {
        fn0b::Phaser ph;
        ph.setSampleRate (44100.0);
        float L[128], R[128];
        for (int i = 0; i < 128; ++i)
            L[i] = R[i] = 0.2f * std::sin ((float) i * 0.2f);
        ph.process (L, R, 128, 0.8f, 0.7f, 0.4f);
        for (int i = 0; i < 128; ++i)
            assert (std::isfinite (L[i]) && std::abs (L[i]) < 4.f);

        ph.reset();
        for (int i = 0; i < 128; ++i)
            L[i] = R[i] = 0.2f * std::sin ((float) i * 0.2f);
        ph.process (L, R, 128, 0.8f, 0.7f, -0.7f);
        for (int i = 0; i < 128; ++i)
            assert (std::isfinite (L[i]) && std::abs (L[i]) < 4.f);

        assert (std::abs (fn0b::waveFold (0.f, 1.f)) < 1e-6f);
        assert (std::abs (fn0b::waveFold (0.9f, 1.f)) <= 1.f + 1e-3f);
        assert (fn0b::hardClip (4.f) == 1.f);

        static fn0b::Reverse rv;
        rv.reset();
        float acc = 0.f;
        for (int i = 0; i < 4096; ++i)
            acc += std::abs (rv.process (i == 0 ? 1.f : 0.f, 512));
        assert (std::isfinite (acc));
    }

    std::printf ("fn0b_check ok  presets=%d\n", fn0b::kPresetCount);
    return 0;
}
