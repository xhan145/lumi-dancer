#include "dance/Choreographer.h"

#include <cmath>

#include "core/LumiMath.h"

namespace lumi
{
Choreographer::Choreographer()
    : idle (1), expressions (1)
{
}

void Choreographer::reset()
{
    idle.reset (1);
    expressions.reset (1);
    smoothed = {};
    hasLastPose = false;
    danceBlend = 0.0f;
    crossfade = 0.0f;
    idleSeconds = 0.0f;
    silenceSeconds = 0.0f;
    hairSpring.snapTo (0.0f);
    previousRootY = 0.0f;
    if (styleAnim != nullptr)
        styleAnim->reset();
}

void Choreographer::setRoutine (Routine routineIn)
{
    activeRoutine = std::move (routineIn);
}

void Choreographer::ensureStyle (const ChoreographerParams& params)
{
    if (styleAnim == nullptr || builtStyle != params.style || builtSeed != params.seed)
    {
        // Capture the outgoing pose so the switch crossfades instead of snapping.
        if (hasLastPose)
        {
            crossfadeFrom = lastPose;
            crossfade = 1.0f;
        }
        styleAnim = createDanceStyle (params.style, params.seed);
        builtStyle = params.style;
        builtSeed = params.seed;
        idle.reset (params.seed);
        expressions.reset (params.seed);
    }
}

AudioReactiveFrame Choreographer::shapeAudio (const AudioReactiveFrame& raw,
                                              const ChoreographerParams& params, float dt)
{
    // Defensive sanitisation first: analysis is already NaN-safe, but a
    // poisoned frame must never stick inside the smoothing state.
    AudioReactiveFrame safe = raw;
    safe.rms  = sanitize (raw.rms);
    safe.peak = sanitize (raw.peak);
    safe.lowEnergy  = sanitize (raw.lowEnergy);
    safe.midEnergy  = sanitize (raw.midEnergy);
    safe.highEnergy = sanitize (raw.highEnergy);
    safe.lowTransient  = sanitize (raw.lowTransient);
    safe.midTransient  = sanitize (raw.midTransient);
    safe.highTransient = sanitize (raw.highTransient);
    safe.transientProbability = sanitize (raw.transientProbability);
    safe.spectralCentroid = sanitize (raw.spectralCentroid);
    safe.stereoWidth = sanitize (raw.stereoWidth);
    const AudioReactiveFrame& rawSafe = safe;

    // Sensitivity scaling first...
    AudioReactiveFrame shaped = rawSafe;
    shaped.lowEnergy  = clamp01 (rawSafe.lowEnergy  * params.lowSens);
    shaped.midEnergy  = clamp01 (rawSafe.midEnergy  * params.midSens);
    shaped.highEnergy = clamp01 (rawSafe.highEnergy * params.highSens);
    shaped.lowTransient  = clamp01 (rawSafe.lowTransient  * params.transientSens);
    shaped.midTransient  = clamp01 (rawSafe.midTransient  * params.transientSens);
    shaped.highTransient = clamp01 (rawSafe.highTransient * params.transientSens);
    shaped.transientProbability = clamp01 (rawSafe.transientProbability * params.transientSens);

    // ...then reaction amount: 0 pulls the frame toward a neutral mid-level
    // "metronome" feel, 1 = as analysed, up to 2 = exaggerated.
    const float reaction = clamp (params.reactionAmount, 0.0f, 2.0f);
    const auto shape = [reaction] (float v, float neutral)
    {
        return clamp01 (neutral + (v - neutral) * reaction);
    };
    shaped.lowEnergy  = shape (shaped.lowEnergy,  0.4f);
    shaped.midEnergy  = shape (shaped.midEnergy,  0.4f);
    shaped.highEnergy = shape (shaped.highEnergy, 0.3f);
    shaped.lowTransient  = clamp01 (shaped.lowTransient  * reaction);
    shaped.midTransient  = clamp01 (shaped.midTransient  * reaction);
    shaped.highTransient = clamp01 (shaped.highTransient * reaction);
    shaped.rms = shape (shaped.rms, 0.25f);

    // Temporal smoothing (user "Motion Smoothing" control).
    const float smoothTime = lerp (0.02f, 0.5f, clamp01 (params.smoothing));
    const float coeff = clamp01 (dt / smoothTime);
    const auto blend = [coeff] (float current, float target)
    {
        return current + (target - current) * coeff;
    };
    smoothed.rms  = blend (smoothed.rms,  shaped.rms);
    smoothed.peak = blend (smoothed.peak, shaped.peak);
    smoothed.lowEnergy  = blend (smoothed.lowEnergy,  shaped.lowEnergy);
    smoothed.midEnergy  = blend (smoothed.midEnergy,  shaped.midEnergy);
    smoothed.highEnergy = blend (smoothed.highEnergy, shaped.highEnergy);
    smoothed.spectralCentroid = blend (smoothed.spectralCentroid, shaped.spectralCentroid);
    smoothed.stereoWidth = blend (smoothed.stereoWidth, shaped.stereoWidth);
    // Transients stay fast (they are accents by definition), but decay smoothly.
    smoothed.lowTransient  = std::max (shaped.lowTransient,  blend (smoothed.lowTransient,  0.0f));
    smoothed.midTransient  = std::max (shaped.midTransient,  blend (smoothed.midTransient,  0.0f));
    smoothed.highTransient = std::max (shaped.highTransient, blend (smoothed.highTransient, 0.0f));
    smoothed.transientProbability = std::max ({ smoothed.lowTransient, smoothed.midTransient,
                                                smoothed.highTransient });
    smoothed.silence = shaped.silence;
    smoothed.absoluteSample = shaped.absoluteSample;
    return smoothed;
}

CharacterPose Choreographer::update (float dt, const BeatClock& clock,
                                     const AudioReactiveFrame& rawAudio,
                                     const ChoreographerParams& params)
{
    dt = clamp (dt, 0.0f, 0.25f);
    ensureStyle (params);

    const AudioReactiveFrame audio = shapeAudio (rawAudio, params, dt);

    // ------------------------------------------------------ dancing or idle?
    if (audio.silence)
        silenceSeconds += dt;
    else
        silenceSeconds = 0.0f;

    const bool wantDance = clock.isPlaying() && silenceSeconds < 1.5f;
    if (wantDance)
        idleSeconds = 0.0f;
    else
        idleSeconds += dt;

    const float blendRate = params.reducedMotion ? 1.2f : 2.5f;
    danceBlend = clamp01 (danceBlend + (wantDance ? 1.0f : -1.0f) * blendRate * dt);

    // ------------------------------------------------------------ dance pose
    double beat = clock.beatPosition();
    if (params.beatLock)
        beat = std::floor (beat * 4.0) / 4.0;   // 16th grid

    const float intensity = params.intensity * (params.reducedMotion ? 0.45f : 1.0f);

    CharacterPose dancePose;
    if (params.useRoutine && ! activeRoutine.empty())
    {
        dancePose = evaluateRoutine (activeRoutine, beat, params.playbackMode,
                                     params.seed, audio, intensity);
        lastRoutineStar = routineActiveStar (activeRoutine, beat, params.playbackMode,
                                             params.seed);
    }
    else
    {
        dancePose = styleAnim->evaluate (beat, clock.bpm(), audio, intensity);
        lastRoutineStar = 0;
    }

    // -------------------------------------------------------------- idle pose
    CharacterPose pose;
    if (danceBlend >= 1.0f || ! params.idleEnabled)
    {
        pose = dancePose;
    }
    else
    {
        const CharacterPose idlePose = idle.update (dt, idleSeconds,
                                                    params.idleMotionAmount
                                                        * (params.reducedMotion ? 0.5f : 1.0f),
                                                    params.sleepDelaySeconds);
        pose = danceBlend <= 0.0f ? idlePose
                                  : lerpPose (idlePose, dancePose, smoothstep (danceBlend));
    }

    // ------------------------------------------------- discontinuity handling
    if (clock.discontinuityFlag() && hasLastPose)
    {
        crossfadeFrom = lastPose;
        crossfade = 1.0f;
    }
    if (crossfade > 0.0f && hasLastPose)
    {
        const float fadeRate = params.reducedMotion ? 2.5f : 5.0f;
        crossfade = std::max (0.0f, crossfade - fadeRate * dt);
        pose = lerpPose (pose, crossfadeFrom, smoothstep (crossfade));
    }

    // ------------------------------------------------------------ expressions
    const ExpressionState face = expressions.update (dt, audio, params.mood,
                                                     silenceSeconds + (wantDance ? 0.0f : idleSeconds));
    // Merge: the style's face is the base; the expression engine modulates.
    pose.eyeOpenAmount    = clamp01 (pose.eyeOpenAmount * face.eyeOpenAmount);
    pose.mouthSmileAmount = clamp01 (0.5f * pose.mouthSmileAmount + 0.5f * face.mouthSmileAmount);
    pose.mouthOpenAmount  = std::max (pose.mouthOpenAmount, face.mouthOpenAmount);
    pose.blushAmount      = std::max (pose.blushAmount, face.blushAmount);
    pose.starEyeAmount    = std::max (pose.starEyeAmount, face.starEyeAmount);
    pose.browRaiseAmount  = clamp (pose.browRaiseAmount + face.browRaiseAmount, -1.0f, 1.0f);

    // ---------------------------------------------- secondary motion (hair)
    const float rootVelY = dt > 1.0e-6f ? (pose.root.position.y - previousRootY) / dt : 0.0f;
    previousRootY = pose.root.position.y;
    const float hairTarget = clamp (-rootVelY * 1.5f, -1.0f, 1.0f);
    hairSpring.update (hairTarget, dt, params.reducedMotion ? 6.0f : 14.0f);
    pose.hairBounceAmount = clamp (pose.hairBounceAmount + hairSpring.value
                                       + audio.highTransient * 0.25f,
                                   -1.0f, 1.0f);

    // ------------------------------------------------- reduced-motion limiter
    if (params.reducedMotion && hasLastPose)
    {
        // Cap the per-frame pose change so nothing ever cuts rapidly.
        pose = lerpPose (lastPose, pose, clamp01 (dt * 10.0f));
    }

    sanitizePose (pose);
    lastPose = pose;
    hasLastPose = true;
    return pose;
}
} // namespace lumi
