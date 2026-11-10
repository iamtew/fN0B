#pragma once

#include "Presets.h"
#include "Units.h"

#include <JuceHeader.h>
#include <array>
#include <cmath>

namespace fn0b
{

// Runs the preset's effect chain over a stereo buffer. The chain order differs per
// preset (clip-then-filter vs filter-then-clip), so it is a switch rather than a
// slot graph: a handful of presets do not justify a generic effect host.
struct Chain
{
    void prepare (double sampleRate, int samplesPerBlock, int numChannels)
    {
        sr = sampleRate > 1.0 ? sampleRate : 44100.0;
        const int ch = juce::jmax (1, numChannels);

        juce::dsp::ProcessSpec spec { sr, (juce::uint32) juce::jmax (1, samplesPerBlock),
                                      (juce::uint32) ch };
        for (auto& f : lp)
        {
            f.prepare (spec);
            f.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
            f.setResonance (0.707f);
            f.setCutoffFrequency (20000.f);
        }
        for (auto& c : crush)
            c.reset();
        for (auto& d : echo)
            d.reset();
        for (auto& d : shimmerDelay)
            d.reset();

        // Shimmer read/write offset pair and its one-pole filter states.
        shOff = { 0.f, 0.f };
        zSh = { 0.f, 0.f };

        juce::dsp::ProcessSpec phaserSpec { sr, (juce::uint32) juce::jmax (1, samplesPerBlock),
                                            (juce::uint32) juce::jmin (2, ch) };
        phaser.prepare (phaserSpec);
        phaser.reset();
        reverb.prepare (phaserSpec);
        reverb.reset();

        // One-pole de-click on cutoff, resonance, drive, delay time and phaser rate,
        // so knob moves do not zipper.
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
        const int ch = juce::jmin (buffer.getNumChannels(), (int) lp.size());

        const bool justChanged = preset != currentPreset;
        if (justChanged)
        {
            for (auto& c : crush)
                c.reset();
            for (auto& d : echo)
                d.reset();
            for (auto& d : shimmerDelay)
                d.reset();
            shOff = { 0.f, 0.f };
            zSh = { 0.f, 0.f };
            reverb.reset();
            phaser.reset();
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
                setFilter (v.cutoffHz, v.res, justChanged);
                processFilter (buffer, ch, n);
                break;
            }

            case 1:  // Filter Funk: resonant low-pass, then soft clip.
            {
                setFilter (v.cutoffHz, v.res, justChanged);
                processFilter (buffer, ch, n);
                const float g = juce::Decibels::decibelsToGain (v.driveDb);
                for (int c = 0; c < ch; ++c)
                {
                    auto* d = buffer.getWritePointer (c);
                    for (int i = 0; i < n; ++i)
                        d[i] = std::tanh (d[i] * g);
                }
                break;
            }

            case 2:  // Bitcrush Destruct: sample-rate reduce, bit reduce, then clip.
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

            case 3:  // Tape Echo: saturated damped repeats, then high-cut.
            {
                // The delay time is smoothed in samples; the read position glides
                // between knob steps, so a turn bends the pitch a little, which
                // is what tape does.
                const int chUsed = juce::jmin (ch, 2);
                const float delaySamp = (float) sr * v.delayMs / 1000.f;
                if (justChanged)
                    smooth[2].setCurrentAndTargetValue (delaySamp);
                else
                    smooth[2].setTargetValue (delaySamp);
                const float damp = std::exp (-juce::MathConstants<float>::twoPi
                                             * 4500.f / (float) sr);
                for (int i = 0; i < n; ++i)
                {
                    const float off = smooth[2].getNextValue();
                    for (int c = 0; c < chUsed; ++c)
                    {
                        auto* d = buffer.getWritePointer (c);
                        auto& unit = echo[(size_t) c];
                        unit.readOffset = off;
                        const float y = unit.processLooped (d[i], v.feedback, v.driveDb, damp);
                        d[i] = d[i] + y;
                    }
                }
                setFilter (v.cutoffHz, 0.707f, justChanged);
                processFilter (buffer, ch, n);
                break;
            }

            case 4:  // Shimmer Verb: reverb with an octave-up feedback bloom, then tone.
            {
                const int chUsed = juce::jmin (ch, 2);
                juce::dsp::Reverb::Parameters rp;
                rp.roomSize   = juce::jlimit (0.f, 1.f, v.reverbSize);
                rp.damping    = 0.4f;
                rp.wetLevel   = 1.f;  // the wrapper replaces the block with the tail;
                rp.dryLevel   = 0.f;  // the dry/wet blend happens below, per sample
                rp.width      = 1.f;
                rp.freezeMode = 0.f;
                reverb.setParameters (rp);
                setFilter (v.cutoffHz, 0.707f, justChanged);

                // Dry is stashed per chunk on the stack so the reverb can work in
                // place without a second heap buffer. 4096 frames = 16 kB/channel.
                constexpr int kChunk = 4096;
                float dry[2][kChunk];
                const float damp = std::exp (-juce::MathConstants<float>::twoPi
                                             * 6000.f / (float) sr);
                for (int start = 0; start < n; start += kChunk)
                {
                    const int m = juce::jmin (kChunk, n - start);
                    for (int c = 0; c < chUsed; ++c)
                        juce::FloatVectorOperations::copy (dry[(size_t) c],
                                                           buffer.getReadPointer (c) + start, m);

                    auto blk = juce::dsp::AudioBlock<float> (buffer)
                                   .getSubsetChannelBlock (0, (size_t) chUsed)
                                   .getSubBlock ((size_t) start, (size_t) m);
                    reverb.process (juce::dsp::ProcessContextReplacing<float> (blk));

                    for (int i = 0; i < m; ++i)
                    {
                        const float hz = smooth[0].getNextValue();
                        const float r = smooth[1].getNextValue();
                        for (int c = 0; c < chUsed; ++c)
                        {
                            auto* d = buffer.getWritePointer (c);
                            auto& unit = shimmerDelay[(size_t) c];
                            // The read head sweeps at twice the write speed:
                            // shOff shrinks by 1 per sample, so read_pos =
                            // write - shOff advances by 2. The tail replays at
                            // double rate, i.e. an octave up: the budget
                            // shimmer. Artifacts are the character, not a bug.
                            // Loop gain is capped by shimmer, so the bloom
                            // decays instead of running away.
                            shOff[(size_t) c] -= 1.f;
                            if (shOff[(size_t) c] < 1.f)
                                shOff[(size_t) c] += (float) Delay::kMaxSamples;
                            unit.readOffset = shOff[(size_t) c];
                            const float wet = d[start + i];
                            const float up = unit.process (wet + v.shimmer * zSh[(size_t) c]);
                            zSh[(size_t) c] = damp * zSh[(size_t) c] + (1.f - damp) * up;
                            const float out = dry[(size_t) c][i] * (1.f - v.mix)
                                              + (wet + v.shimmer * zSh[(size_t) c]) * v.mix;
                            auto& f = lp[(size_t) c];
                            f.setCutoffFrequency (hz);
                            f.setResonance (r);
                            d[start + i] = f.processSample (c, out);
                        }
                    }
                }
                break;
            }

            case 5:  // Phaser Sweep: swept all-pass stack, rate and depth from the knob.
            {
                const int chUsed = juce::jmin (ch, 2);
                if (justChanged)
                    smooth[3].setCurrentAndTargetValue (v.phaserRateHz);
                else
                    smooth[3].setTargetValue (v.phaserRateHz);
                phaser.setRate (smooth[3].skip (n));
                phaser.setDepth (v.phaserDepth);
                phaser.setFeedback (v.feedback);
                phaser.setMix (1.f);
                // Depth-linked trim: the comb peaks get louder as feedback rises.
                const float trim = 0.7f + 0.1f * v.phaserDepth;
                auto blk = juce::dsp::AudioBlock<float> (buffer)
                               .getSubsetChannelBlock (0, (size_t) chUsed);
                phaser.process (juce::dsp::ProcessContextReplacing<float> (blk));
                for (int c = 0; c < chUsed; ++c)
                {
                    auto* d = buffer.getWritePointer (c);
                    for (int i = 0; i < n; ++i)
                        d[i] *= trim;
                }
                break;
            }

            default:
                break;
        }
    }

private:
    void setFilter (float hz, float res, bool snap = false)
    {
        const float nyq = (float) (sr * 0.5) - 1.f;
        const float c = juce::jlimit (20.f, nyq, hz);
        const float r = juce::jlimit (0.1f, 1.0f, res);
        if (snap)
        {
            smooth[0].setCurrentAndTargetValue (c);
            smooth[1].setCurrentAndTargetValue (r);
        }
        else
        {
            smooth[0].setTargetValue (c);
            smooth[1].setTargetValue (r);
        }
    }

    // Sample-major so the smoothers advance once per sample, not once per channel.
    void processFilter (juce::AudioBuffer<float>& buffer, int ch, int n)
    {
        for (int i = 0; i < n; ++i)
        {
            const float hz = smooth[0].getNextValue();
            const float r = smooth[1].getNextValue();
            for (int c = 0; c < ch; ++c)
            {
                auto& f = lp[(size_t) c];
                f.setCutoffFrequency (hz);
                f.setResonance (r);
                auto* d = buffer.getWritePointer (c);
                d[i] = f.processSample (c, d[i]);
            }
        }
    }

    double sr = 44100.0;
    std::array<juce::dsp::StateVariableTPTFilter<float>, 2> lp;
    std::array<BitCrusher, 2> crush;
    std::array<Delay, 2> echo;
    std::array<Delay, 2> shimmerDelay;
    std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>, 4> smooth;
    juce::dsp::Phaser<float> phaser;
    juce::dsp::Reverb reverb;

    // Shimmer loop, per channel: the read/write distance in samples (locked to
    // 1x, so the 2x read doubles the replay rate) and the filtered loop state.
    std::array<float, 2> shOff;
    std::array<float, 2> zSh;
    int currentPreset = -1;
};

} // namespace fn0b
