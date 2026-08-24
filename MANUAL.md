# AirCast — User Manual

**Broadcast Voice Processor** · Fully Modulated · VST3 / AU / Standalone

## 1. What AirCast is

AirCast is a broadcast-style voice/mix processor: a compressor, tone shaping, a "format
character" stage modeled on the mix style of common radio formats, a noise expander/gate,
and a loudness-normalizing true-peak limiter, all in one chain. Point it at a vocal or a full
mix and it'll clean up noise floor, add density and warmth, shape the tone toward a chosen
on-air format, and land the output at a consistent, broadcast-safe loudness with a hard peak
ceiling.

It's stereo-in/stereo-out only (no mono or surround support).

## 2. Installing / finding the plugin

AirCast builds as three formats from one codebase:

- **VST3** — installs to `~/Library/Audio/Plug-Ins/VST3/AirCast.vst3`
- **AU** (Audio Unit) — for Logic, GarageBand, and other AU hosts
- **Standalone** — a self-contained app you can run without a DAW, for quick A/B listening

If your DAW doesn't see AirCast after installing, rescan your plugin folders (most DAWs have
a "rescan plugins" option in preferences).

## 3. The window, top to bottom

The plugin window is a fixed 900×650 and doesn't resize. From top to bottom:

- **Title bar** — "AirCast" / "BROADCAST VOICE PROCESSOR", a **RESET** button (snaps every
  parameter back to its default — this is the *global* default, not the currently-selected
  preset's values), and a live **LUFS readout** showing the current measured integrated
  loudness of what's coming out of the plugin.
- **Format Character** dropdown — selects one of 10 presets (§5). The `?` icon next to it
  explains what the dropdown does.
- **Left and right meter rails** — `INPUT` (far left) and `OUTPUT` (far right) show real
  peak level in and out of the plugin, independent of everything happening inside it.
- **Three knob sections** (§6):
  - **Tone & Character** — Density, Detail, Drive, Highs, Lows, Bass Char
  - **Dynamics / Cleanup** — Exp Thresh, Exp Ratio, Gate Thresh
  - **Loudness & Output** — LUFS Target, LUFS Drive, True Peak, Master Out
- **Toggle row** at the bottom — Mono Bass, Gate To Zero, AES Gating, 60s Window, and
  **Bypass** on the far right (shown in red — it's the one toggle that turns the whole plugin
  off).

Every control has a small `?` help badge next to its label — hover it for a quick tooltip,
click it for a longer persistent explanation.

## 4. Reading the knobs

Each knob is two things layered together, and telling them apart is the whole trick to
reading this interface at a glance:

- **The blue arc** around the outside of the knob shows *where the knob is set* — the
  parameter's current value. This only moves when you turn the knob.
- **The colored ring just inside that** (green → amber → red) is a **live meter**: it shows
  *what the signal is actually doing right now* at that stage — gain reduction, saturation
  amount, EQ activity, whatever's relevant to that control. This moves on its own, in
  real time, as audio passes through.

So a knob can be turned up high (big blue arc) while its meter ring stays green/quiet if
there's nothing for it to act on (e.g. Gate Thresh set aggressively but the input is never
quiet enough to trigger it) — and vice versa. The meter ring is diagnostic: it's the fastest
way to see which stages are actually doing work on your material.

The numeric readout under each knob shows the exact current value.

## 5. Format Character presets

The dropdown at the top sets ten different combinations of EQ, saturation, and compression
*feel* (attack/release speed) tuned to evoke the mix style of common radio formats. **This is
not a transmission simulation** — selecting Talk Radio won't make your output sound like
it's coming through a phone line or an actual AM tuner, except for Talk Radio and Late Night
Talk specifically, which do apply a real AM-bandwidth-style lowpass filter as part of their
character.

| Preset | Character |
|---|---|
| **Clean** | No coloration at all — every knob at its plain default. Selecting this is the same as hitting Reset. |
| **Talk Radio (AM)** | Vocal-forward, AM-bandwidth-limited (real ~6kHz lowpass), moderate compression. |
| **Shock Jock (FM)** | The hottest, punchiest preset — hyped low end, aggressive presence push, fast/pumpy compression. |
| **Public Radio** | The gentlest preset — wide dynamics, slow/transparent compression, smoothed top end, patient loudness averaging. |
| **Late Night Talk (AM)** | Tighter/boxier cousin of Talk Radio — narrower AM bandwidth, more low cut, more grit. |
| **Top 40 (CHR)** | Bright "smile curve" EQ (scooped mids, boosted highs/lows), tight fast pop compression. |
| **Classic Rock (FM)** | Warm low-mids, a little analog-style grit, "glued" rather than clamped compression feel. |
| **News Anchor** | Crisp, controlled, almost no saturation — clean and tight rather than colored. |
| **Urban/Hip-Hop** | The loudest and most bass-forward preset by design — fast low-band compression clamps kick/808 hits without ducking vocals. |
| **Classical/Fine Arts** | The "least processed" preset — wide dynamics, very slow gentle compression, no grit, transparent across the board. |

Selecting a preset instantly moves every knob to that preset's stored values — you can then
tweak any knob afterward without changing the preset selection itself; the dropdown just sets
starting points.

For the full technical breakdown of exactly what each preset changes internally, see
[PRESET_GUIDE.md](PRESET_GUIDE.md).

## 6. Controls reference

Controls are listed in signal-flow order — top to bottom is roughly the order audio actually
passes through them, except the Tone & Character row (Density/Drive/Detail happen before
Highs/Lows/Bass Char in the actual signal path even though they share a row visually).

### Dynamics / Cleanup (processed first)

| Control | Range | What it does |
|---|---|---|
| **Exp Thresh** | -80 to 0 dB | Downward expander threshold. Audio below this level is continuously, proportionally pulled down — not gated on/off — to tame reverb tails, room tone, and noise floor. Fast and transparent by design. |
| **Exp Ratio** | 1:1 to 10:1 | How hard the expander pulls level down once it's below threshold. 1:1 = no effect. |
| **Gate Thresh** | -80 to 0 dB | A second, harder cleanup stage below the expander. Audio below this level gets attenuated — or fully muted if **Gate To Zero** is on — cleaning up dead air between words/phrases. |

### Tone & Character

| Control | Range | What it does |
|---|---|---|
| **Density** | 0–100% | Broadcast-style compression amount. Higher = lower threshold, higher ratio, more squashed dynamic range — the main "always loud" broadcast sound. This is a 3-band compressor internally, so bass, mids, and highs are controlled somewhat independently. |
| **Detail** | 0–10 | A high-frequency exciter that adds harmonic "air" above ~3.5 kHz — useful for restoring clarity/sparkle that heavy Density compression can dull. |
| **Drive** | 0–100% | Post-compression saturation/warmth — soft-clipping distortion blended in, and it also intensifies the selected format's own saturation character. |
| **Highs** | -12 to +12 dB | High-shelf EQ above 8 kHz. |
| **Lows** | -12 to +12 dB | Low-shelf EQ below 150 Hz. |
| **Bass Char** | 0–100% | Adds warm saturation specifically to the band below 120 Hz — thickens bass without touching mids or highs. |

### Loudness & Output (processed last)

| Control | Range | What it does |
|---|---|---|
| **LUFS Target** | -30 to -6 LUFS | The loudness the plugin continuously corrects toward. Typical broadcast targets sit around -24 to -14 LUFS. The live readout at the top of the window shows current measured loudness. |
| **LUFS Drive** | 0–100% | How aggressively the loudness correction is applied. 0% = measurement only, no gain change. 100% = full correction. The correction is always slew-limited (~6 dB/sec) so it can't react instantly and pump on transients. |
| **True Peak** | -3 to 0 dBFS | The hard ceiling. A 2×-oversampled limiter (catches inter-sample peaks a normal limiter would miss) keeps output from exceeding this level, after everything else in the chain. |
| **Master Out** | -24 to +12 dB | A final, plain output trim — applied after the limiter, so raising it *can* push you back over the True Peak ceiling. Every preset leaves this at 0 dB; it's yours to adjust for gain-staging into your next plugin/console. |

### Toggles

| Toggle | What it does |
|---|---|
| **Mono Bass** | Sums everything below 120 Hz to mono while keeping highs in stereo. Standard broadcast-chain move for a solid, mono-compatible low end. |
| **Gate To Zero** | Off = the gate gently turns down below threshold (4:1). On = it hard-mutes (~100:1). |
| **AES Gating** | When on, the loudness measurement ignores blocks quieter than -70 LUFS (silence/pauses), so they don't drag the running average down and distort the LUFS readout or correction. |
| **60s Window** | Switches loudness measurement from a fast ~3-second response to a slow ~60-second integrated average — use this for material with wide dynamic swings you don't want the normalizer chasing moment-to-moment. |
| **Bypass** | Passes audio through completely unprocessed. Useful for A/B comparison. |

## 7. Suggested starting points

- **Podcast/voice cleanup, minimal coloring:** start from **Clean** or **News Anchor**, raise
  Density modestly (40–60%), leave Bass Char and Drive low, use the expander/gate to clean up
  room tone and pauses.
- **"Radio ready" vocal:** pick the format closest to the target station sound, then use
  Detail to restore clarity if it feels dull, and nudge LUFS Target/Drive to taste rather than
  cranking Density further — Density interacts with the loudness stage (see §8), so loudness
  is often better controlled there than by pushing compression harder.
- **Full mix mastering-style pass:** lower LUFS Drive so the normalizer isn't overcorrecting,
  keep True Peak conservative (-1.5 dB or lower) for extra headroom, and watch the OUTPUT
  meter rail rather than relying on ear alone for the first pass.

## 8. Troubleshooting

**Output sounds pumped, squashed, or "over-modulated," especially on Shock Jock, Classic
Rock, or Urban/Hip-Hop.** These presets are intentionally the hottest in the set — high
Density plus a high LUFS Drive is a real interaction: heavier compression flattens the
dynamic range, and the loudness normalizer then pushes the flattened signal's average level
up harder to hit target, leaving the limiter working overtime. If it's too aggressive for
your material, back off **Density** and/or **LUFS Drive** first — that's usually the fastest
fix. The bass and drive saturation stages (Bass Char, Drive, and the format's own
character/grit) are tuned to add only harmonic color, not extra loudness, so if things still
sound hot after backing off Density/LUFS Drive, it's the compression/loudness interaction,
not the saturation stages.

**The meter ring inside a knob isn't moving.** That's often correct, not broken — it means
that stage isn't doing anything to your current material (e.g. Gate Thresh set below your
noise floor, so the gate never engages). Check the knob's blue value arc against your actual
input level.

**LUFS readout shows `--`.** The loudness measurement hasn't accumulated enough signal yet
(it reports `--` below about -69 LUFS measured, i.e. near-silence) — feed it real audio and
it'll populate.

**Section titles ("TONE & CHARACTER" etc.) aren't visible.** Update to the latest build —
this was a rendering bug in earlier versions where the section header text was erased on
every repaint tick; it's fixed as of this manual's writing.

## 9. Technical notes

- Stereo-only (mono/surround buses are not supported and won't load).
- Sample-rate agnostic — all filter cutoffs are recalculated against the current sample rate.
- True-peak limiting runs on a 2× oversampled signal specifically to catch inter-sample
  peaks that a plain limiter would let through.
- The loudness measurement is a practical BS.1770-style approximation (K-weighting + a
  running windowed average) for creative/monitoring use — it is not a certified loudness
  meter for delivery compliance. Verify final loudness with a calibrated meter before
  delivering to a platform with strict loudness specs.
