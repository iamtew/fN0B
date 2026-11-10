# DSP notes

- Soft clip is `tanh(x * 10^(driveDb/20))`. The knob maps drive in dB, not as a raw gain, so the first half of the sweep is subtle and the last quarter is where it bites. tanh saturates to ±1, so output can never exceed full scale no matter the drive.
- Low-pass is a `juce::dsp::StateVariableTPTFilter<float>` per channel. Cutoff and resonance are set per sample from a `SmoothedValue`, so a fast knob move glides instead of zippering. Resonance 1/sqrt(2) is flat; the Filter Funk preset pushes it toward 0.9 for the squelch.
- Bit crusher holds each input sample for `div` samples, then quantises to `bits` bits. At 1x / 16 bits it is inaudible; at 16x / 4 bits it is a fuzz pedal. One instance per channel.
- Warm Drive runs clip → filter. Filter Funk runs filter → clip. The order is the sound; it is not configurable.
- Preset changes snap the smoothers to their targets so the new chain starts settled rather than sliding in.
- Nothing here allocates, locks, or calls into the system on the audio thread. The two filters are prepared in `prepareToPlay`; the rest is plain arithmetic.
- Dry/wet is implicit. Each preset is a full-chain effect; there is no separate mix because the knob already covers the range from subtle to extreme.
