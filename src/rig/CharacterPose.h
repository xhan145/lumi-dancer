// LUMI//DANCER — the 2D character rig.
//
// A pose is a set of bone transforms plus face parameters. Poses are plain
// data: dance styles produce them, the pose blender interpolates them, the
// renderer draws them. All positions are in "character units" relative to the
// root (1.0 ≈ Lumi's height); the renderer scales into pixels.
#pragma once

#include "core/LumiMath.h"

namespace lumi
{
struct CharacterBone
{
    Vec2  position {};                // offset from parent, character units
    float rotationRadians = 0.0f;
    float scale = 1.0f;
};

struct CharacterPose
{
    CharacterBone root;               // whole-body offset (bounce, sway, jumps)
    CharacterBone head;
    CharacterBone torso;
    CharacterBone leftUpperArm;
    CharacterBone leftLowerArm;
    CharacterBone rightUpperArm;
    CharacterBone rightLowerArm;
    CharacterBone leftUpperLeg;
    CharacterBone leftLowerLeg;
    CharacterBone rightUpperLeg;
    CharacterBone rightLowerLeg;

    float eyeOpenAmount   = 1.0f;     // 0 closed .. 1 open
    float mouthSmileAmount = 0.5f;    // 0 flat .. 1 big smile
    float mouthOpenAmount  = 0.0f;    // 0 closed .. 1 open (singing "wah")
    float hairBounceAmount = 0.0f;    // driven by the spring layer
    float blushAmount      = 0.0f;    // cheek tint
    float starEyeAmount    = 0.0f;    // star-shaped highlight during hype
    float browRaiseAmount  = 0.0f;    // -1 frown .. +1 raised
};

// ------------------------------------------------------------------ helpers
inline CharacterBone lerpBone (const CharacterBone& a, const CharacterBone& b, float t)
{
    CharacterBone out;
    out.position        = lerp (a.position, b.position, t);
    out.rotationRadians = lerpAngle (a.rotationRadians, b.rotationRadians, t);
    out.scale           = lerp (a.scale, b.scale, t);
    return out;
}

CharacterPose lerpPose (const CharacterPose& a, const CharacterPose& b, float t);

// Aggregate distance between two poses; used by style-distinctness tests and
// by the blender to decide whether a transition needs smoothing.
float poseDistance (const CharacterPose& a, const CharacterPose& b);

// Clamp every field to sane, finite ranges so a misbehaving style can never
// push the character off-stage or into NaN.
void sanitizePose (CharacterPose& pose);

// Largest |root offset| any sane pose may reach; the stage uses this to keep
// Lumi inside the visible area for every dance.
inline constexpr float kMaxRootOffset = 0.35f;
} // namespace lumi
