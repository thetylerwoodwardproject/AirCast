#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "Parameters.h"
#include "DSP/CodecStage.h"
#include "DSP/ToneShaper.h"
#include "DSP/BroadcastCompressor.h"
#include "DSP/LoudnessProcessor.h"
#include "DSP/DownwardExpander.h"

class AirCastAudioProcessor final : public juce::AudioProcessor
{
public:
    AirCastAudioProcessor();
    ~AirCastAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // Per-stage activity meters, polled by the editor to drive the level ring drawn
    // inside each knob. Most are gain-reduction dB (0 = no activity, negative = more);
    // a few are pre-normalized 0-1 activity fractions (see the getters that fill them
    // in processBlock). Lock-free for UI-thread reads.
    struct Meters
    {
        std::atomic<float> inputL       { -100.0f };
        std::atomic<float> inputR       { -100.0f };
        std::atomic<float> outputL      { -100.0f };
        std::atomic<float> outputR      { -100.0f };

        std::atomic<float> expanderGrDb    { 0.0f };
        std::atomic<float> gateGrDb        { 0.0f };
        std::atomic<float> densityGrDb     { 0.0f };
        std::atomic<float> driveActivity01 { 0.0f };
        std::atomic<float> detailActivity01 { 0.0f };
        std::atomic<float> highsActivity01 { 0.0f };
        std::atomic<float> lowsActivity01  { 0.0f };
        std::atomic<float> bassCharActivity01 { 0.0f };
        std::atomic<float> loudnessCorrectionDb { 0.0f };
        std::atomic<float> measuredLufs    { -70.0f };
        std::atomic<float> truePeakGrDb    { 0.0f };
        std::atomic<float> masterDb        { -100.0f };
    };
    Meters meters;

private:
    void pullParameters();

    DownwardExpander expander;
    // A hard gate is just a downward expander with a much higher ratio; reusing the
    // class gives fast, fully-controlled ballistics and a direct GR getter instead of
    // juce::dsp::NoiseGate, whose hardcoded internal 50ms RMS smoothing made it too
    // sluggish to react within a typical speech pause (confirmed via Tools/DspProbe.cpp).
    DownwardExpander gate;
    BroadcastCompressor compressor;
    ToneShaper toneShaper;
    CodecStage codec;
    LoudnessProcessor loudness;

    // 2 channels (stereo-only plugin), factor 1 => 2x oversampling for true-peak detection.
    juce::dsp::Oversampling<float> oversampler { 2, 1, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true };
    juce::dsp::Limiter<float> truePeakLimiter;

    // Cached raw parameter pointers (lock-free atomics owned by the APVTS).
    std::atomic<float>* pCodecMode      = nullptr;
    std::atomic<float>* pDensity        = nullptr;
    std::atomic<float>* pDetail         = nullptr;
    std::atomic<float>* pDrive          = nullptr;
    std::atomic<float>* pHighs          = nullptr;
    std::atomic<float>* pLows           = nullptr;
    std::atomic<float>* pBassCharacter  = nullptr;
    std::atomic<float>* pMonoBass       = nullptr;
    std::atomic<float>* pExpanderThreshold = nullptr;
    std::atomic<float>* pExpanderRatio     = nullptr;
    std::atomic<float>* pGateThreshold  = nullptr;
    std::atomic<float>* pGateToZero     = nullptr;
    std::atomic<float>* pLufsTarget     = nullptr;
    std::atomic<float>* pLufsDrive      = nullptr;
    std::atomic<float>* pAesLoudness    = nullptr;
    std::atomic<float>* pLongWindow     = nullptr;
    std::atomic<float>* pTruePeak       = nullptr;
    std::atomic<float>* pMasterOut      = nullptr;
    std::atomic<float>* pBypass         = nullptr;

    float masterOutDb = 0.0f;
    bool bypassed = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirCastAudioProcessor)
};
