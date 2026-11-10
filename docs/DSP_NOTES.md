# DSP notes

- Soft clip is `tanh(x * 10^(driveDb/20))`. The knob maps drive in dB, not as a raw gain, so the first half of the sweep is subtle and the last quarter is where it bites. tanh saturates to ±1, so output can never exceed full scale no matter the drive.
- Low-pass is a `juce::dsp::StateVariableTPTFilter<float>` per channel. Cutoff and resonance are set per sample from a `SmoothedValue`, so a fast knob move glides instead of zippering. Resonance 1/sqrt(2) is flat; the Filter Funk preset pushes it toward 0.9 for the squelch.
- Bit crusher holds each input sample for `div` samples, then quantises to `bits` bits. At 1x / 16 bits it is inaudible; at 16x / 4 bits it is a fuzz pedal. One instance per channel.
- Delay is a 96 k-sample circular buffer per channel, linear-interpolated, living in the Chain so it is never allocated on the audio thread. Tape Echo saturates the dry+regen sum onto the loop and damps with a one-pole; the output high-cut is the shared SVF. Sweeping the read offset bends pitch, which is the tape.
- Shimmer Verb is `juce::dsp::Reverb` with a second delay whose read head advances at 2x (octave up). The doubled tail is raw on purpose. Dry/wet is the only preset that needs an explicit mix; the others are full-chain.
- Phaser Sweep is `juce::dsp::Phaser` (six all-pass stages). Rate is smoothed; a depth-linked trim keeps the comb peaks polite.
- Warm Drive runs clip → filter. Filter Funk runs filter → clip. The order is the sound; it is not configurable.
- Preset changes reset the delay/reverb/phaser state and snap the smoothers so the new chain starts settled rather than sliding in.
- Nothing here allocates, locks, or calls into the system on the audio thread. Filters, reverb and phaser are prepared in `prepareToPlay`; the rest is plain arithmetic.
- Dry/wet is implicit. Each preset is a full-chain effect; there is no separate mix because the knob already covers the range from subtle to extreme.
