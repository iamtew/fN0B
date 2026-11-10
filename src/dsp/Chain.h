#pragma once

#include "Presets.h"
#include "Units.h"

#include <JuceHeader.h>
#include <array>

namespace fn0b
{

// Runs the preset's effect chain over a stereo buffer. The chain order differs per
// preset (clip-then-filter vs filter-then-clip), so it is a switch rather than a
// slot graph: three presets do not justify a generic effect host.
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

        // One-pole de-click on cutoff, resonance and drive, so knob moves do not zipper.
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

        if (preset != currentPreset)
        {
            // Snap the smoothers on a preset change so the new chain starts settled.
            for (auto& s : smooth)
            {
                s.setCurrentAndTargetValue (s.getTargetValue());
            }
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
                setFilter (v.cutoffHz, v.res);
                processFilter (buffer, ch, n);
                break;
            }

            case 1:  // Filter Funk: resonant low-pass, then soft clip.
            {
                setFilter (v.cutoffHz, v.res);
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

            default:
                break;
        }
    }

private:
    void setFilter (float hz, float res)
    {
        const float nyq = (float) (sr * 0.5) - 1.f;
        const float c = juce::jlimit (20.f, nyq, hz);
        const float r = juce::jlimit (0.1f, 1.0f, res);
        smooth[0].setTargetValue (c);
        smooth[1].setTargetValue (r);
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
    std::array<juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>, 2> smooth;
    int currentPreset = -1;
};

} // namespace fn0b
