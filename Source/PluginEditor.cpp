#include "PluginEditor.h"
#include "Parameters.h"

namespace
{
    constexpr int helpIconSize = 13;
    constexpr int knobDialSize = 58;

    float normalise (float value, float floorV, float ceilV)
    {
        return juce::jlimit (0.0f, 1.0f, (value - floorV) / (ceilV - floorV));
    }
}

juce::String AirCastAudioProcessorEditor::formatKnobValue (const KnobWithLabel& k, float value)
{
    juce::String s = (k.showPlusSign && value > 0.0f ? "+" : juce::String()) + juce::String (value, 1);
    return s + k.valueSuffix;
}

AirCastAudioProcessorEditor::AirCastAudioProcessorEditor (AirCastAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p), presetRail (p.apvts)
{
    setLookAndFeel (&lookAndFeel);

    titleLabel.setText ("AirCast", juce::dontSendNotification);
    titleLabel.setFont (Theme::sans (24.0f, juce::Font::bold));
    titleLabel.setColour (juce::Label::textColourId, Theme::textPrimary);
    addAndMakeVisible (titleLabel);

    subtitleLabel.setText ("BROADCAST VOICE PROCESSOR", juce::dontSendNotification);
    subtitleLabel.setFont (Theme::mono (10.5f).withExtraKerningFactor (0.1f));
    subtitleLabel.setColour (juce::Label::textColourId, Theme::textFaint);
    addAndMakeVisible (subtitleLabel);

    lufsCaptionLabel.setText ("INTEGRATED LUFS", juce::dontSendNotification);
    lufsCaptionLabel.setFont (Theme::mono (10.0f).withExtraKerningFactor (0.06f));
    lufsCaptionLabel.setColour (juce::Label::textColourId, Theme::textFaint);
    lufsCaptionLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (lufsCaptionLabel);

    lufsReadoutLabel.setText ("--", juce::dontSendNotification);
    lufsReadoutLabel.setFont (Theme::mono (20.0f, juce::Font::bold));
    lufsReadoutLabel.setColour (juce::Label::textColourId, Theme::accentTone);
    lufsReadoutLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (lufsReadoutLabel);

    resetButton.setColour (juce::TextButton::textColourOffId, Theme::resetText);
    resetButton.setColour (juce::TextButton::textColourOnId, Theme::resetText);
    resetButton.setColour (juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    resetButton.onClick = [this]
    {
        for (auto* param : processor.getParameters())
            param->setValueNotifyingHost (param->getDefaultValue());
    };
    addAndMakeVisible (resetButton);

    addAndMakeVisible (presetRail);
    formatHelpIcon = std::make_unique<HelpIcon> (
        "Format character presets: tonal EQ shaping, light saturation, and compression feel "
        "(attack/release) tuned to evoke each format's mix style - not band-limited transmission "
        "simulation, so it won't sound like a phone line or AM tuner. Clean = no coloration.");
    addAndMakeVisible (*formatHelpIcon);

    addAndMakeVisible (inputRail);
    addAndMakeVisible (outputRail);

    addKnob (densityKnob, Param::density, "Density",
        "Broadcast-style compression amount. Higher = lower threshold and higher ratio, "
        "squashing dynamic range for that dense, always-loud radio sound.", Theme::accentTone, 0.0f, -20.0f);
    densityKnob.slider.reductionStyle = true;
    addKnob (detailKnob, Param::detail, "Detail",
        "High-frequency exciter. Adds harmonic 'air' above ~3.5kHz on top of the compressed "
        "signal, since heavy compression tends to dull transients and highs.", Theme::accentTone);
    addKnob (driveKnob, Param::drive, "Drive",
        "Post-compression saturation. Blends in tanh-style soft clipping for warmth/grit, "
        "and also intensifies the format profile's distortion character.", Theme::accentTone);
    addKnob (highsKnob, Param::highs, "Highs",
        "High-shelf EQ above 8kHz. Boost or cut the top end.", Theme::accentTone);
    highsKnob.showPlusSign = true;
    addKnob (lowsKnob, Param::lows, "Lows",
        "Low-shelf EQ below 150Hz. Boost or cut the bottom end.", Theme::accentTone);
    lowsKnob.showPlusSign = true;
    addKnob (bassCharKnob, Param::bassCharacter, "Bass Char",
        "Adds tanh-style saturation for extra low-end warmth/thickness, applied only to the "
        "band below 120Hz so it colors the bass without touching mids or highs.",
        Theme::accentTone);
    bassCharKnob.slider.zeroDotStyle = true;

    addKnob (expanderThresholdKnob, Param::expanderThreshold, "Exp Thresh",
        "Downward expander threshold in dB. Audio below this is continuously pulled down "
        "(proportional to Expander Ratio) to tame reverb tails and noise floor. Fast, fixed "
        "attack/release keep it transparent rather than pumpy - it's not a hard gate.",
        Theme::accentDynamics, 0.0f, -25.0f);
    expanderThresholdKnob.slider.reductionStyle = true;
    addKnob (expanderRatioKnob, Param::expanderRatio, "Exp Ratio",
        "Downward expander ratio. 1:1 = no effect, higher ratios push level below threshold "
        "down harder. Works together with Expander Threshold.",
        Theme::accentDynamics, 0.0f, -25.0f);
    expanderRatioKnob.valueSuffix = ":1";
    expanderRatioKnob.slider.reductionStyle = true;
    addKnob (gateThresholdKnob, Param::gateThreshold, "Gate Thresh",
        "Noise gate threshold in dB. Audio below this level gets attenuated - or muted "
        "entirely if Gate To Zero is on - cleaning up dead air between words.", Theme::accentDynamics, 0.0f, -40.0f);
    gateThresholdKnob.slider.reductionStyle = true;

    addKnob (lufsTargetKnob, Param::lufsTarget, "LUFS Target",
        "Target loudness (LUFS) the normalizer converges toward. Typical broadcast range is "
        "roughly -24 to -14 LUFS. Current measured loudness shows next to the format selector.",
        Theme::accentLoudness, 0.0f, 12.0f);
    addKnob (lufsDriveKnob, Param::lufsDrive, "LUFS Drive",
        "How aggressively the loudness normalizer corrects toward LUFS Target. 0% = no "
        "correction, 100% = full correction, slew-limited to ~6dB/sec to avoid pumping.",
        Theme::accentLoudness, 0.0f, 12.0f);
    addKnob (truePeakKnob, Param::truePeak, "True Peak",
        "True-peak ceiling in dBFS. A 2x-oversampled limiter prevents inter-sample peaks "
        "from exceeding this after everything else in the chain.", Theme::accentLoudness, 0.0f, -10.0f);
    truePeakKnob.slider.reductionStyle = true;
    addKnob (masterOutKnob, Param::masterOut, "Master Out",
        "Final output trim in dB, applied after the true-peak limiter.", Theme::accentLoudness, -24.0f, 0.0f);
    masterOutKnob.showPlusSign = true;

    for (auto* b : { &monoBassButton, &gateToZeroButton, &aesLoudnessButton, &longWindowButton, &bypassButton })
        addAndMakeVisible (b);

    monoBassButton.setTooltip ("Sums everything below 120Hz to mono while keeping highs stereo - "
        "keeps low end solid and mono-compatible, a classic broadcast-chain move.");
    gateToZeroButton.setTooltip ("When on, the gate hard-mutes below threshold (~100:1) instead of "
        "just turning down (4:1).");
    aesLoudnessButton.setTooltip ("When on, the loudness measurement ignores blocks quieter than "
        "-70 LUFS (silence/pauses) so they don't drag the running average down.");
    longWindowButton.setTooltip ("Switches the loudness measurement's averaging window from a fast "
        "~3 second response to a slow ~60 second integrated average.");
    bypassButton.setTooltip ("Bypasses all processing - passes audio through unmodified.");

    monoBassAttachment    = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, Param::monoBass, monoBassButton);
    gateToZeroAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, Param::gateToZero, gateToZeroButton);
    aesLoudnessAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, Param::aesLoudness, aesLoudnessButton);
    longWindowAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, Param::longWindow, longWindowButton);
    bypassAttachment      = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, Param::bypass, bypassButton);

    setResizable (false, false);
    setSize (900, 650);

    startTimerHz (30);
}

AirCastAudioProcessorEditor::~AirCastAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void AirCastAudioProcessorEditor::addKnob (KnobWithLabel& knob, const juce::String& paramID, const juce::String& text,
                                            const juce::String& helpText, juce::Colour accent,
                                            float meterFloorDb, float meterCeilDb)
{
    knob.meterFloorDb = meterFloorDb;
    knob.meterCeilDb = meterCeilDb;
    knob.slider.accentColour = accent;

    knob.slider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    knob.slider.onValueChange = [this, &knob]
    {
        knob.valueLabel.setText (formatKnobValue (knob, (float) knob.slider.getValue()), juce::dontSendNotification);
    };
    addAndMakeVisible (knob.slider);

    knob.label.setText (text, juce::dontSendNotification);
    knob.label.setFont (Theme::sans (12.0f));
    knob.label.setColour (juce::Label::textColourId, Theme::textMuted);
    knob.label.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (knob.label);

    knob.valueLabel.setFont (Theme::mono (12.5f, juce::Font::bold));
    knob.valueLabel.setColour (juce::Label::textColourId, Theme::textSecondary);
    knob.valueLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (knob.valueLabel);

    knob.helpIcon = std::make_unique<HelpIcon> (helpText);
    addAndMakeVisible (*knob.helpIcon);

    knob.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.apvts, paramID, knob.slider);

    knob.valueLabel.setText (formatKnobValue (knob, (float) knob.slider.getValue()), juce::dontSendNotification);
}

void AirCastAudioProcessorEditor::setKnobLevelFromDb (KnobWithLabel& knob, float db)
{
    knob.slider.setMeterLevel01 (normalise (db, knob.meterFloorDb, knob.meterCeilDb));
}

void AirCastAudioProcessorEditor::timerCallback()
{
    const auto& m = processor.meters;

    setKnobLevelFromDb (expanderThresholdKnob, m.expanderGrDb.load());
    setKnobLevelFromDb (expanderRatioKnob, m.expanderGrDb.load());
    setKnobLevelFromDb (gateThresholdKnob, m.gateGrDb.load());

    setKnobLevelFromDb (densityKnob, m.densityGrDb.load());
    detailKnob.slider.setMeterLevel01 (m.detailActivity01.load());
    driveKnob.slider.setMeterLevel01 (m.driveActivity01.load());

    highsKnob.slider.setMeterLevel01 (m.highsActivity01.load());
    lowsKnob.slider.setMeterLevel01 (m.lowsActivity01.load());
    bassCharKnob.slider.setMeterLevel01 (m.bassCharActivity01.load());

    const float correctionDb = std::abs (m.loudnessCorrectionDb.load());
    setKnobLevelFromDb (lufsTargetKnob, correctionDb);
    setKnobLevelFromDb (lufsDriveKnob, correctionDb);

    setKnobLevelFromDb (truePeakKnob, m.truePeakGrDb.load());
    setKnobLevelFromDb (masterOutKnob, m.masterDb.load());

    for (auto* k : { &densityKnob, &detailKnob, &driveKnob, &highsKnob, &lowsKnob, &bassCharKnob,
                      &expanderThresholdKnob, &expanderRatioKnob, &gateThresholdKnob,
                      &lufsTargetKnob, &lufsDriveKnob, &truePeakKnob, &masterOutKnob })
        k->valueLabel.setText (formatKnobValue (*k, (float) k->slider.getValue()), juce::dontSendNotification);

    const float measured = m.measuredLufs.load();
    lufsReadoutLabel.setText (measured <= -69.0f ? juce::String ("--") : juce::String (measured, 1),
                               juce::dontSendNotification);

    inputRail.pushLevels (m.inputL.load(), m.inputR.load());
    outputRail.pushLevels (m.outputL.load(), m.outputR.load());

    presetRail.repaint();
}

void AirCastAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (Theme::bgOuter);

    auto panelBounds = getLocalBounds().reduced (14).toFloat();
    panelBounds.removeFromLeft (76.0f + 10.0f);
    panelBounds.removeFromRight (76.0f + 10.0f);

    g.setColour (Theme::bgPanel);
    g.fillRoundedRectangle (panelBounds, 8.0f);
    g.setColour (Theme::borderColour);
    g.drawRoundedRectangle (panelBounds.reduced (0.5f), 8.0f, 1.0f);

    for (auto& section : sectionHeaders)
    {
        // Work on a local copy - removeFromLeft mutates in place, and section.bounds
        // is stored state that must survive to the next paint() call untouched.
        auto remaining = section.bounds;

        g.setColour (section.colour);
        g.setFont (Theme::mono (10.5f).withExtraKerningFactor (0.1f));
        const int textW = g.getCurrentFont().getStringWidth (section.title) + 10;
        g.drawText (section.title, remaining.removeFromLeft (textW), juce::Justification::centredLeft);

        g.setColour (Theme::borderColour);
        auto lineY = (float) remaining.getCentreY();
        g.drawLine ((float) remaining.getX(), lineY, (float) remaining.getRight(), lineY, 1.0f);
    }
}

void AirCastAudioProcessorEditor::layoutKnobRow (juce::Rectangle<int> rowArea, const std::initializer_list<KnobWithLabel*>& knobs)
{
    const int colWidth = rowArea.getWidth() / (int) knobs.size();
    for (auto* k : knobs)
    {
        auto col = rowArea.removeFromLeft (colWidth);

        auto labelArea = col.removeFromTop (19);
        k->helpIcon->setBounds (labelArea.removeFromRight (helpIconSize + 4).withSizeKeepingCentre (helpIconSize, helpIconSize));
        k->label.setBounds (labelArea);

        k->slider.setBounds (col.removeFromTop (knobDialSize + 4).withSizeKeepingCentre (knobDialSize, knobDialSize));
        k->valueLabel.setBounds (col.removeFromTop (19));
    }
}

void AirCastAudioProcessorEditor::resized()
{
    auto full = getLocalBounds().reduced (14);

    auto leftRail = full.removeFromLeft (76);
    full.removeFromLeft (10);
    auto rightRail = full.removeFromRight (76);
    full.removeFromRight (10);
    inputRail.setBounds (leftRail);
    outputRail.setBounds (rightRail);

    auto content = full.reduced (24, 18);

    auto headerRow = content.removeFromTop (46);
    auto headerLeft = headerRow.removeFromLeft (headerRow.getWidth() / 2);
    titleLabel.setBounds (headerLeft.removeFromTop (30));
    subtitleLabel.setBounds (headerLeft);

    auto resetArea = headerRow.removeFromRight (64);
    resetButton.setBounds (resetArea.withSizeKeepingCentre (64, 24));
    lufsReadoutLabel.setBounds (headerRow.removeFromBottom (24));
    lufsCaptionLabel.setBounds (headerRow.removeFromBottom (14));
    content.removeFromTop (11);

    auto formatRow = content.removeFromTop (34);
    formatHelpIcon->setBounds (formatRow.removeFromRight (helpIconSize + 8).withSizeKeepingCentre (helpIconSize, helpIconSize));
    formatRow.removeFromRight (6);
    presetRail.setBounds (formatRow);
    content.removeFromTop (11);

    constexpr int knobRowHeight = 19 + knobDialSize + 4 + 19;
    constexpr int sectionHeaderHeight = 16;
    constexpr int sectionGap = 9;

    auto layoutSection = [&] (int index, const juce::String& title, juce::Colour accent,
                               const std::initializer_list<KnobWithLabel*>& knobs)
    {
        auto headerArea = content.removeFromTop (sectionHeaderHeight);
        sectionHeaders[(size_t) index] = { headerArea, title, accent };
        content.removeFromTop (sectionGap);
        layoutKnobRow (content.removeFromTop (knobRowHeight), knobs);
        content.removeFromTop (11);
    };

    layoutSection (0, "TONE & CHARACTER", Theme::accentTone,
        { &densityKnob, &detailKnob, &driveKnob, &highsKnob, &lowsKnob, &bassCharKnob });
    layoutSection (1, "DYNAMICS / CLEANUP", Theme::accentDynamics,
        { &expanderThresholdKnob, &expanderRatioKnob, &gateThresholdKnob });
    layoutSection (2, "LOUDNESS & OUTPUT", Theme::accentLoudness,
        { &lufsTargetKnob, &lufsDriveKnob, &truePeakKnob, &masterOutKnob });

    auto toggleRow = content.removeFromBottom (34);
    toggleRow.removeFromTop (9); // top border + padding

    const juce::Font toggleFont = Theme::sans (12.5f);
    juce::Array<AccentToggle*> toggles { &monoBassButton, &gateToZeroButton, &aesLoudnessButton, &longWindowButton };
    int x = toggleRow.getX();
    for (auto* t : toggles)
    {
        const int w = 28 + 8 + toggleFont.getStringWidth (t->getButtonText());
        t->setBounds (x, toggleRow.getY(), w, toggleRow.getHeight());
        x += w + 18;
    }

    const int bypassW = 28 + 8 + toggleFont.getStringWidth (bypassButton.getButtonText());
    bypassButton.setBounds (toggleRow.getRight() - bypassW, toggleRow.getY(), bypassW, toggleRow.getHeight());
}
