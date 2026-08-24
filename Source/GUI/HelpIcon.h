#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "Theme.h"

// A small "?" badge. Click toggles a persistent CallOutBox explanation
// popup (click-to-open only, per design handoff - no hover tooltip needed).
class HelpIcon : public juce::Component
{
public:
    explicit HelpIcon (juce::String helpTextIn) : helpText (std::move (helpTextIn)) {}

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (isMouseOver() ? Theme::accentLoudness : Theme::helpBadgeBg);
        g.fillEllipse (b);
        g.setColour (Theme::helpBadgeText);
        g.setFont (Theme::sans (b.getHeight() * 0.6f, juce::Font::bold));
        g.drawText ("?", getLocalBounds(), juce::Justification::centred);
    }

    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override { repaint(); }

    void mouseUp (const juce::MouseEvent&) override
    {
        auto content = std::make_unique<Content> (helpText);
        juce::CallOutBox::launchAsynchronously (std::move (content), getScreenBounds(), nullptr);
    }

private:
    struct Content : public juce::Component
    {
        explicit Content (const juce::String& text) : message (text)
        {
            juce::AttributedString attr (message);
            attr.setFont (Theme::sans (13.0f));
            attr.setColour (Theme::textPrimary);
            juce::TextLayout layout;
            layout.createLayout (attr, 260.0f);
            setSize (280, (int) layout.getHeight() + 24);
        }

        void paint (juce::Graphics& g) override
        {
            g.fillAll (Theme::bgPanel);
            g.setColour (Theme::textPrimary);
            g.setFont (Theme::sans (13.0f));
            g.drawFittedText (message, getLocalBounds().reduced (12), juce::Justification::topLeft, 20);
        }

        juce::String message;
    };

    juce::String helpText;
};
