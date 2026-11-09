# fN0B - The Knob
VST3 audio effect, single knob, endless fun!

JUCE location `~/JUCE`

Read `./bootstrap/fN0B-Single_Knob_FX.md`

First pass:
- Bootstrap fN0B plugin.
- Implement knob.
- Implement 3 presets.

Update this `AGENTS.md` to be a more full-fledged file appropriate for configuring the agent.

## Color palette
Mint on deep teal-black. Drives `src/ui/Theme.h`, `docs/style.css`, and the og image.

**Colors:**
- #35E0A6 - Mint (accent, the knob cap)
- #F2C14E - Amber (secondary, selection, chevrons)
- #E8F5F0 - Near White (text)
- #7C918B - Sage Grey (secondary text, borders)
- #132020 - Panel
- #0C1414 - Chassis (background)
- #08100F - Ink (inset wells)

**Usage:**
- Primary: Mint
- Secondary: Amber
- Background: Chassis, panels sit on it
- Ink is for wells and inset surfaces, darker than the chassis

