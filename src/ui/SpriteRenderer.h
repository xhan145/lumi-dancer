// LUMI//DANCER — painted-sprite blitter for the anime art style.
//
// Draws one embedded hand-painted frame with bounce/tilt/squash transforms
// derived from the rig pose. Frame *selection* lives in the JUCE-free core
// (dance/SpriteChoreo); this class only decodes and draws.
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "dance/SpriteChoreo.h"

class SpriteRenderer
{
public:
    // Draws the frame anchored feet-down at the bottom-centre of `bounds`.
    static void draw (juce::Graphics& g, const lumi::SpriteState& state,
                      juce::Rectangle<float> bounds, bool userMirror);

    // The decoded image for a frame (shared cache; empty image if missing).
    static const juce::Image& frameImage (lumi::SpriteFrame frame);
};
