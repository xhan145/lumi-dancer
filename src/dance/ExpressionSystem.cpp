#include "dance/ExpressionSystem.h"

#include <algorithm>
#include <cmath>

#include "core/LumiMath.h"

namespace lumi
{
const char* expressionName (Expression e)
{
    switch (e)
    {
        case Expression::Neutral:   return "Neutral";
        case Expression::Happy:     return "Happy";
        case Expression::Excited:   return "Excited";
        case Expression::Focused:   return "Focused";
        case Expression::Sleepy:    return "Sleepy";
        case Expression::Surprised: return "Surprised";
        case Expression::Proud:     return "Proud";
        case Expression::Shy:       return "Shy";
        case Expression::Hyper:     return "Hyper";
        default:                    return "Unknown";
    }
}

ExpressionEngine::ExpressionEngine (uint64_t seed) : rng (seed) {}

void ExpressionEngine::reset (uint64_t seed)
{
    state = {};
    rng.reseed (seed);
    timeInExpression = 0.0f;
    nextBlinkIn = 2.0f;
    blinkPhase = -1.0f;
    energyAvg = 0.0f;
    lookTarget = lookCurrent = 0.0f;
    nextLookIn = 1.5f;
}

void ExpressionEngine::chooseExpression (const AudioReactiveFrame& audio,
                                         ExpressionTheme mood,
                                         float silenceSeconds)
{
    Expression next = state.expression;

    if (silenceSeconds > 30.0f)
        next = Expression::Sleepy;
    else if (audio.transientProbability > 0.85f && energyAvg > 0.5f)
        next = rng.nextBool (0.3f) ? Expression::Surprised : Expression::Excited;
    else if (energyAvg > 0.6f)
        next = Expression::Excited;
    else if (energyAvg > 0.35f)
        next = Expression::Happy;
    else if (energyAvg > 0.12f)
        next = rng.nextBool (0.5f) ? Expression::Focused : Expression::Neutral;
    else
        next = Expression::Neutral;

    // The mood theme biases the pick.
    switch (mood)
    {
        case ExpressionTheme::Cheerful:
            if (next == Expression::Neutral) next = Expression::Happy;
            break;
        case ExpressionTheme::Confident:
            if (next == Expression::Happy) next = Expression::Proud;
            if (next == Expression::Neutral) next = Expression::Focused;
            break;
        case ExpressionTheme::Sleepy:
            if (energyAvg < 0.4f) next = Expression::Sleepy;
            break;
        case ExpressionTheme::Hyper:
            if (energyAvg > 0.25f) next = Expression::Hyper;
            break;
        case ExpressionTheme::Soft:
            if (next == Expression::Excited && rng.nextBool (0.4f)) next = Expression::Shy;
            break;
        default: break;
    }

    if (next != state.expression)
    {
        state.expression = next;
        timeInExpression = 0.0f;
    }
}

ExpressionState ExpressionEngine::update (float dt,
                                          const AudioReactiveFrame& audio,
                                          ExpressionTheme mood,
                                          float silenceSeconds)
{
    dt = clamp (dt, 0.0f, 0.25f);
    timeInExpression += dt;

    const float energyNow = clamp01 (audio.rms * 2.5f);
    energyAvg += (energyNow > energyAvg ? 0.5f : 0.05f) * dt / 0.1f * (energyNow - energyAvg) * 0.1f;
    energyAvg = clamp01 (energyAvg + (energyNow - energyAvg) * clamp01 (dt * 2.0f));

    // Re-evaluate the expression at most every ~1.5 s to avoid flicker.
    if (timeInExpression > 1.5f)
        chooseExpression (audio, mood, silenceSeconds);

    // ------------------------------------------------------------- targets
    float eyeTarget = 1.0f, smile = 0.55f, open = 0.0f, blush = 0.0f;
    float star = 0.0f, brow = 0.0f;

    switch (state.expression)
    {
        case Expression::Neutral:   eyeTarget = 0.95f; smile = 0.5f;  break;
        case Expression::Happy:     smile = 0.8f; blush = 0.25f;      break;
        case Expression::Excited:   smile = 0.9f; open = 0.35f; star = 0.6f; brow = 0.4f; break;
        case Expression::Focused:   eyeTarget = 0.8f; smile = 0.4f; brow = -0.3f; break;
        case Expression::Sleepy:    eyeTarget = 0.35f; smile = 0.35f; break;
        case Expression::Surprised: eyeTarget = 1.0f; open = 0.6f; brow = 0.9f; break;
        case Expression::Proud:     smile = 0.7f; brow = 0.3f; eyeTarget = 0.85f; break;
        case Expression::Shy:       eyeTarget = 0.7f; smile = 0.6f; blush = 0.7f; break;
        case Expression::Hyper:     smile = 1.0f; open = 0.5f; star = 0.9f; brow = 0.5f; break;
        default: break;
    }

    // Star eyes flare with sustained high energy regardless of expression.
    star = std::max (star, clamp01 ((energyAvg - 0.55f) * 2.0f));

    // ------------------------------------------------------------- blinking
    if (blinkPhase >= 0.0f)
    {
        blinkPhase += dt / 0.15f;     // 150 ms blink
        if (blinkPhase >= 1.0f)
        {
            blinkPhase = -1.0f;
            nextBlinkIn = rng.nextRange (1.8f, 4.5f);
        }
    }
    else
    {
        nextBlinkIn -= dt;
        if (nextBlinkIn <= 0.0f)
            blinkPhase = 0.0f;
    }

    float blinkMul = 1.0f;
    if (blinkPhase >= 0.0f)
    {
        // Quick close, quick open.
        blinkMul = blinkPhase < 0.5f ? 1.0f - smoothstep (blinkPhase * 2.0f)
                                     : smoothstep ((blinkPhase - 0.5f) * 2.0f);
    }

    // ------------------------------------------------------------ eye wander
    nextLookIn -= dt;
    if (nextLookIn <= 0.0f)
    {
        lookTarget = rng.nextRange (-0.6f, 0.6f);
        nextLookIn = rng.nextRange (1.2f, 3.5f);
    }
    lookCurrent += (lookTarget - lookCurrent) * clamp01 (dt * 6.0f);

    // ------------------------------------------------------------- smoothing
    const float slew = clamp01 (dt * 8.0f);
    state.eyeOpenAmount    = lerp (state.eyeOpenAmount, eyeTarget, slew) * blinkMul;
    state.mouthSmileAmount = lerp (state.mouthSmileAmount, smile, slew);
    state.mouthOpenAmount  = lerp (state.mouthOpenAmount, open + clamp01 (audio.rms) * 0.2f, slew);
    state.blushAmount      = lerp (state.blushAmount, blush, slew * 0.5f);
    state.starEyeAmount    = lerp (state.starEyeAmount, star, slew * 0.75f);
    state.browRaiseAmount  = lerp (state.browRaiseAmount, brow, slew);
    state.eyeLookX         = lookCurrent;

    return state;
}
} // namespace lumi
