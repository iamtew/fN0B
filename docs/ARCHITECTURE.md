# Architecture

PluginProcessor owns the APVTS and one Chain. The editor owns a knob, a preset bar, and in/out trim sliders that also meter.

Audio thread:

1. Apply input gain (smoothed) to the buffer, then note the input peak
2. Read the preset index and the knob value from the APVTS
3. Resolve them to a set of unit parameters via `fn0b::voiceFor`
4. Run the preset's chain over the buffer, in the order that preset specifies
5. Apply output gain (smoothed), then note the output peak
6. Write the result back in place

The chain is a `switch` on the preset, not a graph. Each preset hardcodes its own order, because the order is part of what the preset sounds like: Warm Drive clips before it filters, Filter Funk filters before it clips.

Message thread:

- The editor draws the knob, the preset bar, and the in/out sliders. Slider wells are meters (post-gain peaks).
- About box checks GitHub tags on a detached thread, result hopped back with `callAsync`.

## Files

- `src/dsp/Presets.h` — the knob-to-parameter mapping. JUCE-free, so the check can compile it with plain `cl`.
- `src/dsp/Units.h` — JUCE-free units: clip, crush, delay, 8-line FDN, 6-stage phaser, peak comp, formants, grain, reverse. Sound first: these exist because they beat the stock JUCE boxes for this plugin.
- `src/dsp/Chain.h` — the JUCE side is the TPT SVFs (LP/HP/BP) and the per-sample smoothers. Reverb, phaser, chorus, grain and the rest are our units. One `switch` on the preset, not a graph.

A preset that needs a unit it does not have yet adds it to `Units.h` and one `case` in `Chain::process`. Units are written once and shared. Do not copy sibling plugin source into this tree.
