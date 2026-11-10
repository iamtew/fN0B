#pragma once

#include <JuceHeader.h>

namespace fn0b
{

// Palette for fN0B. Mint on deep teal-black. Keep in step with the AGENTS.md list,
// docs/style.css and the og image.
inline const juce::Colour kMint    { 0xff35e0a6 };  // accent, the knob flange and cap
inline const juce::Colour kAmber   { 0xfff2c14e };  // ticks, selection
inline const juce::Colour kLight   { 0xffe8f5f0 };  // text
inline const juce::Colour kGrey    { 0xff7c918b };  // secondary text, borders
inline const juce::Colour kPanel   { 0xff132020 };
inline const juce::Colour kChassis { 0xff0c1414 };
inline const juce::Colour kInk     { 0xff08100f };  // inset wells

inline juce::Font boldFont (float pt)
{
    return juce::Font (juce::FontOptions (pt).withStyle ("Bold"));
}

} // namespace fn0b
