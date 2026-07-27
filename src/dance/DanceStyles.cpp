// LUMI//DANCER — the ten dance personalities.
//
// Every style is a deterministic function of (beat position, BPM, audio,
// intensity). Audio reactivity follows the product mapping: low transients
// hit the body, mid transients hit arms/head, high energy shimmers, RMS
// scales overall amplitude. Styles must stay inside sanitizePose() bounds.
#include "dance/DanceAnimation.h"

#include <array>
#include <cmath>

#include "dance/PoseHelpers.h"

namespace lumi
{
using namespace posehelpers;

namespace
{
// Common audio-derived motion scale: quiet music still moves a little, loud
// music approaches full amplitude; intensity is the user control.
float motionScale (const AudioReactiveFrame& audio, float intensity)
{
    const float energy = clamp01 (0.35f + 0.65f * clamp01 (audio.rms * 2.5f));
    return clamp (energy * clamp (intensity, 0.0f, 2.0f), 0.0f, 2.0f);
}

// Kick impact 0..1 with a little sustain from low energy.
float kick (const AudioReactiveFrame& audio)
{
    return clamp01 (audio.lowTransient * 0.8f + audio.lowEnergy * 0.25f);
}

// ------------------------------------------------------------------- Bounce
class BounceStyle final : public DanceAnimation
{
public:
    void reset() override {}
    const char* name() const override { return "Bounce"; }

    CharacterPose evaluate (double beat, double, const AudioReactiveFrame& audio,
                            float intensity) const override
    {
        const float m = motionScale (audio, intensity);
        CharacterPose p = neutralPose();

        addBounce (p, fract (beat), (0.06f + 0.05f * kick (audio)) * m);
        addHeadNod (p, beat, 0.35f * m, 1.0f);
        addArmSwing (p, beat, 0.30f * m, 2.0f);
        p.head.rotationRadians += 0.10f * m * audio.midTransient;

        p.mouthSmileAmount = 0.6f + 0.2f * audio.rms;
        sanitizePose (p);
        return p;
    }
};

// --------------------------------------------------------------- Kawaii Pop
class KawaiiPopStyle final : public DanceAnimation
{
public:
    void reset() override {}
    const char* name() const override { return "Kawaii Pop"; }

    CharacterPose evaluate (double beat, double, const AudioReactiveFrame& audio,
                            float intensity) const override
    {
        const float m = motionScale (audio, intensity);
        CharacterPose p = neutralPose();

        addSideStep (p, beat, 0.10f * m, 2.0f);
        addBounce (p, fract (beat), 0.03f * m);

        // Hand gestures: alternate "paw up"每 two beats, both up on bar starts.
        const int twoBeat = int (std::floor (beat / 2.0));
        const float gesture = smoothstep (float (fract (beat / 2.0)) * 4.0f);
        if (twoBeat % 4 == 3)
            addHandsUp (p, 0.8f * gesture * m);
        else if (twoBeat % 2 == 0)
            p.leftUpperArm.rotationRadians += 1.6f * gesture * m;
        else
            p.rightUpperArm.rotationRadians -= 1.6f * gesture * m;

        // Cute head tilt swapping sides each bar.
        const float tilt = (int (std::floor (beat / 4.0)) % 2 == 0) ? 1.0f : -1.0f;
        p.head.rotationRadians += 0.20f * tilt * m;

        p.blushAmount = 0.5f;
        p.mouthSmileAmount = 0.8f;
        p.starEyeAmount = clamp01 (audio.highEnergy * 0.8f);
        sanitizePose (p);
        return p;
    }
};

// -------------------------------------------------------------------- Orbit
class OrbitStyle final : public DanceAnimation
{
public:
    void reset() override {}
    const char* name() const override { return "Orbit"; }

    CharacterPose evaluate (double beat, double, const AudioReactiveFrame& audio,
                            float intensity) const override
    {
        const float m = motionScale (audio, intensity);
        CharacterPose p = neutralPose();

        // Continuous circular arm rotation (one revolution per two beats) —
        // rotation, not oscillation, is this style's signature.
        const float orbitAngle = float (fract (beat / 2.0)) * kTwoPi;
        p.leftUpperArm.rotationRadians  = orbitAngle;
        p.rightUpperArm.rotationRadians = wrapAngle (orbitAngle + kPi);
        p.leftLowerArm.rotationRadians  = 0.4f * std::sin (orbitAngle);
        p.rightLowerArm.rotationRadians = 0.4f * std::sin (orbitAngle + kPi);

        // Floating: the body drifts on a slow circle and hovers.
        const float drift = float (fract (beat / 4.0)) * kTwoPi;
        p.root.position.x += 0.05f * m * std::cos (drift);
        p.root.position.y += -0.02f * m - 0.03f * m * std::sin (drift);

        p.head.rotationRadians += 0.08f * std::sin (drift);
        p.starEyeAmount = clamp01 (0.4f + audio.highEnergy * 0.6f);
        p.mouthSmileAmount = 0.65f;
        sanitizePose (p);
        return p;
    }
};

// ------------------------------------------------------------------- Groove
class GrooveStyle final : public DanceAnimation
{
public:
    void reset() override {}
    const char* name() const override { return "Groove"; }

    CharacterPose evaluate (double beat, double, const AudioReactiveFrame& audio,
                            float intensity) const override
    {
        const float m = motionScale (audio, intensity) * (0.6f + 0.4f * audio.midEnergy);
        CharacterPose p = neutralPose();

        // Hip sway + shoulder roll; the head lags an eighth behind the hips.
        const float hip = std::sin (float (fract (beat / 2.0)) * kTwoPi);
        p.root.position.x += 0.06f * m * hip;
        p.root.position.y += -0.02f * m * (1.0f - std::fabs (hip));
        p.torso.rotationRadians += 0.12f * m * std::sin (float (fract (beat)) * kTwoPi);
        p.head.rotationRadians  += 0.15f * m * std::sin (float (fract ((beat - 0.5) / 2.0)) * kTwoPi);

        // Relaxed knees.
        p.leftUpperLeg.rotationRadians  += 0.10f * m * std::max (0.0f,  hip);
        p.rightUpperLeg.rotationRadians -= 0.10f * m * std::max (0.0f, -hip);

        addArmSwing (p, beat, 0.18f * m, 2.0f);
        p.rightUpperArm.rotationRadians -= 0.5f * m * audio.midTransient;

        p.eyeOpenAmount = 0.85f;
        p.mouthSmileAmount = 0.6f;
        sanitizePose (p);
        return p;
    }
};

// -------------------------------------------------------------------- Hyper
class HyperStyle final : public DanceAnimation
{
public:
    void reset() override {}
    const char* name() const override { return "Hyper"; }

    CharacterPose evaluate (double beat, double, const AudioReactiveFrame& audio,
                            float intensity) const override
    {
        const float m = motionScale (audio, intensity);
        CharacterPose p = neutralPose();

        // Double-time steps.
        addBounce (p, fract (beat * 2.0), 0.05f * m);
        addSideStep (p, beat * 2.0, 0.06f * m, 2.0f);

        // Jumps on strong hits.
        const float hit = kick (audio);
        if (hit > 0.5f)
            p.root.position.y += 0.10f * m * (hit - 0.5f) * 2.0f;

        // Punchy arms on mid transients, otherwise fast pumping.
        addArmSwing (p, beat * 2.0, 0.35f * m, 1.0f);
        addHandsUp (p, 0.5f * m * audio.midTransient);

        p.eyeOpenAmount = 1.0f;
        p.mouthOpenAmount = clamp01 (0.3f + 0.7f * audio.rms * 2.0f);
        p.mouthSmileAmount = 0.9f;
        p.starEyeAmount = clamp01 (audio.transientProbability);
        sanitizePose (p);
        return p;
    }
};

// -------------------------------------------------------------------- Chill
class ChillStyle final : public DanceAnimation
{
public:
    void reset() override {}
    const char* name() const override { return "Chill"; }

    CharacterPose evaluate (double beat, double, const AudioReactiveFrame& audio,
                            float intensity) const override
    {
        const float m = motionScale (audio, intensity) * 0.4f;
        CharacterPose p = neutralPose();

        // Slow four-beat sway, barely-there nod.
        const float sway = std::sin (float (fract (beat / 4.0)) * kTwoPi);
        p.root.position.x += 0.04f * m * sway;
        p.head.rotationRadians += 0.08f * m * sway;
        p.torso.rotationRadians += 0.04f * m * sway;
        addHeadNod (p, beat, 0.08f * m, 2.0f);

        p.eyeOpenAmount = 0.7f;
        p.mouthSmileAmount = 0.5f;
        p.blushAmount = 0.2f;
        sanitizePose (p);
        return p;
    }
};

// ---------------------------------------------------------------- Breakcore
class BreakcoreStyle final : public DanceAnimation
{
public:
    explicit BreakcoreStyle (uint64_t seedIn) : seed (seedIn) {}
    void reset() override {}
    const char* name() const override { return "Breakcore"; }

    CharacterPose evaluate (double beat, double, const AudioReactiveFrame& audio,
                            float intensity) const override
    {
        const float m = motionScale (audio, intensity);
        CharacterPose p = neutralPose();

        // Quantised 16th-note pose snapping: each step index hashes to a
        // discrete pose. Controlled frame skipping = the pose holds within a
        // step, then snaps. Kept readable by bounding every amplitude.
        const uint64_t step = uint64_t (std::floor (beat * 4.0));
        const float h1 = hash01 (step * 2654435761u + seed);
        const float h2 = hash01 (step * 40503u + seed * 3u + 1u);
        const float h3 = hash01 (step * 9176u + seed * 7u + 2u);

        p.root.position.x += (h1 - 0.5f) * 0.16f * m;
        p.root.position.y += -h2 * 0.06f * m;
        p.head.rotationRadians += (h3 - 0.5f) * 0.8f * m;
        p.leftUpperArm.rotationRadians  += (h2 - 0.5f) * 2.4f * m;
        p.rightUpperArm.rotationRadians += (h1 - 0.5f) * -2.4f * m;
        p.leftLowerArm.rotationRadians  += (h3 - 0.5f) * 1.2f * m;
        p.rightLowerArm.rotationRadians += (h2 - 0.5f) * -1.2f * m;
        p.leftUpperLeg.rotationRadians  += (h1 - 0.5f) * 0.8f * m;
        p.rightUpperLeg.rotationRadians += (h3 - 0.5f) * -0.8f * m;

        // Sharp beat-level accent so it still reads as dancing, not noise.
        addBounce (p, fract (beat), 0.04f * m + 0.05f * m * kick (audio));

        p.eyeOpenAmount = 0.9f;
        p.mouthOpenAmount = clamp01 (audio.transientProbability * 0.8f);
        p.mouthSmileAmount = 0.7f;
        sanitizePose (p);
        return p;
    }

private:
    uint64_t seed;
};

// ------------------------------------------------------------- Drum & Bass
class DrumAndBassStyle final : public DanceAnimation
{
public:
    void reset() override {}
    const char* name() const override { return "Drum & Bass"; }

    CharacterPose evaluate (double beat, double, const AudioReactiveFrame& audio,
                            float intensity) const override
    {
        const float m = motionScale (audio, intensity);
        CharacterPose p = neutralPose();

        // Half-time upper body...
        const float halfTime = std::sin (float (fract (beat / 2.0)) * kTwoPi);
        p.torso.rotationRadians += 0.10f * m * halfTime;
        p.head.rotationRadians  += 0.18f * m * std::sin (float (fract ((beat - 0.25) / 2.0)) * kTwoPi);
        p.root.position.y += -0.025f * m * (1.0f + halfTime) * 0.5f;

        // ...fast double-time footwork.
        const float feet = float (fract (beat * 2.0));
        p.leftUpperLeg.rotationRadians  += 0.5f * m * std::max (0.0f, std::sin (feet * kTwoPi));
        p.rightUpperLeg.rotationRadians -= 0.5f * m * std::max (0.0f, -std::sin (feet * kTwoPi));
        p.leftLowerLeg.rotationRadians  += 0.35f * m * std::max (0.0f, std::sin (feet * kTwoPi + 0.6f));
        p.rightLowerLeg.rotationRadians -= 0.35f * m * std::max (0.0f, -std::sin (feet * kTwoPi + 0.6f));

        // Strong snare reaction: sharp arm snap on mid transients.
        addArmSwing (p, beat, 0.15f * m, 2.0f);
        const float snare = audio.midTransient;
        p.leftUpperArm.rotationRadians += 1.4f * m * snare;
        p.head.position.y += -0.02f * m * snare;

        p.mouthSmileAmount = 0.65f;
        sanitizePose (p);
        return p;
    }
};

// ------------------------------------------------------------------- Trance
class TranceStyle final : public DanceAnimation
{
public:
    void reset() override {}
    const char* name() const override { return "Trance"; }

    CharacterPose evaluate (double beat, double, const AudioReactiveFrame& audio,
                            float intensity) const override
    {
        const float m = motionScale (audio, intensity);
        CharacterPose p = neutralPose();

        // Sixteen-beat (4-bar) phrase: the body rises through the phrase and
        // releases at the top — the classic build-and-lift.
        const float phrase = float (fract (beat / 16.0));
        const float rise = easeInOutCubic (phrase < 0.9f ? phrase / 0.9f : (1.0f - phrase) / 0.1f);
        p.root.position.y += -0.05f * m * rise + 0.02f * m;

        // Flowing overhead arm waves, left trailing right.
        const float wave = float (fract (beat / 4.0)) * kTwoPi;
        p.leftUpperArm.rotationRadians  = 1.8f + 0.5f * m * std::sin (wave);
        p.rightUpperArm.rotationRadians = -1.8f - 0.5f * m * std::sin (wave + 0.8f);
        p.leftLowerArm.rotationRadians  = 0.4f * m * std::sin (wave + 0.4f);
        p.rightLowerArm.rotationRadians = -0.4f * m * std::sin (wave + 1.2f);

        // High-frequency shimmer trembles the hands.
        const float shimmer = audio.highEnergy * m;
        p.leftLowerArm.rotationRadians  += 0.08f * shimmer * std::sin (float (beat) * 40.0f);
        p.rightLowerArm.rotationRadians += 0.08f * shimmer * std::cos (float (beat) * 40.0f);

        addSideStep (p, beat, 0.05f * m, 4.0f);

        p.eyeOpenAmount = 0.8f;
        p.starEyeAmount = clamp01 (rise * audio.highEnergy + 0.2f);
        p.mouthSmileAmount = 0.6f;
        sanitizePose (p);
        return p;
    }
};

// ---------------------------------------------------------------- Freestyle
// Deterministically walks the style library: every four-beat segment hashes
// (segment index, seed) to one of the other styles, avoiding immediate
// repeats, and crossfades over the segment boundary's first half-beat.
class FreestyleStyle final : public DanceAnimation
{
public:
    explicit FreestyleStyle (uint64_t seedIn)
        : seed (seedIn)
    {
        for (int i = 0; i < kPoolSize; ++i)
            pool[size_t (i)] = createDanceStyle (poolStyles[size_t (i)], seedIn + uint64_t (i));
    }

    void reset() override
    {
        for (auto& s : pool)
            s->reset();
    }

    const char* name() const override { return "Freestyle"; }

    CharacterPose evaluate (double beat, double bpm, const AudioReactiveFrame& audio,
                            float intensity) const override
    {
        const double segLen = 4.0;
        const uint64_t seg = uint64_t (std::floor (std::max (0.0, beat) / segLen));
        const int current = pickForSegment (seg);
        const CharacterPose pose = pool[size_t (current)]->evaluate (beat, bpm, audio, intensity);

        // Crossfade from the previous segment's style across the boundary.
        const double local = beat - double (seg) * segLen;
        if (seg > 0 && local < 0.5)
        {
            const int previous = pickForSegment (seg - 1);
            if (previous != current)
            {
                const CharacterPose prevPose =
                    pool[size_t (previous)]->evaluate (beat, bpm, audio, intensity);
                return lerpPose (prevPose, pose, smoothstep (float (local / 0.5)));
            }
        }
        return pose;
    }

private:
    static constexpr int kPoolSize = 6;
    static constexpr DanceStyle poolStyles[kPoolSize] = {
        DanceStyle::Bounce, DanceStyle::KawaiiPop, DanceStyle::Orbit,
        DanceStyle::Groove, DanceStyle::Hyper, DanceStyle::Trance
    };

    int pickForSegment (uint64_t seg) const
    {
        int pick = int (hash01 (seg * 0x9e3779b97f4a7c15ull + seed) * float (kPoolSize));
        pick = clamp (pick, 0, kPoolSize - 1);
        if (seg > 0)
        {
            // Repeat avoidance: nudge to the next slot when we'd repeat.
            int prev = int (hash01 ((seg - 1) * 0x9e3779b97f4a7c15ull + seed) * float (kPoolSize));
            prev = clamp (prev, 0, kPoolSize - 1);
            if (pick == prev)
                pick = (pick + 1) % kPoolSize;
        }
        return pick;
    }

    uint64_t seed;
    std::array<std::unique_ptr<DanceAnimation>, kPoolSize> pool;
};
} // namespace

const char* danceStyleName (DanceStyle style)
{
    switch (style)
    {
        case DanceStyle::Bounce:      return "Bounce";
        case DanceStyle::KawaiiPop:   return "Kawaii Pop";
        case DanceStyle::Orbit:       return "Orbit";
        case DanceStyle::Groove:      return "Groove";
        case DanceStyle::Hyper:       return "Hyper";
        case DanceStyle::Chill:       return "Chill";
        case DanceStyle::Breakcore:   return "Breakcore";
        case DanceStyle::DrumAndBass: return "Drum & Bass";
        case DanceStyle::Trance:      return "Trance";
        case DanceStyle::Freestyle:   return "Freestyle";
        default:                      return "Unknown";
    }
}

std::unique_ptr<DanceAnimation> createDanceStyle (DanceStyle style, uint64_t seed)
{
    switch (style)
    {
        case DanceStyle::KawaiiPop:   return std::make_unique<KawaiiPopStyle>();
        case DanceStyle::Orbit:       return std::make_unique<OrbitStyle>();
        case DanceStyle::Groove:      return std::make_unique<GrooveStyle>();
        case DanceStyle::Hyper:       return std::make_unique<HyperStyle>();
        case DanceStyle::Chill:       return std::make_unique<ChillStyle>();
        case DanceStyle::Breakcore:   return std::make_unique<BreakcoreStyle> (seed);
        case DanceStyle::DrumAndBass: return std::make_unique<DrumAndBassStyle>();
        case DanceStyle::Trance:      return std::make_unique<TranceStyle>();
        case DanceStyle::Freestyle:   return std::make_unique<FreestyleStyle> (seed);
        case DanceStyle::Bounce:
        default:                      return std::make_unique<BounceStyle>();
    }
}
} // namespace lumi
