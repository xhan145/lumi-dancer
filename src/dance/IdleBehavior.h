// LUMI//DANCER — idle behaviour when the transport is stopped or the input
// is silent. A tiny state machine: breathing with occasional gestures, then
// sitting, then sleep after the configured delay. Deterministic per seed.
#pragma once

#include <cstdint>

#include "core/SeededRng.h"
#include "rig/CharacterPose.h"

namespace lumi
{
enum class IdleState : int
{
    Breathing = 0,
    LookAround,
    Wave,
    Stretch,
    CheckStar,     // peeks at the floating companion star
    Sitting,
    Sleeping,
};

class IdleBehavior
{
public:
    explicit IdleBehavior (uint64_t seed = 1);

    void reset (uint64_t seed);

    // Called while idle; idleSeconds counts time since the music stopped.
    // motionAmount 0..1 scales gesture amplitude; sleepDelay in seconds.
    CharacterPose update (float dt, float idleSeconds, float motionAmount,
                          float sleepDelaySeconds);

    IdleState currentState() const { return idleState; }

private:
    void pickNextGesture();

    IdleState idleState = IdleState::Breathing;
    SeededRng rng;
    float time = 0.0f;             // total idle time integrated from dt
    float stateTime = 0.0f;        // time in current state
    float nextGestureIn = 4.0f;
    float gestureDuration = 2.0f;
};
} // namespace lumi
