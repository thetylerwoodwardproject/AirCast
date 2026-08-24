#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "Theme.h"

// A juce::ToggleButton whose visual is a pill switch (drawn by
// AirCastLookAndFeel::drawToggleButton), colour-coded by accent.
// Bypass uses isDanger=true: border/label stay red regardless of state.
class AccentToggle : public juce::ToggleButton
{
public:
    AccentToggle (const juce::String& text, juce::Colour accent, bool danger = false)
        : juce::ToggleButton (text), accentColour (accent), isDanger (danger) {}

    juce::Colour accentColour;
    bool isDanger;
};
