#include "ui/SpriteRenderer.h"

#include <array>

#include "LumiSpritesData.h"
#include "core/LumiMath.h"

namespace
{
struct FrameResource
{
    const char* data;
    int size;
};

// Order must match lumi::SpriteFrame.
const std::array<FrameResource, size_t (lumi::SpriteFrame::Count)>& frameResources()
{
    static const std::array<FrameResource, size_t (lumi::SpriteFrame::Count)> table {{
        { LumiSprites::pose01_stand_png,    LumiSprites::pose01_stand_pngSize },
        { LumiSprites::pose02_cheer_png,    LumiSprites::pose02_cheer_pngSize },
        { LumiSprites::pose03_clasp_png,    LumiSprites::pose03_clasp_pngSize },
        { LumiSprites::pose04_excited_png,  LumiSprites::pose04_excited_pngSize },
        { LumiSprites::pose05_stepup_png,   LumiSprites::pose05_stepup_pngSize },
        { LumiSprites::pose06_shy_png,      LumiSprites::pose06_shy_pngSize },
        { LumiSprites::pose07_wink_png,     LumiSprites::pose07_wink_pngSize },
        { LumiSprites::pose08_heart_png,    LumiSprites::pose08_heart_pngSize },
        { LumiSprites::pose09_point_png,    LumiSprites::pose09_point_pngSize },
        { LumiSprites::pose10_kick_png,     LumiSprites::pose10_kick_pngSize },
        { LumiSprites::pose11_paws_png,     LumiSprites::pose11_paws_pngSize },
        { LumiSprites::pose12_winkpose_png, LumiSprites::pose12_winkpose_pngSize },
        { LumiSprites::pose13_lean_png,     LumiSprites::pose13_lean_pngSize },
        { LumiSprites::pose14_jump_png,     LumiSprites::pose14_jump_pngSize },
        { LumiSprites::pose15_cool_png,     LumiSprites::pose15_cool_pngSize },
        { LumiSprites::pose16_sitting_png,  LumiSprites::pose16_sitting_pngSize },
    }};
    return table;
}
} // namespace

const juce::Image& SpriteRenderer::frameImage (lumi::SpriteFrame frame)
{
    static std::array<juce::Image, size_t (lumi::SpriteFrame::Count)> cache;
    static bool loaded = false;
    if (! loaded)
    {
        loaded = true;
        for (size_t i = 0; i < cache.size(); ++i)
        {
            const auto& res = frameResources()[i];
            if (res.data != nullptr && res.size > 0)
                cache[i] = juce::ImageFileFormat::loadFrom (res.data, size_t (res.size));
        }
    }

    const int index = lumi::clamp (int (frame), 0, int (lumi::SpriteFrame::Count) - 1);
    return cache[size_t (index)];
}

void SpriteRenderer::draw (juce::Graphics& g, const lumi::SpriteState& state,
                           juce::Rectangle<float> bounds, bool userMirror)
{
    const juce::Image& image = frameImage (state.frame);
    if (! image.isValid() || bounds.isEmpty())
        return;

    // Height budget mirrors the vector renderer's framing contract: room for
    // the maximum root lift so no dance can push her out of frame.
    const float targetH = juce::jmin (bounds.getHeight() / 1.42f,
                                      bounds.getWidth() * 1.10f);
    const float baseScale = targetH / float (image.getHeight());

    // Cartoon squash: wider when landing, taller when stretching. Approx
    // volume-preserving.
    const float squash = lumi::clamp (state.squash, 0.85f, 1.15f);
    const float scaleX = baseScale * squash;
    const float scaleY = baseScale * (2.0f - squash);

    const bool flip = state.mirrored != userMirror;

    // Feet anchor: bottom-centre of the stage plus the pose's root offset.
    const float anchorX = bounds.getCentreX() + state.offsetX * targetH
                              * (userMirror ? -1.0f : 1.0f);
    const float anchorY = bounds.getBottom() - targetH * 0.02f
                          + state.offsetY * targetH;

    juce::AffineTransform t =
        juce::AffineTransform::translation (-float (image.getWidth()) * 0.5f,
                                            -float (image.getHeight()))
            .scaled (flip ? -scaleX : scaleX, scaleY)
            .rotated (state.rotation)
            .translated (anchorX, anchorY);

    // Soft ground shadow under the feet.
    g.setColour (juce::Colour (0xff17121f).withAlpha (0.28f));
    g.fillEllipse (anchorX - targetH * 0.17f, bounds.getBottom() - targetH * 0.045f,
                   targetH * 0.34f, targetH * 0.045f);

    // drawImageTransformed modulates by the current opacity — reset it after
    // the low-alpha shadow, or Lumi fades to a ghost.
    g.setOpacity (1.0f);
    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
    g.drawImageTransformed (image, t);
}
