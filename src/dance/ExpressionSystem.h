// LUMI//DANCER — facial expression engine.
//
// Runs on the animation thread at frame rate. Deterministic for a given
// (seed, elapsed time, inputs) so tests can reproduce blink and expression
// sequences exactly.
#pragma once

#include <cstdint>

#include "analysis/AudioReactiveFrame.h"
#include "core/Palette.h"
#include "core/SeededRng.h"

namespace lumi
{
enum class Expression : int
{
    Neutral = 0,
    Happy,
    Excited,
    Focused,
    Sleepy,
    Surprised,
    Proud,
    Shy,
    Hyper,
    Count
};

const char* expressionName (Expression e);

struct ExpressionState
{
    Expression expression = Expression::Neutral;
    float eyeOpenAmount   = 1.0f;
    float mouthSmileAmount = 0.5f;
    float mouthOpenAmount  = 0.0f;
    float blushAmount      = 0.0f;
    float starEyeAmount    = 0.0f;
    float browRaiseAmount  = 0.0f;
    float eyeLookX         = 0.0f;   // -1..1, subtle eye wander
};

class ExpressionEngine
{
public:
    explicit ExpressionEngine (uint64_t seed = 1);

    void reset (uint64_t seed);

    // dt in seconds. silenceSeconds: how long the input has been silent.
    ExpressionState update (float dt,
                            const AudioReactiveFrame& audio,
                            ExpressionTheme mood,
                            float silenceSeconds);

    Expression current() const { return state.expression; }

private:
    void chooseExpression (const AudioReactiveFrame& audio,
                           ExpressionTheme mood,
                           float silenceSeconds);

    ExpressionState state;
    SeededRng rng;
    float timeInExpression = 0.0f;
    float nextBlinkIn = 2.0f;
    float blinkPhase = -1.0f;        // <0 = not blinking, else 0..1
    float energyAvg = 0.0f;
    float lookTarget = 0.0f;
    float lookCurrent = 0.0f;
    float nextLookIn = 1.5f;
};
} // namespace lumi
