#include "rig/CharacterPose.h"

#include <cmath>

namespace lumi
{
namespace
{
    template <typename Fn>
    void forEachBone (CharacterPose& p, Fn&& fn)
    {
        fn (p.root); fn (p.head); fn (p.torso);
        fn (p.leftUpperArm); fn (p.leftLowerArm);
        fn (p.rightUpperArm); fn (p.rightLowerArm);
        fn (p.leftUpperLeg); fn (p.leftLowerLeg);
        fn (p.rightUpperLeg); fn (p.rightLowerLeg);
    }

    template <typename Fn>
    void forEachBonePair (const CharacterPose& a, const CharacterPose& b, Fn&& fn)
    {
        fn (a.root, b.root); fn (a.head, b.head); fn (a.torso, b.torso);
        fn (a.leftUpperArm, b.leftUpperArm); fn (a.leftLowerArm, b.leftLowerArm);
        fn (a.rightUpperArm, b.rightUpperArm); fn (a.rightLowerArm, b.rightLowerArm);
        fn (a.leftUpperLeg, b.leftUpperLeg); fn (a.leftLowerLeg, b.leftLowerLeg);
        fn (a.rightUpperLeg, b.rightUpperLeg); fn (a.rightLowerLeg, b.rightLowerLeg);
    }
} // namespace

CharacterPose lerpPose (const CharacterPose& a, const CharacterPose& b, float t)
{
    t = clamp01 (t);
    CharacterPose out;

    out.root          = lerpBone (a.root, b.root, t);
    out.head          = lerpBone (a.head, b.head, t);
    out.torso         = lerpBone (a.torso, b.torso, t);
    out.leftUpperArm  = lerpBone (a.leftUpperArm,  b.leftUpperArm,  t);
    out.leftLowerArm  = lerpBone (a.leftLowerArm,  b.leftLowerArm,  t);
    out.rightUpperArm = lerpBone (a.rightUpperArm, b.rightUpperArm, t);
    out.rightLowerArm = lerpBone (a.rightLowerArm, b.rightLowerArm, t);
    out.leftUpperLeg  = lerpBone (a.leftUpperLeg,  b.leftUpperLeg,  t);
    out.leftLowerLeg  = lerpBone (a.leftLowerLeg,  b.leftLowerLeg,  t);
    out.rightUpperLeg = lerpBone (a.rightUpperLeg, b.rightUpperLeg, t);
    out.rightLowerLeg = lerpBone (a.rightLowerLeg, b.rightLowerLeg, t);

    out.eyeOpenAmount    = lerp (a.eyeOpenAmount,    b.eyeOpenAmount,    t);
    out.mouthSmileAmount = lerp (a.mouthSmileAmount, b.mouthSmileAmount, t);
    out.mouthOpenAmount  = lerp (a.mouthOpenAmount,  b.mouthOpenAmount,  t);
    out.hairBounceAmount = lerp (a.hairBounceAmount, b.hairBounceAmount, t);
    out.blushAmount      = lerp (a.blushAmount,      b.blushAmount,      t);
    out.starEyeAmount    = lerp (a.starEyeAmount,    b.starEyeAmount,    t);
    out.browRaiseAmount  = lerp (a.browRaiseAmount,  b.browRaiseAmount,  t);
    return out;
}

float poseDistance (const CharacterPose& a, const CharacterPose& b)
{
    float sum = 0.0f;
    forEachBonePair (a, b, [&sum] (const CharacterBone& x, const CharacterBone& y)
    {
        const Vec2 d = x.position - y.position;
        sum += std::sqrt (d.x * d.x + d.y * d.y);
        sum += std::fabs (wrapAngle (x.rotationRadians - y.rotationRadians)) * 0.25f;
        sum += std::fabs (x.scale - y.scale) * 0.5f;
    });
    sum += std::fabs (a.eyeOpenAmount - b.eyeOpenAmount) * 0.1f;
    sum += std::fabs (a.mouthSmileAmount - b.mouthSmileAmount) * 0.1f;
    return sum;
}

void sanitizePose (CharacterPose& pose)
{
    forEachBone (pose, [] (CharacterBone& bone)
    {
        bone.position.x = clamp (sanitize (bone.position.x), -kMaxRootOffset, kMaxRootOffset);
        bone.position.y = clamp (sanitize (bone.position.y), -kMaxRootOffset, kMaxRootOffset);
        bone.rotationRadians = clamp (sanitize (bone.rotationRadians), -kPi, kPi);
        bone.scale = clamp (sanitize (bone.scale, 1.0f), 0.25f, 2.0f);
    });

    pose.eyeOpenAmount    = clamp01 (sanitize (pose.eyeOpenAmount, 1.0f));
    pose.mouthSmileAmount = clamp01 (sanitize (pose.mouthSmileAmount, 0.5f));
    pose.mouthOpenAmount  = clamp01 (sanitize (pose.mouthOpenAmount));
    pose.hairBounceAmount = clamp (sanitize (pose.hairBounceAmount), -1.0f, 1.0f);
    pose.blushAmount      = clamp01 (sanitize (pose.blushAmount));
    pose.starEyeAmount    = clamp01 (sanitize (pose.starEyeAmount));
    pose.browRaiseAmount  = clamp (sanitize (pose.browRaiseAmount), -1.0f, 1.0f);
}
} // namespace lumi
