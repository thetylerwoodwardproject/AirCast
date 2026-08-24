#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Theme.h"
#include "../Parameters.h"

// Horizontal format/preset picker: active preset as a filled pill, the rest
// as plain text, overflowing into a "+N more" popup menu when they don't
// all fit in one row. Reads/writes the codecMode choice parameter directly.
class PresetRail : public juce::Component
{
public:
    explicit PresetRail (juce::AudioProcessorValueTreeState& state) : apvts (state)
    {
        choiceParam = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (Param::codecMode));
        names = Param::codecModeChoices();
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (Theme::bgFormatRow);
        g.fillRoundedRectangle (getLocalBounds().toFloat(), 6.0f);
        g.setColour (Theme::borderColour);
        g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), 6.0f, 1.0f);

        auto area = getLocalBounds().reduced (10, 6);
        auto labelArea = area.removeFromLeft (58);
        g.setColour (Theme::textFaint);
        g.setFont (Theme::mono (10.0f));
        g.drawText ("FORMAT", labelArea, juce::Justification::centredLeft);
        area.removeFromLeft (7);

        itemRects.clear();
        const int activeIdx = choiceParam != nullptr ? choiceParam->getIndex() : 0;

        const juce::Font pillFont = Theme::sans (13.0f, juce::Font::bold);
        const juce::Font textFont = Theme::sans (12.5f);
        const juce::Font moreFont = textFont;

        juce::Array<int> visible;
        int x = area.getX();
        for (int idx = 0; idx < names.size(); ++idx)
        {
            const bool isActive = idx == activeIdx;
            const juce::Font& f = isActive ? pillFont : textFont;
            const int itemW = f.getStringWidth (names[idx]) + (isActive ? 16 : 8);
            const int remainingAfter = names.size() - idx - 1;
            const int moreW = remainingAfter > 0
                ? moreFont.getStringWidth ("+" + juce::String (remainingAfter) + " more") + 14 : 0;

            if (idx > 0 && x + itemW + (remainingAfter > 0 ? moreW + 7 : 0) > area.getRight())
                break;

            visible.add (idx);
            x += itemW + 7;
        }

        x = area.getX();
        for (int idx : visible)
        {
            const bool isActive = idx == activeIdx;
            const juce::Font& f = isActive ? pillFont : textFont;
            const int itemW = f.getStringWidth (names[idx]) + (isActive ? 16 : 8);
            juce::Rectangle<int> r (x, area.getY(), itemW, area.getHeight());

            if (isActive)
            {
                g.setColour (Theme::accentTone);
                g.fillRoundedRectangle (r.toFloat(), 4.0f);
                g.setColour (Theme::bgPanel);
            }
            else
            {
                g.setColour (Theme::textFainter);
            }
            g.setFont (f);
            g.drawText (names[idx], r, juce::Justification::centred);
            itemRects.add ({ r, idx });
            x += itemW + 7;
        }

        const int remaining = names.size() - visible.size();
        hasMore = remaining > 0;
        if (hasMore)
        {
            const juce::String moreText = "+" + juce::String (remaining) + " more";
            const int moreW = moreFont.getStringWidth (moreText) + 4;
            moreRect = { x, area.getY(), moreW, area.getHeight() };
            g.setColour (Theme::textFainter);
            g.setFont (moreFont);
            g.drawText (moreText, moreRect, juce::Justification::centredLeft);
        }
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        for (auto& item : itemRects)
            if (item.bounds.contains (e.getPosition())) { setPreset (item.index); return; }

        if (hasMore && moreRect.contains (e.getPosition()))
        {
            const int activeIdx = choiceParam != nullptr ? choiceParam->getIndex() : 0;
            juce::PopupMenu menu;
            for (int i = 0; i < names.size(); ++i)
                menu.addItem (i + 1, names[i], true, i == activeIdx);
            menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                [this] (int result) { if (result > 0) setPreset (result - 1); });
        }
    }

private:
    void setPreset (int idx)
    {
        if (choiceParam == nullptr)
            return;

        *choiceParam = idx; // updates codec + UI pill

        const auto v = Param::getPresetValues (idx);
        applyParam (Param::density,           v.density);
        applyParam (Param::detail,            v.detail);
        applyParam (Param::drive,             v.drive);
        applyParam (Param::highs,             v.highs);
        applyParam (Param::lows,              v.lows);
        applyParam (Param::bassCharacter,     v.bassCharacter);
        applyParam (Param::monoBass,          v.monoBass ? 1.0f : 0.0f);
        applyParam (Param::expanderThreshold, v.expanderThreshold);
        applyParam (Param::expanderRatio,     v.expanderRatio);
        applyParam (Param::gateThreshold,     v.gateThreshold);
        applyParam (Param::gateToZero,        v.gateToZero ? 1.0f : 0.0f);
        applyParam (Param::lufsTarget,        v.lufsTarget);
        applyParam (Param::lufsDrive,         v.lufsDrive);
        applyParam (Param::aesLoudness,       v.aesLoudness ? 1.0f : 0.0f);
        applyParam (Param::longWindow,        v.longWindow ? 1.0f : 0.0f);
        applyParam (Param::truePeak,          v.truePeak);
        applyParam (Param::masterOut,         v.masterOut);

        repaint();
    }

    // Mirrors PluginEditor's Reset button (setValueNotifyingHost + convertTo0to1), but for
    // an arbitrary target value instead of the parameter's default.
    void applyParam (const juce::String& paramID, float actualValue)
    {
        if (auto* p = apvts.getParameter (paramID))
            p->setValueNotifyingHost (p->convertTo0to1 (actualValue));
    }

    struct Item { juce::Rectangle<int> bounds; int index; };

    juce::AudioProcessorValueTreeState& apvts;
    juce::AudioParameterChoice* choiceParam = nullptr;
    juce::StringArray names;
    juce::Array<Item> itemRects;
    juce::Rectangle<int> moreRect;
    bool hasMore = false;
};
