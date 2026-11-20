#pragma once

#include "Presets.h"
#include "Units.h"

#include <JuceHeader.h>
#include <array>
#include <cmath>

namespace fn0b
{

// Runs the preset's effect chain over a stereo buffer. The chain order differs per
// preset, so it is a switch rather than a slot graph: presets do not justify a
// generic effect host.
struct Chain
{
    void prepare (double sampleRate, int samplesPerBlock, int numChannels)
    {
        sr = sampleRate > 1.0 ? sampleRate : 44100.0;
        const int ch = juce::jmax (1, numChannels);
        juce::ignoreUnused (samplesPerBlock);

        juce::dsp::ProcessSpec spec { sr, (juce::uint32) juce::jmax (1, samplesPerBlock),
                                      (juce::uint32) ch };
        auto prep = [&spec] (auto& f, juce::dsp::StateVariableTPTFilterType t)
        {
            f.prepare (spec);
            f.setType (t);
            f.setResonance (0.707f);
            f.setCutoffFrequency (20000.f);
        };
        for (auto& f : lp)
            prep (f, juce::dsp::StateVariableTPTFilterType::lowpass);
        for (auto& f : hp)
            prep (f, juce::dsp::StateVariableTPTFilterType::highpass);
        for (auto& f : bp)
            prep (f, juce::dsp::StateVariableTPTFilterType::bandpass);

        for (auto& c : crush)
            c.reset();
        resetDelays();
        reverb.setSampleRate (sr);
        phaser.setSampleRate (sr);
        for (auto& c : comp)
            c.setSampleRate (sr);
        for (auto& g : grain)
            g.reset();
        for (auto& r : revw)
            r.reset();
        for (auto& r : revw2)
            r.reset();
        for (auto& f : form)
            f.reset();
        for (auto& o : pole)
            o.reset();

        lfo = wowPh = flutPh = ringPh = formPh = 0.f;
        pitchOff = { (float) (Delay::kMaxSamples / 2), (float) (Delay::kMaxSamples / 2) };
        pitchOff2 = pitchOff;
        rng = 0xC0FFEEu;

        for (auto& s : smooth)
            s.reset (sr, 0.02);

        currentPreset = -1;
    }

    void process (juce::AudioBuffer<float>& buffer, int preset, float knob)
    {
        const int n = buffer.getNumSamples();
        if (n <= 0)
            return;

        const Voice v = voiceFor (preset, knob);
        const int ch = juce::jmin (buffer.getNumChannels(), 2);
        float* L = buffer.getWritePointer (0);
        float* R = ch > 1 ? buffer.getWritePointer (1) : L;

        const bool justChanged = preset != currentPreset;
        if (justChanged)
        {
            resetDelays();
            reverb.reset();
            phaser.reset();
            for (auto& c : crush)
                c.reset();
            for (auto& c : comp)
                c.reset();
            for (auto& g : grain)
                g.reset();
            for (auto& r : revw)
                r.reset();
            for (auto& r : revw2)
                r.reset();
            for (auto& f : form)
                f.reset();
            for (auto& o : pole)
                o.reset();
            lfo = wowPh = flutPh = ringPh = formPh = 0.f;
            pitchOff = { (float) (Delay::kMaxSamples / 2), (float) (Delay::kMaxSamples / 2) };
            pitchOff2 = pitchOff;
            currentPreset = preset;
        }

        switch (preset)
        {
            case 0:  // Warm Drive: soft clip, then low-pass.
            {
                const float g = juce::Decibels::decibelsToGain (v.driveDb);
                for (int c = 0; c < ch; ++c)
                {
                    auto* d = buffer.getWritePointer (c);
                    for (int i = 0; i < n; ++i)
                        d[i] = std::tanh (d[i] * g);
                }
                setLp (v.cutoffHz, v.res, justChanged);
                processLp (buffer, ch, n);
                break;
            }

            case 1:  // Filter Funk: resonant low-pass, then soft clip.
            {
                setLp (v.cutoffHz, v.res, justChanged);
                processLp (buffer, ch, n);
                const float g = juce::Decibels::decibelsToGain (v.driveDb);
                for (int c = 0; c < ch; ++c)
                {
                    auto* d = buffer.getWritePointer (c);
                    for (int i = 0; i < n; ++i)
                        d[i] = std::tanh (d[i] * g);
                }
                break;
            }

            case 2:  // Bitcrush Destruct
            {
                const float g = juce::Decibels::decibelsToGain (v.driveDb);
                for (int c = 0; c < ch; ++c)
                {
                    auto* d = buffer.getWritePointer (c);
                    auto& unit = crush[(size_t) c];
                    for (int i = 0; i < n; ++i)
                        d[i] = std::tanh (unit.process (d[i], v.crushDiv, v.bits) * g);
                }
                break;
            }

            case 3:  // Tape Echo
            {
                const float delaySamp = clampDelay (v.delayMs);
                setSmooth (2, delaySamp, justChanged);
                const float damp = onePoleHz (4500.f);
                for (int i = 0; i < n; ++i)
                {
                    const float off = smooth[2].getNextValue();
                    for (int c = 0; c < ch; ++c)
                    {
                        auto* d = buffer.getWritePointer (c);
                        auto& unit = echo[(size_t) c];
                        unit.readOffset = off;
                        d[i] = d[i] + unit.processLooped (d[i], v.feedback, v.driveDb, damp);
                    }
                }
                setLp (v.cutoffHz, 0.707f, justChanged);
                processLp (buffer, ch, n);
                break;
            }

            case 4:  // Shimmer Verb: FDN + octave inject, then tone.
            {
                reverb.process (L, R, n, v.reverbSize, 0.4f, v.mix, v.shimmer);
                setLp (v.cutoffHz, 0.707f, justChanged);
                processLp (buffer, ch, n);
                break;
            }

            case 5:  // Phaser Sweep
            {
                setSmooth (3, v.phaserRateHz, justChanged);
                const float rate = smooth[3].skip (n);
                phaser.process (L, R, n, rate, v.phaserDepth, v.feedback);
                const float trim = 0.7f + 0.1f * v.phaserDepth;
                buffer.applyGain (trim);
                break;
            }

            case 6:  // Chorus Dimension
                processChorus (buffer, ch, n, v.chorusRateHz, v.chorusDepth, v.mix, v.width, justChanged);
                break;

            case 7:  // Dual Delay Cascade
            {
                setSmooth (2, clampDelay (v.delayMs), justChanged);
                setSmooth (4, clampDelay (v.delayMs2), justChanged);
                const float damp = onePoleHz (v.cutoffHz);
                for (int i = 0; i < n; ++i)
                {
                    const float o1 = smooth[2].getNextValue();
                    const float o2 = smooth[4].getNextValue();
                    for (int c = 0; c < ch; ++c)
                    {
                        auto* d = buffer.getWritePointer (c);
                        echo[(size_t) c].readOffset = o1;
                        echo2[(size_t) c].readOffset = o2;
                        const float a = echo[(size_t) c].processLooped (d[i], v.feedback * 0.5f, 0.f, damp);
                        const float b = echo2[(size_t) c].processLooped (a, v.feedback, 0.f, damp);
                        d[i] = d[i] + a + b;
                    }
                }
                setLp (v.cutoffHz, 0.707f, justChanged);
                processLp (buffer, ch, n);
                break;
            }

            case 8:  // Tremolo Pulse
            {
                const float dph = kTwoPi * v.tremRateHz / (float) sr;
                const float g = juce::Decibels::decibelsToGain (v.driveDb);
                for (int i = 0; i < n; ++i)
                {
                    lfo += dph;
                    if (lfo > kTwoPi) lfo -= kTwoPi;
                    const float s = std::sin (lfo);
                    const float sq = s >= 0.f ? 1.f : -1.f;
                    const float w = s + (sq - s) * v.shape;
                    const float env = 1.f - v.tremDepth + v.tremDepth * (0.5f + 0.5f * w);
                    for (int c = 0; c < ch; ++c)
                    {
                        auto* d = buffer.getWritePointer (c);
                        d[i] = std::tanh (d[i] * env * g);
                    }
                }
                break;
            }

            case 9:  // Space Station
            {
                setSmooth (2, clampDelay (v.delayMs), justChanged);
                const float dph = kTwoPi * 0.18f / (float) sr;
                const float damp = onePoleHz (5000.f);
                for (int i = 0; i < n; ++i)
                {
                    lfo += dph;
                    if (lfo > kTwoPi) lfo -= kTwoPi;
                    const float wob = 1.f + v.modDepth * 0.035f * std::sin (lfo);
                    const float off = smooth[2].getNextValue() * wob;
                    for (int c = 0; c < ch; ++c)
                    {
                        auto* d = buffer.getWritePointer (c);
                        echo[(size_t) c].readOffset = off;
                        d[i] = d[i] + echo[(size_t) c].processLooped (d[i], v.feedback, 1.f, damp);
                    }
                }
                reverb.process (L, R, n, v.reverbSize, 0.35f, v.mix, 0.f);
                break;
            }

            case 10: // Crush & Bloom
            {
                const float g = juce::Decibels::decibelsToGain (v.driveDb);
                for (int c = 0; c < ch; ++c)
                {
                    auto* d = buffer.getWritePointer (c);
                    for (int i = 0; i < n; ++i)
                        d[i] = std::tanh (d[i] * g);
                }
                setHp (v.hpHz, 0.707f, justChanged);
                processHp (buffer, ch, n);
                reverb.process (L, R, n, v.reverbSize, 0.45f, v.mix, 0.f);
                break;
            }

            case 11: // Meltglass
            {
                const float g = juce::Decibels::decibelsToGain (v.driveDb);
                for (int c = 0; c < ch; ++c)
                {
                    auto* d = buffer.getWritePointer (c);
                    for (int i = 0; i < n; ++i)
                        d[i] = std::tanh (d[i] * g);
                }
                processChorus (buffer, ch, n, v.chorusRateHz, v.chorusDepth, v.mix, 1.15f, justChanged);
                setLp (v.cutoffHz, 0.707f, justChanged);
                processLp (buffer, ch, n);
                break;
            }

            case 12: // Parking Garage
            {
                const float combSamp = clampDelay (v.combMs);
                setSmooth (2, combSamp, justChanged);
                const float fb = v.feedback > 0.92f ? 0.92f : v.feedback;
                for (int i = 0; i < n; ++i)
                {
                    const float off = smooth[2].getNextValue();
                    for (int c = 0; c < ch; ++c)
                    {
                        auto* d = buffer.getWritePointer (c);
                        echo[(size_t) c].readOffset = off;
                        const float y = echo[(size_t) c].read();
                        echo[(size_t) c].writeSample (d[i] + y * fb);
                        d[i] = d[i] + y;
                    }
                }
                reverb.process (L, R, n, v.reverbSize, 0.4f, v.mix, 0.f);
                break;
            }

            case 13: // Broken Broadcast
            {
                setBp (v.cutoffHz, v.res, justChanged);
                processBp (buffer, ch, n);
                for (int c = 0; c < ch; ++c)
                {
                    auto* d = buffer.getWritePointer (c);
                    for (int i = 0; i < n; ++i)
                    {
                        float y = crush[(size_t) c].process (d[i], v.crushDiv, v.bits);
                        y += noiseUni (rng) * v.noiseMix;
                        d[i] = y;
                    }
                }
                break;
            }

            case 14: // Tape Ghost
            {
                const float g = juce::Decibels::decibelsToGain (v.driveDb);
                const float base = (float) sr * 0.012f;
                const float aShelf = onePoleHz (2500.f);
                const float shelfG = juce::Decibels::decibelsToGain (v.shelfDb) - 1.f;
                const float wowDph = kTwoPi * 0.55f / (float) sr;
                const float flDph  = kTwoPi * 6.2f / (float) sr;
                for (int i = 0; i < n; ++i)
                {
                    wowPh += wowDph;
                    flutPh += flDph;
                    if (wowPh > kTwoPi) wowPh -= kTwoPi;
                    if (flutPh > kTwoPi) flutPh -= kTwoPi;
                    const float mod = 1.f + v.wow * std::sin (wowPh) + v.flutter * std::sin (flutPh);
                    const float off = base * mod;
                    for (int c = 0; c < ch; ++c)
                    {
                        auto* d = buffer.getWritePointer (c);
                        const float sat = std::tanh (d[i] * g);
                        echo[(size_t) c].readOffset = off;
                        const float y = echo[(size_t) c].process (sat);
                        const float lf = pole[(size_t) c].process (y, aShelf);
                        d[i] = y + shelfG * (y - lf);
                    }
                }
                break;
            }

            case 15: // Rubber Room
            {
                setSmooth (2, clampDelay (v.delayMs), justChanged);
                const float dph = kTwoPi * 0.7f / (float) sr;
                const float cents = v.modDepth; // 0..0.2 = 0..20 cents as fraction of semitone-ish
                const float fb = v.feedback > 0.8f ? 0.8f : v.feedback;
                for (int i = 0; i < n; ++i)
                {
                    lfo += dph;
                    if (lfo > kTwoPi) lfo -= kTwoPi;
                    const float off = smooth[2].getNextValue()
                                    * (1.f + cents * 0.12f * std::sin (lfo));
                    for (int c = 0; c < ch; ++c)
                    {
                        auto* d = buffer.getWritePointer (c);
                        echo[(size_t) c].readOffset = off;
                        const float y = echo[(size_t) c].processLooped (d[i], fb, 0.f, 0.85f);
                        d[i] = d[i] * (1.f - v.mix) + y * v.mix;
                    }
                }
                break;
            }

            case 16: // Laser Hive
            {
                setHp (v.hpHz, v.res, justChanged);
                processHp (buffer, ch, n);
                phaser.process (L, R, n, v.phaserRateHz, v.phaserDepth, v.feedback);
                break;
            }

            case 17: // Velvet Damage
            {
                const float g = juce::Decibels::decibelsToGain (v.driveDb);
                for (int c = 0; c < ch; ++c)
                {
                    auto* d = buffer.getWritePointer (c);
                    for (int i = 0; i < n; ++i)
                    {
                        const float s = std::tanh (d[i] * g);
                        d[i] = comp[(size_t) c].process (s, 0.35f, v.ratio);
                    }
                }
                break;
            }

            case 18: // Event Horizon
            {
                reverb.process (L, R, n, v.reverbSize, 0.45f, v.mix, 0.f, v.decay);
                setLp (v.cutoffHz, 0.707f, justChanged);
                processLp (buffer, ch, n);
                const float g = juce::Decibels::decibelsToGain (v.driveDb);
                for (int c = 0; c < ch; ++c)
                {
                    auto* d = buffer.getWritePointer (c);
                    for (int i = 0; i < n; ++i)
                        d[i] = std::tanh (d[i] * g);
                }
                break;
            }

            case 19: // Shatterbox
            {
                setLp (v.cutoffHz, 0.707f, justChanged);
                setHp (v.hpHz, 0.707f, justChanged);
                const float gLo = juce::Decibels::decibelsToGain (v.driveLo);
                const float gMid = juce::Decibels::decibelsToGain (v.driveMid);
                const float gHi = juce::Decibels::decibelsToGain (v.driveHi);
                for (int i = 0; i < n; ++i)
                {
                    const float fLo = smooth[0].getNextValue();
                    const float fHi = smooth[5].getNextValue();
                    for (int c = 0; c < ch; ++c)
                    {
                        auto* d = buffer.getWritePointer (c);
                        lp[(size_t) c].setCutoffFrequency (fLo);
                        hp[(size_t) c].setCutoffFrequency (fHi);
                        const float x = d[i];
                        const float lo = lp[(size_t) c].processSample (c, x);
                        const float hi = hp[(size_t) c].processSample (c, x);
                        const float mid = x - lo - hi;
                        d[i] = std::tanh (lo * gLo) + std::tanh (mid * gMid) + std::tanh (hi * gHi);
                    }
                }
                break;
            }

            case 20: // Hypercolor
            {
                const float a = onePoleHz (1000.f);
                const float tiltG = juce::Decibels::decibelsToGain (v.tilt) - 1.f;
                const float aEx = onePoleHz (4000.f);
                for (int i = 0; i < n; ++i)
                {
                    float sL = L[i], sR = ch > 1 ? R[i] : L[i];
                    const float lfL = pole[0].process (sL, a);
                    const float lfR = pole[1].process (sR, a);
                    sL += tiltG * (sL - lfL);
                    sR += tiltG * (sR - lfR);
                    float m = 0.5f * (sL + sR);
                    float s = 0.5f * (sL - sR) * v.width;
                    sL = m + s;
                    sR = m - s;
                    const float hiL = sL - pole[2].process (sL, aEx);
                    const float hiR = sR - pole[3].process (sR, aEx);
                    sL += v.excite * std::tanh (hiL * 4.f);
                    sR += v.excite * std::tanh (hiR * 4.f);
                    L[i] = sL;
                    if (ch > 1) R[i] = sR;
                }
                break;
            }

            case 21: // Gravity Well
            {
                const float ratio = std::exp2 (v.pitch / 12.f);
                setSmooth (2, clampDelay (v.delayMs), justChanged);
                setBp (v.cutoffHz, v.res, justChanged);
                const float fb = v.feedback > 0.78f ? 0.78f : v.feedback;
                for (int i = 0; i < n; ++i)
                {
                    const float off = smooth[2].getNextValue();
                    const float f = smooth[0].getNextValue();
                    const float r = smooth[1].getNextValue();
                    for (int c = 0; c < ch; ++c)
                    {
                        auto* d = buffer.getWritePointer (c);
                        const float p = processPitch (echo2[(size_t) c], pitchOff[(size_t) c], d[i], ratio);
                        bp[(size_t) c].setCutoffFrequency (f);
                        bp[(size_t) c].setResonance (r);
                        echo[(size_t) c].readOffset = off;
                        const float y = echo[(size_t) c].read();
                        const float filt = bp[(size_t) c].processSample (c, y);
                        echo[(size_t) c].writeSample (p + filt * fb);
                        d[i] = p + y;
                    }
                }
                break;
            }

            case 22: // Glitch Garden
            {
                const bool frz = v.freeze > 0.35f;
                for (int i = 0; i < n; ++i)
                {
                    for (int c = 0; c < ch; ++c)
                    {
                        auto* d = buffer.getWritePointer (c);
                        grain[(size_t) c].write (d[i], frz);
                        float y = grain[(size_t) c].read (v.grain, v.scramble, rng);
                        y = crush[(size_t) c].process (y, v.crushDiv, v.bits);
                        d[i] = y;
                    }
                }
                break;
            }

            case 23: // Unstable Orbit
            {
                setSmooth (2, clampDelay (v.delayMs), justChanged);
                setSmooth (4, clampDelay (v.delayMs2), justChanged);
                const float dph = kTwoPi * 0.35f / (float) sr;
                const float fb = v.feedback > 0.7f ? 0.7f : v.feedback;
                const float g = juce::Decibels::decibelsToGain (v.driveDb);
                for (int i = 0; i < n; ++i)
                {
                    lfo += dph;
                    if (lfo > kTwoPi) lfo -= kTwoPi;
                    const float m = v.modDepth * 0.08f;
                    const float o1 = smooth[2].getNextValue() * (1.f + m * std::sin (lfo));
                    const float o2 = smooth[4].getNextValue() * (1.f - m * std::sin (lfo));
                    for (int c = 0; c < ch; ++c)
                    {
                        auto* d = buffer.getWritePointer (c);
                        echo[(size_t) c].readOffset = o1;
                        echo2[(size_t) c].readOffset = o2;
                        const float a = echo[(size_t) c].read();
                        const float b = echo2[(size_t) c].read();
                        echo[(size_t) c].writeSample (d[i] + b * fb);
                        echo2[(size_t) c].writeSample (d[i] + a * fb);
                        d[i] = std::tanh ((d[i] * (1.f - v.mix) + 0.5f * (a + b) * v.mix) * g);
                    }
                }
                break;
            }

            case 24: // Bone Saw
            {
                const float combSamp = clampDelay (v.combMs);
                setSmooth (2, combSamp, justChanged);
                const float fb = v.feedback > 0.88f ? 0.88f : v.feedback;
                const float foldAmt = clamp01 (v.driveDb / 18.f);
                const float clipG = 1.f + v.shape * 4.f;
                for (int i = 0; i < n; ++i)
                {
                    const float off = smooth[2].getNextValue();
                    for (int c = 0; c < ch; ++c)
                    {
                        auto* d = buffer.getWritePointer (c);
                        const float folded = waveFold (d[i], foldAmt);
                        echo[(size_t) c].readOffset = off;
                        const float y = echo[(size_t) c].read();
                        echo[(size_t) c].writeSample (folded + y * fb);
                        d[i] = hardClip ((folded + y) * clipG);
                    }
                }
                break;
            }

            case 25: // Time Parasite
            {
                const int win = 256 + (int) (v.grain * (float) sr * 0.12f);
                const float ratio = std::exp2 (v.pitch / 12.f);
                const float fb = v.feedback > 0.6f ? 0.6f : v.feedback;
                for (int i = 0; i < n; ++i)
                {
                    for (int c = 0; c < ch; ++c)
                    {
                        auto* d = buffer.getWritePointer (c);
                        const float r1 = revw[(size_t) c].process (d[i], win);
                        const float p = processPitch (echo[(size_t) c], pitchOff[(size_t) c], r1, ratio);
                        const float r2 = revw2[(size_t) c].process (p + fb * pole[(size_t) c].z, win);
                        pole[(size_t) c].z = r2;
                        d[i] = d[i] * (1.f - v.mix) + r2 * v.mix;
                    }
                }
                break;
            }

            case 26: // Formant Storm
            {
                const float dph = kTwoPi * v.tremRateHz / (float) sr;
                const float g = juce::Decibels::decibelsToGain (v.driveDb);
                for (int i = 0; i < n; ++i)
                {
                    formPh += dph;
                    if (formPh > kTwoPi) formPh -= kTwoPi;
                    const float morph = clamp01 (v.formant + v.modDepth * 0.25f * std::sin (formPh));
                    for (int c = 0; c < ch; ++c)
                    {
                        auto* d = buffer.getWritePointer (c);
                        const float y = form[(size_t) c].process (d[i], morph, 6.f, sr);
                        d[i] = std::tanh (y * g);
                    }
                }
                break;
            }

            case 27: // Black Hole Chorus
            {
                processChorus (buffer, ch, n, v.chorusRateHz, v.chorusDepth, 0.7f, 1.3f, justChanged);
                const float ratio = std::exp2 (v.pitch / 12.f);
                for (int i = 0; i < n; ++i)
                {
                    for (int c = 0; c < ch; ++c)
                    {
                        auto* d = buffer.getWritePointer (c);
                        const float p = processPitch (echo2[(size_t) c], pitchOff[(size_t) c], d[i], ratio);
                        d[i] = 0.55f * d[i] + 0.45f * p;
                    }
                }
                reverb.process (L, R, n, v.reverbSize, 0.35f, v.mix, 0.f, v.decay);
                break;
            }

            case 28: // Circuit Melt
            {
                const float dph = kTwoPi * v.ringHz / (float) sr;
                setLp (v.cutoffHz, v.res, justChanged);
                for (int i = 0; i < n; ++i)
                {
                    ringPh += dph;
                    if (ringPh > kTwoPi) ringPh -= kTwoPi;
                    const float car = std::sin (ringPh);
                    const float hz = smooth[0].getNextValue();
                    const float r = smooth[1].getNextValue();
                    for (int c = 0; c < ch; ++c)
                    {
                        auto* d = buffer.getWritePointer (c);
                        float y = d[i] * car;
                        y = crush[(size_t) c].process (y, v.crushDiv, v.bits);
                        lp[(size_t) c].setCutoffFrequency (hz);
                        lp[(size_t) c].setResonance (r);
                        d[i] = lp[(size_t) c].processSample (c, y);
                    }
                }
                break;
            }

            case 29: // Echo Psychosis
            {
                setSmooth (2, clampDelay (v.delayMs), justChanged);
                setSmooth (4, clampDelay (v.delayMs2), justChanged);
                const float d3 = clampDelay (v.delayMs * 0.73f + v.delayMs2 * 0.21f);
                setSmooth (7, d3, justChanged);
                const float dph = kTwoPi * 0.22f / (float) sr;
                const float fb = v.feedback > 0.72f ? 0.72f : v.feedback;
                for (int i = 0; i < n; ++i)
                {
                    lfo += dph;
                    if (lfo > kTwoPi) lfo -= kTwoPi;
                    const float drift = v.modDepth * 0.04f;
                    const float o1 = smooth[2].getNextValue() * (1.f + drift * std::sin (lfo));
                    const float o2 = smooth[4].getNextValue() * (1.f + drift * std::sin (lfo * 1.17f + 1.1f));
                    const float o3 = smooth[7].getNextValue() * (1.f + drift * std::sin (lfo * 0.81f + 2.2f));
                    for (int c = 0; c < ch; ++c)
                    {
                        auto* d = buffer.getWritePointer (c);
                        echo[(size_t) c].readOffset = o1;
                        echo2[(size_t) c].readOffset = o2;
                        echo3[(size_t) c].readOffset = o3;
                        const float a = echo[(size_t) c].read();
                        const float b = echo2[(size_t) c].read();
                        const float e = echo3[(size_t) c].read();
                        const float sum = (a + b + e) * (1.f / 3.f);
                        echo[(size_t) c].writeSample (d[i] + sum * fb);
                        echo2[(size_t) c].writeSample (d[i] + sum * fb);
                        echo3[(size_t) c].writeSample (d[i] + sum * fb);
                        d[i] = d[i] * (1.f - v.mix) + sum * v.mix;
                    }
                }
                break;
            }

            case 30: // Singularity
            {
                const float ratio = std::exp2 (v.pitch / 12.f);
                const bool frzGrain = v.freeze > 0.25f;
                for (int i = 0; i < n; ++i)
                {
                    for (int c = 0; c < ch; ++c)
                    {
                        auto* d = buffer.getWritePointer (c);
                        const float p = processPitch (echo[(size_t) c], pitchOff[(size_t) c], d[i], ratio);
                        grain[(size_t) c].write (p, frzGrain);
                        d[i] = grain[(size_t) c].read (v.grain, v.scramble, rng);
                    }
                }
                reverb.process (L, R, n, v.reverbSize, 0.3f, v.mix, 0.25f, 0.4f, v.freeze);
                break;
            }

            default:
                break;
        }
    }

private:
    void resetDelays()
    {
        for (auto& d : echo)
            d.reset();
        for (auto& d : echo2)
            d.reset();
        for (auto& d : echo3)
            d.reset();
    }

    float clampDelay (float ms) const
    {
        const float s = (float) sr * ms / 1000.f;
        return juce::jlimit (1.f, (float) Delay::kMaxSamples - 2.f, s);
    }

    float onePoleHz (float hz) const
    {
        return std::exp (-kTwoPi * hz / (float) sr);
    }

    void setSmooth (int i, float x, bool snap)
    {
        if (snap)
            smooth[(size_t) i].setCurrentAndTargetValue (x);
        else
            smooth[(size_t) i].setTargetValue (x);
    }

    void setLp (float hz, float res, bool snap)
    {
        setSmooth (0, juce::jlimit (20.f, (float) (sr * 0.5) - 1.f, hz), snap);
        setSmooth (1, juce::jlimit (0.1f, 1.0f, res), snap);
    }

    void setHp (float hz, float res, bool snap)
    {
        setSmooth (5, juce::jlimit (20.f, (float) (sr * 0.5) - 1.f, hz), snap);
        setSmooth (6, juce::jlimit (0.1f, 1.0f, res), snap);
    }

    void setBp (float hz, float res, bool snap)
    {
        setLp (hz, res, snap);
    }

    void processLp (juce::AudioBuffer<float>& buffer, int ch, int n)
    {
        processSvf (lp, buffer, ch, n, 0, 1);
    }

    void processHp (juce::AudioBuffer<float>& buffer, int ch, int n)
    {
        processSvf (hp, buffer, ch, n, 5, 6);
    }

    void processBp (juce::AudioBuffer<float>& buffer, int ch, int n)
    {
        processSvf (bp, buffer, ch, n, 0, 1);
    }

    void processSvf (std::array<juce::dsp::StateVariableTPTFilter<float>, 2>& f,
                     juce::AudioBuffer<float>& buffer, int ch, int n, int iHz, int iR)
    {
        for (int i = 0; i < n; ++i)
        {
            const float hz = smooth[(size_t) iHz].getNextValue();
            const float r = smooth[(size_t) iR].getNextValue();
            for (int c = 0; c < ch; ++c)
            {
                f[(size_t) c].setCutoffFrequency (hz);
                f[(size_t) c].setResonance (r);
                auto* d = buffer.getWritePointer (c);
                d[i] = f[(size_t) c].processSample (c, d[i]);
            }
        }
    }

    static float processPitch (Delay& d, float& off, float x, float ratio)
    {
        // write advances 1; subtract (r-1) so the read head advances at r.
        off -= (ratio - 1.f);
        while (off < 1.f)
            off += (float) Delay::kMaxSamples;
        while (off > (float) Delay::kMaxSamples - 2.f)
            off -= (float) Delay::kMaxSamples;
        d.readOffset = off;
        return d.process (x);
    }

    void processChorus (juce::AudioBuffer<float>& buffer, int ch, int n,
                        float rateHz, float depth, float mix, float width, bool snap)
    {
        setSmooth (3, rateHz, snap);
        const float base = (float) sr * 0.012f;
        const float span = (float) sr * 0.008f * clamp01 (depth);
        const float dph = kTwoPi * juce::jmax (0.01f, smooth[3].skip (n)) / (float) sr;
        float* L = buffer.getWritePointer (0);
        float* R = ch > 1 ? buffer.getWritePointer (1) : L;
        for (int i = 0; i < n; ++i)
        {
            lfo += dph;
            if (lfo > kTwoPi) lfo -= kTwoPi;
            const float sL = std::sin (lfo);
            const float sR = std::sin (lfo + 1.5707963f);
            echo[0].readOffset = base + span * sL;
            echo[1].readOffset = base + span * sR;
            float yL = echo[0].process (L[i]);
            float yR = echo[1].process (ch > 1 ? R[i] : L[i]);
            yL = L[i] * (1.f - mix) + yL * mix;
            yR = (ch > 1 ? R[i] : L[i]) * (1.f - mix) + yR * mix;
            const float m = 0.5f * (yL + yR);
            const float s = 0.5f * (yL - yR) * width;
            L[i] = m + s;
            if (ch > 1)
                R[i] = m - s;
        }
    }

    double sr = 44100.0;
    std::array<juce::dsp::StateVariableTPTFilter<float>, 2> lp, hp, bp;
    std::array<BitCrusher, 2> crush;
    std::array<Delay, 2> echo, echo2, echo3;
    FdReverb reverb;
    Phaser phaser;
    std::array<PeakComp, 2> comp;
    std::array<Formant, 2> form;
    std::array<Grain, 2> grain;
    std::array<Reverse, 2> revw, revw2;
    std::array<OnePole, 4> pole;
    std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>, 8> smooth;
    std::array<float, 2> pitchOff {}, pitchOff2 {};
    float lfo = 0.f, wowPh = 0.f, flutPh = 0.f, ringPh = 0.f, formPh = 0.f;
    uint32_t rng = 1;
    int currentPreset = -1;
};

} // namespace fn0b
