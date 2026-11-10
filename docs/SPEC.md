# fN0B spec

This file is the product spec.

Single-knob effect. A preset is one effect or a short chain, and the knob moves that preset's key parameters together. Sister of fR3Z0, part of Stupid Audio. MIT.

- One knob, one preset selector, nothing else
- Presets are fixed chains, tuned so every knob position sounds musical
- Not a generic effect host, not a modular rack

## Product

- VST3
- Stereo in, stereo out. No MIDI, no sidechain, no extra buses.
- Knob is 0..1, shown as a percentage. Double-click resets to 0%.
- Preset bar: arrows step, middle opens the list
- Window scales 40–150%, aspect locked
- About + GitHub tag check (title-bar name + version)
- Jucer `version=` is the binary, About dialog, git tag, and `just build-dist` name. Tags match that string (no `v` prefix).

Default sound: Warm Drive at 0%, a clean pass-through.

## Presets

Shipped in code. The knob-to-parameter mapping lives in `src/dsp/Presets.h`.

| Preset | Chain | Knob 0 → 100% |
| --- | --- | --- |
| Warm Drive | soft clip → low-pass | drive 0→+18 dB, cutoff 20 kHz→4 kHz |
| Filter Funk | low-pass → soft clip | cutoff 200→8000 Hz, resonance 0.2→0.9, drive 0→+12 dB |
| Bitcrush Destruct | sample-rate reduce → bit reduce → soft clip | rate 1x→16x, bits 16→4, drive 0→+12 dB |
| Tape Echo | delay (damped, clipped feedback) → low-pass | delay 60→450 ms, feedback 15→60%, drive 0→+6 dB, cutoff 12 kHz→2.5 kHz |
| Shimmer Verb | reverb + octave-up feedback loop → low-pass | room 0.15→1, wet 10→45%, shimmer 0→50%, cutoff 20 kHz→6 kHz |
| Phaser Sweep | 6-stage phaser | rate 0.12→3.5 Hz, depth 0.2→1, feedback 10→70% |

## Do not

- Generic effect-graph host or reorderable chain
- More than a handful of presets in the first pass
- CMake (Projucer only)
- Allocations, locks, or system calls in `processBlock`
- Commercial plugin code

## Stack

- JUCE from `%USERPROFILE%\JUCE`, Projucer workflow only
- C++17, APVTS for all parameters
- Just + justfile

## UI

Mint on deep teal-black. Do not invent another palette.

- `#35e0a6` mint (accent, the knob flange and cap)
- `#f2c14e` amber (ticks, selection)
- `#e8f5f0` near white (text)
- `#7c918b` sage grey (secondary text, borders)
- `#132020` panel
- `#0c1414` chassis (background)
- `#08100f` ink (inset wells)

`assets/logo.png` is the Affinity export. Source of truth. Never overwrite. `just icon` / `just docs-logo` derive the rest.

## Layout

```
src/
  PluginProcessor.h/.cpp
  PluginEditor.h/.cpp
  dsp/Presets.h Units.h Chain.h
  parameters/ParameterIDs.h ParameterLayout.h
  ui/Theme.h VersionCompare.h
tests/fn0b_check.cpp
docs/
justfile
fN0B.jucer
```

README stays small. LICENSE is MIT. `docs/` is the gh-pages site.
