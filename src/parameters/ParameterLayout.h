#pragma once

#include "ParameterIDs.h"
#include "../dsp/Presets.h"

#include <JuceHeader.h>

// Knob + preset, plus in/out trim (dB). The trim is not part of the preset mapping.
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

    auto gainText = [] (float v, int)
    {
        return String (v, 1) + " dB";
    };
    auto gainParse = [] (const String& t)
    {
        return jlimit (-12.f, 12.f, t.getFloatValue());
    };

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::inputGain, 1 }, "Input Gain",
        NormalisableRange<float> (-12.f, 12.f), 0.f,
        AudioParameterFloatAttributes()
            .withStringFromValueFunction (gainText)
            .withValueFromStringFunction (gainParse)));

    layout.add (std::make_unique<AudioParameterFloat> (
        ParameterID { ParamID::outputGain, 1 }, "Output Gain",
        NormalisableRange<float> (-12.f, 12.f), 0.f,
        AudioParameterFloatAttributes()
            .withStringFromValueFunction (gainText)
            .withValueFromStringFunction (gainParse)));

    return layout;
}
