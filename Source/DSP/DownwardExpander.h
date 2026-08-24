#pragma once
#include <juce_dsp/juce_dsp.h>

// Fast downward expander: continuously pulls down room tone, reverb tails,
// and noise floor sitting below threshold, proportional to how far below
// (ratio), rather than hard-muting. Attack/release are fixed short so it
// stays transparent - see NoiseGate for the hard on/off cousin of this.
class DownwardExpander
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;
        attackCoeff = calcCoeff (attackMs);
        releaseCoeff = calcCoeff (releaseMs);
        reset();
    }

    void reset()
    {
        envelopeDb = -100.0f;
        currentReductionDb = 0.0f;
    }

    void setThresholdDb (float db) { threshold = db; }
    void setRatio (float r)        { ratio = juce::jmax (1.0f, r); }

    void process (juce::AudioBuffer<float>& buffer)
    {
        const int numCh = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();

        for (int i = 0; i < numSamples; ++i)
        {
            float peak = 0.0f;
            for (int ch = 0; ch < numCh; ++ch)
                peak = juce::jmax (peak, std::abs (buffer.getSample (ch, i)));

            const float levelDb = juce::Decibels::gainToDecibels (peak, -100.0f);
            const float coeff = levelDb > envelopeDb ? attackCoeff : releaseCoeff;
            envelopeDb += (levelDb - envelopeDb) * coeff;

            // Below noiseFloorDb there's no real signal to speak of (digital silence
            // sits at the -100dB gainToDecibels floor) - the formula below would still
            // compute a large, ratio-dependent "reduction" against that floor even
            // though there's nothing there to actually reduce, which read as the ring
            // tracking the knob itself rather than real audio. Require genuine signal.
            const float reductionDb = (envelopeDb < threshold && envelopeDb > noiseFloorDb)
                ? (envelopeDb - threshold) * (1.0f - 1.0f / ratio)
                : 0.0f;
            currentReductionDb = reductionDb;

            const float gainLin = juce::Decibels::decibelsToGain (reductionDb);
            for (int ch = 0; ch < numCh; ++ch)
                buffer.setSample (ch, i, buffer.getSample (ch, i) * gainLin);
        }
    }

    // Negative dB = currently reducing gain; 0 = fully open.
    float getLastReductionDb() const { return currentReductionDb; }

private:
    float calcCoeff (float ms) const
    {
        return 1.0f - std::exp (-1.0f / (0.001f * ms * (float) sampleRate));
    }

    double sampleRate = 44100.0;
    float threshold = -45.0f;
    float ratio = 2.0f;
    static constexpr float noiseFloorDb = -90.0f;
    static constexpr float attackMs = 2.0f;
    static constexpr float releaseMs = 60.0f;
    float attackCoeff = 0.5f, releaseCoeff = 0.1f;
    float envelopeDb = -100.0f;
    float currentReductionDb = 0.0f;
};
