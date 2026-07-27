// Painted-sprite resource + renderer tests: every embedded frame decodes
// with sane dimensions and real alpha, and the blitter puts pixels on the
// canvas inside bounds.
#include <juce_gui_extra/juce_gui_extra.h>

#include "JuceTestFramework.h"
#include "dance/SpriteChoreo.h"
#include "ui/SpriteRenderer.h"

namespace
{
void maybeSave (const juce::Image& image, const char* name)
{
    const auto dir = juce::SystemStats::getEnvironmentVariable ("LUMI_SNAPSHOT_DIR", {});
    if (dir.isEmpty())
        return;
    const juce::File file = juce::File (dir).getChildFile (juce::String (name) + ".png");
    file.getParentDirectory().createDirectory();
    juce::FileOutputStream stream (file);
    if (stream.openedOk())
    {
        stream.setPosition (0);
        stream.truncate();
        juce::PNGImageFormat png;
        png.writeImageToStream (image, stream);
    }
}
} // namespace

JT_TEST (sprites_all_frames_decode_with_alpha)
{
    for (int i = 0; i < int (lumi::SpriteFrame::Count); ++i)
    {
        const juce::Image& image = SpriteRenderer::frameImage (lumi::SpriteFrame (i));
        _ctx.report (image.isValid(),
                     (std::string ("decodes: ") + lumi::spriteFrameName (lumi::SpriteFrame (i))).c_str(),
                     __FILE__, __LINE__);
        if (! image.isValid())
            continue;

        JT_CHECK (image.getWidth() > 80 && image.getHeight() > 200);
        JT_CHECK (image.hasAlphaChannel());

        // Real cut-out: some fully transparent and some opaque pixels.
        int transparent = 0, opaque = 0;
        for (int y = 0; y < image.getHeight(); y += 4)
            for (int x = 0; x < image.getWidth(); x += 4)
            {
                const auto a = image.getPixelAt (x, y).getAlpha();
                if (a < 10) ++transparent;
                else if (a > 240) ++opaque;
            }
        JT_CHECK (transparent > 50);
        JT_CHECK (opaque > 200);
    }
}

JT_TEST (sprites_renderer_draws_inside_bounds)
{
    lumi::SpriteState state;
    state.frame = lumi::SpriteFrame::Cheer;
    state.squash = 1.05f;
    state.rotation = 0.1f;

    juce::Image canvas (juce::Image::ARGB, 360, 480, true);
    {
        juce::Graphics g (canvas);
        SpriteRenderer::draw (g, state, { 0.0f, 0.0f, 360.0f, 480.0f }, false);
    }
    maybeSave (canvas, "lumi-painted-cheer");

    int drawn = 0;
    bool edgeClean = true;
    for (int y = 0; y < canvas.getHeight(); y += 2)
        for (int x = 0; x < canvas.getWidth(); x += 2)
            if (canvas.getPixelAt (x, y).getAlpha() > 30)
                ++drawn;
    for (int x = 0; x < canvas.getWidth(); ++x)
        if (canvas.getPixelAt (x, 0).getAlpha() > 30)
            edgeClean = false;
    JT_GT (drawn, 3000);
    JT_CHECK (edgeClean);

    // Mirrored draw changes pixels.
    juce::Image mirroredCanvas (juce::Image::ARGB, 360, 480, true);
    {
        juce::Graphics g (mirroredCanvas);
        state.mirrored = true;
        SpriteRenderer::draw (g, state, { 0.0f, 0.0f, 360.0f, 480.0f }, false);
    }
    int different = 0;
    for (int y = 0; y < canvas.getHeight(); y += 2)
        for (int x = 0; x < canvas.getWidth(); x += 2)
            if (canvas.getPixelAt (x, y) != mirroredCanvas.getPixelAt (x, y))
                ++different;
    JT_GT (different, 500);
}

JT_TEST (sprites_painted_sitting_snapshot)
{
    lumi::SpriteState state;
    state.frame = lumi::SpriteFrame::Sitting;
    juce::Image canvas (juce::Image::ARGB, 360, 480, true);
    {
        juce::Graphics g (canvas);
        SpriteRenderer::draw (g, state, { 0.0f, 0.0f, 360.0f, 480.0f }, false);
    }
    maybeSave (canvas, "lumi-painted-sitting");
    JT_CHECK (canvas.isValid());
}
