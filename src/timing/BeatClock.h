// LUMI//DANCER — host synchronisation model.
//
// The audio thread publishes a HostTimingSnapshot each block; the animation
// side feeds snapshots plus its own frame delta into a BeatClock, which
// produces a continuous, seek/loop-safe beat position. Pure C++ so the whole
// recovery behaviour is unit-testable without a host.
#pragma once

#include <cstdint>

namespace lumi
{
struct HostTimingSnapshot
{
    bool   hasBpm      = false;
    double bpm         = 120.0;
    bool   hasPpq      = false;
    double ppq         = 0.0;
    bool   isPlaying   = false;
    bool   isRecording = false;
    bool   isLooping   = false;
    double loopStartPpq = 0.0;
    double loopEndPpq   = 0.0;
    int    timeSigNumerator   = 4;
    int    timeSigDenominator = 4;
    double sampleRate  = 48000.0;
    int64_t samplePos  = 0;
};

enum class SyncStatus : int
{
    HostSync = 0,   // locked to host PPQ
    Stopped,        // host timeline available but transport stopped
    FreeMode,       // no host timeline (standalone / host without transport)
};

class BeatClock
{
public:
    // Advance by dt seconds using the most recent snapshot.
    void update (const HostTimingSnapshot& snapshot, double dtSeconds);

    void reset();

    double beatPosition() const { return beat; }                 // continuous beats
    double beatPhase()    const;                                  // [0,1) within a beat
    double barPhase()     const;                                  // [0,1) within a bar
    double bpm()          const { return currentBpm; }
    int    beatsPerBar()  const { return sigNumerator; }
    bool   isPlaying()    const { return playing; }
    SyncStatus status()   const { return syncStatus; }

    // True for one update() after a discontinuity (seek, loop wrap, restart)
    // so the animation layer can crossfade instead of snapping poses.
    bool discontinuityFlag() const { return discontinuity; }

private:
    double beat = 0.0;
    double currentBpm = 120.0;
    int    sigNumerator = 4;
    bool   playing = false;
    bool   wasPlaying = false;
    bool   discontinuity = false;
    SyncStatus syncStatus = SyncStatus::FreeMode;
};
} // namespace lumi
