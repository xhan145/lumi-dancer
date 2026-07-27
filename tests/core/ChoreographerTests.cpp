// Choreographer integration tests: idle/dance blending, style switching
// without pose snaps, reduced motion, discontinuity crossfades, routines.
#include "TestFramework.h"

#include "dance/Choreographer.h"

using namespace lumi;

namespace
{
AudioReactiveFrame musicAudio()
{
    AudioReactiveFrame a;
    a.rms = 0.35f;
    a.lowEnergy = 0.7f;
    a.midEnergy = 0.5f;
    a.highEnergy = 0.4f;
    a.lowTransient = 0.3f;
    a.silence = false;
    return a;
}

AudioReactiveFrame silentAudio()
{
    AudioReactiveFrame a;
    a.silence = true;
    return a;
}

BeatClock playingClock (double& hostPpq, double dt = 1.0 / 60.0)
{
    (void) hostPpq;
    (void) dt;
    return BeatClock {};
}

void advancePlaying (BeatClock& clock, double& ppq, double seconds)
{
    const double dt = 1.0 / 60.0;
    const int steps = int (seconds * 60.0);
    for (int i = 0; i < steps; ++i)
    {
        HostTimingSnapshot snap;
        snap.hasBpm = true;
        snap.bpm = 120.0;
        snap.hasPpq = true;
        snap.ppq = ppq;
        snap.isPlaying = true;
        clock.update (snap, dt);
        ppq += 2.0 * dt;   // 120 BPM
    }
}

void advanceStopped (BeatClock& clock, double ppq, double seconds)
{
    const double dt = 1.0 / 60.0;
    const int steps = int (seconds * 60.0);
    for (int i = 0; i < steps; ++i)
    {
        HostTimingSnapshot snap;
        snap.hasBpm = true;
        snap.bpm = 120.0;
        snap.hasPpq = true;
        snap.ppq = ppq;
        snap.isPlaying = false;
        clock.update (snap, dt);
    }
}
} // namespace

LD_TEST (choreographer_dances_when_playing_idles_when_stopped)
{
    Choreographer choreo;
    ChoreographerParams params;
    BeatClock clock;
    clock.reset();
    double ppq = 0.0;

    // Play for 2 s with music.
    for (int i = 0; i < 120; ++i)
    {
        advancePlaying (clock, ppq, 1.0 / 60.0);
        choreo.update (1.0f / 60.0f, clock, musicAudio(), params);
    }
    LD_GT (choreo.activityLevel(), 0.9f);

    // Stop for 3 s.
    for (int i = 0; i < 180; ++i)
    {
        advanceStopped (clock, ppq, 1.0 / 60.0);
        choreo.update (1.0f / 60.0f, clock, silentAudio(), params);
    }
    LD_LT (choreo.activityLevel(), 0.1f);
    LD_GT (choreo.idleTimeSeconds(), 2.0f);
}

LD_TEST (choreographer_no_pose_snap_on_style_change)
{
    Choreographer choreo;
    ChoreographerParams params;
    params.style = DanceStyle::Bounce;
    BeatClock clock;
    clock.reset();
    double ppq = 0.0;

    CharacterPose prev;
    float maxJump = 0.0f;
    for (int i = 0; i < 360; ++i)
    {
        if (i == 180)
            params.style = DanceStyle::Trance;   // switch mid-dance
        advancePlaying (clock, ppq, 1.0 / 60.0);
        const CharacterPose p = choreo.update (1.0f / 60.0f, clock, musicAudio(), params);
        if (i > 0)
            maxJump = std::max (maxJump, poseDistance (prev, p));
        prev = p;
    }
    // The style switch itself must not exceed normal frame-to-frame movement
    // by an order of magnitude (crossfade prevents teleporting limbs).
    LD_LT (maxJump, 1.0f);
}

LD_TEST (choreographer_crossfades_seek_discontinuity)
{
    Choreographer choreo;
    ChoreographerParams params;
    BeatClock clock;
    clock.reset();
    double ppq = 0.0;

    CharacterPose prev;
    float maxJump = 0.0f;
    for (int i = 0; i < 240; ++i)
    {
        if (i == 120)
            ppq = 64.0;   // arrangement jump
        advancePlaying (clock, ppq, 1.0 / 60.0);
        const CharacterPose p = choreo.update (1.0f / 60.0f, clock, musicAudio(), params);
        if (i > 0)
            maxJump = std::max (maxJump, poseDistance (prev, p));
        prev = p;
    }
    LD_LT (maxJump, 1.0f);
}

LD_TEST (choreographer_reduced_motion_moves_less)
{
    ChoreographerParams normal;
    ChoreographerParams reduced;
    reduced.reducedMotion = true;

    float movement[2] = { 0.0f, 0.0f };
    int idx = 0;
    for (ChoreographerParams* params : { &normal, &reduced })
    {
        Choreographer choreo;
        BeatClock clock;
        clock.reset();
        double ppq = 0.0;
        CharacterPose prev;
        for (int i = 0; i < 300; ++i)
        {
            advancePlaying (clock, ppq, 1.0 / 60.0);
            const CharacterPose p = choreo.update (1.0f / 60.0f, clock, musicAudio(), *params);
            if (i > 60)
                movement[idx] += poseDistance (prev, p);
            prev = p;
        }
        ++idx;
    }
    LD_LT (movement[1], movement[0] * 0.7f);
}

LD_TEST (choreographer_routine_mode_uses_routine)
{
    Choreographer choreo;
    ChoreographerParams params;
    params.useRoutine = true;
    params.seed = 42;

    ChoreoParams cp;
    choreo.setRoutine (generateRoutine (2, 4, 42, cp));

    BeatClock clock;
    clock.reset();
    double ppq = 0.0;
    for (int i = 0; i < 240; ++i)
    {
        advancePlaying (clock, ppq, 1.0 / 60.0);
        choreo.update (1.0f / 60.0f, clock, musicAudio(), params);
    }
    LD_CHECK (choreo.activeRoutineStar() != 0);
    LD_CHECK (findStar (choreo.activeRoutineStar()) != nullptr);
}

LD_TEST (choreographer_output_always_sane)
{
    Choreographer choreo;
    ChoreographerParams params;
    params.style = DanceStyle::Breakcore;
    params.intensity = 2.0f;

    BeatClock clock;
    clock.reset();
    double ppq = 0.0;

    AudioReactiveFrame nasty = musicAudio();
    nasty.rms = std::numeric_limits<float>::quiet_NaN();
    nasty.lowTransient = std::numeric_limits<float>::infinity();

    bool allSane = true;
    for (int i = 0; i < 300; ++i)
    {
        advancePlaying (clock, ppq, 1.0 / 60.0);
        const CharacterPose p = choreo.update (1.0f / 60.0f, clock, nasty, params);
        if (! std::isfinite (p.root.position.x) || ! std::isfinite (p.head.rotationRadians)
            || std::fabs (p.root.position.x) > kMaxRootOffset + 1e-5f)
            allSane = false;
    }
    LD_CHECK (allSane);
}
