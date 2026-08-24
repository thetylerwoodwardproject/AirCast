#pragma once
#include <juce_dsp/juce_dsp.h>

// Continuous loudness normalizer approximating ITU-R BS.1770 K-weighting
// (a two-stage shelf + high-pass), with an exponentially-windowed running
// loudness estimate and a slew-limited makeup/attenuation gain servo.
// This is a practical approximation for creative/monitoring use, not a
// certified loudness meter.
class LoudnessProcessor
{
public:
    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;
        blockSeconds = (double) spec.maximumBlockSize / sampleRate;

        preShelf.prepare(spec);
        rlbHighPass.prepare(spec);
        const double nyquistMargin = sampleRate * 0.45;
        *preShelf.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, juce::jlimit(20.0, nyquistMargin, 1500.0), 0.7f, juce::Decibels::decibelsToGain(4.0f));
        *rlbHighPass.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, juce::jlimit(20.0, nyquistMargin, 60.0), 0.5f);

        measureBuf.setSize((int) spec.numChannels, (int) spec.maximumBlockSize);
        reset();
    }

    void reset()
    {
        preShelf.reset();
        rlbHighPass.reset();
        meanSquareAvg = 1.0e-6f;
        currentGainDb = 0.0f;
    }

    void setTargetLufs(float lufs)      { target = lufs; }
    void setDriveAmount01(float v)      { driveAmount = v; }
    void setAesGating(bool shouldGate)  { aesGating = shouldGate; }
    void setLongWindow(bool useLong)    { windowTauSeconds = useLong ? 60.0f : 3.0f; }

    float getMeasuredLufs() const { return lastMeasuredLufs; }
    float getCurrentCorrectionDb() const { return currentGainDb; }

    void process(juce::AudioBuffer<float>& buffer)
    {
        const int numCh = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();

        measureBuf.setSize(numCh, numSamples, false, false, true);
        measureBuf.makeCopyOf(buffer, true);

        juce::dsp::AudioBlock<float> block(measureBuf);
        juce::dsp::ProcessContextReplacing<float> ctx(block);
        preShelf.process(ctx);
        rlbHighPass.process(ctx);

        double sumSq = 0.0;
        for (int ch = 0; ch < numCh; ++ch)
        {
            auto* d = measureBuf.getReadPointer(ch);
            for (int i = 0; i < numSamples; ++i)
                sumSq += (double) d[i] * (double) d[i];
        }
        const float blockMeanSquare = (float) (sumSq / (double) juce::jmax(1, numCh * numSamples));
        const float momentaryLufs = -0.691f + 10.0f * std::log10(blockMeanSquare + 1.0e-12f);

        const bool includeBlock = (! aesGating) || momentaryLufs > -70.0f;
        if (includeBlock)
        {
            const double thisBlockSeconds = (double) numSamples / sampleRate;
            const float alpha = (float) juce::jlimit(0.0, 1.0, thisBlockSeconds / (double) windowTauSeconds);
            meanSquareAvg += (blockMeanSquare - meanSquareAvg) * alpha;
        }

        const float measuredLufs = -0.691f + 10.0f * std::log10(meanSquareAvg + 1.0e-12f);
        lastMeasuredLufs = measuredLufs;
        const float desiredCorrectionDb = juce::jlimit(-24.0f, 24.0f, target - measuredLufs);
        const float targetGainDb = desiredCorrectionDb * driveAmount;

        const float maxStepDb = (float) (6.0 * blockSeconds); // slew limit ~6 dB/sec
        currentGainDb += juce::jlimit(-maxStepDb, maxStepDb, targetGainDb - currentGainDb);

        buffer.applyGain(juce::Decibels::decibelsToGain(currentGainDb));
    }

private:
    double sampleRate = 44100.0;
    double blockSeconds = 0.01;

    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> preShelf, rlbHighPass;
    juce::AudioBuffer<float> measureBuf;

    float meanSquareAvg = 1.0e-6f;
    float currentGainDb = 0.0f;
    float lastMeasuredLufs = -70.0f;
    float target = -14.0f;
    float driveAmount = 0.0f;
    float windowTauSeconds = 3.0f;
    bool aesGating = false;
};
