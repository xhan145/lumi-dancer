#include "ui/LumiRenderer.h"

#include <cmath>

#include "core/LumiMath.h"
#include "ui/Theme.h"

namespace lumi
{
namespace
{
using juce::AffineTransform;
using juce::Colour;
using juce::Graphics;
using juce::Path;
using juce::Point;

struct Metrics
{
    Point<float> origin;   // hip anchor in pixels
    float H = 100.0f;      // character height in pixels
    float outlineWidth = 1.5f;
};

Colour col (Rgba c) { return Colour (c.r, c.g, c.b); }

// The unified-silhouette technique: stroke the whole multi-subpath region
// FIRST (underneath), then fill on top. Any number of subshapes welds into
// one clean outlined silhouette — two draw ops per region, no interior seams.
void outlinedFill (Graphics& g, const Path& path, Colour fill, Colour outline,
                   float outlineWidth)
{
    g.setColour (outline);
    g.strokePath (path, juce::PathStrokeType (outlineWidth * 2.0f,
                                              juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));
    g.setColour (fill);
    g.fillPath (path);
}

// Capsule subpath along `angle` (0 = straight down) added into an existing
// path; returns the segment end point for chaining.
Point<float> addCapsule (Path& path, Point<float> start, float angle,
                         float length, float thickness)
{
    const Point<float> end (start.x + std::sin (angle) * length,
                            start.y + std::cos (angle) * length);
    Path capsule;
    capsule.addRoundedRectangle (-thickness * 0.5f, -thickness * 0.35f,
                                 thickness, length + thickness * 0.7f,
                                 thickness * 0.5f);
    capsule.applyTransform (AffineTransform::rotation (-angle).translated (start.x, start.y));
    path.addPath (capsule);
    return end;
}

void addStar (Path& path, Point<float> centre, float radius, float rotation = 0.0f)
{
    path.addStar (centre, 5, radius * 0.45f, radius, rotation);
}

void fillStar (Graphics& g, Point<float> centre, float radius, Colour fill,
               float rotation = 0.0f)
{
    Path star;
    addStar (star, centre, radius, rotation);
    g.setColour (fill);
    g.fillPath (star);
}

// ------------------------------------------------------------------- pieces
void drawCompanionStar (Graphics& g, const Metrics& m, const CharacterPose& pose,
                        const RenderLook& look)
{
    const auto accent = accentColours[size_t (look.accent)];
    const float bob = std::sin (float (look.timeSeconds) * 2.1f) * m.H * 0.02f;
    const float drift = std::sin (float (look.timeSeconds) * 0.7f) * m.H * 0.03f;
    const Point<float> pos (m.origin.x + m.H * 0.34f + drift,
                            m.origin.y - m.H * 0.72f + bob
                                + pose.hairBounceAmount * m.H * 0.02f);
    g.setColour (col (accent.soft).withAlpha (0.35f));
    g.fillEllipse (pos.x - m.H * 0.055f, pos.y - m.H * 0.055f, m.H * 0.11f, m.H * 0.11f);
    fillStar (g, pos, m.H * 0.038f, col (accent.main),
              std::sin (float (look.timeSeconds) * 1.3f) * 0.35f);
}

void drawLegs (Graphics& g, const Metrics& m, const CharacterPose& pose,
               const RenderLook& look, Colour outlineColour)
{
    const auto outfit = outfitColours[size_t (look.outfit)];
    const auto accent = accentColours[size_t (look.accent)];
    const Colour skin = col (palette::cream);
    const float upperLen = m.H * 0.115f;
    const float lowerLen = m.H * 0.105f;
    const float thick = m.H * 0.052f;

    const auto leg = [&] (Point<float> hip, const CharacterBone& upper,
                          const CharacterBone& lower)
    {
        hip.x += upper.position.x * m.H;
        hip.y += upper.position.y * m.H;

        // Whole leg = one two-capsule chain, welded by outline-under-fill.
        Path legPath;
        const Point<float> knee = addCapsule (legPath, hip, upper.rotationRadians,
                                              upperLen, thick);
        const float lowerAngle = upper.rotationRadians + lower.rotationRadians;
        const Point<float> ankle = addCapsule (legPath, knee, lowerAngle,
                                               lowerLen, thick * 0.92f);
        outlinedFill (g, legPath, skin, outlineColour, m.outlineWidth * 0.5f);

        // Boot: one rounded shape with a single gold trim stripe.
        const float bootW = thick * 1.5f;
        const float bootH = thick * 1.15f;
        Path boot;
        boot.addRoundedRectangle (ankle.x - bootW * 0.5f, ankle.y - bootH * 0.15f,
                                  bootW, bootH, bootH * 0.4f);
        outlinedFill (g, boot, col (outfit.boots), outlineColour, m.outlineWidth * 0.5f);
        g.setColour (col (accent.main));
        g.fillRoundedRectangle (ankle.x - bootW * 0.5f, ankle.y - bootH * 0.15f,
                                bootW, bootH * 0.32f, bootH * 0.18f);
    };

    leg ({ m.origin.x - m.H * 0.055f, m.origin.y + m.H * 0.02f },
         pose.leftUpperLeg, pose.leftLowerLeg);
    leg ({ m.origin.x + m.H * 0.055f, m.origin.y + m.H * 0.02f },
         pose.rightUpperLeg, pose.rightLowerLeg);
}

void drawBody (Graphics& g, const Metrics& m, const CharacterPose& pose,
               const RenderLook& look, Colour outlineColour)
{
    const auto outfit = outfitColours[size_t (look.outfit)];
    const auto accent = accentColours[size_t (look.accent)];

    Graphics::ScopedSaveState save (g);
    const Point<float> waist (m.origin.x + pose.torso.position.x * m.H,
                              m.origin.y + pose.torso.position.y * m.H);
    g.addTransform (AffineTransform::rotation (pose.torso.rotationRadians,
                                               waist.x, waist.y));

    const float torsoW = m.H * 0.20f * pose.torso.scale;
    const float torsoH = m.H * 0.24f * pose.torso.scale;
    const float skirtW = m.H * 0.27f;
    const float skirtH = m.H * 0.10f;

    // One dress silhouette: rounded shoulders flowing into a flared hem.
    Path dress;
    dress.startNewSubPath (waist.x - torsoW * 0.42f, waist.y - torsoH * 0.92f);
    dress.quadraticTo (waist.x, waist.y - torsoH * 1.12f,
                       waist.x + torsoW * 0.42f, waist.y - torsoH * 0.92f);
    dress.quadraticTo (waist.x + torsoW * 0.55f, waist.y - torsoH * 0.35f,
                       waist.x + torsoW * 0.48f, waist.y - skirtH * 0.55f);
    dress.quadraticTo (waist.x + skirtW * 0.62f, waist.y + skirtH * 0.85f,
                       waist.x + skirtW * 0.42f, waist.y + skirtH * 1.05f);
    dress.quadraticTo (waist.x, waist.y + skirtH * 1.30f,
                       waist.x - skirtW * 0.42f, waist.y + skirtH * 1.05f);
    dress.quadraticTo (waist.x - skirtW * 0.62f, waist.y + skirtH * 0.85f,
                       waist.x - torsoW * 0.48f, waist.y - skirtH * 0.55f);
    dress.quadraticTo (waist.x - torsoW * 0.55f, waist.y - torsoH * 0.35f,
                       waist.x - torsoW * 0.42f, waist.y - torsoH * 0.92f);
    dress.closeSubPath();
    outlinedFill (g, dress, col (outfit.jacket), outlineColour, m.outlineWidth * 0.5f);

    // Cream bib panel — the only inner colour block.
    Path bib;
    bib.startNewSubPath (waist.x - torsoW * 0.22f, waist.y - torsoH * 0.98f);
    bib.quadraticTo (waist.x, waist.y - torsoH * 1.06f,
                     waist.x + torsoW * 0.22f, waist.y - torsoH * 0.98f);
    bib.quadraticTo (waist.x + torsoW * 0.26f, waist.y - torsoH * 0.30f,
                     waist.x + torsoW * 0.18f, waist.y - torsoH * 0.06f);
    bib.quadraticTo (waist.x, waist.y + torsoH * 0.04f,
                     waist.x - torsoW * 0.18f, waist.y - torsoH * 0.06f);
    bib.quadraticTo (waist.x - torsoW * 0.26f, waist.y - torsoH * 0.30f,
                     waist.x - torsoW * 0.22f, waist.y - torsoH * 0.98f);
    bib.closeSubPath();
    g.setColour (col (outfit.top));
    g.fillPath (bib);

    // Tiny gold star on the chest.
    fillStar (g, { waist.x, waist.y - torsoH * 0.58f }, torsoW * 0.10f,
              col (accent.main));

    // Orbit-ring belt.
    if ((look.accessories & accOrbitBelt) != 0)
    {
        Path ring;
        ring.addEllipse (waist.x - skirtW * 0.52f, waist.y - skirtH * 0.42f,
                         skirtW * 1.04f, skirtH * 0.8f);
        g.setColour (col (accent.main).withAlpha (0.9f));
        g.strokePath (ring, juce::PathStrokeType (m.H * 0.008f));
        fillStar (g, { waist.x + skirtW * 0.5f, waist.y - skirtH * 0.02f },
                  m.H * 0.016f, col (accent.soft));
    }
}

void drawArms (Graphics& g, const Metrics& m, const CharacterPose& pose,
               const RenderLook& look, Colour outlineColour)
{
    const auto outfit = outfitColours[size_t (look.outfit)];
    const float upperLen = m.H * 0.115f;
    const float lowerLen = m.H * 0.10f;
    const float thick = m.H * 0.050f;

    const float shoulderY = m.origin.y - m.H * 0.215f;

    const auto arm = [&] (Point<float> shoulder, const CharacterBone& upper,
                          const CharacterBone& lower)
    {
        shoulder.x += upper.position.x * m.H;
        shoulder.y += upper.position.y * m.H;

        // Paw-sleeve: the whole arm is one welded chain with a slightly
        // wider rounded end — reads as a cosy sleeve with a hidden hand.
        Path armPath;
        const Point<float> elbow = addCapsule (armPath, shoulder,
                                               upper.rotationRadians,
                                               upperLen, thick * 1.1f);
        addCapsule (armPath, elbow, upper.rotationRadians + lower.rotationRadians,
                    lowerLen, thick * 1.25f);
        outlinedFill (g, armPath, col (outfit.jacket), outlineColour,
                      m.outlineWidth * 0.5f);
    };

    arm ({ m.origin.x - m.H * 0.125f + pose.torso.position.x * m.H,
           shoulderY + pose.torso.position.y * m.H },
         pose.leftUpperArm, pose.leftLowerArm);
    arm ({ m.origin.x + m.H * 0.125f + pose.torso.position.x * m.H,
           shoulderY + pose.torso.position.y * m.H },
         pose.rightUpperArm, pose.rightLowerArm);
}

void drawFace (Graphics& g, const Metrics& m, const CharacterPose& pose,
               const RenderLook& look, juce::Rectangle<float> face)
{
    const Colour iris = col (palette::softPurple).darker (0.1f);
    const Colour irisDeep = col (palette::deepPlum);

    const float eyeW = face.getWidth() * 0.185f;
    const float eyeHFull = face.getHeight() * 0.30f;
    const float eyeH = juce::jmax (eyeHFull * pose.eyeOpenAmount, face.getHeight() * 0.02f);
    const float eyeY = face.getCentreY() + face.getHeight() * 0.03f;
    const float eyeDx = face.getWidth() * 0.21f;
    const float lookShift = look.eyeLookX * eyeW * 0.25f;

    for (int side = -1; side <= 1; side += 2)
    {
        const float cx = face.getCentreX() + eyeDx * float (side) + lookShift;
        juce::Rectangle<float> eye (cx - eyeW * 0.5f, eyeY - eyeH * 0.5f, eyeW, eyeH);

        if (pose.eyeOpenAmount < 0.12f)
        {
            // Closed: happy arc.
            Path lid;
            lid.startNewSubPath (eye.getX(), eyeY);
            lid.quadraticTo (cx, eyeY + eyeHFull * 0.28f, eye.getRight(), eyeY);
            g.setColour (irisDeep);
            g.strokePath (lid, juce::PathStrokeType (m.H * 0.012f,
                                                     juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));
            continue;
        }

        // One outlined iris + one sparkle (or a star in star-eye mode).
        g.setColour (irisDeep);
        g.fillEllipse (eye.expanded (m.H * 0.004f));
        g.setColour (iris);
        g.fillEllipse (eye);

        if (pose.starEyeAmount > 0.35f)
        {
            fillStar (g, { cx, eyeY }, eyeW * 0.34f,
                      col (palette::softGold).withAlpha (pose.starEyeAmount));
        }
        else
        {
            g.setColour (juce::Colours::white.withAlpha (0.95f));
            g.fillEllipse (cx - eyeW * 0.16f, eyeY - eyeH * 0.26f,
                           eyeW * 0.24f, eyeH * 0.28f);
        }

        // Brow.
        const float browLift = pose.browRaiseAmount * face.getHeight() * 0.05f;
        Path brow;
        brow.startNewSubPath (cx - eyeW * 0.45f, eyeY - eyeHFull * 0.72f - browLift);
        brow.quadraticTo (cx, eyeY - eyeHFull * (0.88f + 0.1f * pose.browRaiseAmount) - browLift,
                          cx + eyeW * 0.45f, eyeY - eyeHFull * 0.72f - browLift);
        g.setColour (irisDeep.withAlpha (0.8f));
        g.strokePath (brow, juce::PathStrokeType (m.H * 0.008f,
                                                  juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));
    }

    // Blush: both cheeks in one path.
    if (pose.blushAmount > 0.02f)
    {
        const float bw = face.getWidth() * 0.14f, bh = face.getHeight() * 0.07f;
        Path blush;
        blush.addEllipse (face.getCentreX() - eyeDx - bw * 0.7f, eyeY + eyeHFull * 0.55f, bw, bh);
        blush.addEllipse (face.getCentreX() + eyeDx - bw * 0.3f, eyeY + eyeHFull * 0.55f, bw, bh);
        g.setColour (col (palette::roseAccent).withAlpha (0.35f * pose.blushAmount));
        g.fillPath (blush);
    }

    // Mouth: smile arc, or open "singing" shape.
    const float mouthY = face.getY() + face.getHeight() * 0.78f;
    const float mouthW = face.getWidth() * (0.16f + 0.10f * pose.mouthSmileAmount);
    if (pose.mouthOpenAmount > 0.25f)
    {
        const float mh = face.getHeight() * 0.10f * pose.mouthOpenAmount;
        g.setColour (irisDeep);
        g.fillRoundedRectangle (face.getCentreX() - mouthW * 0.4f, mouthY - mh * 0.3f,
                                mouthW * 0.8f, mh, mh * 0.45f);
    }
    else
    {
        Path mouth;
        const float curve = face.getHeight() * (0.02f + 0.075f * pose.mouthSmileAmount);
        mouth.startNewSubPath (face.getCentreX() - mouthW * 0.5f, mouthY);
        mouth.quadraticTo (face.getCentreX(), mouthY + curve,
                           face.getCentreX() + mouthW * 0.5f, mouthY);
        g.setColour (irisDeep);
        g.strokePath (mouth, juce::PathStrokeType (m.H * 0.010f,
                                                   juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
    }
}

void drawHead (Graphics& g, const Metrics& m, const CharacterPose& pose,
               const RenderLook& look, Colour outlineColour)
{
    const auto hair = hairColours[size_t (look.hair)];
    const auto accent = accentColours[size_t (look.accent)];
    const Colour skin = col (palette::cream);

    Graphics::ScopedSaveState save (g);

    const Point<float> neck (m.origin.x + (pose.torso.position.x + pose.head.position.x) * m.H,
                             m.origin.y - m.H * 0.235f
                                 + (pose.torso.position.y + pose.head.position.y) * m.H);
    g.addTransform (AffineTransform::rotation (pose.head.rotationRadians, neck.x, neck.y));

    const float headW = m.H * 0.46f * pose.head.scale;
    const float headH = m.H * 0.40f * pose.head.scale;
    juce::Rectangle<float> head (neck.x - headW * 0.5f, neck.y - headH * 0.94f,
                                 headW, headH);
    const float hairBob = pose.hairBounceAmount * m.H * 0.015f;

    // Back hair: bob silhouette + both side locks, welded into one shape.
    Path backHair;
    backHair.addEllipse (head.expanded (headW * 0.10f, headH * 0.10f)
                             .translated (0.0f, hairBob * 0.6f));
    backHair.addRoundedRectangle (head.getX() - headW * 0.10f,
                                  head.getCentreY() - headH * 0.10f + hairBob,
                                  headW * 0.22f, headH * 0.48f, headW * 0.11f);
    backHair.addRoundedRectangle (head.getRight() - headW * 0.12f,
                                  head.getCentreY() - headH * 0.10f + hairBob,
                                  headW * 0.22f, headH * 0.48f, headW * 0.11f);
    outlinedFill (g, backHair, col (hair.shade), outlineColour, m.outlineWidth * 0.5f);

    // Face.
    g.setColour (skin);
    g.fillEllipse (head);
    g.setColour (outlineColour);
    g.drawEllipse (head, m.outlineWidth);

    drawFace (g, m, pose, look, head);

    // Front hair: bangs + both space buns welded into one shape over the face.
    Path frontHair;
    {
        Path bangs;
        const float fringeTop = head.getY() - headH * 0.06f + hairBob;
        const float fringeBottom = head.getY() + headH * 0.34f + hairBob;
        const float w = head.getWidth();
        bangs.startNewSubPath (head.getX() - headW * 0.04f, fringeTop + headH * 0.25f);
        bangs.quadraticTo (head.getX() + headW * 0.02f, fringeTop,
                           head.getCentreX(), fringeTop - headH * 0.02f);
        bangs.quadraticTo (head.getRight() - headW * 0.02f, fringeTop,
                           head.getRight() + headW * 0.04f, fringeTop + headH * 0.25f);
        bangs.lineTo (head.getRight() + headW * 0.01f, fringeBottom);
        bangs.quadraticTo (head.getX() + w * 0.83f, fringeBottom + headH * 0.10f,
                           head.getX() + w * 0.66f, fringeBottom - headH * 0.015f);
        bangs.quadraticTo (head.getX() + w * 0.50f, fringeBottom + headH * 0.12f,
                           head.getX() + w * 0.34f, fringeBottom - headH * 0.015f);
        bangs.quadraticTo (head.getX() + w * 0.17f, fringeBottom + headH * 0.10f,
                           head.getX() - headW * 0.01f, fringeBottom);
        bangs.closeSubPath();
        frontHair.addPath (bangs);
    }
    const float bunR = headW * 0.135f;
    const Point<float> bunLeft (head.getX() + headW * 0.10f, head.getY() + hairBob);
    const Point<float> bunRight (head.getRight() - headW * 0.10f, head.getY() + hairBob);
    frontHair.addEllipse (bunLeft.x - bunR, bunLeft.y - bunR, bunR * 2.0f, bunR * 2.0f);
    frontHair.addEllipse (bunRight.x - bunR, bunRight.y - bunR, bunR * 2.0f, bunR * 2.0f);
    outlinedFill (g, frontHair, col (hair.base), outlineColour, m.outlineWidth * 0.5f);

    // Single soft highlight across the fringe.
    g.setColour (col (hair.highlight).withAlpha (0.55f));
    g.fillEllipse (head.getX() + headW * 0.16f,
                   head.getY() - headH * 0.03f + hairBob,
                   headW * 0.30f, headH * 0.075f);

    // Gold star clip on the right bun.
    if ((look.accessories & accStarClip) != 0)
        fillStar (g, { bunRight.x + bunR * 0.15f, bunRight.y + bunR * 0.25f },
                  bunR * 0.55f, col (accent.main), 0.3f);

    // Crescent pin in the left bangs.
    if ((look.accessories & accCrescentPin) != 0)
    {
        Path crescent;
        const float cr = headW * 0.07f;
        const Point<float> cp (head.getX() + headW * 0.16f,
                               head.getY() + headH * 0.22f + hairBob);
        crescent.addCentredArc (cp.x, cp.y, cr, cr, 0.0f, 0.4f, kPi + 1.4f, true);
        crescent.addCentredArc (cp.x + cr * 0.35f, cp.y - cr * 0.1f, cr * 0.75f, cr * 0.75f,
                                0.0f, kPi + 1.2f, 0.6f, false);
        crescent.closeSubPath();
        g.setColour (col (accent.main));
        g.fillPath (crescent);
    }

    // Headphones: band + both cups in one welded shape.
    if ((look.accessories & accHeadphones) != 0)
    {
        Path phones;
        Path band;
        band.addCentredArc (head.getCentreX(), head.getY() + headH * 0.30f,
                            headW * 0.56f, headH * 0.52f, 0.0f, -kHalfPi * 1.25f,
                            kHalfPi * 1.25f, true);
        juce::PathStrokeType (m.H * 0.020f, juce::PathStrokeType::curved,
                              juce::PathStrokeType::rounded)
            .createStrokedPath (phones, band);
        const float cupW = headW * 0.16f, cupH = headH * 0.26f;
        phones.addRoundedRectangle (head.getX() - cupW * 0.62f,
                                    head.getCentreY() - cupH * 0.4f,
                                    cupW, cupH, cupW * 0.35f);
        phones.addRoundedRectangle (head.getRight() - cupW * 0.38f,
                                    head.getCentreY() - cupH * 0.4f,
                                    cupW, cupH, cupW * 0.35f);
        outlinedFill (g, phones, col (palette::deepPlum).brighter (0.12f),
                      col (accent.main), m.outlineWidth * 0.4f);
    }
}
} // namespace

void LumiRenderer::draw (juce::Graphics& g, const CharacterPose& pose,
                         const RenderLook& look, juce::Rectangle<float> bounds)
{
    if (bounds.isEmpty())
        return;

    // Height budget guarantees the worst case fits: vertical extent is
    // 0.42H below-origin anchor + ~0.78H head/buns/companion star above the
    // origin + 0.35H maximum root lift; horizontal is ±0.73H (arms out plus
    // root shift). Safe bounds for every dance, per the framing contract.
    Metrics m;
    m.H = juce::jmin (bounds.getHeight() / 1.58f, bounds.getWidth() / 1.50f);
    m.outlineWidth = juce::jmax (1.0f, m.H * (look.highContrast ? 0.012f : 0.008f));

    // Hip anchor: horizontally centred, body occupying the lower ~2/3.
    m.origin = { bounds.getCentreX() + pose.root.position.x * m.H,
                 bounds.getBottom() - m.H * 0.42f + pose.root.position.y * m.H };

    Graphics::ScopedSaveState save (g);
    if (look.mirror)
        g.addTransform (AffineTransform::scale (-1.0f, 1.0f,
                                                bounds.getCentreX(), bounds.getCentreY()));
    g.addTransform (AffineTransform::scale (pose.root.scale, pose.root.scale,
                                            m.origin.x, m.origin.y));

    const Colour outlineColour = look.highContrast
                                     ? juce::Colours::black
                                     : col (palette::shadow).withAlpha (0.85f);

    // Soft ground shadow.
    g.setColour (col (palette::shadow).withAlpha (0.30f));
    g.fillEllipse (m.origin.x - m.H * 0.20f,
                   bounds.getBottom() - m.H * 0.065f,
                   m.H * 0.40f, m.H * 0.05f);

    if ((look.accessories & accCompanionStar) != 0)
        drawCompanionStar (g, m, pose, look);

    drawLegs (g, m, pose, look, outlineColour);
    drawBody (g, m, pose, look, outlineColour);
    drawArms (g, m, pose, look, outlineColour);
    drawHead (g, m, pose, look, outlineColour);
}
} // namespace lumi
