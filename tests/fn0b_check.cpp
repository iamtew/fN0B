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
        for (int i = 0; i <= 100; ++i)
        {
            const auto v = fn0b::voiceFor (p, (float) i / 100.f);
            assert (std::isfinite (v.driveDb) && std::isfinite (v.cutoffHz)
                    && std::isfinite (v.res) && std::isfinite (v.crushDiv) && std::isfinite (v.bits));
            assert (v.driveDb >= 0.f && v.driveDb <= 18.f);
            assert (v.cutoffHz >= 20.f && v.cutoffHz <= 20000.f);
            assert (v.res > 0.f && v.res <= 1.f);
            assert (v.crushDiv >= 1.f && v.bits >= 4.f && v.bits <= 16.f);

            // Drive is non-decreasing in the knob for every preset.
            assert (v.driveDb + 1e-4f >= prevDrive);
            prevDrive = v.driveDb;

            // Cutoff moves one way only. Preset 0 closes down, 1 opens up, 2 holds.
            if (p == 0)
                assert (v.cutoffHz <= prevCut + 1e-3f || prevCut < 0.f);
            else if (p == 1)
                assert (v.cutoffHz + 1e-3f >= prevCut);
            prevCut = v.cutoffHz;
        }
    }

    // Endpoints match the preset tables.
    assert (std::abs (fn0b::voiceFor (0, 0.f).driveDb - 0.f) < 1e-4f);
    assert (std::abs (fn0b::voiceFor (0, 1.f).driveDb - 18.f) < 1e-4f);
    assert (std::abs (fn0b::voiceFor (0, 0.f).cutoffHz - 20000.f) < 1e-1f);
    assert (std::abs (fn0b::voiceFor (0, 1.f).cutoffHz - 4000.f) < 1e-1f);
    assert (std::abs (fn0b::voiceFor (1, 0.f).cutoffHz - 200.f) < 1e-1f);
    assert (std::abs (fn0b::voiceFor (1, 1.f).cutoffHz - 8000.f) < 1e-1f);
    assert (std::abs (fn0b::voiceFor (2, 0.f).crushDiv - 1.f) < 1e-4f);
    assert (std::abs (fn0b::voiceFor (2, 1.f).crushDiv - 16.f) < 1e-3f);

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

    std::printf ("fn0b_check ok  presets=%d\n", fn0b::kPresetCount);
    return 0;
}
