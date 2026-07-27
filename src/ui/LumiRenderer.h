// LUMI//DANCER — the procedural Lumi character renderer.
//
// Draws the whole mascot from layered vector paths driven by a CharacterPose.
// No image assets: Lumi is code. The renderer is a pure function of
// (pose, look, bounds) so the plugin editor and the overlay share it.
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "core/Palette.h"
#include "rig/CharacterPose.h"

namespace lumi
{
struct RenderLook
{
    HairPalette hair = HairPalette::Lavender;
    Outfit outfit = Outfit::StarHoodie;
    GoldAccent accent = GoldAccent::SoftGold;
    uint32_t accessories = accStarClip | accOrbitBelt | accCompanionStar;
    bool mirror = false;
    bool highContrast = false;
    float eyeLookX = 0.0f;          // -1..1 pupil wander
    double timeSeconds = 0.0;       // companion-star bobbing
};

class LumiRenderer
{
public:
    // Draws Lumi centred in `bounds`; the character height is
    // bounds.height * 0.82 before pose root offsets.
    static void draw (juce::Graphics& g, const CharacterPose& pose,
                      const RenderLook& look, juce::Rectangle<float> bounds);
};
} // namespace lumi
