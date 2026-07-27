// Host synchronisation tests: phase tracking, tempo changes, stop/restart,
// seek, loop wrap, free-mode fallback.
#include "TestFramework.h"

#include "timing/BeatClock.h"

using namespace lumi;

namespace
{
HostTimingSnapshot playingSnap (double ppq, double bpm = 120.0)
{
    HostTimingSnapshot s;
    s.hasBpm = true;
    s.bpm = bpm;
    s.hasPpq = true;
    s.ppq = ppq;
    s.isPlaying = true;
    return s;
}

// Simulate a host running steadily: the snapshot ppq advances with real time.
double runSteady (BeatClock& clock, double startPpq, double seconds, double bpm,
                  double tickRate = 60.0)
{
    const int ticks = int (seconds * tickRate);
    const double dt = 1.0 / tickRate;
    double ppq = startPpq;
    for (int i = 0; i < ticks; ++i)
    {
        clock.update (playingSnap (ppq, bpm), dt);
        ppq += bpm / 60.0 * dt;
    }
    return ppq;
}
} // namespace

LD_TEST (beatclock_tracks_host_ppq)
{
    BeatClock clock;
    clock.reset();
    const double endPpq = runSteady (clock, 0.0, 4.0, 120.0);   // 8 beats

    LD_NEAR (clock.beatPosition(), endPpq, 0.1);
    LD_CHECK (clock.status() == SyncStatus::HostSync);
    LD_CHECK (clock.isPlaying());
    LD_GE (clock.beatPhase(), 0.0);
    LD_LT (clock.beatPhase(), 1.0);
}

LD_TEST (beatclock_tempo_change_updates_rate)
{
    BeatClock clock;
    clock.reset();
    runSteady (clock, 0.0, 2.0, 120.0);
    LD_NEAR (clock.bpm(), 120.0, 1e-9);

    runSteady (clock, clock.beatPosition(), 2.0, 174.0);
    LD_NEAR (clock.bpm(), 174.0, 1e-9);

    // After the change, position still tracks the host closely.
    const double before = clock.beatPosition();
    runSteady (clock, before, 1.0, 174.0);
    LD_NEAR (clock.beatPosition() - before, 174.0 / 60.0, 0.15);
}

LD_TEST (beatclock_seek_snaps_and_flags_discontinuity)
{
    BeatClock clock;
    clock.reset();
    runSteady (clock, 0.0, 2.0, 120.0);

    clock.update (playingSnap (64.0), 1.0 / 60.0);   // arrangement jump
    LD_NEAR (clock.beatPosition(), 64.0, 0.2);
    LD_CHECK (clock.discontinuityFlag());

    clock.update (playingSnap (64.05), 1.0 / 60.0);
    LD_CHECK (! clock.discontinuityFlag());
}

LD_TEST (beatclock_loop_wrap_recovers)
{
    BeatClock clock;
    clock.reset();
    runSteady (clock, 6.0, 1.0, 120.0);   // approaching loop end at beat 8

    clock.update (playingSnap (0.0), 1.0 / 60.0);   // loop wraps to start
    LD_NEAR (clock.beatPosition(), 0.0, 0.2);
    LD_CHECK (clock.discontinuityFlag());

    runSteady (clock, 0.02, 1.0, 120.0);
    LD_NEAR (clock.beatPosition(), 2.0, 0.15);
}

LD_TEST (beatclock_stop_and_restart)
{
    BeatClock clock;
    clock.reset();
    runSteady (clock, 0.0, 2.0, 120.0);

    HostTimingSnapshot stopped = playingSnap (4.0);
    stopped.isPlaying = false;
    clock.update (stopped, 1.0 / 60.0);
    LD_CHECK (! clock.isPlaying());
    LD_CHECK (clock.status() == SyncStatus::Stopped);

    // While stopped the position holds (host cursor stays at 4).
    for (int i = 0; i < 120; ++i)
        clock.update (stopped, 1.0 / 60.0);
    LD_NEAR (clock.beatPosition(), 4.0, 1e-6);

    // Restart flags a discontinuity so the pose blender can crossfade.
    clock.update (playingSnap (4.0), 1.0 / 60.0);
    LD_CHECK (clock.isPlaying());
    LD_CHECK (clock.discontinuityFlag());
}

LD_TEST (beatclock_seek_while_stopped_follows_cursor)
{
    BeatClock clock;
    clock.reset();
    HostTimingSnapshot stopped = playingSnap (4.0);
    stopped.isPlaying = false;
    clock.update (stopped, 1.0 / 60.0);

    stopped.ppq = 32.0;   // user clicks elsewhere in the arrangement
    clock.update (stopped, 1.0 / 60.0);
    LD_NEAR (clock.beatPosition(), 32.0, 1e-6);
    LD_CHECK (clock.discontinuityFlag());
}

LD_TEST (beatclock_free_mode_without_host_timeline)
{
    BeatClock clock;
    clock.reset();

    HostTimingSnapshot noTransport;   // hasPpq = false (standalone app)
    noTransport.hasBpm = false;

    for (int i = 0; i < 120; ++i)
        clock.update (noTransport, 1.0 / 60.0);

    LD_CHECK (clock.status() == SyncStatus::FreeMode);
    LD_CHECK (clock.isPlaying());                       // keeps dancing
    LD_NEAR (clock.beatPosition(), 4.0, 0.1);           // 2 s at default 120 BPM
}

LD_TEST (beatclock_rejects_invalid_input)
{
    BeatClock clock;
    clock.reset();

    HostTimingSnapshot bad = playingSnap (1.0);
    bad.bpm = std::numeric_limits<double>::quiet_NaN();
    clock.update (bad, 1.0 / 60.0);
    LD_CHECK (std::isfinite (clock.beatPosition()));
    LD_NEAR (clock.bpm(), 120.0, 1e-9);   // NaN tempo ignored, default kept

    bad.bpm = -50.0;
    clock.update (bad, 1.0 / 60.0);
    LD_NEAR (clock.bpm(), 120.0, 1e-9);

    // Absurd dt is clamped, not integrated.
    clock.update (playingSnap (2.0), 9999.0);
    LD_CHECK (std::isfinite (clock.beatPosition()));
}

LD_TEST (beatclock_bar_phase_uses_time_signature)
{
    BeatClock clock;
    clock.reset();
    HostTimingSnapshot s = playingSnap (3.0);   // beat 3 of a 4/4 bar
    s.timeSigNumerator = 4;
    clock.update (s, 0.0);
    LD_NEAR (clock.barPhase(), 0.75, 0.02);

    s.timeSigNumerator = 3;                     // beat 3 of 3/4 = bar start
    s.ppq = 3.0;
    clock.update (s, 0.0);
    LD_NEAR (clock.barPhase(), 0.0, 0.02);
    LD_EQ (clock.beatsPerBar(), 3);
}
