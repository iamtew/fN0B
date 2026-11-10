#pragma once

#include "ParameterIDs.h"
#include "../dsp/Presets.h"

#include <JuceHeader.h>

// Two parameters: the knob (0..1, shown as a percentage) and the preset choice.
inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using namespace juce;

    AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::knob, 1 }, "Knob",
        NormalisableRange<float> (0.f, 1.f), 0.f,
        AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int)
            {
                return String (roundToInt (v * 100.f)) + "%";
            })
            .withValueFromStringFunction ([] (const String& t)
            {
                return jlimit (0.f, 1.f, t.getFloatValue() / 100.f);
            })));

    layout.add (std::make_unique<AudioParameterChoice> (
        ParameterID { ParamID::preset, 1 }, "Preset",
        StringArray (fn0b::kPresetNames, fn0b::kPresetCount), 0));

    return layout;
}
