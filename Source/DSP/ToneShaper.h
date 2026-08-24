#pragma once
#include <juce_dsp/juce_dsp.h>

// Highs/Lows shelving, low-end "bass character" saturation, and
// mono-bass summing below a fixed crossover (keeps low end mono-compatible,
// classic broadcast-chain move). Exposes per-control activity for metering:
// Highs/Lows report how much shelf gain is "live" (dialed amount, weighted by
// how much signal is actually present to act on); Bass Char reports actual
// saturation intensity.
class ToneShaper
{
public:
    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;
        numChannels = spec.numChannels;

        highShelf.prepare(spec);
        lowShelf.prepare(spec);

        crossoverLow.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);
        crossoverHigh.setType(juce::dsp::LinkwitzRileyFilterType::highpass);
        crossoverLow.setCutoffFrequency(crossoverHz);
        crossoverHigh.setCutoffFrequency(crossoverHz);
        crossoverLow.prepare(spec);
        crossoverHigh.prepare(spec);

        // Separate filter pair for bass-saturation band splitting - kept independent
        // of crossoverLow/crossoverHigh above since those hold running state for the
        // mono-bass split, and interleaving two different signals through the same
        // stateful IIR filter in one block would corrupt both.
        satCrossoverLow.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);
        satCrossoverHigh.setType(juce::dsp::LinkwitzRileyFilterType::highpass);
        satCrossoverLow.setCutoffFrequency(crossoverHz);
        satCrossoverHigh.setCutoffFrequency(crossoverHz);
        satCrossoverLow.prepare(spec);
        satCrossoverHigh.prepare(spec);

        updateShelves();
        reset();
    }

    void reset()
    {
        highShelf.reset();
        lowShelf.reset();
        crossoverLow.reset();
        crossoverHigh.reset();
        satCrossoverLow.reset();
        satCrossoverHigh.reset();
    }

    void setHighsDb(float db)        { if (highsDb != db) { highsDb = db; updateShelves(); } }
    void setLowsDb(float db)         { if (lowsDb != db)  { lowsDb = db;  updateShelves(); } }
    void setBassCharacter01(float v) { bassCharacter = v; }
    void setMonoBass(bool shouldBeMono) { monoBass = shouldBeMono; }

    void process(juce::AudioBuffer<float>& buffer)
    {
        const float presence01 = juce::jlimit(0.0f, 1.0f,
            (blockPeakDb(buffer) - (-40.0f)) / ((-6.0f) - (-40.0f)));

        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> ctx(block);

        highShelf.process(ctx);
        lowShelf.process(ctx);

        lastHighsActivity01 = juce::jlimit(0.0f, 1.0f, std::abs(highsDb) / 12.0f) * presence01;
        lastLowsActivity01  = juce::jlimit(0.0f, 1.0f, std::abs(lowsDb) / 12.0f) * presence01;

        lastBassCharActivity01 = 0.0f;
        if (bassCharacter > 0.0001f)
            applyBassSaturation(buffer);

        if (monoBass && buffer.getNumChannels() > 1)
            applyMonoBass(buffer);
    }

    float getHighsActivity01() const    { return lastHighsActivity01; }
    float getLowsActivity01() const     { return lastLowsActivity01; }
    float getBassCharActivity01() const { return lastBassCharActivity01; }

private:
    static float blockPeakDb(const juce::AudioBuffer<float>& b)
    {
        return juce::Decibels::gainToDecibels(b.getMagnitude(0, b.getNumSamples()), -100.0f);
    }

    void updateShelves()
    {
        const double nyquistMargin = sampleRate * 0.45;
        *highShelf.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(
            sampleRate, juce::jlimit(20.0, nyquistMargin, 8000.0), 0.7f, juce::Decibels::decibelsToGain(highsDb));
        *lowShelf.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(
            sampleRate, juce::jlimit(20.0, nyquistMargin, 150.0), 0.7f, juce::Decibels::decibelsToGain(lowsDb));
    }

    // Shapes only the band below crossoverHz, so "Bass Character" colors the low end
    // the way its name promises instead of waveshaping the full mix. The curve is
    // tanh(k*x)/k rather than tanh(k*x)/tanh(k): dividing by k instead of tanh(k)
    // keeps the slope at x=0 equal to 1 (quiet/moderate bass passes through at unity,
    // no gain added), where the old divisor gave a slope of k/tanh(k) - effectively a
    // hidden multi-dB makeup-gain stage that pushed everything short of full scale
    // toward the ceiling instead of just rounding off peaks.
    void applyBassSaturation(juce::AudioBuffer<float>& buffer)
    {
        const int numSamples = buffer.getNumSamples();
        satLowBuf.setSize(buffer.getNumChannels(), numSamples, false, false, true);
        satHighBuf.setSize(buffer.getNumChannels(), numSamples, false, false, true);
        satLowBuf.makeCopyOf(buffer, true);
        satHighBuf.makeCopyOf(buffer, true);

        juce::dsp::AudioBlock<float> lowBlock(satLowBuf);
        juce::dsp::AudioBlock<float> highBlock(satHighBuf);
        juce::dsp::ProcessContextReplacing<float> lowCtx(lowBlock);
        juce::dsp::ProcessContextReplacing<float> highCtx(highBlock);
        satCrossoverLow.process(lowCtx);
        satCrossoverHigh.process(highCtx);

        const float k = 1.0f + bassCharacter * 8.0f;
        double sumAbsDelta = 0.0;
        int64_t count = 0;

        for (int ch = 0; ch < satLowBuf.getNumChannels(); ++ch)
        {
            auto* low = satLowBuf.getWritePointer(ch);
            for (int i = 0; i < numSamples; ++i)
            {
                float x = low[i];
                float shaped = std::tanh(x * k) / k;
                float delta = (shaped - x) * bassCharacter;
                low[i] = x + delta;
                sumAbsDelta += std::abs(delta);
                ++count;
            }
        }

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* out = buffer.getWritePointer(ch);
            auto* low = satLowBuf.getReadPointer(ch);
            auto* high = satHighBuf.getReadPointer(ch);
            for (int i = 0; i < numSamples; ++i)
                out[i] = low[i] + high[i];
        }

        const float meanAbsDelta = count > 0 ? (float) (sumAbsDelta / (double) count) : 0.0f;
        lastBassCharActivity01 = juce::jlimit(0.0f, 1.0f, meanAbsDelta * 8.0f);
    }

    void applyMonoBass(juce::AudioBuffer<float>& buffer)
    {
        const int numSamples = buffer.getNumSamples();
        lowBuf.setSize(buffer.getNumChannels(), numSamples, false, false, true);
        highBuf.setSize(buffer.getNumChannels(), numSamples, false, false, true);
        lowBuf.makeCopyOf(buffer, true);
        highBuf.makeCopyOf(buffer, true);

        juce::dsp::AudioBlock<float> lowBlock(lowBuf);
        juce::dsp::AudioBlock<float> highBlock(highBuf);
        juce::dsp::ProcessContextReplacing<float> lowCtx(lowBlock);
        juce::dsp::ProcessContextReplacing<float> highCtx(highBlock);

        crossoverLow.process(lowCtx);
        crossoverHigh.process(highCtx);

        // Sum the low band to mono, keep the high band per-channel.
        for (int i = 0; i < numSamples; ++i)
        {
            float sum = 0.0f;
            for (int ch = 0; ch < lowBuf.getNumChannels(); ++ch)
                sum += lowBuf.getSample(ch, i);
            float monoLow = sum / (float) lowBuf.getNumChannels();

            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                buffer.setSample(ch, i, monoLow + highBuf.getSample(ch, i));
        }
    }

    double sampleRate = 44100.0;
    int numChannels = 2;

    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> highShelf, lowShelf;
    juce::dsp::LinkwitzRileyFilter<float> crossoverLow, crossoverHigh;
    juce::dsp::LinkwitzRileyFilter<float> satCrossoverLow, satCrossoverHigh;
    juce::AudioBuffer<float> lowBuf, highBuf;
    juce::AudioBuffer<float> satLowBuf, satHighBuf;

    float highsDb = 0.0f, lowsDb = 0.0f, bassCharacter = 0.0f;
    bool monoBass = false;
    static constexpr float crossoverHz = 120.0f;

    float lastHighsActivity01 = 0.0f, lastLowsActivity01 = 0.0f, lastBassCharActivity01 = 0.0f;
};
