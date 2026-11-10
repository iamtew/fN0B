# Architecture

PluginProcessor owns the APVTS and one Chain. The editor owns a knob and a preset bar.

Audio thread:

1. Read the preset index and the knob value from the APVTS
2. Resolve them to a set of unit parameters via `fn0b::voiceFor`
3. Run the preset's chain over the buffer, in the order that preset specifies
4. Write the result back in place

The chain is a `switch` on the preset, not a graph. Each preset hardcodes its own order, because the order is part of what the preset sounds like: Warm Drive clips before it filters, Filter Funk filters before it clips.

Message thread:

- The editor draws the knob and the preset bar. Nothing else.
- About box checks GitHub tags on a detached thread, result hopped back with `callAsync`.

## Files

- `src/dsp/Presets.h` — the knob-to-parameter mapping. JUCE-free, so the check can compile it with plain `cl`.
- `src/dsp/Units.h` — `softClip`, `BitCrusher`, `Delay`. Also JUCE-free.
- `src/dsp/Chain.h` — the JUCE side: two `StateVariableTPTFilter`s, crushers, delay lines, `juce::dsp::Reverb`, `juce::dsp::Phaser`, and the per-sample smoothers.

A preset that needs a unit it does not have yet adds it to `Units.h` (or a JUCE `dsp` widget in `Chain.h`) and one `case` in `Chain::process`. Units are written once and shared.
