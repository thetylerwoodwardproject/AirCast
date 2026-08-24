#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace Param
{
    constexpr auto codecMode      = "codecMode";
    constexpr auto density        = "density";
    constexpr auto detail         = "detail";
    constexpr auto drive          = "drive";
    constexpr auto highs          = "highs";
    constexpr auto lows           = "lows";
    constexpr auto bassCharacter  = "bassCharacter";
    constexpr auto monoBass       = "monoBass";
    constexpr auto expanderThreshold = "expanderThreshold";
    constexpr auto expanderRatio     = "expanderRatio";
    constexpr auto gateThreshold  = "gateThreshold";
    constexpr auto gateToZero     = "gateToZero";
    constexpr auto lufsTarget     = "lufsTarget";
    constexpr auto lufsDrive      = "lufsDrive";
    constexpr auto aesLoudness    = "aesLoudness";
    constexpr auto longWindow     = "longWindow";
    constexpr auto truePeak       = "truePeak";
    constexpr auto masterOut      = "masterOut";
    constexpr auto bypass         = "bypass";

    // "Clean" is unprocessed passthrough; the rest are original signal-path
    // characters tuned to evoke the on-air feel of common radio formats -
    // not decodes or emulations of any station's actual hardware chain.
    inline juce::StringArray codecModeChoices()
    {
        return { "Clean", "Talk Radio (AM)", "Shock Jock (FM)", "Public Radio", "Late Night Talk (AM)", "Top 40 (CHR)",
                  "Classic Rock (FM)", "News Anchor", "Urban/Hip-Hop", "Classical/Fine Arts" };
    }

    struct PresetValues
    {
        float density, detail, drive, highs, lows, bassCharacter;
        bool  monoBass;
        float expanderThreshold, expanderRatio, gateThreshold;
        bool  gateToZero;
        float lufsTarget, lufsDrive;
        bool  aesLoudness, longWindow;
        float truePeak, masterOut;
    };

    // Indexed by codecMode / CodecStage::Mode order. "Clean" is every value at its
    // createLayout() default, so selecting Clean resets the panel like the Reset button.
    // masterOut is held at 0 for every preset (gain-staging, not tonal character);
    // bypass is intentionally not part of this table (manual safety toggle).
    inline const PresetValues& getPresetValues (int codecModeIndex)
    {
        static const PresetValues table[10] =
        {
            // density detail drive highs lows bassChar monoBass expThresh expRatio gateThresh gateToZero lufsTarget lufsDrive aesLoud longWindow truePeak masterOut
            { 50.0f, 1.0f, 25.0f,  0.0f,  0.0f,  0.0f, false, -45.0f, 2.0f, -55.0f, true,  -14.0f,  0.0f, false, false, -0.5f, 0.0f }, // Clean
            { 68.0f, 2.5f, 30.0f,  0.0f, -1.0f,  0.0f, true,  -40.0f, 3.5f, -45.0f, true,  -16.0f, 35.0f, true,  false, -1.0f, 0.0f }, // TalkRadio
            { 80.0f, 3.5f, 55.0f,  2.0f,  3.0f, 45.0f, true,  -50.0f, 2.5f, -52.0f, true,  -11.0f, 65.0f, true,  false, -1.5f, 0.0f }, // ShockJock
            { 28.0f, 0.5f,  8.0f, -1.0f,  1.5f,  8.0f, false, -65.0f, 1.3f, -70.0f, false, -20.0f, 10.0f, true,  true,  -0.3f, 0.0f }, // PublicRadio
            { 65.0f, 2.0f, 55.0f, -1.0f, -2.0f,  0.0f, true,  -38.0f, 4.0f, -42.0f, true,  -15.0f, 30.0f, true,  false, -1.0f, 0.0f }, // LateNightTalk
            { 75.0f, 4.5f, 32.0f,  3.0f,  3.5f, 18.0f, false, -52.0f, 2.0f, -60.0f, false,  -9.0f, 75.0f, false, false, -1.5f, 0.0f }, // Top40
            { 58.0f, 2.5f, 35.0f,  1.0f,  2.0f, 28.0f, false, -55.0f, 2.0f, -62.0f, false, -12.0f, 55.0f, false, false, -1.2f, 0.0f }, // ClassicRock
            { 48.0f, 3.0f,  5.0f, -0.5f, -1.5f,  0.0f, true,  -42.0f, 4.0f, -48.0f, true,  -16.0f, 40.0f, true,  false, -1.0f, 0.0f }, // NewsAnchor
            { 82.0f, 4.0f, 42.0f,  2.5f,  6.0f, 55.0f, true,  -50.0f, 2.5f, -58.0f, false,  -8.0f, 80.0f, false, false, -1.8f, 0.0f }, // UrbanHipHop
            { 10.0f, 0.3f,  0.0f,  0.0f,  0.0f,  0.0f, false, -70.0f, 1.2f, -72.0f, false, -23.0f,  5.0f, false, true,  -0.2f, 0.0f }, // Classical
        };
        return table[juce::jlimit (0, 9, codecModeIndex)];
    }

    inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
    {
        using namespace juce;
        std::vector<std::unique_ptr<RangedAudioParameter>> params;

        params.push_back(std::make_unique<AudioParameterChoice>(
            ParameterID { codecMode, 1 }, "Codec", codecModeChoices(), 0));

        params.push_back(std::make_unique<AudioParameterFloat>(
            ParameterID { density, 1 }, "Density",
            NormalisableRange<float>(0.0f, 100.0f), 50.0f));

        params.push_back(std::make_unique<AudioParameterFloat>(
            ParameterID { detail, 1 }, "Detail",
            NormalisableRange<float>(0.0f, 10.0f), 1.0f));

        params.push_back(std::make_unique<AudioParameterFloat>(
            ParameterID { drive, 1 }, "Drive",
            NormalisableRange<float>(0.0f, 100.0f), 25.0f));

        params.push_back(std::make_unique<AudioParameterFloat>(
            ParameterID { highs, 1 }, "Highs",
            NormalisableRange<float>(-12.0f, 12.0f), 0.0f));

        params.push_back(std::make_unique<AudioParameterFloat>(
            ParameterID { lows, 1 }, "Lows",
            NormalisableRange<float>(-12.0f, 12.0f), 0.0f));

        params.push_back(std::make_unique<AudioParameterFloat>(
            ParameterID { bassCharacter, 1 }, "Bass Character",
            NormalisableRange<float>(0.0f, 100.0f), 0.0f));

        params.push_back(std::make_unique<AudioParameterBool>(
            ParameterID { monoBass, 1 }, "Mono Bass", false));

        params.push_back(std::make_unique<AudioParameterFloat>(
            ParameterID { expanderThreshold, 1 }, "Expander Threshold",
            NormalisableRange<float>(-80.0f, 0.0f), -45.0f));

        params.push_back(std::make_unique<AudioParameterFloat>(
            ParameterID { expanderRatio, 1 }, "Expander Ratio",
            NormalisableRange<float>(1.0f, 10.0f), 2.0f));

        params.push_back(std::make_unique<AudioParameterFloat>(
            ParameterID { gateThreshold, 1 }, "Gate Threshold",
            NormalisableRange<float>(-80.0f, 0.0f), -55.0f));

        params.push_back(std::make_unique<AudioParameterBool>(
            ParameterID { gateToZero, 1 }, "Gate To Zero", true));

        params.push_back(std::make_unique<AudioParameterFloat>(
            ParameterID { lufsTarget, 1 }, "LUFS Target",
            NormalisableRange<float>(-30.0f, -6.0f), -14.0f));

        params.push_back(std::make_unique<AudioParameterFloat>(
            ParameterID { lufsDrive, 1 }, "LUFS Drive",
            NormalisableRange<float>(0.0f, 100.0f), 0.0f));

        params.push_back(std::make_unique<AudioParameterBool>(
            ParameterID { aesLoudness, 1 }, "AES Loudness Mode", false));

        params.push_back(std::make_unique<AudioParameterBool>(
            ParameterID { longWindow, 1 }, "60s Window", false));

        params.push_back(std::make_unique<AudioParameterFloat>(
            ParameterID { truePeak, 1 }, "True Peak",
            NormalisableRange<float>(-3.0f, 0.0f), -0.5f));

        params.push_back(std::make_unique<AudioParameterFloat>(
            ParameterID { masterOut, 1 }, "Master Out",
            NormalisableRange<float>(-24.0f, 12.0f), 0.0f));

        params.push_back(std::make_unique<AudioParameterBool>(
            ParameterID { bypass, 1 }, "Bypass", false));

        return { params.begin(), params.end() };
    }
}
