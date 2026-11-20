# fN0B spec

This file is the product spec.

Single-knob effect. A preset is one effect or a short chain, and the knob moves that preset's key parameters together. Sister of fR3Z0, part of Stupid Audio. MIT.

- One knob, one preset selector, plus in/out trim sliders (not extra knobs)
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

## Sound first

The product is the sound. JUCE `dsp` is a convenience, not a ceiling. Choose the implementation that sounds best for that preset. Prefer a small custom unit in `src/dsp/Units.h` when we can hear the difference. Keep JUCE when it is already the right algorithm (TPT SVF). Do not copy sibling source into this tree. No alloc, locks, or system calls in `processBlock`.

## Presets

Shipped in code. The knob-to-parameter mapping lives in `src/dsp/Presets.h`. Indices 0–5 stay put so old sessions do not jump.

| Preset | Chain | Knob 0 → 100% |
| --- | --- | --- |
| Warm Drive | soft clip → low-pass | drive 0→+18 dB, cutoff 20 kHz→4 kHz |
| Filter Funk | low-pass → soft clip | cutoff 200→8000 Hz, resonance 0.2→0.9, drive 0→+12 dB |
| Bitcrush Destruct | sample-rate reduce → bit reduce → soft clip | rate 1x→16x, bits 16→4, drive 0→+12 dB |
| Tape Echo | delay (damped, clipped feedback) → low-pass | delay 60→450 ms, feedback 15→60%, drive 0→+6 dB, cutoff 12 kHz→2.5 kHz |
| Shimmer Verb | FDN reverb + octave-up inject → low-pass | room 0.15→1, wet 10→45%, shimmer 0→50%, cutoff 20 kHz→6 kHz |
| Phaser Sweep | 6-stage allpass phaser | rate 0.12→3.5 Hz, depth 0.2→1, feedback 10→70% |
| Chorus Dimension | dual modulated delays | rate, depth/detune, mix + width |
| Dual Delay Cascade | short delay → long delay, shared feedback | time scale, feedback, loop high-cut |
| Tremolo Pulse | amplitude LFO → peak sat | rate, depth, sine→square + drive |
| Space Station | modulated delay → FDN → pitch drift | size (delay+decay), mod depth, feedback |
| Crush & Bloom | sat → high-pass → short FDN | drive, HP opens, reverb mix/decay |
| Meltglass | sat → chorus → low-pass | drive 0→+18 dB, chorus depth 0→70%, rate 0.08→0.4 Hz, LP 20 kHz→4 kHz |
| Parking Garage | comb → FDN | comb 8→45 ms, FB 10→85%, wet 5→60%, size 20→100% |
| Broken Broadcast | bandpass → crush → noise | BP 3 kHz→1.2 kHz, bits 16→4, rate 1x→16x, noise 0→12% |
| Tape Ghost | sat → wow/flutter → high shelf | sat 0→70%, wow 0→0.8%, flutter 0→0.3%, shelf 0→−12 dB |
| Rubber Room | delay → pitch modulation | delay 40→300 ms, FB 0→75%, mod 0→20 cents, wet 0→50% |
| Laser Hive | resonant high-pass → phaser | HP 80 Hz→2.5 kHz, res 0.2→0.9, depth 10→100%, FB 0→70% |
| Velvet Damage | soft clip → peak compressor | drive 0→+24 dB, ratio 1.5→8, auto makeup |
| Event Horizon | FDN → low-pass → sat | size 10→100%, decay 0.5→20 s, LP 20 kHz→2 kHz, sat 0→30% |
| Shatterbox | 3-band distortion | lo 0→+6 dB, mid 0→+18 dB, hi 0→+30 dB, crossovers drift up |
| Hypercolor | tilt EQ → width → exciter | tilt flat→+8 dB, width 100→180%, excite 0→60% |
| Gravity Well | pitch down → feedback delay → resonant BP in loop | dive, FB+res, delay stretches |
| Glitch Garden | sample-and-hold scramble → crush → freeze | density, bits+rate, grain size |
| Unstable Orbit | dual detuned delays, opposite LFO → sat | mod depth, cross-FB, time base |
| Bone Saw | wavefold → comb → hard clip | fold, comb freq+res, clip |
| Time Parasite | reverse → pitch up → reverse with feedback | window, pitch, regeneration |
| Formant Storm | vowel bank → formant LFO → drive | morph, mod speed/depth, drive |
| Black Hole Chorus | chorus → pitch-down wet → huge FDN | chorus, dive, size/decay |
| Circuit Melt | ring mod → crush → screaming filter | ring Hz, crush, cutoff+res |
| Echo Psychosis | triple delay + pitch drift + shared FB | time scale, drift, feedback |
| Singularity | pitch up → grain cloud → FDN freeze | height, grain density/size, freeze |

## Do not

- Generic effect-graph host or reorderable chain
- CMake (Projucer only)
- Allocations, locks, or system calls in `processBlock`
- Commercial plugin code
- Copy sibling plugin source into this tree

## Stack

- JUCE from `%USERPROFILE%\JUCE`, Projucer workflow only
- C++17, APVTS for all parameters
- Just + justfile

## UI

Mint on deep teal-black. Do not invent another palette.

- `#35e0a6` mint (accent, the knob flange and cap)
- `#f2c14e` amber (ticks, pointer, selection)
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
