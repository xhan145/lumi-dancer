// LUMI//DANCER — shared pose construction blocks used by dance styles, the
// move library and idle behaviour. All amounts are in character units and
// pre-clamped by sanitizePose() at the end of every evaluation.
#pragma once

#include <cmath>
#include <cstdint>

#include "core/LumiMath.h"
#include "rig/CharacterPose.h"

namespace lumi::posehelpers
{
// Relaxed standing pose: arms slightly out, soft smile.
inline CharacterPose neutralPose()
{
    CharacterPose p;
    p.leftUpperArm.rotationRadians  = 0.25f;
    p.rightUpperArm.rotationRadians = -0.25f;
    p.leftLowerArm.rotationRadians  = 0.15f;
    p.rightLowerArm.rotationRadians = -0.15f;
    p.mouthSmileAmount = 0.55f;
    return p;
}

// Vertical bounce with impact flattening at the bottom of each beat.
inline void addBounce (CharacterPose& p, double beatPhase, float amount)
{
    const float s = std::sin (float (beatPhase) * kPi);
    p.root.position.y += -amount * s * s;             // dip within the beat
    p.torso.scale     += amount * 0.15f * (1.0f - s); // squash on landing
}

// Lateral weight shift; period is in beats (2 = step left, step right).
inline void addSideStep (CharacterPose& p, double beat, float amount, float periodBeats = 2.0f)
{
    const float phase = float (fract (beat / periodBeats));
    const float sway  = std::sin (phase * kTwoPi);
    p.root.position.x += amount * sway;
    p.leftUpperLeg.rotationRadians  += amount * 1.2f * std::max (0.0f,  sway);
    p.rightUpperLeg.rotationRadians -= amount * 1.2f * std::max (0.0f, -sway);
    p.leftLowerLeg.rotationRadians  += amount * 0.6f * std::max (0.0f,  sway);
    p.rightLowerLeg.rotationRadians -= amount * 0.6f * std::max (0.0f, -sway);
}

// Opposing arm swing (walking/dancing arms), rate in beats per cycle.
inline void addArmSwing (CharacterPose& p, double beat, float amount, float periodBeats = 1.0f)
{
    const float s = std::sin (float (fract (beat / periodBeats)) * kTwoPi);
    p.leftUpperArm.rotationRadians  += amount * s;
    p.rightUpperArm.rotationRadians += amount * s;      // same sign: rotations mirror visually
    p.leftLowerArm.rotationRadians  += amount * 0.6f * s;
    p.rightLowerArm.rotationRadians += amount * 0.6f * s;
}

// Head nod at the given rate.
inline void addHeadNod (CharacterPose& p, double beat, float amount, float periodBeats = 1.0f)
{
    const float s = std::sin (float (fract (beat / periodBeats)) * kTwoPi);
    p.head.rotationRadians += amount * 0.5f * s;
    p.head.position.y      += -amount * 0.03f * (s * s);
}

// Hands-up "cheer" gesture, 0..1.
inline void addHandsUp (CharacterPose& p, float amount)
{
    p.leftUpperArm.rotationRadians  += amount * 2.2f;
    p.rightUpperArm.rotationRadians += -amount * 2.2f;
    p.leftLowerArm.rotationRadians  += amount * 0.8f;
    p.rightLowerArm.rotationRadians += -amount * 0.8f;
}

// Deterministic per-index hash in [0,1) — used for glitch/freestyle picks.
inline float hash01 (uint64_t x)
{
    x ^= x >> 33; x *= 0xff51afd7ed558ccdull;
    x ^= x >> 33; x *= 0xc4ceb9fe1a85ec53ull;
    x ^= x >> 33;
    return float (x >> 40) * (1.0f / 16777216.0f);
}
} // namespace lumi::posehelpers
