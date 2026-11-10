# fN0B – Cursor Rules

Spec: [docs/SPEC.md](docs/SPEC.md).

Single knob effect. Pick a preset, turn the knob. VST3. Part of Stupid Audio; sister of fR3Z0.

## Stack
- JUCE (Projucer workflow only – no CMake)
- C++17, real-time safe: no allocations, no locks, no system calls in processBlock
- APVTS for all parameters
- Just + justfile
- JUCE from `%USERPROFILE%\JUCE`
- `assets/logo.png` is the Affinity export. Source of truth. Never overwrite. `just icon` / `just docs-logo` derive the rest.

## Layout
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

## Version
- Jucer `version=` is what the binary and About dialog show.
- Git tags must match that string (`0.0.1`).
- `just build-dist` names the zip and MSI from the jucer version.

## Presets
- A preset is one effect or a short chain; the knob or the preset selector is all the user gets.
- The knob-to-parameter mapping lives in `src/dsp/Presets.h` and stays JUCE-free so `tests/fn0b_check.cpp` can compile it with plain `cl`.
- Chain order is per preset and is part of the sound. Do not make it configurable.

## Colors
- #35e0a6 Mint (accent, the knob flange and body)
- #f2c14e Amber (flutes, pointer, selection)
- #e8f5f0 Near White (text)
- #7c918b Sage Grey (secondary text, borders)
- #132020 Panel
- #0c1414 Chassis (background)
- #08100f Ink (inset wells)

## Do NOT
- Introduce CMake
- Allocate on the audio thread
- Build a generic effect graph, a reorderable chain, or a modular rack
- Copy commercial plugin code
