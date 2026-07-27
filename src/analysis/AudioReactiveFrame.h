// LUMI//DANCER — the analysis snapshot handed from the audio thread to the
// animation system. Trivially copyable so it can travel through an atomic
// double-buffer without locks.
#pragma once

#include <cstdint>

namespace lumi
{
struct AudioReactiveFrame
{
    int64_t absoluteSample = 0;

    float rms  = 0.0f;
    float peak = 0.0f;

    // Band energies normalised to roughly [0,1] by an adaptive slow-tracking
    // reference so quiet mixes still animate.
    float lowEnergy  = 0.0f;
    float midEnergy  = 0.0f;
    float highEnergy = 0.0f;

    // Overall transient probability plus per-band onsets so the mapping layer
    // can distinguish kicks (low), snares (mid) and hats (high).
    float transientProbability = 0.0f;
    float lowTransient  = 0.0f;
    float midTransient  = 0.0f;
    float highTransient = 0.0f;

    float spectralCentroid = 0.0f;   // 0..1 (0 = dark, 1 = bright)
    float stereoWidth      = 0.0f;   // 0..1 (0 = mono)

    bool silence = true;
};
} // namespace lumi
