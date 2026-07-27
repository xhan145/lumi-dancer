// LUMI//DANCER — painted-sprite choreography.
//
// The Painted (anime) art style steps through hand-painted sprite poses on
// a beat grid instead of posing the vector rig. Frame selection lives here
// in the JUCE-free core so it is deterministic and unit-testable; the UI
// layer only blits. Motion (bounce, tilt, squash) is derived from the
// Choreographer's CharacterPose, so every existing audio mapping, idle
// blend and accessibility behaviour carries over unchanged.
#pragma once

#include <cstdint>

#include "analysis/AudioReactiveFrame.h"
#include "dance/DanceAnimation.h"
#include "dance/IdleBehavior.h"
#include "rig/CharacterPose.h"

namespace lumi
{
// Order matches the embedded resource set (resources/sprites/poseNN_*.png).
enum class SpriteFrame : int
{
    Stand = 0,     // neutral standing
    Cheer,         // arm up, leg up
    Clasp,         // hands clasped, happy bounce
    Excited,       // >_< arms crossed
    StepUp,        // knee up, singing
    Shy,           // hands down, soft smile
    Wink,          // wink cheer
    Heart,         // paw-hands forward
    Paws,          // both paws up
    Kick,          // wink side-kick
    PawsSmall,     // compact paws-up stand
    WinkPose,      // wink hip pose
    Lean,          // hand-on-hip lean
    JumpKick,      // kick-back jump
    Cool,          // sunglasses kneel (the drop!)
    Sitting,       // sitting on the floor
    Count
};

const char* spriteFrameName (SpriteFrame f);

struct SpriteState
{
    SpriteFrame frame = SpriteFrame::Stand;
    float offsetX = 0.0f;      // character units, from the pose root
    float offsetY = 0.0f;
    float rotation = 0.0f;     // radians, gentle body tilt
    float squash = 1.0f;       // vertical squash/stretch (1 = neutral)
    bool mirrored = false;
};

// Frame for a dancing Lumi. Pure function of its inputs: identical
// (style, beat, audio, seed) always yields the same frame, so locked seeds
// reproduce painted choreography exactly.
SpriteFrame spriteFrameForDance (DanceStyle style, double beat,
                                 const AudioReactiveFrame& audio, uint64_t seed);

// Frame while idle (transport stopped / silence).
SpriteFrame spriteFrameForIdle (IdleState idleState);

// Full sprite state: frame choice plus motion derived from the rig pose.
// `activityLevel` is the Choreographer's idle→dance blend (0..1).
SpriteState evaluateSpriteState (DanceStyle style, double beat,
                                 const AudioReactiveFrame& audio, uint64_t seed,
                                 const CharacterPose& pose, float activityLevel,
                                 IdleState idleState);
} // namespace lumi
