# AirCast — UI Design Brief

## What this is

AirCast is an original audio plugin (VST3 / AU / Standalone, built in JUCE/C++ for macOS) — a broadcast-style
signal processor: compressor, tone shaping, a "format character" stage, a downward expander + noise gate,
and a loudness/true-peak limiter chain, aimed at giving vocals/mixes that dense, radio-ready sound.

**Important technical constraint:** this is a native desktop plugin window, not a web page. It's painted with
JUCE's C++ `Graphics` API (rectangles, ellipses, paths, gradients, text) — there's no HTML/CSS/DOM underneath.
Any visual design produced elsewhere (Claude Design, Figma, etc.) has to be **hand-translated into JUCE paint
code** afterward — it can't be dropped in directly. So treat this as **a look-and-feel reference** (colors,
proportions, knob styling, layout composition, iconography, typography pairing) rather than a shippable asset.
Flat/vector-style design translates far more easily than photographic textures, blurs, or complex layered
shadows — JUCE can do gradients and drop shadows but everything has to be redrawn as code, so simpler wins.

Fixed window size right now: 820×600, not resizable (open to changing this).

## Current visual style (what exists today — polish this, don't necessarily replace it)

- Outer background: near-black navy `#1c1f26`
- Inner panel: dark slate `#2a2f3a`, rounded corners
- Knobs: custom-painted rotary dials with three concentric rings:
  1. Background value track: `#3a3f4b`
  2. Value arc (shows the parameter's current setting): sky blue `#5ac8fa`
  3. **Live level-meter ring**, drawn *inside* the knob, color-coded by how hot the signal is at that stage:
     green `#2ecc71` → amber `#f5a623` → red `#e74c3c`
  - Knob face: dark disc `#20232c` with a white pointer/indicator line
- Text: white on dark, ~13px labels, 24px bold title
- Small circular "?" help badges (`#4a5468`, brightens to `#5ac8fa` on hover) sit next to each control's
  label — hover shows a tooltip, click shows a persistent popup with a longer explanation
- No branding/logo yet

## Layout structure

```
┌─────────────────────────────────────────────────────────┐
│  AirCast                                        [Reset]  │
│  [Format Character dropdown ▾] [?]      "-16.2 LUFS"     │
│                                                            │
│  Row 1 — Tone & Character                                │
│  (Density) (Detail) (Drive) (Highs) (Lows) (Bass Char)    │
│                                                            │
│  Row 2 — Dynamics / Cleanup                               │
│  (Exp Thresh) (Exp Ratio) (Gate Thresh)                   │
│                                                            │
│  Row 3 — Loudness & Output                                │
│  (LUFS Target) (LUFS Drive) (True Peak) (Master Out)      │
│                                                            │
│  [Mono Bass] [Gate To Zero] [AES Gating] [60s Window] [Bypass] │
└─────────────────────────────────────────────────────────┘
```
Each `(Knob)` is: help-icon-next-to-label, then a rotary knob with the built-in meter ring described above.
The bottom row is five toggle switches (checkboxes today — open to a nicer toggle/switch visual).

## Vibe direction (starting point — change freely)

Think professional broadcast hardware processor (Omnia, Orban Optimod, TC Electronic rack gear) rather than
a soft "modern SaaS" look: dark chassis, precise typography, LED-style meter accents, a sense of "engineering
tool" rather than "consumer app." Should feel confident and a little utilitarian, not playful.

## Full control list (for labeling/grouping reference)

**Format Character** (dropdown, 10 options) — tonal EQ + compression-feel + saturation preset:
Clean, Talk Radio (AM), Shock Jock (FM), Public Radio, Late Night Talk (AM), Top 40 (CHR),
Classic Rock (FM), News Anchor, Urban/Hip-Hop, Classical/Fine Arts

| Control | Range | Default | What it does |
|---|---|---|---|
| Density | 0–100% | 50% | Broadcast-style compression amount |
| Detail | 0–10 | 1 | High-frequency exciter/clarity, adds harmonic "air" |
| Drive | 0–100% | 25% | Post-compression saturation/warmth |
| Highs | -12–+12 dB | 0 | High-shelf EQ above 8kHz |
| Lows | -12–+12 dB | 0 | Low-shelf EQ below 150Hz |
| Bass Char | 0–100% | 0% | Low-end saturation warmth |
| Exp Thresh | -80–0 dB | -45 | Downward expander threshold (fast, transparent noise/reverb cleanup) |
| Exp Ratio | 1–10 | 2 | Downward expander ratio |
| Gate Thresh | -80–0 dB | -55 | Noise gate threshold (hard mute for dead air) |
| LUFS Target | -30–-6 | -14 | Target loudness for the normalizer |
| LUFS Drive | 0–100% | 0% | How hard the loudness normalizer corrects toward target |
| True Peak | -3–0 dBFS | -0.5 | True-peak limiter ceiling |
| Master Out | -24–+12 dB | 0 | Final output trim |

Toggles: **Mono Bass** (sum lows to mono), **Gate To Zero** (hard mute vs. gentle gate), **AES Gating**
(loudness measurement ignores silence), **60s Window** (slow vs. fast loudness averaging), **Bypass**.

## What I'd want back from a design pass

- A color palette (can keep or replace the current dark navy/slate/blue scheme)
- Knob face + meter-ring treatment — how to make "value arc + live level meter in one dial" read clearly
  at a glance, ideally with a clearer visual distinction between "where the knob is set" and "what the
  signal is doing right now"
- Typography pairing (family + weights) for title / labels / value readouts
- A toggle-switch visual nicer than a plain checkbox
- Overall composition/spacing — is the 3-row-of-knobs + toggle-row layout the right structure, or is there
  a better way to group these 13 controls + 1 dropdown + 5 toggles
- Ideally a static mockup image of the full panel I can use as a reference while I rewrite the JUCE
  `LookAndFeel`/paint code to match
