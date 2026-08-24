#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    float bufferPeakDb (const juce::AudioBuffer<float>& b)
    {
        return juce::Decibels::gainToDecibels (b.getMagnitude (0, b.getNumSamples()), -100.0f);
    }

    float channelPeakDb (const juce::AudioBuffer<float>& b, int channel)
    {
        if (channel >= b.getNumChannels())
            return -100.0f;
        return juce::Decibels::gainToDecibels (b.getMagnitude (channel, 0, b.getNumSamples()), -100.0f);
    }
}

AirCastAudioProcessor::AirCastAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", Param::createLayout())
{
    pCodecMode     = apvts.getRawParameterValue (Param::codecMode);
    pDensity       = apvts.getRawParameterValue (Param::density);
    pDetail        = apvts.getRawParameterValue (Param::detail);
    pDrive         = apvts.getRawParameterValue (Param::drive);
    pHighs         = apvts.getRawParameterValue (Param::highs);
    pLows          = apvts.getRawParameterValue (Param::lows);
    pBassCharacter = apvts.getRawParameterValue (Param::bassCharacter);
    pMonoBass      = apvts.getRawParameterValue (Param::monoBass);
    pExpanderThreshold = apvts.getRawParameterValue (Param::expanderThreshold);
    pExpanderRatio     = apvts.getRawParameterValue (Param::expanderRatio);
    pGateThreshold = apvts.getRawParameterValue (Param::gateThreshold);
    pGateToZero    = apvts.getRawParameterValue (Param::gateToZero);
    pLufsTarget    = apvts.getRawParameterValue (Param::lufsTarget);
    pLufsDrive     = apvts.getRawParameterValue (Param::lufsDrive);
    pAesLoudness   = apvts.getRawParameterValue (Param::aesLoudness);
    pLongWindow    = apvts.getRawParameterValue (Param::longWindow);
    pTruePeak      = apvts.getRawParameterValue (Param::truePeak);
    pMasterOut     = apvts.getRawParameterValue (Param::masterOut);
    pBypass        = apvts.getRawParameterValue (Param::bypass);
}

AirCastAudioProcessor::~AirCastAudioProcessor() = default;

void AirCastAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels = (juce::uint32) getTotalNumOutputChannels();

    expander.prepare (spec);
    gate.prepare (spec);

    compressor.prepare (spec);
    toneShaper.prepare (spec);
    codec.prepare (spec);
    loudness.prepare (spec);

    oversampler.initProcessing ((size_t) samplesPerBlock);
    oversampler.reset();
    truePeakLimiter.prepare ({ spec.sampleRate * 2.0, spec.maximumBlockSize * 2, spec.numChannels });
    truePeakLimiter.setRelease (50.0f);

    pullParameters();
}

void AirCastAudioProcessor::releaseResources()
{
    oversampler.reset();
}

bool AirCastAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Stereo-only: the mono-bass crossover and the 2-channel oversampler both assume it.
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo();
}

void AirCastAudioProcessor::pullParameters()
{
    expander.setThresholdDb (pExpanderThreshold->load());
    expander.setRatio (pExpanderRatio->load());

    gate.setThresholdDb (pGateThreshold->load());
    gate.setRatio (pGateToZero->load() > 0.5f ? 100.0f : 4.0f);

    codec.setMode ((int) pCodecMode->load());
    codec.setDriveAmount (pDrive->load() / 100.0f);

    compressor.setDensity01 (pDensity->load() / 100.0f);
    compressor.setDrive01 (pDrive->load() / 100.0f);
    compressor.setDetail01 (pDetail->load() / 10.0f);
    compressor.setAttackRelease (codec.getLowAttackMs(),  codec.getLowReleaseMs(),
                                  codec.getAttackMs(),     codec.getReleaseMs(),
                                  codec.getHighAttackMs(), codec.getHighReleaseMs());

    toneShaper.setHighsDb (pHighs->load());
    toneShaper.setLowsDb (pLows->load());
    toneShaper.setBassCharacter01 (pBassCharacter->load() / 100.0f);
    toneShaper.setMonoBass (pMonoBass->load() > 0.5f);

    loudness.setTargetLufs (pLufsTarget->load());
    loudness.setDriveAmount01 (pLufsDrive->load() / 100.0f);
    loudness.setAesGating (pAesLoudness->load() > 0.5f);
    loudness.setLongWindow (pLongWindow->load() > 0.5f);

    truePeakLimiter.setThreshold (pTruePeak->load());

    masterOutDb = pMasterOut->load();
    bypassed = pBypass->load() > 0.5f;
}

void AirCastAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    pullParameters();

    for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    meters.inputL.store (channelPeakDb (buffer, 0));
    meters.inputR.store (channelPeakDb (buffer, 1));

    if (bypassed)
    {
        meters.outputL.store (channelPeakDb (buffer, 0));
        meters.outputR.store (channelPeakDb (buffer, 1));
        meters.expanderGrDb.store (0.0f);
        meters.gateGrDb.store (0.0f);
        meters.densityGrDb.store (0.0f);
        meters.driveActivity01.store (0.0f);
        meters.detailActivity01.store (0.0f);
        meters.highsActivity01.store (0.0f);
        meters.lowsActivity01.store (0.0f);
        meters.bassCharActivity01.store (0.0f);
        meters.loudnessCorrectionDb.store (0.0f);
        meters.truePeakGrDb.store (0.0f);
        meters.masterDb.store (-100.0f);
        return;
    }

    // Downward expander (quick, transparent reduction of low-level noise/reverb)
    expander.process (buffer);
    meters.expanderGrDb.store (expander.getLastReductionDb());

    // Gate (hard mute for dead air)
    gate.process (buffer);
    meters.gateGrDb.store (gate.getLastReductionDb());

    // Density / Detail / Drive
    compressor.process (buffer);
    meters.densityGrDb.store (compressor.getDensityGrDb());
    meters.driveActivity01.store (compressor.getDriveActivity01());
    meters.detailActivity01.store (compressor.getDetailActivity01());

    // Highs / Lows / Bass character / Mono bass
    toneShaper.process (buffer);
    meters.highsActivity01.store (toneShaper.getHighsActivity01());
    meters.lowsActivity01.store (toneShaper.getLowsActivity01());
    meters.bassCharActivity01.store (toneShaper.getBassCharActivity01());

    // Codec character (station/format profile)
    {
        juce::dsp::AudioBlock<float> block (buffer);
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        codec.process (ctx);
    }

    // LUFS loudness normalization
    loudness.process (buffer);
    meters.measuredLufs.store (loudness.getMeasuredLufs());
    meters.loudnessCorrectionDb.store (loudness.getCurrentCorrectionDb());

    // True-peak limiting (oversampled)
    {
        const float preLimiterDb = bufferPeakDb (buffer);
        juce::dsp::AudioBlock<float> block (buffer);
        auto oversampledBlock = oversampler.processSamplesUp (block);
        juce::dsp::ProcessContextReplacing<float> ctx (oversampledBlock);
        truePeakLimiter.process (ctx);
        oversampler.processSamplesDown (block);
        meters.truePeakGrDb.store (juce::jmin (0.0f, bufferPeakDb (buffer) - preLimiterDb));
    }

    // Master out trim
    buffer.applyGain (juce::Decibels::decibelsToGain (masterOutDb));
    meters.masterDb.store (bufferPeakDb (buffer));
    meters.outputL.store (channelPeakDb (buffer, 0));
    meters.outputR.store (channelPeakDb (buffer, 1));
}

juce::AudioProcessorEditor* AirCastAudioProcessor::createEditor()
{
    return new AirCastAudioProcessorEditor (*this);
}

void AirCastAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
    {
        if (auto xml = state.createXml())
            copyXmlToBinary (*xml, destData);
    }
}

void AirCastAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AirCastAudioProcessor();
}
