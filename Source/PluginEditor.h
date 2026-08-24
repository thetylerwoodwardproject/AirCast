#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "GUI/Theme.h"
#include "GUI/MeteredKnob.h"
#include "GUI/AirCastLookAndFeel.h"
#include "GUI/HelpIcon.h"
#include "GUI/MeterRail.h"
#include "GUI/PresetRail.h"
#include "GUI/AccentToggle.h"

class AirCastAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                           private juce::Timer
{
public:
    explicit AirCastAudioProcessorEditor (AirCastAudioProcessor&);
    ~AirCastAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct KnobWithLabel
    {
        MeteredKnob slider;
        juce::Label label;
        juce::Label valueLabel;
        std::unique_ptr<HelpIcon> helpIcon;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
        float meterFloorDb = -60.0f;
        float meterCeilDb = 0.0f;
        bool showPlusSign = false;
        juce::String valueSuffix;
    };

    struct SectionHeader
    {
        juce::Rectangle<int> bounds;
        juce::String title;
        juce::Colour colour;
    };

    void addKnob (KnobWithLabel& knob, const juce::String& paramID, const juce::String& text, const juce::String& helpText,
                  juce::Colour accent, float meterFloorDb = -60.0f, float meterCeilDb = 0.0f);
    void layoutKnobRow (juce::Rectangle<int> rowArea, const std::initializer_list<KnobWithLabel*>& knobs);
    void timerCallback() override;
    static void setKnobLevelFromDb (KnobWithLabel& knob, float db);
    static juce::String formatKnobValue (const KnobWithLabel& knob, float value);

    AirCastAudioProcessor& processor;
    AirCastLookAndFeel lookAndFeel;

    juce::Label titleLabel, subtitleLabel;
    juce::Label lufsCaptionLabel, lufsReadoutLabel;
    juce::TextButton resetButton { "RESET" };

    PresetRail presetRail;
    std::unique_ptr<HelpIcon> formatHelpIcon;

    MeterRail inputRail { "INPUT" };
    MeterRail outputRail { "OUTPUT" };

    std::array<SectionHeader, 3> sectionHeaders;

    KnobWithLabel densityKnob, detailKnob, driveKnob, highsKnob, lowsKnob, bassCharKnob,
                  expanderThresholdKnob, expanderRatioKnob, gateThresholdKnob,
                  lufsTargetKnob, lufsDriveKnob, truePeakKnob, masterOutKnob;

    AccentToggle monoBassButton    { "Mono Bass",    Theme::accentDynamics };
    AccentToggle gateToZeroButton  { "Gate To Zero", Theme::accentDynamics };
    AccentToggle aesLoudnessButton { "AES Gating",   Theme::accentDynamics };
    AccentToggle longWindowButton  { "60s Window",   Theme::accentDynamics };
    AccentToggle bypassButton      { "Bypass",       Theme::danger, true };

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> monoBassAttachment, gateToZeroAttachment,
        aesLoudnessAttachment, longWindowAttachment, bypassAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AirCastAudioProcessorEditor)
};
