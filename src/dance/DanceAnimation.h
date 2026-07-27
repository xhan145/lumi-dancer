// LUMI//DANCER — dance style interface and registry.
//
// A DanceAnimation is a pure function of musical time and audio analysis:
// evaluate() must be deterministic for identical inputs so host seeks, loops
// and locked seeds reproduce identical choreography. Styles hold no mutable
// per-frame state; anything that looks like memory is derived from the beat
// position (see Freestyle's segment hashing).
#pragma once

#include <memory>

#include "analysis/AudioReactiveFrame.h"
#include "rig/CharacterPose.h"

namespace lumi
{
class DanceAnimation
{
public:
    virtual ~DanceAnimation() = default;

    virtual void reset() = 0;

    virtual CharacterPose evaluate (double beatPosition,
                                    double beatsPerMinute,
                                    const AudioReactiveFrame& audio,
                                    float intensity) const = 0;

    virtual const char* name() const = 0;
};

enum class DanceStyle : int
{
    Bounce = 0,
    KawaiiPop,
    Orbit,
    Groove,
    Hyper,
    Chill,
    Breakcore,
    DrumAndBass,
    Trance,
    Freestyle,
    Count
};

const char* danceStyleName (DanceStyle style);

// Factory. `seed` only influences styles with stochastic choreography
// (Freestyle, Breakcore pose hashing); deterministic for a given seed.
std::unique_ptr<DanceAnimation> createDanceStyle (DanceStyle style, uint64_t seed = 0);
} // namespace lumi
