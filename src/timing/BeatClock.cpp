#include "timing/BeatClock.h"

#include <algorithm>
#include <cmath>

#include "core/LumiMath.h"

namespace lumi
{
void BeatClock::reset()
{
    beat = 0.0;
    currentBpm = 120.0;
    sigNumerator = 4;
    playing = false;
    wasPlaying = false;
    discontinuity = false;
    syncStatus = SyncStatus::FreeMode;
}

void BeatClock::update (const HostTimingSnapshot& snap, double dtSeconds)
{
    discontinuity = false;
    dtSeconds = std::clamp (dtSeconds, 0.0, 0.25);

    if (snap.hasBpm && std::isfinite (snap.bpm) && snap.bpm > 1.0 && snap.bpm < 999.0)
        currentBpm = snap.bpm;
    if (snap.timeSigNumerator >= 1 && snap.timeSigNumerator <= 32)
        sigNumerator = snap.timeSigNumerator;

    const double beatsPerSecond = currentBpm / 60.0;

    if (snap.hasPpq)
    {
        playing = snap.isPlaying;

        if (playing)
        {
            syncStatus = SyncStatus::HostSync;

            // Advance locally, then correct toward the host position. The host
            // snapshot is at most one audio block old, so small errors are
            // slewed away silently; large errors mean seek/loop/restart.
            const double predicted = beat + beatsPerSecond * dtSeconds;
            const double error = snap.ppq + beatsPerSecond * dtSeconds - predicted;

            if (std::fabs (error) > 0.5)
            {
                beat = snap.ppq;
                discontinuity = true;
            }
            else
            {
                beat = predicted + error * 0.25;
            }

            if (! wasPlaying)
                discontinuity = true;   // restart after stop
        }
        else
        {
            syncStatus = SyncStatus::Stopped;
            // Follow the host cursor while stopped so a seek before pressing
            // play starts the dance from the right musical position.
            if (std::fabs (snap.ppq - beat) > 1.0e-6)
            {
                beat = snap.ppq;
                discontinuity = true;
            }
        }
    }
    else
    {
        // FREE MODE: no host timeline. Keep dancing at the best-known tempo.
        syncStatus = SyncStatus::FreeMode;
        playing = true;
        beat += beatsPerSecond * dtSeconds;
    }

    if (! std::isfinite (beat))
    {
        beat = 0.0;
        discontinuity = true;
    }

    wasPlaying = playing;
}

double BeatClock::beatPhase() const
{
    return fract (beat);
}

double BeatClock::barPhase() const
{
    const int num = std::max (1, sigNumerator);
    return fract (beat / double (num));
}
} // namespace lumi
