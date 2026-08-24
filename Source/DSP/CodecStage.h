#pragma once
#include <juce_dsp/juce_dsp.h>

// Format "character" stage: tonal EQ shaping + light saturation color tuned
// to evoke the mix style of a few well-known radio formats. FM formats stay
// full-range in, full-range out - only tonal balance and grit amount change.
// TalkRadio and LateNightTalk additionally get a real cascaded lowpass
// (~5-6kHz) modeling AM receiver IF bandwidth narrowing, since real AM tuners
// are genuinely band-limited well below 5kHz ("telephone quality"), not just
// darker-sounding.
// Also exposes a per-profile attack/release "feel" that the compressor stage
// picks up, so each profile pumps differently, not just sounds different.
class CodecStage
{
public:
    enum Mode { Clean = 0, TalkRadio, ShockJock, PublicRadio, LateNightTalk, Top40,
                ClassicRock, NewsAnchor, UrbanHipHop, Classical };

    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;
        lowCut.prepare(spec);
        lowShelf.prepare(spec);
        presence.prepare(spec);
        highShelf.prepare(spec);
        amLowpassA.prepare(spec);
        amLowpassB.prepare(spec);
        setMode(mode);
        reset();
    }

    void reset()
    {
        lowCut.reset();
        lowShelf.reset();
        presence.reset();
        highShelf.reset();
        amLowpassA.reset();
        amLowpassB.reset();
    }

    void setMode(int newMode)
    {
        mode = newMode;
        amLowpassHz = 0.0f; // only TalkRadio/LateNightTalk (AM formats) enable this below

        switch (mode)
        {
            // Syndicated AM talk: vocal presence forward, light rumble cut. Real cascaded
            // lowpass (below, ~6kHz) models AM receiver IF narrowing - high shelf stays flat
            // since the actual bandwidth limit now does the darkening, not a shelf cut.
            // Low/high bands barely react (talk has little bass, and a slow top keeps
            // sibilance from pumping); mid carries the real "feel" since that's the vocal band.
            case TalkRadio:
                lowCutHz = 90.0f;  lowShelfDb = 0.0f;
                presenceHz = 2500.0f; presenceDb = 3.0f; presenceQ = 1.1f;
                highShelfDb = 0.0f;
                amLowpassHz = 6000.0f;
                grit = 0.16f; attackMs = 10.0f; releaseMs = 150.0f;
                lowAttackMs = 14.0f; lowReleaseMs = 220.0f;
                highAttackMs = 14.0f; highReleaseMs = 200.0f;
                break;
            // Shock-jock FM: hyped low end for body/jingles, pushed upper-mids for edge,
            // brighter top, fast/punchy compression. Low band is tightest of all (fast
            // stinger/jingle hits), high band a touch slower so it doesn't crackle.
            case ShockJock:
                lowCutHz = 60.0f;  lowShelfDb = 1.5f;
                presenceHz = 3200.0f; presenceDb = 4.0f; presenceQ = 1.0f;
                highShelfDb = 2.0f;
                grit = 0.28f; attackMs = 3.0f; releaseMs = 80.0f;
                lowAttackMs = 2.0f; lowReleaseMs = 60.0f;
                highAttackMs = 5.0f; highReleaseMs = 100.0f;
                break;
            // Public radio: broad warmth, smoothed top end, gentle slow compression across
            // every band - nothing here should feel like it's reacting to transients.
            case PublicRadio:
                lowCutHz = 40.0f;  lowShelfDb = 1.5f;
                presenceHz = 1000.0f; presenceDb = 0.0f; presenceQ = 0.7f;
                highShelfDb = -1.0f;
                grit = 0.04f; attackMs = 20.0f; releaseMs = 250.0f;
                lowAttackMs = 25.0f; lowReleaseMs = 300.0f;
                highAttackMs = 25.0f; highReleaseMs = 280.0f;
                break;
            // Late-night AM talk: more low cut and a narrower AM lowpass than daytime Talk
            // Radio (nighttime skywave noise, phone-heavy callers - narrower IF bandwidth
            // reads as boxier). Presence stays in the same 2-3kHz intelligibility band as
            // daytime talk rather than dipping lower/boxier. Low/high bands stay gentle like
            // daytime Talk Radio - it's still speech, just a touch tighter throughout.
            case LateNightTalk:
                lowCutHz = 120.0f; lowShelfDb = 0.0f;
                presenceHz = 2400.0f; presenceDb = 2.5f; presenceQ = 1.3f;
                highShelfDb = 0.0f;
                amLowpassHz = 5000.0f;
                grit = 0.30f; attackMs = 6.0f; releaseMs = 100.0f;
                lowAttackMs = 10.0f; lowReleaseMs = 160.0f;
                highAttackMs = 9.0f; highReleaseMs = 140.0f;
                break;
            // Top 40 FM: "smile" curve - punchy low shelf, bright top, tight fast compression.
            // Low band is the tightest/fastest of any format (pop kick/bass punch); high
            // band a shade slower to tame the bright top without dulling it.
            case Top40:
                lowCutHz = 40.0f;  lowShelfDb = 2.0f;
                presenceHz = 700.0f; presenceDb = -1.0f; presenceQ = 0.9f;
                highShelfDb = 3.0f;
                grit = 0.15f; attackMs = 2.0f; releaseMs = 60.0f;
                lowAttackMs = 1.5f; lowReleaseMs = 45.0f;
                highAttackMs = 3.0f; highReleaseMs = 70.0f;
                break;
            // Classic rock FM: warm low-mids, modest presence push, a little analog-style
            // grit, musical glue. Low band leans slightly slower than mid for a glued,
            // less clamped bass guitar/kick feel rather than a tight pop bass.
            case ClassicRock:
                lowCutHz = 50.0f;  lowShelfDb = 1.5f;
                presenceHz = 2000.0f; presenceDb = 1.5f; presenceQ = 0.8f;
                highShelfDb = 1.0f;
                grit = 0.18f; attackMs = 8.0f; releaseMs = 180.0f;
                lowAttackMs = 10.0f; lowReleaseMs = 220.0f;
                highAttackMs = 10.0f; highReleaseMs = 200.0f;
                break;
            // News anchor: crisp and controlled, tighter/cleaner than Talk Radio, almost no
            // grit. High shelf stays flat/slightly cut, not boosted - the presence band
            // already sits near sibilance range and real anchor chains manage that with
            // de-essing, not more top end. Low band barely reacts - anchor voice has little
            // bass content to compress.
            case NewsAnchor:
                lowCutHz = 100.0f; lowShelfDb = 0.0f;
                presenceHz = 3000.0f; presenceDb = 2.0f; presenceQ = 1.2f;
                highShelfDb = -0.3f;
                grit = 0.03f; attackMs = 15.0f; releaseMs = 120.0f;
                lowAttackMs = 20.0f; lowReleaseMs = 180.0f;
                highAttackMs = 18.0f; highReleaseMs = 150.0f;
                break;
            // Urban/hip-hop: bass-forward low shelf, tight bright top, loud fast pump. Low
            // band gets its own fast attack/release to clamp kick/808 transients right at
            // the source; mid band (vocals) keeps its own feel independent of what the low
            // band is doing, which is the whole point - a single wideband compressor
            // clamping instantly on kick/808 transients would duck vocals along with the
            // bass, which is exactly what BroadcastCompressor's 3-band split now avoids.
            case UrbanHipHop:
                lowCutHz = 30.0f;  lowShelfDb = 4.0f;
                presenceHz = 3500.0f; presenceDb = 1.0f; presenceQ = 0.9f;
                highShelfDb = 2.5f;
                grit = 0.20f; attackMs = 3.5f; releaseMs = 70.0f;
                lowAttackMs = 1.5f; lowReleaseMs = 40.0f;
                highAttackMs = 5.0f; highReleaseMs = 90.0f;
                break;
            // Classical/fine arts: the "least processed" profile - wide dynamics, very slow
            // gentle compression, no grit, uniformly transparent across all three bands.
            case Classical:
                lowCutHz = 25.0f;  lowShelfDb = 0.0f;
                presenceHz = 1000.0f; presenceDb = 0.0f; presenceQ = 0.7f;
                highShelfDb = 0.0f;
                grit = 0.0f; attackMs = 30.0f; releaseMs = 400.0f;
                lowAttackMs = 35.0f; lowReleaseMs = 450.0f;
                highAttackMs = 35.0f; highReleaseMs = 450.0f;
                break;
            case Clean:
            default:
                lowCutHz = 20.0f;  lowShelfDb = 0.0f;
                presenceHz = 1000.0f; presenceDb = 0.0f; presenceQ = 0.7f;
                highShelfDb = 0.0f;
                grit = 0.0f; attackMs = 8.0f; releaseMs = 140.0f;
                lowAttackMs = 8.0f; lowReleaseMs = 140.0f;
                highAttackMs = 8.0f; highReleaseMs = 140.0f;
                break;
        }

        updateFilters();
    }

    // driveAmount: 0-1, scales the profile-appropriate saturation on top of `grit`
    void setDriveAmount(float driveAmount01) { drive = driveAmount01; }

    // Mid-band (vocal) attack/release - the primary "feel" of the format.
    float getAttackMs() const  { return attackMs; }
    float getReleaseMs() const { return releaseMs; }

    // Low/high-band attack/release, for BroadcastCompressor's 3-band split.
    float getLowAttackMs() const   { return lowAttackMs; }
    float getLowReleaseMs() const  { return lowReleaseMs; }
    float getHighAttackMs() const  { return highAttackMs; }
    float getHighReleaseMs() const { return highReleaseMs; }

    template <typename ProcessContext>
    void process(const ProcessContext& context)
    {
        if (mode == Clean)
            return;

        auto&& outBlock = context.getOutputBlock();
        lowCut.process(context);
        lowShelf.process(context);
        presence.process(context);
        highShelf.process(context);

        if (amLowpassHz > 0.0f)
        {
            amLowpassA.process(context);
            amLowpassB.process(context);
        }

        const float amount = juce::jlimit(0.0f, 1.0f, grit + drive * 0.35f);
        if (amount <= 0.0001f)
            return;

        // tanh(k*x)/k, not tanh(k*x)/tanh(k): keeps unity slope at x=0 so quiet/moderate
        // signal isn't boosted, only genuine peaks get rounded off (same fix as
        // ToneShaper's bass saturation - see its comment for the full explanation).
        const float k = 1.0f + amount * 6.0f;

        for (size_t ch = 0; ch < outBlock.getNumChannels(); ++ch)
        {
            auto* data = outBlock.getChannelPointer(ch);
            for (size_t i = 0; i < outBlock.getNumSamples(); ++i)
            {
                float x = data[i];
                float shaped = std::tanh(x * k) / k;
                data[i] = x + (shaped - x) * amount;
            }
        }
    }

private:
    void updateFilters()
    {
        const double nyquistMargin = sampleRate * 0.45;
        auto clampHz = [nyquistMargin] (double hz) { return juce::jlimit(20.0, nyquistMargin, hz); };

        *lowCut.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(
            sampleRate, clampHz(lowCutHz), 0.707f);
        *lowShelf.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(
            sampleRate, clampHz(120.0), 0.7f, juce::Decibels::decibelsToGain(lowShelfDb));
        *presence.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(
            sampleRate, clampHz(presenceHz), presenceQ, juce::Decibels::decibelsToGain(presenceDb));
        *highShelf.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(
            sampleRate, clampHz(8000.0), 0.7f, juce::Decibels::decibelsToGain(highShelfDb));

        if (amLowpassHz > 0.0f)
        {
            // Two cascaded 2-pole Butterworth stages ~= 4-pole (24dB/oct) lowpass, steep
            // enough to read as a real AM IF bandwidth limit rather than just a dark shelf.
            auto coeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(
                sampleRate, clampHz(amLowpassHz), 0.707f);
            *amLowpassA.state = *coeffs;
            *amLowpassB.state = *coeffs;
        }
    }

    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> lowCut, lowShelf, presence, highShelf;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> amLowpassA, amLowpassB;
    double sampleRate = 44100.0;
    int mode = Clean;

    float lowCutHz = 20.0f, lowShelfDb = 0.0f;
    float presenceHz = 1000.0f, presenceDb = 0.0f, presenceQ = 0.7f;
    float highShelfDb = 0.0f;
    float amLowpassHz = 0.0f; // 0 = disabled (FM formats); TalkRadio/LateNightTalk set ~5-6kHz
    float grit = 0.0f, drive = 0.0f;
    float attackMs = 8.0f, releaseMs = 140.0f; // mid-band
    float lowAttackMs = 8.0f, lowReleaseMs = 140.0f;
    float highAttackMs = 8.0f, highReleaseMs = 140.0f;
};
