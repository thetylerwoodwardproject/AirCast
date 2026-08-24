# AirCast Signal Chain & Preset Reference

This document traces exactly what happens to audio in AirCast, stage by stage, and then
walks through each of the 10 Format Character presets to explain how their specific
parameter values play out through that chain. Source of truth: `Source/PluginProcessor.cpp`
(`processBlock`), `Source/Parameters.h` (preset table), and the DSP classes under `Source/DSP/`.

## 1. The signal chain, in order

Every block of audio passes through these stages in this exact order
(`PluginProcessor.cpp:117-193`):

```
input
  │
  ▼
1. Downward Expander   — DownwardExpander.h   (continuous, proportional noise/room-tone reduction)
  │
  ▼
2. Gate                — DownwardExpander.h   (same class, driven much harder → acts as a hard mute)
  │
  ▼
3. Density / Drive /    BroadcastCompressor.h (3-band compression, then wideband saturation
   Detail                                       + wideband HF exciter)
  │
  ▼
4. Highs / Lows /       ToneShaper.h          (shelving EQ, low-band saturation, mono-bass sum)
   Bass Character
  │
  ▼
5. Codec / Format       CodecStage.h          (per-format tonal EQ + AM bandwidth limiting
   Character                                    + saturation "grit")
  │
  ▼
6. LUFS Loudness        LoudnessProcessor.h   (continuous gain servo toward a target loudness)
   Normalization
  │
  ▼
7. True-Peak Limiter    juce::dsp::Limiter    (oversampled 2x, brickwall-ish ceiling)
  │
  ▼
8. Master Out trim      (plain gain, held at 0 dB by every preset)
  │
  ▼
output
```

Each stage reports a metering value the editor polls to drive the knob-ring meters
(`Meters` struct in `PluginProcessor.h`), but metering doesn't affect the audio — it's read-only.

## 2. What each stage actually does

### 2.1 Downward Expander (`expanderThreshold`, `expanderRatio`)

A continuous (not on/off) envelope follower with a 2 ms attack / 60 ms release. Below
`expanderThreshold`, it pulls gain down proportionally to how far below threshold the signal
is, scaled by `expanderRatio` — a ratio of 1 does nothing, higher ratios pull harder. It's
meant to quietly tame room tone and reverb tails without being audible as gating. It ignores
anything below -90 dBFS (true digital silence) so the meter doesn't chase noise-floor
artifacts.

### 2.2 Gate (`gateThreshold`, `gateToZero`)

The *exact same class* as the expander, just parameterized harder. `gateToZero` picks the
ratio: `true` → ratio 100 (effectively a hard mute below threshold), `false` → ratio 4 (a
gentler gate). This exists as a second instance of `DownwardExpander` rather than
`juce::dsp::NoiseGate` because JUCE's built-in gate has a fixed ~50 ms RMS smoothing window
that's too slow to catch speech pauses cleanly (see the comment in `PluginProcessor.h`).

### 2.3 Density / Drive / Detail (`BroadcastCompressor.h`)

Three independent controls, all inside one class:

- **Density** drives a genuine **3-band compressor** (crossovers at 180 Hz and 2.8 kHz, phase
  corrected with an allpass so the bands sum back to unity when no gain reduction is
  happening). `Density` 0→100 maps to threshold 0 → -28 dB and ratio 1:1 → 8:1 on all three
  bands, except the low band gets `ratio × 1.3` (capped at 12:1) since bass transients
  tolerate harder gain reduction before pumping becomes audible. **This compressor has no
  makeup gain of its own** — it can only ever reduce peak level, never add to it. Its real
  effect on perceived loudness is indirect: heavier compression flattens crest factor (the
  gap between peak and average level), and that flattened signal then gets pushed back up to
  the target LUFS by stage 6 — so a denser signal ends up closer to the peak ceiling by the
  time it reaches the limiter, which is what makes high-Density presets feel more processed
  and "hot," and why gain-reducing Density can still make things sound louder/more squashed
  downstream.
- **Drive** is a separate wideband saturation stage (`applyDrive`), a `tanh` waveshaper blended
  in proportional to the Drive amount.
- **Detail** is a high-frequency (>3.5 kHz) exciter: it highpasses a copy of the signal,
  runs it through a fixed `tanh(x*4)` shaper, and adds a scaled copy back in (mix 0→40% as
  Detail goes 0→10) for "air"/clarity.

The codec stage (§2.5) also feeds this class the per-format attack/release times for each
band, so the *speed* of Density's compression is format-dependent even though the *amount*
is purely the Density knob.

### 2.4 Highs / Lows / Bass Character / Mono Bass (`ToneShaper.h`)

- **Highs**/**Lows** are simple shelving filters (high shelf @ 8 kHz, low shelf @ 150 Hz).
- **Bass Character** runs a `tanh` waveshaper on just the band below 120 Hz (split via a
  dedicated Linkwitz-Riley crossover, summed back with the untouched high band afterward) —
  it colors low end specifically, it does not touch the rest of the spectrum.
- **Mono Bass**, when enabled, splits below 120 Hz again (a *separate* crossover instance
  from Bass Character's, since two different signals can't share one stateful filter in the
  same block) and sums that band to mono while leaving the high band per-channel — standard
  broadcast-chain mono-compatibility move.

### 2.5 Codec / Format Character (`CodecStage.h`)

This is what the Format Character dropdown actually controls. Selecting a format sets a
fixed bundle of values (tabulated in §3): a low cut, a low shelf, a presence-band peak
filter, a high shelf, an optional AM-bandwidth lowpass (only Talk Radio and Late Night Talk),
a `grit` saturation amount, and six attack/release times (mid-band "feel," plus separate
low-band and high-band feel that get handed to the Density compressor in §2.3). After the
EQ chain, it applies a `tanh` waveshaper driven by `grit + Drive×0.35`, blended in
proportional to that same combined amount. **`grit` is nonzero for every format except
Clean**, so some amount of this saturation is always active once you leave Clean, independent
of the Drive knob.

### 2.6 LUFS Loudness Normalization (`lufsTarget`, `lufsDrive`, `aesLoudness`, `longWindow`)

A continuous loudness servo, not a lookahead normalizer: it K-weights the signal (high shelf
+ high-pass, approximating BS.1770), tracks a running mean-square level with a time constant
of 3 s (or 60 s if `longWindow` is on), and computes a correction toward `lufsTarget`. Only
`lufsDrive` percent of the full correction is actually applied (0% = measurement only, no
gain change; 100% = fully corrected), and the applied gain is slew-limited to ~6 dB/second so
it can't react instantly to transients. `aesLoudness` (AES gating), when on, excludes blocks
quieter than -70 LUFS from the running average so silence doesn't drag the measured loudness
down.

### 2.7 True-Peak Limiter (`truePeak`)

A `juce::dsp::Limiter` running on a 2× oversampled signal (to catch inter-sample peaks that a
non-oversampled limiter would miss), threshold set directly from the `truePeak` parameter,
release fixed at 50 ms. This is the last line of defense against exceeding the ceiling — but
it's a fast-attack limiter, not a true zero-latency brickwall, so a signal arriving here
already very hot (dense + loud) makes it work harder and pump more audibly, even though it
will still hold the peak near the threshold.

### 2.8 Master Out

A plain gain trim. Every preset holds it at 0 dB — it's gain-staging, not part of any
preset's tonal character, left for the user to adjust manually if needed.

## 3. Preset parameter table

Values as stored in `Parameters.h::getPresetValues()`. `bassCharacter`/`density`/`drive`
range 0–100 (normalized to 0–1 before use); `highs`/`lows` in dB; thresholds in dB;
`lufsTarget`/`truePeak` in dB.

| Preset | Density | Detail | Drive | Highs | Lows | Bass Char | Mono Bass | Exp Thr | Exp Ratio | Gate Thr | Gate→0 | LUFS Tgt | LUFS Drive | AES | 60s | True Pk |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Clean | 50 | 1.0 | 25 | 0 | 0 | 0 | No | -45 | 2.0 | -55 | Yes | -14 | 0 | No | No | -0.5 |
| Talk Radio (AM) | 68 | 2.5 | 30 | 0 | -1 | 0 | Yes | -40 | 3.5 | -45 | Yes | -16 | 35 | Yes | No | -1.0 |
| Shock Jock (FM) | 80 | 3.5 | 55 | 2 | 3 | 45 | Yes | -50 | 2.5 | -52 | Yes | -11 | 65 | Yes | No | -1.5 |
| Public Radio | 28 | 0.5 | 8 | -1 | 1.5 | 8 | No | -65 | 1.3 | -70 | No | -20 | 10 | Yes | Yes | -0.3 |
| Late Night Talk (AM) | 65 | 2.0 | 55 | -1 | -2 | 0 | Yes | -38 | 4.0 | -42 | Yes | -15 | 30 | Yes | No | -1.0 |
| Top 40 (CHR) | 75 | 4.5 | 32 | 3 | 3.5 | 18 | No | -52 | 2.0 | -60 | No | -9 | 75 | No | No | -1.5 |
| Classic Rock (FM) | 58 | 2.5 | 35 | 1 | 2 | 28 | No | -55 | 2.0 | -62 | No | -12 | 55 | No | No | -1.2 |
| News Anchor | 48 | 3.0 | 5 | -0.5 | -1.5 | 0 | Yes | -42 | 4.0 | -48 | Yes | -16 | 40 | Yes | No | -1.0 |
| Urban/Hip-Hop | 82 | 4.0 | 42 | 2.5 | 6 | 55 | Yes | -50 | 2.5 | -58 | No | -8 | 80 | No | No | -1.8 |
| Classical/Fine Arts | 10 | 0.3 | 0 | 0 | 0 | 0 | No | -70 | 1.2 | -72 | No | -23 | 5 | No | Yes | -0.2 |

Codec-stage values (§2.5), not exposed as knobs — set entirely by the dropdown:

| Preset | Low Cut | Low Shelf | Presence | High Shelf | AM LPF | Grit | Mid Atk/Rel | Low Atk/Rel | High Atk/Rel |
|---|---|---|---|---|---|---|---|---|---|
| Clean | 20 Hz | 0 dB | 1000 Hz / 0 dB / Q0.7 | 0 dB | off | 0 | 8/140 ms | 8/140 ms | 8/140 ms |
| Talk Radio | 90 Hz | 0 dB | 2500 Hz / +3 dB / Q1.1 | 0 dB | 6000 Hz | 0.16 | 10/150 ms | 14/220 ms | 14/200 ms |
| Shock Jock | 60 Hz | +1.5 dB | 3200 Hz / +4 dB / Q1.0 | +2 dB | off | 0.28 | 3/80 ms | 2/60 ms | 5/100 ms |
| Public Radio | 40 Hz | +1.5 dB | 1000 Hz / 0 dB / Q0.7 | -1 dB | off | 0.04 | 20/250 ms | 25/300 ms | 25/280 ms |
| Late Night Talk | 120 Hz | 0 dB | 2400 Hz / +2.5 dB / Q1.3 | 0 dB | 5000 Hz | 0.30 | 6/100 ms | 10/160 ms | 9/140 ms |
| Top 40 | 40 Hz | +2 dB | 700 Hz / -1 dB / Q0.9 | +3 dB | off | 0.15 | 2/60 ms | 1.5/45 ms | 3/70 ms |
| Classic Rock | 50 Hz | +1.5 dB | 2000 Hz / +1.5 dB / Q0.8 | +1 dB | off | 0.18 | 8/180 ms | 10/220 ms | 10/200 ms |
| News Anchor | 100 Hz | 0 dB | 3000 Hz / +2 dB / Q1.2 | -0.3 dB | off | 0.03 | 15/120 ms | 20/180 ms | 18/150 ms |
| Urban/Hip-Hop | 30 Hz | +4 dB | 3500 Hz / +1 dB / Q0.9 | +2.5 dB | off | 0.20 | 3.5/70 ms | 1.5/40 ms | 5/90 ms |
| Classical | 25 Hz | 0 dB | 1000 Hz / 0 dB / Q0.7 | 0 dB | off | 0 | 30/400 ms | 35/450 ms | 35/450 ms |

## 4. Preset-by-preset walkthrough

### Clean
Every value sits at `createLayout()`'s default — selecting Clean is equivalent to hitting
Reset. The codec stage's `process()` returns immediately for `mode == Clean`, so there's no
EQ, no AM filtering, and no grit saturation at all; the only things touching the signal are
the expander/gate (moderate, default settings), a mild Density compressor (threshold -14 dB,
ratio ~4.5:1 wideband), a light Drive/Detail touch, and a moderate loudness normalizer
(`lufsDrive` 0% — meaning it *measures* loudness for the readout but applies zero correction
gain). This is the reference point every other preset departs from.

### Talk Radio (AM)
Mono-bass on, no Bass Character coloring, `grit` 0.16 (subtle). The defining move is the
codec stage's 6 kHz cascaded lowpass (`amLowpassA`/`amLowpassB`, two 2-pole stages ≈24 dB/oct)
modeling real AM receiver IF bandwidth — this is what does the "telephone quality" darkening,
not a shelf. Presence gets a real +3 dB push at 2.5 kHz for intelligibility. Density (68) with
slow-ish mid attack/release (10/150 ms) gives noticeably audible but not punchy compression.
LUFS target -16 with 35% drive is a moderate loudness pull.

### Shock Jock (FM)
The hottest preset in the table: Density 80, Drive 55, Bass Character 45, grit 0.28, LUFS
target -11 with 65% drive, True Peak ceiling tightened to -1.5 dB. Fast codec attack/release
(mid 3/80 ms, low 2/60 ms) makes the Density compressor pump audibly and quickly on
jingles/stingers — that's deliberate "feel," not a bug. With Density this high, crest factor
gets flattened hard, and 65% LUFS drive then pushes the average level aggressively toward
-11 LUFS — the combination is exactly the mechanism described in §2.3, which is why this
preset stresses the limiter (§2.7) the most and is the one most likely to still sound
"processed" even after the saturation-curve fixes.

### Public Radio
The gentlest non-Clean preset. Density only 28 (threshold -8 dB, ratio ~2.4:1), grit 0.04
(barely audible), very slow codec attack/release across all three bands (20-25 ms attack,
250-300 ms release) so nothing pumps or reacts to transients. LUFS target -20 with only 10%
drive — the loudness normalizer barely nudges gain. `aesLoudness` and `longWindow` are both
on, so loudness measurement ignores silence and averages over 60 seconds — a slow, patient
read appropriate for programming with wide dynamic range.

### Late Night Talk (AM)
Similar shape to Talk Radio but tighter: narrower AM lowpass (5000 Hz vs. 6000 Hz — nighttime
skywave/phone-heavy callers read boxier), higher low cut (120 Hz vs. 90 Hz), Drive pushed to
55 (vs. Talk Radio's 30) for more grit despite Bass Character staying at 0. Density (65) and
codec feel (6/100 ms mid) sit close to Talk Radio's, so most of the differentiation from Talk
Radio comes from the codec EQ/filtering, not the compressor.

### Top 40 (CHR)
The classic "smile curve": low shelf +2 dB, high shelf +3 dB, presence actually *cut* -1 dB
at 700 Hz (scooped mids so the shelf boosts read as bright/punchy rather than boxy). Fastest
low-band feel in the whole table (1.5/45 ms) for tight pop kick/bass punch. Density 75 is
high, LUFS drive 75% is the second-highest in the table — this preset is loud and tightly
controlled by design, leaning on the limiter more than Bass Character (only 18) or grit
(0.15, moderate) to get its "radio bright" sound.

### Classic Rock (FM)
Moderate across the board rather than extreme anywhere: Density 58, Drive 35, Bass Character
28, grit 0.18. Codec attack/release (8/180 ms mid, 10/220 ms low) is deliberately a touch
slower than Top 40's for a "glued" bass-guitar/kick feel instead of a tightly clamped pop
bass. This is the preset the user flagged as the second-worst offender for the
over-modulation issue — not because any single value is extreme, but because Density (58),
Drive (35), and Bass Character (28) are all simultaneously mid-to-high, so the three curve
fixes in §2.3/§2.4/§2.5 (Drive, Bass Character, and grit's shared `tanh` shaper) and the
Density→loudness interaction (§2.3) all compound on this preset at once, more than on a
preset that's extreme in only one dimension.

### News Anchor
Tight and controlled but with very little saturation character: grit 0.03 (near-silent),
Bass Character 0, Drive only 5. High shelf is actually slightly *cut* (-0.3 dB) rather than
boosted — the +2 dB presence push at 3 kHz already sits near sibilance range, and the design
intent (per the code comment) is that real anchor chains manage that with de-essing, not more
top end, so nothing here should add brightness on top of it. Density 48 and mid attack/release
15/120 ms are close to Clean's defaults — this preset differentiates almost entirely through
EQ shape, not through compression aggressiveness.

### Urban/Hip-Hop
The single most extreme preset by the numbers: Density 82 (highest), Bass Character 55
(highest), low shelf +4 dB, LUFS drive 80% (highest), LUFS target -8 (highest/loudest), True
Peak ceiling -1.8 dB (most headroom reserved, i.e. the tightest limiter threshold). The
low-band codec feel is the fastest in the table (1.5/40 ms) specifically so kick/808
transients get clamped right at the source without ducking the vocal band — the whole reason
Density runs a 3-band (not wideband) compressor in the first place (§2.3). Even with the
curve fixes, this preset will still read as the "hottest" one, because that's the explicit
design target (LUFS -8 is 6 dB louder than Shock Jock's -11) — it's meant to be the loudest,
most bass-forward, most heavily limited preset in the set.

### Classical/Fine Arts
The inverse of Urban/Hip-Hop: Density 10 (threshold -2.5 dB, ratio ~1.7:1 — barely
compressing), grit 0, Bass Character 0, Drive 0, slowest codec attack/release in the table
(30-35 ms attack, 400-450 ms release), LUFS target -23 (quietest/widest-dynamic-range
target) with only 5% drive, and `longWindow` on for a patient 60-second loudness average.
Essentially every stage is dialed toward transparency; this preset is the closest thing to
Clean plus a slightly tightened low cut (25 Hz) and a much wider expander/gate window
(threshold -70/-72 dB) so quiet passages aren't touched at all.

## 5. Recent fixes that changed preset behavior

Three stages (`ToneShaper::applyBassSaturation`, `CodecStage::process`'s grit shaper, and
`BroadcastCompressor::applyDrive`) originally used a `tanh(k·x)/tanh(k)` waveshaper, which
has a slope of `k/tanh(k)` at `x=0` — for the `k` values these presets use (roughly 2-9),
that's a multi-dB *gain increase* on quiet-to-moderate signal, not gentle saturation. All
three now use `tanh(k·x)/k`, which has unity slope at zero (no gain added to non-peak
content) and only compresses as the signal approaches full scale. Bass Character's saturation
was also moved from running on the full-bandwidth buffer to running on a dedicated sub-120 Hz
band, matching what the control's name promises. Presets with higher `bassCharacter`/`drive`
values and nonzero `grit` (i.e. every non-Clean preset, to varying degrees) are the ones whose
audible character changed as a result — see §4 for how much each preset was carrying in these
three parameters.
