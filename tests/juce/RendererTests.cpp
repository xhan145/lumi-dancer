// LUMI//DANCER — renderer smoke tests: Lumi actually appears on the canvas
// with her identity palette, stays inside bounds, and every dance style +
// customisation renders without throwing.
//
// Set LUMI_SNAPSHOT_DIR to write PNG snapshots for human inspection.
#include <juce_gui_extra/juce_gui_extra.h>

#include "JuceTestFramework.h"
#include "dance/DanceAnimation.h"
#include "ui/LumiRenderer.h"

namespace
{
juce::Image renderPose (const lumi::CharacterPose& pose, const lumi::RenderLook& look,
                        int width = 360, int height = 480)
{
    juce::Image image (juce::Image::ARGB, width, height, true);
    juce::Graphics g (image);
    lumi::LumiRenderer::draw (g, pose, look, { 0.0f, 0.0f, float (width), float (height) });
    return image;
}

// Count pixels within a loose distance of a reference colour.
int countNear (const juce::Image& image, juce::Colour reference, int tolerance = 40)
{
    int count = 0;
    for (int y = 0; y < image.getHeight(); y += 2)
        for (int x = 0; x < image.getWidth(); x += 2)
        {
            const juce::Colour c = image.getPixelAt (x, y);
            if (c.getAlpha() < 200)
                continue;
            if (std::abs (int (c.getRed()) - int (reference.getRed())) < tolerance
                && std::abs (int (c.getGreen()) - int (reference.getGreen())) < tolerance
                && std::abs (int (c.getBlue()) - int (reference.getBlue())) < tolerance)
                ++count;
        }
    return count;
}

void maybeSaveSnapshot (const juce::Image& image, const char* name)
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

JT_TEST (renderer_lumi_shows_identity_palette)
{
    lumi::CharacterPose pose;
    lumi::sanitizePose (pose);
    lumi::RenderLook look;

    const juce::Image image = renderPose (pose, look);
    maybeSaveSnapshot (image, "lumi-neutral");

    // Lavender hair, cream face/outfit, gold accents must all be present.
    JT_GT (countNear (image, juce::Colour (0xffb9a3ff)), 40);    // lavender
    JT_GT (countNear (image, juce::Colour (0xfffff7e6)), 40);    // cream
    JT_GT (countNear (image, juce::Colour (0xffe4b84c), 55), 3); // gold
    // And a meaningful amount of the canvas is actually drawn on.
    int drawn = 0;
    for (int y = 0; y < image.getHeight(); y += 2)
        for (int x = 0; x < image.getWidth(); x += 2)
            if (image.getPixelAt (x, y).getAlpha() > 100)
                ++drawn;
    JT_GT (drawn, 2000);
}

JT_TEST (renderer_every_style_pose_stays_inside_bounds)
{
    lumi::AudioReactiveFrame audio;
    audio.rms = 0.6f;
    audio.lowEnergy = 0.9f;
    audio.lowTransient = 1.0f;
    audio.silence = false;

    for (int styleIndex = 0; styleIndex < int (lumi::DanceStyle::Count); ++styleIndex)
    {
        auto style = lumi::createDanceStyle (lumi::DanceStyle (styleIndex), 7);
        bool clean = true;
        for (int step = 0; step < 16; ++step)
        {
            const auto pose = style->evaluate (double (step) / 4.0, 140.0, audio, 2.0f);
            const juce::Image image = renderPose (pose, {}, 320, 420);

            // Nothing may hit the outermost pixel ring (character escaping).
            for (int x = 0; x < image.getWidth() && clean; ++x)
                if (image.getPixelAt (x, 0).getAlpha() > 30)
                    clean = false;
        }
        _ctx.report (clean, (std::string ("style stays in frame: ")
                             + lumi::danceStyleName (lumi::DanceStyle (styleIndex))).c_str(),
                     __FILE__, __LINE__);
    }
}

JT_TEST (renderer_customisation_changes_pixels)
{
    lumi::CharacterPose pose;
    lumi::sanitizePose (pose);

    lumi::RenderLook lavender;
    lumi::RenderLook plum;
    plum.hair = lumi::HairPalette::DeepPlum;
    plum.outfit = lumi::Outfit::ProducerOutfit;

    const juce::Image a = renderPose (pose, lavender);
    const juce::Image b = renderPose (pose, plum);
    maybeSaveSnapshot (b, "lumi-plum-producer");

    int different = 0;
    for (int y = 0; y < a.getHeight(); y += 2)
        for (int x = 0; x < a.getWidth(); x += 2)
            if (a.getPixelAt (x, y) != b.getPixelAt (x, y))
                ++different;
    JT_GT (different, 500);

    // Accessories toggle visibly.
    lumi::RenderLook bare = lavender;
    bare.accessories = 0;
    const juce::Image c = renderPose (pose, bare);
    int accessoryPixels = 0;
    for (int y = 0; y < a.getHeight(); y += 2)
        for (int x = 0; x < a.getWidth(); x += 2)
            if (a.getPixelAt (x, y) != c.getPixelAt (x, y))
                ++accessoryPixels;
    JT_GT (accessoryPixels, 60);
}

JT_TEST (renderer_expressions_and_mirror_render)
{
    lumi::CharacterPose pose;
    pose.eyeOpenAmount = 0.0f;          // closed-eye arcs
    pose.mouthOpenAmount = 0.8f;        // singing
    pose.starEyeAmount = 1.0f;
    pose.blushAmount = 1.0f;
    pose.hairBounceAmount = 0.8f;
    lumi::sanitizePose (pose);

    lumi::RenderLook look;
    look.accessories = lumi::accStarClip | lumi::accHeadphones | lumi::accCrescentPin
                     | lumi::accOrbitBelt | lumi::accCompanionStar;
    const juce::Image expressive = renderPose (pose, look);
    maybeSaveSnapshot (expressive, "lumi-expressive");

    look.mirror = true;
    const juce::Image mirrored = renderPose (pose, look);

    int different = 0;
    for (int y = 0; y < expressive.getHeight(); y += 2)
        for (int x = 0; x < expressive.getWidth(); x += 2)
            if (expressive.getPixelAt (x, y) != mirrored.getPixelAt (x, y))
                ++different;
    JT_GT (different, 200);
}

JT_TEST (renderer_dancing_snapshot_for_inspection)
{
    // A representative dancing frame (Kawaii Pop mid-gesture) for the
    // human-inspection snapshot set.
    lumi::AudioReactiveFrame audio;
    audio.rms = 0.45f;
    audio.lowEnergy = 0.8f;
    audio.midEnergy = 0.5f;
    audio.highEnergy = 0.6f;
    audio.silence = false;

    auto style = lumi::createDanceStyle (lumi::DanceStyle::KawaiiPop, 7);
    const auto pose = style->evaluate (6.6, 128.0, audio, 1.2f);
    const juce::Image image = renderPose (pose, {}, 420, 560);
    maybeSaveSnapshot (image, "lumi-kawaii-dance");
    JT_CHECK (image.isValid());
}
