#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "Theme.h"

// Vertical L/R LED segment meter rail (INPUT or OUTPUT), with peak-hold readout.
// Call pushLevels() ~30Hz with raw per-block peak dB; this owns its own bar/peak
// ballistics (fast attack, slow release; peak holds then decays).
class MeterRail : public juce::Component
{
public:
    explicit MeterRail (juce::String label) : railLabel (std::move (label)) {}

    void pushLevels (float rawLdB, float rawRdB)
    {
        updateChannel (barL, peakL, holdCounterL, rawLdB);
        updateChannel (barR, peakR, holdCounterR, rawRdB);
        repaint();
    }

    void resized() override
    {
        auto area = getLocalBounds();
        labelArea = area.removeFromTop (16);
        area.removeFromTop (4);
        peakArea = area.removeFromBottom (38);
        area.removeFromBottom (4);
        channelLabelArea = area.removeFromBottom (14);
        area.removeFromBottom (4);
        meterArea = area;
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (Theme::textFaint);
        g.setFont (Theme::mono (9.5f));
        g.drawText (railLabel, labelArea, juce::Justification::centred);

        const int gap = 4;
        const int meterW = (meterArea.getWidth() - gap) / 2;
        auto lRect = meterArea.withWidth (meterW);
        auto rRect = meterArea.withX (meterArea.getX() + meterW + gap).withWidth (meterW);

        drawFluidMeter (g, lRect, barL, peakL);
        drawFluidMeter (g, rRect, barR, peakR);

        g.setColour (Theme::textFainter);
        g.setFont (Theme::mono (9.0f));
        g.drawText ("L", lRect.getX(), channelLabelArea.getY(), meterW, channelLabelArea.getHeight(), juce::Justification::centred);
        g.drawText ("R", rRect.getX(), channelLabelArea.getY(), meterW, channelLabelArea.getHeight(), juce::Justification::centred);

        g.setColour (Theme::borderColour);
        g.drawLine ((float) peakArea.getX(), (float) peakArea.getY(),
                    (float) peakArea.getRight(), (float) peakArea.getY(), 1.0f);

        // Use a local copy - peakArea is a member and removeFromTop() mutates in place,
        // which was previously shrinking it to nothing after a couple of repaints.
        auto peakContent = peakArea;
        const float peakDb = juce::jmax (peakL, peakR);
        const bool hot = peakDb > -3.0f;
        g.setColour (hot ? Theme::accentTone : Theme::textSecondary);
        g.setFont (Theme::mono (15.0f, juce::Font::bold));
        juce::String peakText = peakDb <= -99.0f ? juce::String ("--") : juce::String (peakDb, 1);
        g.drawText (peakText, peakContent.removeFromTop (22), juce::Justification::centred);

        g.setColour (Theme::textFaint);
        g.setFont (Theme::mono (8.5f));
        g.drawText ("dBFS PK", peakContent, juce::Justification::centred);
    }

private:
    static void updateChannel (float& bar, float& peak, int& holdCounter, float rawDb)
    {
        // Light damping on both attack and release (a few frames' worth) so the bar
        // reads as a fluent, analog-style follower rather than snapping frame to frame.
        const float coeff = rawDb > bar ? 0.55f : 0.3f;
        bar += (rawDb - bar) * coeff;

        if (rawDb >= peak)
        {
            peak = rawDb;
            holdCounter = holdTicks;
        }
        else if (holdCounter > 0)
        {
            --holdCounter;
        }
        else
        {
            peak = juce::jmax (rawDb, peak - decayPerTick);
        }
    }

    // Continuous fluid bar: gapless gradient fill (green -> amber -> red, anchored to
    // the bar's full range so rising fill reveals more of a fixed gradient rather than
    // jump-cutting between flat colours) plus a thin decaying peak-hold line.
    static void drawFluidMeter (juce::Graphics& g, juce::Rectangle<int> rectI, float barDb, float peakDb)
    {
        auto rect = rectI.toFloat();
        constexpr float radius = 7.0f;
        constexpr float floorDb = -48.0f, ceilDb = 0.0f;

        g.setColour (Theme::segmentUnlit);
        g.fillRoundedRectangle (rect, radius);

        const float norm = juce::jlimit (0.0f, 1.0f, (barDb - floorDb) / (ceilDb - floorDb));
        const float fillH = rect.getHeight() * norm;

        if (fillH > 0.5f)
        {
            juce::Path clip;
            clip.addRoundedRectangle (rect, radius);
            juce::Graphics::ScopedSaveState save (g);
            g.reduceClipRegion (clip);

            juce::ColourGradient grad (Theme::accentDynamics, rect.getX(), rect.getBottom(),
                                        Theme::meterHot, rect.getX(), rect.getY(), false);
            grad.addColour (0.78, Theme::accentTone);
            g.setGradientFill (grad);

            g.fillRect (rect.withY (rect.getBottom() - fillH).withHeight (fillH));
        }

        const float peakNorm = juce::jlimit (0.0f, 1.0f, (peakDb - floorDb) / (ceilDb - floorDb));
        if (peakNorm > 0.005f)
        {
            const float peakY = rect.getBottom() - rect.getHeight() * peakNorm;
            g.setColour (juce::Colour (0xffe8ecef));
            g.fillRect (juce::Rectangle<float> (rect.getX(), peakY - 1.0f, rect.getWidth(), 2.0f));
        }
    }

    juce::String railLabel;
    juce::Rectangle<int> labelArea, meterArea, channelLabelArea, peakArea;

    float barL = -100.0f, barR = -100.0f, peakL = -100.0f, peakR = -100.0f;
    int holdCounterL = 0, holdCounterR = 0;
    static constexpr int holdTicks = 45;       // ~1.5s at 30Hz
    static constexpr float decayPerTick = 0.6f; // ~18dB/sec at 30Hz
};
