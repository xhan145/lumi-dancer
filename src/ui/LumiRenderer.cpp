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

// Rounded capsule from `start`, pointing at `angle` (0 = straight down),
// of the given length/thickness. Returns the end point for limb chaining.
Point<float> drawCapsule (Graphics& g, Point<float> start, float angle,
                          float length, float thickness,
                          Colour fill, Colour outlineColour, float outlineWidth)
{
    const Point<float> end (start.x + std::sin (angle) * length,
                            start.y + std::cos (angle) * length);
    Path p;
    p.addRoundedRectangle (-thickness * 0.5f, -thickness * 0.35f,
                           thickness, length + thickness * 0.7f,
                           thickness * 0.5f);
    const AffineTransform t = AffineTransform::rotation (-angle)
                                  .translated (start.x, start.y);
    p.applyTransform (t);
    g.setColour (fill);
    g.fillPath (p);
    g.setColour (outlineColour);
    g.strokePath (p, juce::PathStrokeType (outlineWidth));
    return end;
}

void drawStar (Graphics& g, Point<float> centre, float radius, Colour fill,
               float rotation = 0.0f)
{
    Path star;
    star.addStar (centre, 5, radius * 0.45f, radius, rotation);
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
    // Soft glow, then the star.
    g.setColour (col (accent.soft).withAlpha (0.35f));
    g.fillEllipse (pos.x - m.H * 0.055f, pos.y - m.H * 0.055f, m.H * 0.11f, m.H * 0.11f);
    drawStar (g, pos, m.H * 0.038f, col (accent.main),
              std::sin (float (look.timeSeconds) * 1.3f) * 0.35f);
    drawStar (g, { pos.x + m.H * 0.05f, pos.y - m.H * 0.04f }, m.H * 0.014f,
              col (accent.soft));
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

    const Point<float> hipL (m.origin.x - m.H * 0.055f, m.origin.y + m.H * 0.02f);
    const Point<float> hipR (m.origin.x + m.H * 0.055f, m.origin.y + m.H * 0.02f);

    const auto drawLeg = [&] (Point<float> hip, const CharacterBone& upper,
                              const CharacterBone& lower)
    {
        hip.x += upper.position.x * m.H;
        hip.y += upper.position.y * m.H;
        const Point<float> knee = drawCapsule (g, hip, upper.rotationRadians,
                                               upperLen, thick, skin, outlineColour,
                                               m.outlineWidth);
        const float lowerAngle = upper.rotationRadians + lower.rotationRadians;
        const Point<float> ankle = drawCapsule (g, knee, lowerAngle, lowerLen,
                                                thick * 0.92f, skin, outlineColour,
                                                m.outlineWidth);
        // Rounded boot with a gold trim band.
        const float bootW = thick * 1.5f;
        const float bootH = thick * 1.15f;
        juce::Rectangle<float> boot (ankle.x - bootW * 0.5f,
                                     ankle.y - bootH * 0.15f, bootW, bootH);
        g.setColour (col (outfit.boots));
        g.fillRoundedRectangle (boot, bootH * 0.4f);
        g.setColour (col (accent.main));
        g.fillRoundedRectangle (boot.removeFromTop (bootH * 0.32f), bootH * 0.18f);
        g.setColour (outlineColour);
        g.drawRoundedRectangle (ankle.x - bootW * 0.5f, ankle.y - bootH * 0.15f,
                                bootW, bootH, bootH * 0.4f, m.outlineWidth);
    };

    drawLeg (hipL, pose.leftUpperLeg, pose.leftLowerLeg);
    drawLeg (hipR, pose.rightUpperLeg, pose.rightLowerLeg);
}

void drawTorso (Graphics& g, const Metrics& m, const CharacterPose& pose,
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

    // Skirt: soft flare.
    Path skirt;
    skirt.startNewSubPath (waist.x - torsoW * 0.42f, waist.y - skirtH * 0.6f);
    skirt.quadraticTo (waist.x - skirtW * 0.62f, waist.y + skirtH * 0.9f,
                       waist.x - skirtW * 0.45f, waist.y + skirtH);
    skirt.quadraticTo (waist.x, waist.y + skirtH * 1.25f,
                       waist.x + skirtW * 0.45f, waist.y + skirtH);
    skirt.quadraticTo (waist.x + skirtW * 0.62f, waist.y + skirtH * 0.9f,
                       waist.x + torsoW * 0.42f, waist.y - skirtH * 0.6f);
    skirt.closeSubPath();
    g.setColour (col (outfit.skirt));
    g.fillPath (skirt);
    g.setColour (outlineColour);
    g.strokePath (skirt, juce::PathStrokeType (m.outlineWidth));

    // Torso: cream top under an open jacket.
    juce::Rectangle<float> torso (waist.x - torsoW * 0.5f, waist.y - torsoH,
                                  torsoW, torsoH);
    g.setColour (col (outfit.top));
    g.fillRoundedRectangle (torso, torsoW * 0.28f);

    // Jacket halves.
    const float lapel = torsoW * 0.34f;
    juce::Rectangle<float> jacketL (torso.getX() - torsoW * 0.06f, torso.getY() - torsoH * 0.03f,
                                    lapel, torsoH * 1.02f);
    juce::Rectangle<float> jacketR (torso.getRight() - lapel + torsoW * 0.06f,
                                    torso.getY() - torsoH * 0.03f, lapel, torsoH * 1.02f);
    g.setColour (col (outfit.jacket));
    g.fillRoundedRectangle (jacketL, lapel * 0.4f);
    g.fillRoundedRectangle (jacketR, lapel * 0.4f);
    g.setColour (col (outfit.jacketShade));
    g.fillRoundedRectangle (jacketL.removeFromBottom (torsoH * 0.18f), lapel * 0.3f);
    g.fillRoundedRectangle (jacketR.removeFromBottom (torsoH * 0.18f), lapel * 0.3f);
    g.setColour (outlineColour);
    g.drawRoundedRectangle (torso, torsoW * 0.28f, m.outlineWidth);

    // Tiny gold star on the chest.
    drawStar (g, { waist.x, waist.y - torsoH * 0.58f }, torsoW * 0.10f,
              col (accent.main));

    // Orbit-ring belt.
    if ((look.accessories & accOrbitBelt) != 0)
    {
        Path ring;
        ring.addEllipse (waist.x - skirtW * 0.52f, waist.y - skirtH * 0.42f,
                         skirtW * 1.04f, skirtH * 0.8f);
        g.setColour (col (accent.main).withAlpha (0.9f));
        g.strokePath (ring, juce::PathStrokeType (m.H * 0.008f));
        drawStar (g, { waist.x + skirtW * 0.5f, waist.y - skirtH * 0.02f },
                  m.H * 0.016f, col (accent.soft));
        drawStar (g, { waist.x - skirtW * 0.45f, waist.y + skirtH * 0.14f },
                  m.H * 0.012f, col (accent.main));
    }
}

void drawArms (Graphics& g, const Metrics& m, const CharacterPose& pose,
               const RenderLook& look, Colour outlineColour)
{
    const auto outfit = outfitColours[size_t (look.outfit)];
    const Colour skin = col (palette::cream);
    const float upperLen = m.H * 0.115f;
    const float lowerLen = m.H * 0.10f;
    const float thick = m.H * 0.050f;

    const float shoulderY = m.origin.y - m.H * 0.215f;
    const Point<float> shoulderL (m.origin.x - m.H * 0.125f + pose.torso.position.x * m.H,
                                  shoulderY + pose.torso.position.y * m.H);
    const Point<float> shoulderR (m.origin.x + m.H * 0.125f + pose.torso.position.x * m.H,
                                  shoulderY + pose.torso.position.y * m.H);

    const auto drawArm = [&] (Point<float> shoulder, const CharacterBone& upper,
                              const CharacterBone& lower, bool isLeft)
    {
        shoulder.x += upper.position.x * m.H;
        shoulder.y += upper.position.y * m.H;
        // rotation 0 = hanging down; positive opens outward for the left arm,
        // mirrored for the right (styles use opposite signs already).
        const float upperAngle = isLeft ? upper.rotationRadians : -(-upper.rotationRadians);
        // Sleeve then skin forearm.
        const Point<float> elbow = drawCapsule (g, shoulder, upperAngle, upperLen,
                                                thick * 1.15f, col (outfit.jacket),
                                                outlineColour, m.outlineWidth);
        const float lowerAngle = upperAngle + lower.rotationRadians;
        const Point<float> wrist = drawCapsule (g, elbow, lowerAngle, lowerLen,
                                                thick * 0.9f, skin, outlineColour,
                                                m.outlineWidth);
        // Rounded mitten hand.
        const float handR = thick * 0.62f;
        g.setColour (skin);
        g.fillEllipse (wrist.x - handR, wrist.y - handR * 0.6f, handR * 2.0f, handR * 2.0f);
        g.setColour (outlineColour);
        g.drawEllipse (wrist.x - handR, wrist.y - handR * 0.6f, handR * 2.0f, handR * 2.0f,
                       m.outlineWidth);
    };

    drawArm (shoulderL, pose.leftUpperArm, pose.leftLowerArm, true);
    drawArm (shoulderR, pose.rightUpperArm, pose.rightLowerArm, false);
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

        // Big kawaii eye: dark rim, purple iris, sparkle highlights.
        g.setColour (irisDeep);
        g.fillEllipse (eye.expanded (m.H * 0.004f));
        g.setColour (iris);
        g.fillEllipse (eye);
        g.setColour (iris.brighter (0.5f));
        g.fillEllipse (eye.withTrimmedTop (eye.getHeight() * 0.55f).reduced (eyeW * 0.18f, 0.0f));

        if (pose.starEyeAmount > 0.35f)
        {
            drawStar (g, { cx, eyeY }, eyeW * 0.34f,
                      col (palette::softGold).withAlpha (pose.starEyeAmount));
        }
        else
        {
            g.setColour (juce::Colours::white.withAlpha (0.95f));
            g.fillEllipse (cx - eyeW * 0.16f, eyeY - eyeH * 0.26f, eyeW * 0.22f, eyeH * 0.26f);
            g.fillEllipse (cx + eyeW * 0.08f, eyeY + eyeH * 0.05f, eyeW * 0.12f, eyeH * 0.14f);
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

    // Blush.
    if (pose.blushAmount > 0.02f)
    {
        g.setColour (col (palette::roseAccent).withAlpha (0.35f * pose.blushAmount));
        const float bw = face.getWidth() * 0.14f, bh = face.getHeight() * 0.07f;
        g.fillEllipse (face.getCentreX() - eyeDx - bw * 0.7f, eyeY + eyeHFull * 0.55f, bw, bh);
        g.fillEllipse (face.getCentreX() + eyeDx - bw * 0.3f, eyeY + eyeHFull * 0.55f, bw, bh);
    }

    // Mouth: smile arc blended with an open "singing" ellipse.
    const float mouthY = face.getY() + face.getHeight() * 0.78f;
    const float mouthW = face.getWidth() * (0.16f + 0.10f * pose.mouthSmileAmount);
    if (pose.mouthOpenAmount > 0.25f)
    {
        const float mh = face.getHeight() * 0.10f * pose.mouthOpenAmount;
        g.setColour (irisDeep);
        g.fillRoundedRectangle (face.getCentreX() - mouthW * 0.4f, mouthY - mh * 0.3f,
                                mouthW * 0.8f, mh, mh * 0.45f);
        g.setColour (col (palette::roseAccent).withAlpha (0.7f));
        g.fillRoundedRectangle (face.getCentreX() - mouthW * 0.25f, mouthY + mh * 0.25f,
                                mouthW * 0.5f, mh * 0.4f, mh * 0.2f);
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

    // Back hair: a soft bob silhouette slightly larger than the head, with
    // two shoulder-length side locks.
    Path backHair;
    backHair.addEllipse (head.expanded (headW * 0.10f, headH * 0.10f)
                             .translated (0.0f, hairBob * 0.6f));
    backHair.addRoundedRectangle (head.getX() - headW * 0.10f,
                                  head.getCentreY() - headH * 0.10f + hairBob,
                                  headW * 0.22f, headH * 0.48f, headW * 0.11f);
    backHair.addRoundedRectangle (head.getRight() - headW * 0.12f,
                                  head.getCentreY() - headH * 0.10f + hairBob,
                                  headW * 0.22f, headH * 0.48f, headW * 0.11f);
    g.setColour (col (hair.shade));
    g.fillPath (backHair);

    // Face.
    g.setColour (skin);
    g.fillEllipse (head);
    g.setColour (outlineColour);
    g.drawEllipse (head, m.outlineWidth);

    drawFace (g, m, pose, look, head);

    // Bangs: scalloped fringe across the forehead.
    Path bangs;
    const float fringeTop = head.getY() - headH * 0.06f + hairBob;
    const float fringeBottom = head.getY() + headH * 0.34f + hairBob;
    bangs.startNewSubPath (head.getX() - headW * 0.04f, fringeTop + headH * 0.25f);
    bangs.quadraticTo (head.getX() + headW * 0.02f, fringeTop,
                       head.getCentreX(), fringeTop - headH * 0.02f);
    bangs.quadraticTo (head.getRight() - headW * 0.02f, fringeTop,
                       head.getRight() + headW * 0.04f, fringeTop + headH * 0.25f);
    bangs.lineTo (head.getRight() + headW * 0.01f, fringeBottom);
    // Three scallops back across the forehead.
    const float w = head.getWidth();
    bangs.quadraticTo (head.getX() + w * 0.83f, fringeBottom + headH * 0.10f,
                       head.getX() + w * 0.66f, fringeBottom - headH * 0.015f);
    bangs.quadraticTo (head.getX() + w * 0.50f, fringeBottom + headH * 0.12f,
                       head.getX() + w * 0.34f, fringeBottom - headH * 0.015f);
    bangs.quadraticTo (head.getX() + w * 0.17f, fringeBottom + headH * 0.10f,
                       head.getX() - headW * 0.01f, fringeBottom);
    bangs.closeSubPath();
    g.setColour (col (hair.base));
    g.fillPath (bangs);
    g.setColour (col (hair.highlight).withAlpha (0.55f));
    g.fillEllipse (head.getX() + w * 0.16f, fringeTop + headH * 0.03f,
                   w * 0.30f, headH * 0.075f);
    g.setColour (outlineColour);
    g.strokePath (bangs, juce::PathStrokeType (m.outlineWidth * 0.8f));

    // Space buns.
    const float bunR = headW * 0.135f;
    const Point<float> bunL (head.getX() + headW * 0.10f, head.getY() + hairBob);
    const Point<float> bunR_ (head.getRight() - headW * 0.10f, head.getY() + hairBob);
    for (const auto& bun : { bunL, bunR_ })
    {
        g.setColour (col (hair.base));
        g.fillEllipse (bun.x - bunR, bun.y - bunR, bunR * 2.0f, bunR * 2.0f);
        g.setColour (col (hair.highlight).withAlpha (0.5f));
        g.fillEllipse (bun.x - bunR * 0.45f, bun.y - bunR * 0.65f, bunR * 0.8f, bunR * 0.5f);
        g.setColour (outlineColour);
        g.drawEllipse (bun.x - bunR, bun.y - bunR, bunR * 2.0f, bunR * 2.0f,
                       m.outlineWidth * 0.8f);
    }

    // Gold star clip on the right bun.
    if ((look.accessories & accStarClip) != 0)
        drawStar (g, { bunR_.x + bunR * 0.15f, bunR_.y + bunR * 0.25f },
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

    // Headphones: band over the head, cups over the side locks.
    if ((look.accessories & accHeadphones) != 0)
    {
        Path band;
        band.addCentredArc (head.getCentreX(), head.getY() + headH * 0.30f,
                            headW * 0.56f, headH * 0.52f, 0.0f, -kHalfPi * 1.25f,
                            kHalfPi * 1.25f, true);
        g.setColour (col (palette::deepPlum).brighter (0.15f));
        g.strokePath (band, juce::PathStrokeType (m.H * 0.020f,
                                                  juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));
        const float cupW = headW * 0.16f, cupH = headH * 0.26f;
        g.setColour (col (palette::deepPlum).brighter (0.1f));
        g.fillRoundedRectangle (head.getX() - cupW * 0.62f, head.getCentreY() - cupH * 0.4f,
                                cupW, cupH, cupW * 0.35f);
        g.fillRoundedRectangle (head.getRight() - cupW * 0.38f, head.getCentreY() - cupH * 0.4f,
                                cupW, cupH, cupW * 0.35f);
        g.setColour (col (accent.main));
        g.drawRoundedRectangle (head.getX() - cupW * 0.62f, head.getCentreY() - cupH * 0.4f,
                                cupW, cupH, cupW * 0.35f, m.H * 0.006f);
        g.drawRoundedRectangle (head.getRight() - cupW * 0.38f, head.getCentreY() - cupH * 0.4f,
                                cupW, cupH, cupW * 0.35f, m.H * 0.006f);
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
    drawTorso (g, m, pose, look, outlineColour);
    drawArms (g, m, pose, look, outlineColour);
    drawHead (g, m, pose, look, outlineColour);
}
} // namespace lumi
