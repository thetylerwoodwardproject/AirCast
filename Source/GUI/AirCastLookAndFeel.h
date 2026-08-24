#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "MeteredKnob.h"
#include "AccentToggle.h"
#include "Theme.h"

// Draws rotary knobs as: background track -> section-accent value arc ->
// live level-meter arc (green/amber/red segments) -> centre disc -> pointer.
// Also draws AccentToggle as a pill switch. Matches the "2a" design handoff.
class AirCastLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                            float sliderPos, const float rotaryStartAngle, const float rotaryEndAngle,
                            juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (2.0f);
        auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto centre = bounds.getCentre();
        auto toAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        const float outerR = radius * 0.75f;
        const float innerR = radius * 0.53f;
        const float strokeW = juce::jmax (2.0f, radius * 0.11f);
        const float discR = radius * 0.36f;

        auto* metered = dynamic_cast<MeteredKnob*> (&slider);
        const juce::Colour accent = metered != nullptr ? metered->accentColour : Theme::accentTone;

        // Outer background track (full sweep)
        juce::Path track;
        track.addCentredArc (centre.x, centre.y, outerR, outerR, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (Theme::knobTrack);
        g.strokePath (track, juce::PathStrokeType (strokeW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        const bool zeroDot = metered != nullptr && metered->zeroDotStyle && sliderPos < 0.001f;

        if (zeroDot)
        {
            auto dotPos = centre.getPointOnCircumference (outerR, 0.0f); // straight up (12 o'clock)
            g.setColour (accent);
            g.fillEllipse (dotPos.x - 2.5f, dotPos.y - 2.5f, 5.0f, 5.0f);
        }
        else
        {
            juce::Path valueArc;
            valueArc.addCentredArc (centre.x, centre.y, outerR, outerR, 0.0f, rotaryStartAngle, toAngle, true);
            g.setColour (accent);
            g.strokePath (valueArc, juce::PathStrokeType (strokeW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // Inner live-level meter: only the lit portion is drawn (no idle track), in
        // green/amber/red segments so a knob showing a hot level reads red at a glance.
        if (metered != nullptr)
        {
            const float level = metered->getMeterLevel01();
            if (level > 0.005f)
            {
                const bool reduction = metered->reductionStyle;
                auto drawSegment = [&] (float from, float to, juce::Colour colour)
                {
                    if (to <= from) return;
                    // Reduction meters grow from the end angle backward (mirror the segment),
                    // so the ring reads as being eaten into from the max side rather than
                    // building up from zero like a normal activity meter.
                    const float segFrom = reduction ? 1.0f - to   : from;
                    const float segTo   = reduction ? 1.0f - from : to;
                    juce::Path seg;
                    seg.addCentredArc (centre.x, centre.y, innerR, innerR, 0.0f,
                                        rotaryStartAngle + segFrom * (rotaryEndAngle - rotaryStartAngle),
                                        rotaryStartAngle + segTo * (rotaryEndAngle - rotaryStartAngle), true);
                    g.setColour (colour);
                    g.strokePath (seg, juce::PathStrokeType (strokeW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                };

                drawSegment (0.0f, juce::jmin (level, 0.7f), Theme::accentDynamics);
                drawSegment (0.7f, juce::jmin (level, 0.9f), Theme::accentTone);
                drawSegment (0.9f, level, Theme::meterHot);
            }
        }

        // Centre disc (punches through the rings) + pointer
        g.setColour (Theme::centreDisc);
        g.fillEllipse (centre.x - discR, centre.y - discR, discR * 2.0f, discR * 2.0f);

        juce::Path pointer;
        const float pointerLength = discR * 0.85f;
        pointer.addRectangle (-1.0f, -pointerLength, 2.0f, pointerLength * 0.65f);
        pointer.applyTransform (juce::AffineTransform::rotation (toAngle).translated (centre));
        g.setColour (juce::Colours::white);
        g.fillPath (pointer);
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&,
                                bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat();
        if (shouldDrawButtonAsDown || shouldDrawButtonAsHighlighted)
        {
            g.setColour (Theme::resetBorder.withAlpha (0.25f));
            g.fillRoundedRectangle (bounds, 5.0f);
        }
        g.setColour (Theme::resetBorder);
        g.drawRoundedRectangle (bounds.reduced (0.5f), 5.0f, 1.0f);
    }

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                            bool /*shouldDrawButtonAsHighlighted*/, bool /*shouldDrawButtonAsDown*/) override
    {
        auto* accentBtn = dynamic_cast<AccentToggle*> (&button);
        const juce::Colour accent = accentBtn != nullptr ? accentBtn->accentColour : Theme::accentDynamics;
        const bool danger = accentBtn != nullptr && accentBtn->isDanger;
        const bool on = button.getToggleState();

        constexpr float pillW = 28.0f, pillH = 14.0f;
        auto bounds = button.getLocalBounds().toFloat();
        auto pillArea = bounds.removeFromLeft (pillW).withSizeKeepingCentre (pillW, pillH);

        juce::Colour trackColour = danger ? Theme::danger.withAlpha (on ? 0.35f : 0.0f)
                                           : (on ? accent.withAlpha (0.22f) : juce::Colour (0xff1c2027));
        juce::Colour borderColour = danger ? Theme::danger : (on ? accent : juce::Colour (0xff2a2f38));
        juce::Colour thumbColour  = danger ? Theme::danger : (on ? accent : juce::Colour (0xff4a5058));

        g.setColour (trackColour);
        g.fillRoundedRectangle (pillArea, pillH * 0.5f);
        g.setColour (borderColour);
        g.drawRoundedRectangle (pillArea, pillH * 0.5f, 1.0f);

        const float thumbD = 10.0f;
        const float thumbX = on ? pillArea.getRight() - thumbD - 2.0f : pillArea.getX() + 2.0f;
        g.setColour (thumbColour);
        g.fillEllipse (thumbX, pillArea.getCentreY() - thumbD * 0.5f, thumbD, thumbD);

        auto textArea = bounds.withTrimmedLeft (8.0f);
        g.setColour (danger ? Theme::danger : (on ? Theme::textSecondary : Theme::textMuted));
        g.setFont (Theme::sans (12.5f));
        g.drawFittedText (button.getButtonText(), textArea.toNearestInt(), juce::Justification::centredLeft, 1);
    }
};
