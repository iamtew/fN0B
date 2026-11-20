# DSP notes

The product is the sound. JUCE `dsp` is a convenience, not a ceiling. Custom units live in `src/dsp/Units.h` so `just check` can hear the maths without JUCE. TPT SVF stays JUCE because it is already the good filter. Freeverb and the stock phaser/chorus do not; we use an 8-line Householder FDN, a 6-stage allpass phaser, and dual-delay chorus instead. Sibling plugins are a reference, not a copy source.

- Soft clip is `tanh(x * 10^(driveDb/20))`. The knob maps drive in dB, not as a raw gain, so the first half of the sweep is subtle and the last quarter is where it bites. tanh saturates to ±1, so output can never exceed full scale no matter the drive.
- Low/high/band-pass is a `juce::dsp::StateVariableTPTFilter<float>` per channel. Cutoff and resonance are set per sample from a `SmoothedValue`, so a fast knob move glides instead of zippering. Resonance 1/sqrt(2) is flat; Filter Funk pushes it toward 0.9 for the squelch.
- Bit crusher holds each input sample for `div` samples, then quantises to `bits` bits. At 1x / 16 bits it is inaudible; at 16x / 4 bits it is a fuzz pedal. One instance per channel.
- Delay is a 96 k-sample circular buffer per channel, linear-interpolated, living in the Chain so it is never allocated on the audio thread. Tape Echo saturates the dry+regen sum onto the loop and damps with a one-pole; the output high-cut is the shared SVF. Sweeping the read offset bends pitch, which is the tape.
- Shimmer Verb is the FDN with octave-up inject into the tank, then the TPT tone filter. The doubled tail is raw on purpose. Dry/wet is explicit only where the preset is a parallel space (verb, chorus, rubber room); the others are full-chain.
- Phaser Sweep is six first-order allpasses, stereo LFO, feedback from the knob. Rate is smoothed; a depth-linked trim keeps the comb peaks polite.
- Warm Drive runs clip → filter. Filter Funk runs filter → clip. The order is the sound; it is not configurable.
- Loop gains (shimmer, freeze, delay feedback, grain) are capped so 100% is extreme but decays instead of running away.
- Preset changes reset delay/reverb/phaser/grain state and snap the smoothers so the new chain starts settled rather than sliding in.
- Nothing here allocates, locks, or calls into the system on the audio thread. Filters are prepared in `prepareToPlay`; FDN and delay lines are fixed arrays on the Chain.
- Dry/wet is implicit unless that preset is a space sitting on the dry signal. The knob already covers subtle to extreme.
- Bipolar presets (31–40) keep the stored knob at 0..1. Throw is `2k − 1`; noon is rest. Halo Rail is the exception: noon is a light chorus, not dry.
