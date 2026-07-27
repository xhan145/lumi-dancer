// LUMI//DANCER — the animation brain.
//
// Runs on the message thread at frame rate. Consumes the latest analysis
// frame + beat clock state, applies user reaction controls, delegates to the
// active dance style (or constellation routine), blends idle behaviour,
// expressions and secondary hair motion, and guarantees pose continuity
// across style changes, seeks and transport stops.
#pragma once

#include <memory>

#include "analysis/AudioReactiveFrame.h"
#include "constellation/RoutineEngine.h"
#include "dance/DanceAnimation.h"
#include "dance/ExpressionSystem.h"
#include "dance/IdleBehavior.h"
#include "timing/BeatClock.h"

namespace lumi
{
struct ChoreographerParams
{
    DanceStyle style = DanceStyle::Bounce;
    float intensity      = 1.0f;   // 0..2
    float reactionAmount = 1.0f;   // 0..2
    float lowSens        = 1.0f;   // 0..2
    float midSens        = 1.0f;
    float highSens       = 1.0f;
    float transientSens  = 1.0f;
    float smoothing      = 0.35f;  // 0..1
    bool  beatLock       = false;  // quantise musical time to a 16th grid

    ExpressionTheme mood = ExpressionTheme::Soft;
    uint64_t seed = 1;

    bool  reducedMotion = false;
    bool  idleEnabled   = true;
    float sleepDelaySeconds = 180.0f;
    float idleMotionAmount  = 0.7f;

    bool useRoutine = false;
    PlaybackMode playbackMode = PlaybackMode::Loop;
};

class Choreographer
{
public:
    Choreographer();

    void setRoutine (Routine routine);
    const Routine& routine() const { return activeRoutine; }

    // Produces the final, sanitised pose for this frame.
    CharacterPose update (float dt, const BeatClock& clock,
                          const AudioReactiveFrame& rawAudio,
                          const ChoreographerParams& params);

    // 0 = fully idle, 1 = fully dancing (drives particles and UI hints).
    float activityLevel() const { return danceBlend; }

    // Smoothed, sensitivity-scaled frame the last update used (UI meters).
    const AudioReactiveFrame& reactiveFrame() const { return smoothed; }

    Expression currentExpression() const { return expressions.current(); }
    float idleTimeSeconds() const { return idleSeconds; }
    IdleState idleState() const { return idle.currentState(); }
    uint64_t activeRoutineStar() const { return lastRoutineStar; }

    void reset();

private:
    void ensureStyle (const ChoreographerParams& params);
    AudioReactiveFrame shapeAudio (const AudioReactiveFrame& raw,
                                   const ChoreographerParams& params, float dt);

    std::unique_ptr<DanceAnimation> styleAnim;
    DanceStyle builtStyle = DanceStyle::Count;
    uint64_t builtSeed = 0;

    IdleBehavior idle;
    ExpressionEngine expressions;
    Routine activeRoutine;

    AudioReactiveFrame smoothed;
    CharacterPose lastPose;
    bool hasLastPose = false;

    float danceBlend   = 0.0f;   // idle→dance mix
    float crossfade    = 0.0f;   // 1 → 0 after style change / discontinuity
    CharacterPose crossfadeFrom;

    float idleSeconds    = 0.0f;
    float silenceSeconds = 0.0f;
    Spring hairSpring;
    float previousRootY = 0.0f;
    uint64_t lastRoutineStar = 0;
};
} // namespace lumi
