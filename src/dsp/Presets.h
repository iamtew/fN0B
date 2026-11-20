#pragma once

// The whole musical brain of fN0B: which preset, and how the one knob maps onto
// that preset's parameters. Deliberately free of JUCE so tests/fn0b_check.cpp can
// compile it with plain cl, the same way the sibling checks do.

namespace fn0b
{

inline constexpr int kPresetCount = 41;
inline constexpr int kFirstBipolar = 31;

inline const char* const kPresetNames[kPresetCount] = {
    "Warm Drive",        "Filter Funk",      "Bitcrush Destruct", "Tape Echo",
    "Shimmer Verb",      "Phaser Sweep",     "Chorus Dimension",  "Dual Delay Cascade",
    "Tremolo Pulse",     "Space Station",    "Crush & Bloom",     "Meltglass",
    "Parking Garage",    "Broken Broadcast", "Tape Ghost",        "Rubber Room",
    "Laser Hive",        "Velvet Damage",    "Event Horizon",     "Shatterbox",
    "Hypercolor",        "Gravity Well",     "Glitch Garden",     "Unstable Orbit",
    "Bone Saw",          "Time Parasite",    "Formant Storm",     "Black Hole Chorus",
    "Circuit Melt",      "Echo Psychosis",   "Singularity",
    "Filter Fork",       "Tiltfork",         "Width Rift",        "Pitch Pendulum",
    "Backspin",          "Halo Rail",        "Hollow Jet",        "Rust Chrome",
    "Squeeze Bloom",     "Satin Fold"
};

inline bool isBipolar (int preset)
{
    return preset >= kFirstBipolar && preset < kPresetCount;
}

// Knob 0..1 → −1..1 for bipolar presets. Noon is 0.5.
inline float signedThrow (float k)
{
    return 2.f * k - 1.f;
}

inline float mag (float t)
{
    return t < 0.f ? -t : t;
}

// One resolved set of unit parameters for the current knob position.
// Unused fields stay at their neutral value, so a preset only fills what it uses.
struct Voice
{
    float driveDb      = 0.f;
    float cutoffHz     = 20000.f;
    float res          = 0.707f;
    float crushDiv     = 1.f;
    float bits         = 16.f;
    float delayMs      = 0.f;
    float delayMs2     = 0.f;
    float feedback     = 0.f;
    float mix          = 0.f;
    float reverbSize   = 0.f;
    float shimmer      = 0.f;
    float phaserRateHz = 0.f;
    float phaserDepth  = 0.f;
    float chorusRateHz = 0.f;
    float chorusDepth  = 0.f;
    float width        = 1.f;
    float tremRateHz   = 0.f;
    float tremDepth    = 0.f;
    float shape        = 0.f;
    float wow          = 0.f;
    float flutter      = 0.f;
    float shelfDb      = 0.f;
    float pitch        = 0.f;   // semitones (signed)
    float hpHz         = 20.f;
    float decay        = 0.f;   // 0..1 extra reverb decay
    float noiseMix     = 0.f;
    float combMs       = 0.f;
    float ratio        = 1.f;
    float driveLo      = 0.f;
    float driveMid     = 0.f;
    float driveHi      = 0.f;
    float tilt         = 0.f;
    float excite       = 0.f;
    float grain        = 0.f;
    float scramble     = 0.f;
    float ringHz       = 0.f;
    float formant      = 0.f;
    float freeze       = 0.f;
    float modDepth     = 0.f;
};

inline float lerp (float a, float b, float t)
{
    return a + (b - a) * t;
}

// knob 0..1 to unit parameters. Unipolar presets are monotonic in knob (the
// check asserts that). Bipolar presets (31+) map signedThrow(k) around noon.
inline Voice voiceFor (int preset, float knob)
{
    const float k = knob < 0.f ? 0.f : (knob > 1.f ? 1.f : knob);
    Voice v;

    switch (preset)
    {
        case 0:  // Warm Drive
            v.driveDb  = lerp (0.f, 18.f, k);
            v.cutoffHz = lerp (20000.f, 4000.f, k);
            break;

        case 1:  // Filter Funk
            v.cutoffHz = lerp (200.f, 8000.f, k);
            v.res      = lerp (0.2f, 0.9f, k);
            v.driveDb  = lerp (0.f, 12.f, k);
            break;

        case 2:  // Bitcrush Destruct
            v.crushDiv = lerp (1.f, 16.f, k);
            v.bits     = lerp (16.f, 4.f, k);
            v.driveDb  = lerp (0.f, 12.f, k);
            break;

        case 3:  // Tape Echo
            v.delayMs  = lerp (60.f, 450.f, k);
            v.feedback = lerp (0.15f, 0.6f, k);
            v.driveDb  = lerp (0.f, 6.f, k);
            v.cutoffHz = lerp (12000.f, 2500.f, k);
            break;

        case 4:  // Shimmer Verb
            v.reverbSize = lerp (0.15f, 1.f, k);
            v.mix        = lerp (0.1f, 0.45f, k);
            v.shimmer    = lerp (0.f, 0.5f, k);
            v.cutoffHz   = lerp (20000.f, 6000.f, k);
            break;

        case 5:  // Phaser Sweep
            v.phaserRateHz = lerp (0.12f, 3.5f, k);
            v.phaserDepth  = lerp (0.2f, 1.f, k);
            v.feedback     = lerp (0.1f, 0.7f, k);
            break;

        case 6:  // Chorus Dimension
            v.chorusRateHz = lerp (0.15f, 1.2f, k);
            v.chorusDepth  = lerp (0.12f, 0.85f, k);
            v.mix          = lerp (0.25f, 0.7f, k);
            v.width        = lerp (1.f, 1.6f, k);
            break;

        case 7:  // Dual Delay Cascade
            v.delayMs  = lerp (80.f, 320.f, k);
            v.delayMs2 = lerp (210.f, 640.f, k);
            v.feedback = lerp (0.15f, 0.62f, k);
            v.cutoffHz = lerp (12000.f, 2800.f, k);
            break;

        case 8:  // Tremolo Pulse
            v.tremRateHz = lerp (1.2f, 12.f, k);
            v.tremDepth  = lerp (0.25f, 1.f, k);
            v.shape      = lerp (0.f, 1.f, k);
            v.driveDb    = lerp (0.f, 6.f, k);
            break;

        case 9:  // Space Station
            v.delayMs    = lerp (90.f, 420.f, k);
            v.reverbSize = lerp (0.2f, 1.f, k);
            v.mix        = lerp (0.2f, 0.55f, k);
            v.modDepth   = lerp (0.f, 0.7f, k);
            v.feedback   = lerp (0.2f, 0.55f, k);
            break;

        case 10: // Crush & Bloom
            v.driveDb    = lerp (0.f, 18.f, k);
            v.hpHz       = lerp (80.f, 400.f, k);
            v.mix        = lerp (0.08f, 0.45f, k);
            v.reverbSize = lerp (0.12f, 0.55f, k);
            break;

        case 11: // Meltglass
            v.driveDb      = lerp (0.f, 18.f, k);
            v.chorusDepth  = lerp (0.f, 0.7f, k);
            v.chorusRateHz = lerp (0.08f, 0.4f, k);
            v.cutoffHz     = lerp (20000.f, 4000.f, k);
            v.mix          = 0.55f;
            break;

        case 12: // Parking Garage
            v.combMs     = lerp (8.f, 45.f, k);
            v.feedback   = lerp (0.1f, 0.85f, k);
            v.mix        = lerp (0.05f, 0.6f, k);
            v.reverbSize = lerp (0.2f, 1.f, k);
            break;

        case 13: // Broken Broadcast
            v.cutoffHz = lerp (3000.f, 1200.f, k);
            v.res      = 0.55f;
            v.bits     = lerp (16.f, 4.f, k);
            v.crushDiv = lerp (1.f, 16.f, k);
            v.noiseMix = lerp (0.f, 0.12f, k);
            break;

        case 14: // Tape Ghost
            v.driveDb = lerp (0.f, 12.f, k);          // sat 0→70% as dB-ish
            v.wow     = lerp (0.f, 0.008f, k);
            v.flutter = lerp (0.f, 0.003f, k);
            v.shelfDb = lerp (0.f, -12.f, k);
            break;

        case 15: // Rubber Room
            v.delayMs  = lerp (40.f, 300.f, k);
            v.feedback = lerp (0.f, 0.75f, k);
            v.modDepth = lerp (0.f, 0.2f, k);         // 20 cents at full as fraction of semitone
            v.mix      = lerp (0.f, 0.5f, k);
            break;

        case 16: // Laser Hive
            v.hpHz         = lerp (80.f, 2500.f, k);
            v.res          = lerp (0.2f, 0.9f, k);
            v.phaserDepth  = lerp (0.1f, 1.f, k);
            v.feedback     = lerp (0.f, 0.7f, k);
            v.phaserRateHz = lerp (0.3f, 2.2f, k);
            break;

        case 17: // Velvet Damage
            v.driveDb = lerp (0.f, 24.f, k);
            v.ratio   = lerp (1.5f, 8.f, k);
            break;

        case 18: // Event Horizon
            v.reverbSize = lerp (0.1f, 1.f, k);
            v.decay      = lerp (0.f, 1.f, k);        // 0.5 s → 20 s via FDN
            v.cutoffHz   = lerp (20000.f, 2000.f, k);
            v.driveDb    = lerp (0.f, 6.f, k);        // sat 0→30%
            v.mix        = lerp (0.25f, 0.85f, k);
            break;

        case 19: // Shatterbox
            v.driveLo  = lerp (0.f, 6.f, k);
            v.driveMid = lerp (0.f, 18.f, k);
            v.driveHi  = lerp (0.f, 30.f, k);
            v.cutoffHz = lerp (200.f, 320.f, k);      // lo|mid xover
            v.hpHz     = lerp (2500.f, 3800.f, k);    // mid|hi xover
            break;

        case 20: // Hypercolor
            v.tilt   = lerp (0.f, 8.f, k);
            v.width  = lerp (1.f, 1.8f, k);
            v.excite = lerp (0.f, 0.6f, k);
            break;

        case 21: // Gravity Well
            v.pitch    = lerp (-0.3f, -12.f, k);
            v.feedback = lerp (0.2f, 0.72f, k);
            v.res      = lerp (0.3f, 0.92f, k);
            v.delayMs  = lerp (120.f, 480.f, k);
            v.cutoffHz = lerp (900.f, 280.f, k);
            break;

        case 22: // Glitch Garden
            v.scramble = lerp (0.05f, 0.9f, k);
            v.bits     = lerp (12.f, 4.f, k);
            v.crushDiv = lerp (2.f, 16.f, k);
            v.grain    = lerp (0.08f, 0.7f, k);
            v.freeze   = lerp (0.f, 0.65f, k);
            break;

        case 23: // Unstable Orbit
            v.modDepth = lerp (0.05f, 0.85f, k);
            v.feedback = lerp (0.15f, 0.62f, k);
            v.delayMs  = lerp (90.f, 40.f, k);        // time base speeds up
            v.delayMs2 = lerp (130.f, 55.f, k);
            v.driveDb  = lerp (0.f, 6.f, k);
            v.mix      = 0.55f;
            break;

        case 24: // Bone Saw
            v.driveDb  = lerp (0.f, 18.f, k);         // fold amount via drive
            v.combMs   = lerp (12.f, 3.5f, k);
            v.feedback = lerp (0.2f, 0.8f, k);
            v.res      = lerp (0.3f, 0.9f, k);
            v.shape    = lerp (0.2f, 1.f, k);         // final clip
            break;

        case 25: // Time Parasite
            v.grain    = lerp (0.12f, 0.7f, k);       // reverse window
            v.pitch    = lerp (0.f, 7.f, k);
            v.feedback = lerp (0.1f, 0.55f, k);
            v.mix      = lerp (0.3f, 0.7f, k);
            break;

        case 26: // Formant Storm
            v.formant      = lerp (0.f, 1.f, k);
            v.tremRateHz   = lerp (0.2f, 8.f, k);
            v.modDepth     = lerp (0.1f, 0.8f, k);
            v.driveDb      = lerp (0.f, 10.f, k);
            break;

        case 27: // Black Hole Chorus
            v.chorusDepth  = lerp (0.3f, 0.95f, k);
            v.chorusRateHz = lerp (0.2f, 0.9f, k);
            v.pitch        = lerp (-0.2f, -12.f, k);
            v.reverbSize   = lerp (0.25f, 1.f, k);
            v.decay        = lerp (0.1f, 0.8f, k);
            v.mix          = lerp (0.3f, 0.7f, k);
            break;

        case 28: // Circuit Melt
            v.ringHz   = lerp (80.f, 1800.f, k);
            v.crushDiv = lerp (1.f, 12.f, k);
            v.bits     = lerp (12.f, 5.f, k);
            v.cutoffHz = lerp (8000.f, 400.f, k);
            v.res      = lerp (0.4f, 0.97f, k);
            break;

        case 29: // Echo Psychosis
            v.delayMs  = lerp (100.f, 380.f, k);
            v.delayMs2 = lerp (145.f, 510.f, k);
            v.modDepth = lerp (0.f, 0.55f, k);
            v.feedback = lerp (0.18f, 0.68f, k);
            v.mix      = lerp (0.25f, 0.6f, k);
            break;

        case 30: // Singularity
            v.pitch    = lerp (5.f, 19.f, k);
            v.grain    = lerp (0.15f, 0.8f, k);
            v.scramble = lerp (0.2f, 0.85f, k);
            v.reverbSize = lerp (0.3f, 1.f, k);
            v.freeze   = lerp (0.f, 0.8f, k);
            v.mix      = lerp (0.35f, 0.75f, k);
            break;

        case 31: // Filter Fork
        {
            const float t = signedThrow (k);
            const float a = mag (t);
            v.res = lerp (0.2f, 0.7f, a);
            if (t < 0.f)
            {
                v.cutoffHz = lerp (20000.f, 200.f, a);
                v.hpHz     = 20.f;
            }
            else
            {
                v.cutoffHz = 20000.f;
                v.hpHz     = lerp (20.f, 2500.f, a);
            }
            break;
        }

        case 32: // Tiltfork
        {
            const float t = signedThrow (k);
            v.tilt = t < 0.f ? lerp (0.f, -8.f, mag (t)) : lerp (0.f, 8.f, mag (t));
            break;
        }

        case 33: // Width Rift
        {
            const float t = signedThrow (k);
            v.width = t < 0.f ? lerp (1.f, 0.f, mag (t)) : lerp (1.f, 1.8f, mag (t));
            break;
        }

        case 34: // Pitch Pendulum
        {
            const float t = signedThrow (k);
            const float a = mag (t);
            v.pitch = t < 0.f ? lerp (0.f, -12.f, a) : lerp (0.f, 12.f, a);
            v.mix   = a;
            break;
        }

        case 35: // Backspin
        {
            const float t = signedThrow (k);
            const float a = mag (t);
            v.mix = lerp (0.f, 0.5f, a);
            if (t < 0.f)
            {
                v.delayMs = lerp (40.f, 300.f, a);
                v.shape   = 0.f;          // reverse
            }
            else
            {
                v.delayMs  = lerp (40.f, 450.f, a);
                v.feedback = lerp (0.f, 0.55f, a);
                v.shape    = 1.f;         // forward
            }
            break;
        }

        case 36: // Halo Rail
        {
            const float t = signedThrow (k);
            const float a = mag (t);
            if (t < 0.f)
            {
                v.delayMs      = lerp (12.f, 28.f, a);
                v.chorusDepth  = lerp (0.15f, 0.7f, a);
                v.chorusRateHz = lerp (0.15f, 0.8f, a);
                v.width        = lerp (1.f, 1.5f, a);
                v.mix          = lerp (0.2f, 0.55f, a);
            }
            else
            {
                v.delayMs      = lerp (12.f, 2.f, a);
                v.chorusDepth  = lerp (0.12f, 0.9f, a);
                v.chorusRateHz = lerp (0.15f, 0.8f, a);
                v.feedback     = lerp (0.f, 0.7f, a);
                v.mix          = lerp (0.2f, 0.7f, a);
                v.width        = 1.f;
            }
            break;
        }

        case 37: // Hollow Jet
        {
            const float t = signedThrow (k);
            const float a = mag (t);
            v.phaserRateHz = lerp (0.3f, 2.f, a);
            v.phaserDepth  = lerp (0.25f, 0.9f, a);
            v.feedback     = t < 0.f ? lerp (0.f, -0.7f, a) : lerp (0.f, 0.7f, a);
            break;
        }

        case 38: // Rust Chrome
        {
            const float t = signedThrow (k);
            const float a = mag (t);
            if (t < 0.f)
            {
                v.bits     = lerp (16.f, 6.f, a);
                v.crushDiv = lerp (1.f, 8.f, a);
                v.wow      = lerp (0.f, 0.006f, a);
                v.shelfDb  = lerp (0.f, -10.f, a);
            }
            else
            {
                v.shelfDb = lerp (0.f, 8.f, a);
                v.excite  = lerp (0.f, 0.55f, a);
            }
            break;
        }

        case 39: // Squeeze Bloom
        {
            const float t = signedThrow (k);
            const float a = mag (t);
            if (t < 0.f)
            {
                v.ratio = lerp (1.f, 8.f, a);
            }
            else
            {
                v.mix        = lerp (0.f, 0.45f, a);
                v.reverbSize = lerp (0.1f, 0.55f, a);
                v.ratio      = 1.f;
            }
            break;
        }

        case 40: // Satin Fold
        {
            const float t = signedThrow (k);
            const float a = mag (t);
            if (t < 0.f)
            {
                v.driveDb  = lerp (0.f, 18.f, a);
                v.cutoffHz = lerp (20000.f, 6000.f, a);
            }
            else
            {
                v.shape = lerp (0.f, 0.8f, a); // fold amount
            }
            break;
        }

        default:
            break;
    }

    return v;
}

} // namespace fn0b
