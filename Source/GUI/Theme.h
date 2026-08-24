#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// Design tokens from the "2a" handoff (see DESIGN_BRIEF.md / the mockup handoff README).
// IBM Plex Sans/Mono aren't bundled - substituting the system default sans/mono fonts,
// as the handoff explicitly allows.
namespace Theme
{
    // Backgrounds
    static const juce::Colour bgOuter        (0xff0d0f13);
    static const juce::Colour bgPanel        (0xff15181d);
    static const juce::Colour bgRail         (0xff101318);
    static const juce::Colour bgFormatRow    (0xff101318);
    static const juce::Colour borderColour   (0xff23272f);
    static const juce::Colour knobTrack      (0xff23272f);
    static const juce::Colour centreDisc     (0xff0d0f13);
    static const juce::Colour segmentUnlit   (0xff1c2027);

    // Text
    static const juce::Colour textPrimary    (0xfff4f6f8);
    static const juce::Colour textSecondary  (0xffc7ccd2);
    static const juce::Colour textMuted      (0xff9aa5b1);
    static const juce::Colour textFaint      (0xff4a5058);
    static const juce::Colour textFainter    (0xff5b6472);

    // Section accents
    static const juce::Colour accentTone     (0xfff0a832); // amber - Tone & Character
    static const juce::Colour accentDynamics (0xff3ecf8e); // green - Dynamics/Cleanup
    static const juce::Colour accentLoudness (0xff4fd3f0); // cyan - Loudness & Output

    static const juce::Colour danger         (0xffc0505a);
    static const juce::Colour dangerBright   (0xffe5555f);
    static const juce::Colour meterHot       (0xffe74c3c);

    static const juce::Colour helpBadgeBg    (0xff262b32);
    static const juce::Colour helpBadgeText  (0xff8a929e);

    static const juce::Colour resetBorder    (0xff333944);
    static const juce::Colour resetText      (0xffb7bec7);

    inline juce::Font sans (float size, juce::Font::FontStyleFlags style = juce::Font::plain)
    {
        return juce::Font (juce::Font::getDefaultSansSerifFontName(), size, style);
    }

    inline juce::Font mono (float size, juce::Font::FontStyleFlags style = juce::Font::plain)
    {
        return juce::Font (juce::Font::getDefaultMonospacedFontName(), size, style);
    }
}
