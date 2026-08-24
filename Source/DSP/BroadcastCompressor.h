#pragma once
#include <juce_dsp/juce_dsp.h>

// Density -> compression amount, Drive -> post-comp saturation,
// Detail -> a high-frequency exciter mixed back in for "clarity".
// Density drives a 3-band compressor (low/mid/high split via cascaded
// Linkwitz-Riley crossovers with an allpass phase correction on the low band)
// rather than a single wideband compressor, so a kick/808 transient in the low
// band no longer ducks vocal-band gain reduction along with it - most audible
// on bass-forward formats like UrbanHipHop. Drive and Detail stay wideband,
// applied after the bands are summed back together.
// Exposes per-stage activity (gain reduction / saturation amount) for metering,
// measured directly around each processing step rather than a shared level.
class BroadcastCompressor
{
public:
    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        crossoverLowLP.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);
        crossoverLowHP.setType(juce::dsp::LinkwitzRileyFilterType::highpass);
        crossoverHighLP.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);
        crossoverHighHP.setType(juce::dsp::LinkwitzRileyFilterType::highpass);
        lowPhaseCorrect.setType(juce::dsp::LinkwitzRileyFilterType::allpass);

        for (auto* f : { &crossoverLowLP, &crossoverLowHP, &crossoverHighLP, &crossoverHighHP, &lowPhaseCorrect })
            f->prepare(spec);

        crossoverLowLP.setCutoffFrequency((float) lowMidHz);
        crossoverLowHP.setCutoffFrequency((float) lowMidHz);
        crossoverHighLP.setCutoffFrequency((float) midHighHz);
        crossoverHighHP.setCutoffFrequency((float) midHighHz);
        lowPhaseCorrect.setCutoffFrequency((float) midHighHz);

        for (auto* c : { &lowComp, &midComp, &highComp })
        {
            c->prepare(spec);
            c->setAttack(8.0f);
            c->setRelease(140.0f);
        }

        lowBuf.setSize((int) spec.numChannels, (int) spec.maximumBlockSize);
        midBuf.setSize((int) spec.numChannels, (int) spec.maximumBlockSize);
        highBuf.setSize((int) spec.numChannels, (int) spec.maximumBlockSize);
        restBuf.setSize((int) spec.numChannels, (int) spec.maximumBlockSize);

        exciterHighPass.prepare(spec);
        *exciterHighPass.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(
            spec.sampleRate, juce::jlimit(20.0, spec.sampleRate * 0.45, 3500.0));
        exciterBuf.setSize((int) spec.numChannels, (int) spec.maximumBlockSize);

        updateCompressors();
    }

    void reset()
    {
        for (auto* f : { &crossoverLowLP, &crossoverLowHP, &crossoverHighLP, &crossoverHighHP, &lowPhaseCorrect })
            f->reset();
        for (auto* c : { &lowComp, &midComp, &highComp })
            c->reset();
        exciterHighPass.reset();
    }

    void setDensity01(float v) { if (density != v) { density = v; updateCompressors(); } }
    void setDrive01(float v)   { drive = v; }
    void setDetail01(float v)  { detail = v; } // 0-1, scaled from the 0-10 UI range

    // Lets the active codec/format profile set each band compressor's "feel"
    // (fast and punchy vs. slow and transparent) independently of the Density amount.
    void setAttackRelease(float lowAtkMs, float lowRelMs,
                           float midAtkMs, float midRelMs,
                           float highAtkMs, float highRelMs)
    {
        lowComp.setAttack(lowAtkMs);   lowComp.setRelease(lowRelMs);
        midComp.setAttack(midAtkMs);   midComp.setRelease(midRelMs);
        highComp.setAttack(highAtkMs); highComp.setRelease(highRelMs);
    }

    void process(juce::AudioBuffer<float>& buffer)
    {
        const int numCh = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();

        lowBuf.setSize(numCh, numSamples, false, false, true);
        midBuf.setSize(numCh, numSamples, false, false, true);
        highBuf.setSize(numCh, numSamples, false, false, true);
        restBuf.setSize(numCh, numSamples, false, false, true);

        lowBuf.makeCopyOf(buffer, true);
        restBuf.makeCopyOf(buffer, true);

        // First crossover: low band vs. everything above lowMidHz.
        processInPlace(lowBuf, crossoverLowLP);
        processInPlace(restBuf, crossoverLowHP);

        // Second crossover: split the rest into mid/high at midHighHz.
        midBuf.makeCopyOf(restBuf, true);
        highBuf.makeCopyOf(restBuf, true);
        processInPlace(midBuf, crossoverHighLP);
        processInPlace(highBuf, crossoverHighHP);

        // The low band only passed through one crossover stage while mid/high passed
        // through two, so it needs an allpass at midHighHz to match their phase before
        // the three bands are summed back together.
        processInPlace(lowBuf, lowPhaseCorrect);

        const float lowPreDb  = blockPeakDb(lowBuf);
        const float midPreDb  = blockPeakDb(midBuf);
        const float highPreDb = blockPeakDb(highBuf);

        processInPlace(lowBuf, lowComp);
        processInPlace(midBuf, midComp);
        processInPlace(highBuf, highComp);

        const float lowGr  = juce::jmin(0.0f, blockPeakDb(lowBuf)  - lowPreDb);
        const float midGr  = juce::jmin(0.0f, blockPeakDb(midBuf)  - midPreDb);
        const float highGr = juce::jmin(0.0f, blockPeakDb(highBuf) - highPreDb);
        lastLowGrDb = lowGr; lastMidGrDb = midGr; lastHighGrDb = highGr;
        lastDensityGrDb = juce::jmin(lowGr, juce::jmin(midGr, highGr));

        for (int ch = 0; ch < numCh; ++ch)
        {
            auto* out = buffer.getWritePointer(ch);
            auto* lo = lowBuf.getReadPointer(ch);
            auto* mi = midBuf.getReadPointer(ch);
            auto* hi = highBuf.getReadPointer(ch);
            for (int i = 0; i < numSamples; ++i)
                out[i] = lo[i] + mi[i] + hi[i];
        }

        lastDriveActivity01 = 0.0f;
        if (drive > 0.0001f)
            applyDrive(buffer);

        lastDetailActivity01 = 0.0f;
        if (detail > 0.0001f)
            applyDetailExciter(buffer);
    }

    float getDensityGrDb() const     { return lastDensityGrDb; }
    float getDriveActivity01() const { return lastDriveActivity01; }
    float getDetailActivity01() const { return lastDetailActivity01; }

    // Per-band gain reduction, for finer metering later if the UI wants it.
    float getLowBandGrDb() const  { return lastLowGrDb; }
    float getMidBandGrDb() const  { return lastMidGrDb; }
    float getHighBandGrDb() const { return lastHighGrDb; }

private:
    static float blockPeakDb(const juce::AudioBuffer<float>& b)
    {
        return juce::Decibels::gainToDecibels(b.getMagnitude(0, b.getNumSamples()), -100.0f);
    }

    template <typename Processor>
    static void processInPlace(juce::AudioBuffer<float>& b, Processor& proc)
    {
        juce::dsp::AudioBlock<float> block(b);
        juce::dsp::ProcessContextReplacing<float> ctx(block);
        proc.process(ctx);
    }

    void updateCompressors()
    {
        const float threshold = juce::jmap(density, 0.0f, 1.0f, 0.0f, -28.0f);
        const float ratio     = juce::jmap(density, 0.0f, 1.0f, 1.0f, 8.0f);

        // The low band can take a harder ratio at the same Density setting - bass
        // transients tolerate more gain reduction before pumping becomes audible, and
        // it's what keeps kick/808 hits from overrunning the mix without touching mid/high.
        lowComp.setThreshold(threshold);
        lowComp.setRatio(juce::jmin(12.0f, ratio * 1.3f));

        midComp.setThreshold(threshold);
        midComp.setRatio(ratio);

        highComp.setThreshold(threshold);
        highComp.setRatio(ratio);
    }

    // tanh(k*x)/k, not tanh(k*x)/tanh(k): keeps unity slope at x=0 so quiet/moderate
    // signal isn't boosted, only genuine peaks get rounded off (same fix as
    // ToneShaper's bass saturation - see its comment for the full explanation).
    void applyDrive(juce::AudioBuffer<float>& buffer)
    {
        const float k = 1.0f + drive * 6.0f;
        double sumAbsDelta = 0.0;
        int64_t count = 0;

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* data = buffer.getWritePointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                float x = data[i];
                float shaped = std::tanh(x * k) / k;
                float delta = (shaped - x) * drive;
                data[i] = x + delta;
                sumAbsDelta += std::abs(delta);
                ++count;
            }
        }
        const float meanAbsDelta = count > 0 ? (float) (sumAbsDelta / (double) count) : 0.0f;
        lastDriveActivity01 = juce::jlimit(0.0f, 1.0f, meanAbsDelta * 8.0f);
    }

    void applyDetailExciter(juce::AudioBuffer<float>& buffer)
    {
        const int numCh = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();
        exciterBuf.setSize(numCh, numSamples, false, false, true);
        exciterBuf.makeCopyOf(buffer, true);

        juce::dsp::AudioBlock<float> block(exciterBuf);
        juce::dsp::ProcessContextReplacing<float> ctx(block);
        exciterHighPass.process(ctx);

        const float mix = juce::jmap(detail, 0.0f, 1.0f, 0.0f, 0.4f);
        double sumAbsAdded = 0.0;
        int64_t count = 0;

        for (int ch = 0; ch < numCh; ++ch)
        {
            auto* out = buffer.getWritePointer(ch);
            auto* ex = exciterBuf.getReadPointer(ch);
            for (int i = 0; i < numSamples; ++i)
            {
                float shaped = std::tanh(ex[i] * 4.0f);
                float added = shaped * mix;
                out[i] += added;
                sumAbsAdded += std::abs(added);
                ++count;
            }
        }
        const float meanAbsAdded = count > 0 ? (float) (sumAbsAdded / (double) count) : 0.0f;
        lastDetailActivity01 = juce::jlimit(0.0f, 1.0f, meanAbsAdded * 12.0f);
    }

    static constexpr double lowMidHz = 180.0;
    static constexpr double midHighHz = 2800.0;

    juce::dsp::LinkwitzRileyFilter<float> crossoverLowLP, crossoverLowHP, crossoverHighLP, crossoverHighHP, lowPhaseCorrect;
    juce::dsp::Compressor<float> lowComp, midComp, highComp;
    juce::AudioBuffer<float> lowBuf, midBuf, highBuf, restBuf;

    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> exciterHighPass
    {
        juce::dsp::IIR::Coefficients<float>::makeHighPass(44100.0, 3500.0)
    };
    juce::AudioBuffer<float> exciterBuf;

    float density = 0.5f, drive = 0.0f, detail = 0.0f;
    float lastDensityGrDb = 0.0f, lastDriveActivity01 = 0.0f, lastDetailActivity01 = 0.0f;
    float lastLowGrDb = 0.0f, lastMidGrDb = 0.0f, lastHighGrDb = 0.0f;
};
