#include "dance/IdleBehavior.h"

#include <cmath>

#include "core/LumiMath.h"
#include "dance/PoseHelpers.h"

namespace lumi
{
using namespace posehelpers;

IdleBehavior::IdleBehavior (uint64_t seed) : rng (seed) {}

void IdleBehavior::reset (uint64_t seed)
{
    idleState = IdleState::Breathing;
    rng.reseed (seed);
    time = 0.0f;
    stateTime = 0.0f;
    nextGestureIn = 4.0f;
    gestureDuration = 2.0f;
}

void IdleBehavior::pickNextGesture()
{
    static constexpr IdleState gestures[] = {
        IdleState::LookAround, IdleState::Wave, IdleState::Stretch, IdleState::CheckStar
    };
    idleState = gestures[rng.nextInt (4)];
    stateTime = 0.0f;
    gestureDuration = rng.nextRange (1.8f, 3.0f);
}

CharacterPose IdleBehavior::update (float dt, float idleSeconds, float motionAmount,
                                    float sleepDelaySeconds)
{
    dt = clamp (dt, 0.0f, 0.25f);
    time += dt;
    stateTime += dt;
    const float m = clamp01 (motionAmount);

    // Long inactivity escalates: sit at 60 s, sleep after the user delay.
    if (sleepDelaySeconds > 0.0f && idleSeconds > sleepDelaySeconds)
    {
        if (idleState != IdleState::Sleeping) { idleState = IdleState::Sleeping; stateTime = 0.0f; }
    }
    else if (idleSeconds > 60.0f)
    {
        if (idleState != IdleState::Sitting && idleState != IdleState::Sleeping)
        {
            idleState = IdleState::Sitting;
            stateTime = 0.0f;
        }
    }
    else if (idleState == IdleState::Sitting || idleState == IdleState::Sleeping)
    {
        idleState = IdleState::Breathing;
        stateTime = 0.0f;
    }

    // Gesture scheduling while simply breathing.
    if (idleState == IdleState::Breathing)
    {
        nextGestureIn -= dt;
        if (nextGestureIn <= 0.0f)
        {
            pickNextGesture();
            nextGestureIn = rng.nextRange (5.0f, 11.0f);
        }
    }
    else if (idleState != IdleState::Sitting && idleState != IdleState::Sleeping
             && stateTime > gestureDuration)
    {
        idleState = IdleState::Breathing;
        stateTime = 0.0f;
    }

    // ---------------------------------------------------------------- pose
    CharacterPose p = neutralPose();

    // Breathing underlies everything: slow torso scale + tiny rise.
    const float breath = std::sin (time * kTwoPi * 0.22f);   // ~13 breaths/min
    p.torso.scale += 0.015f * breath;
    p.root.position.y += -0.004f * breath;
    p.hairBounceAmount = 0.05f * breath;

    const float g = smoothstep (clamp01 (stateTime / 0.5f))
                  * smoothstep (clamp01 ((gestureDuration - stateTime) / 0.5f));

    switch (idleState)
    {
        case IdleState::LookAround:
            p.head.rotationRadians += 0.25f * m * g * std::sin (stateTime * 1.8f);
            break;

        case IdleState::Wave:
            p.rightUpperArm.rotationRadians += -1.9f * m * g;
            p.rightLowerArm.rotationRadians += (-0.4f + 0.45f * std::sin (stateTime * 9.0f)) * m * g;
            p.mouthSmileAmount = 0.5f + 0.3f * g;
            break;

        case IdleState::Stretch:
            addHandsUp (p, 0.9f * m * g);
            p.torso.scale += 0.03f * m * g;
            p.eyeOpenAmount = 1.0f - 0.6f * g;
            break;

        case IdleState::CheckStar:
            // Looks up-left toward the floating companion star.
            p.head.rotationRadians += 0.3f * m * g;
            p.head.position.y += -0.01f * m * g;
            p.leftUpperArm.rotationRadians += 1.2f * m * g;
            p.mouthSmileAmount = 0.5f + 0.25f * g;
            break;

        case IdleState::Sitting:
        {
            const float sink = smoothstep (clamp01 (stateTime / 0.8f));
            p.root.position.y += 0.12f * sink;
            p.leftUpperLeg.rotationRadians  += 1.2f * sink;
            p.rightUpperLeg.rotationRadians -= 1.2f * sink;
            p.leftLowerLeg.rotationRadians  -= 1.0f * sink;
            p.rightLowerLeg.rotationRadians += 1.0f * sink;
            p.leftUpperArm.rotationRadians  += 0.2f * sink;
            p.rightUpperArm.rotationRadians -= 0.2f * sink;
            break;
        }

        case IdleState::Sleeping:
        {
            const float sink = smoothstep (clamp01 (stateTime / 1.5f));
            p.root.position.y += 0.12f * sink;
            p.head.rotationRadians += 0.35f * sink;
            p.eyeOpenAmount = 1.0f - 0.97f * sink;
            p.mouthSmileAmount = 0.4f;
            p.leftUpperLeg.rotationRadians  += 1.2f * sink;
            p.rightUpperLeg.rotationRadians -= 1.2f * sink;
            p.leftLowerLeg.rotationRadians  -= 1.0f * sink;
            p.rightLowerLeg.rotationRadians += 1.0f * sink;
            // Slower, deeper sleep breathing.
            p.torso.scale += 0.02f * std::sin (time * kTwoPi * 0.12f) * sink;
            break;
        }

        case IdleState::Breathing:
        default:
            break;
    }

    sanitizePose (p);
    return p;
}
} // namespace lumi
