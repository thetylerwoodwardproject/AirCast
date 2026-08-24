#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "Theme.h"

// A rotary slider that also displays a live level meter (0-1) as a ring
// drawn inside the knob face by AirCastLookAndFeel::drawRotarySlider.
// The outer "value set" arc colour is the knob's section accent; the inner
// meter ring is drawn in green/amber/red segments up to the live level.
class MeteredKnob : public juce::Slider
{
public:
    MeteredKnob() : juce::Slider (juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow) {}

    void setMeterLevel01 (float v)
    {
        v = juce::jlimit (0.0f, 1.0f, v);
        if (std::abs (v - meterLevel) > 0.001f)
        {
            meterLevel = v;
            repaint();
        }
    }

    float getMeterLevel01() const { return meterLevel; }

    juce::Colour accentColour = Theme::accentTone;
    bool zeroDotStyle = false; // Bass Char: dot at zero instead of a filled arc when value == 0
    bool reductionStyle = false; // Gain-reduction meters: ring fills from the end angle backward, not from the start

private:
    float meterLevel = 0.0f;
};
