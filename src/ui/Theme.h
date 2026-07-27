// LUMI//DANCER — UI theme: palette bridging (core Rgba → juce::Colour) and
// the product look-and-feel (rounded lavender controls on deep plum).
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/Palette.h"

namespace lumi::theme
{
inline juce::Colour toColour (Rgba c) { return juce::Colour (c.r, c.g, c.b); }

inline juce::Colour lavender()   { return toColour (palette::lavender); }
inline juce::Colour softPurple() { return toColour (palette::softPurple); }
inline juce::Colour deepPlum()   { return toColour (palette::deepPlum); }
inline juce::Colour gold()       { return toColour (palette::gold); }
inline juce::Colour softGold()   { return toColour (palette::softGold); }
inline juce::Colour cream()      { return toColour (palette::cream); }
inline juce::Colour rose()       { return toColour (palette::roseAccent); }
inline juce::Colour shadow()     { return toColour (palette::shadow); }
inline juce::Colour panel()      { return toColour (palette::plumPanel); }
inline juce::Colour panelDeep()  { return toColour (palette::plumDeep); }
inline juce::Colour outline()    { return toColour (palette::outline); }

class LumiLookAndFeel : public juce::LookAndFeel_V4
{
public:
    explicit LumiLookAndFeel (bool highContrastIn = false)
        : highContrast (highContrastIn)
    {
        const auto text = highContrast ? juce::Colours::white : cream();
        setColour (juce::ResizableWindow::backgroundColourId, panelDeep());
        setColour (juce::Label::textColourId, text);
        setColour (juce::Slider::textBoxTextColourId, text);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::thumbColourId, gold());
        setColour (juce::Slider::trackColourId, softPurple());
        setColour (juce::Slider::backgroundColourId, outline());
        setColour (juce::Slider::rotarySliderFillColourId, lavender());
        setColour (juce::Slider::rotarySliderOutlineColourId, outline());
        setColour (juce::ComboBox::backgroundColourId, panel());
        setColour (juce::ComboBox::textColourId, text);
        setColour (juce::ComboBox::outlineColourId, outline());
        setColour (juce::ComboBox::arrowColourId, gold());
        setColour (juce::PopupMenu::backgroundColourId, panel());
        setColour (juce::PopupMenu::textColourId, text);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, softPurple());
        setColour (juce::PopupMenu::highlightedTextColourId, cream());
        setColour (juce::TextButton::buttonColourId, panel());
        setColour (juce::TextButton::buttonOnColourId, softPurple());
        setColour (juce::TextButton::textColourOffId, text);
        setColour (juce::TextButton::textColourOnId, cream());
        setColour (juce::ToggleButton::textColourId, text);
        setColour (juce::ToggleButton::tickColourId, gold());
        setColour (juce::ToggleButton::tickDisabledColourId, outline());
        setColour (juce::TooltipWindow::backgroundColourId, panel());
        setColour (juce::TooltipWindow::textColourId, text);
        setColour (juce::TooltipWindow::outlineColourId, softPurple());
        setColour (juce::GroupComponent::textColourId, softGold());
        setColour (juce::GroupComponent::outlineColourId, outline());
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float startAngle, float endAngle,
                           juce::Slider& slider) override
    {
        const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (4.0f);
        const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const auto centre = bounds.getCentre();
        const float angle = startAngle + sliderPos * (endAngle - startAngle);
        const float thickness = juce::jmax (2.5f, radius * 0.16f);

        juce::Path track;
        track.addCentredArc (centre.x, centre.y, radius - thickness, radius - thickness,
                             0.0f, startAngle, endAngle, true);
        g.setColour (findColour (juce::Slider::rotarySliderOutlineColourId));
        g.strokePath (track, juce::PathStrokeType (thickness, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));

        juce::Path value;
        value.addCentredArc (centre.x, centre.y, radius - thickness, radius - thickness,
                             0.0f, startAngle, angle, true);
        g.setColour (slider.isEnabled() ? findColour (juce::Slider::rotarySliderFillColourId)
                                        : outline());
        g.strokePath (value, juce::PathStrokeType (thickness, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));

        // Gold star thumb.
        const float starRadius = thickness * 0.9f;
        const auto tip = centre.getPointOnCircumference (radius - thickness, angle);
        juce::Path star;
        star.addStar (tip, 5, starRadius * 0.45f, starRadius, 0.0f);
        g.setColour (gold());
        g.fillPath (star);
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override
    {
        return withDefaultMetrics (juce::FontOptions (13.0f));
    }

    juce::Font getLabelFont (juce::Label& label) override
    {
        return withDefaultMetrics (juce::FontOptions (
            juce::jmax (11.0f, label.getFont().getHeight())));
    }

private:
    bool highContrast;
};
} // namespace lumi::theme
