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
                    && std::isfinite (v.delayMs) && std::isfinite (v.feedback) && std::isfinite (v.mix)
                    && std::isfinite (v.reverbSize) && std::isfinite (v.shimmer)
                    && std::isfinite (v.phaserRateHz) && std::isfinite (v.phaserDepth));
            assert (v.driveDb >= 0.f && v.driveDb <= 18.f);
            assert (v.cutoffHz >= 20.f && v.cutoffHz <= 20000.f);
            assert (v.res > 0.f && v.res <= 1.f);
            assert (v.crushDiv >= 1.f && v.bits >= 4.f && v.bits <= 16.f);
            assert (v.delayMs >= 0.f && v.delayMs <= 450.f);
            assert (v.feedback >= 0.f && v.feedback <= 0.7f);
            assert (v.mix >= 0.f && v.mix <= 1.f);
            assert (v.reverbSize >= 0.f && v.reverbSize <= 1.f);
            assert (v.shimmer >= 0.f && v.shimmer <= 0.5f);
            assert (v.phaserRateHz >= 0.f && v.phaserRateHz <= 3.5f);
            assert (v.phaserDepth >= 0.f && v.phaserDepth <= 1.f);

            // Drive is non-decreasing in the knob for every preset.
            assert (v.driveDb + 1e-4f >= prevDrive);
            prevDrive = v.driveDb;

            // Cutoff moves one way only. 0/3/4 close down, 1 opens up, 2/5 hold.
            if (p == 0 || p == 3 || p == 4)
                assert (v.cutoffHz <= prevCut + 1e-3f || prevCut < 0.f);
            else if (p == 1)
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

    std::printf ("fn0b_check ok  presets=%d\n", fn0b::kPresetCount);
    return 0;
}
