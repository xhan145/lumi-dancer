// LUMI//DANCER — real-time audio analysis.
//
// Runs entirely on the audio thread inside processBlock. Everything is
// preallocated in prepare(); process() performs no allocation, no locking,
// no I/O. The engine never modifies the audio it reads.
#pragma once

#include <array>
#include <cstdint>

#include "analysis/AudioReactiveFrame.h"

namespace lumi
{
// Simple biquad used for the band-split filters (RBJ cookbook coefficients).
struct Biquad
{
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
    float z1 = 0.0f, z2 = 0.0f;

    void reset() { z1 = z2 = 0.0f; }

    float process (float x)
    {
        const float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }

    void makeLowpass (float freq, float sampleRate, float q = 0.7071f);
    void makeHighpass (float freq, float sampleRate, float q = 0.7071f);
};

class AnalysisEngine
{
public:
    static constexpr int kFftOrder = 8;             // 256-point FFT
    static constexpr int kFftSize  = 1 << kFftOrder;

    AnalysisEngine();

    // Message/prepare thread only. Allocates nothing after it returns.
    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    // Audio thread. Reads (never writes) up to the first two channels.
    // Returns the freshly produced frame; also retrievable via latestFrame().
    const AudioReactiveFrame& process (const float* const* channels,
                                       int numChannels, int numSamples);

    const AudioReactiveFrame& latestFrame() const { return frame; }

private:
    void computeSpectrum();

    double sr = 48000.0;
    int64_t samplePos = 0;

    // Band-split filters (12 dB/oct each way, mono downmix).
    Biquad low1, low2;           // lowpass 150 Hz
    Biquad midHp1, midHp2;       // highpass 150 Hz ...
    Biquad midLp1, midLp2;       // ... then lowpass 2 kHz
    Biquad high1, high2;         // highpass 2 kHz

    // Envelope followers.
    struct BandState
    {
        float env      = 0.0f;   // block energy, smoothed
        float fastEnv  = 0.0f;   // transient detector fast follower
        float slowEnv  = 0.0f;   // transient detector slow follower
        float slowRef  = 1.0e-4f;// adaptive normalisation reference
        float transient = 0.0f;  // decaying transient output
    };
    std::array<BandState, 3> bands;   // low, mid, high

    float rmsSmooth  = 0.0f;
    float peakHold   = 0.0f;
    float widthSmooth = 0.0f;
    float centroidSmooth = 0.0f;
    float silenceSeconds = 0.0f;

    // Smoothing coefficients, derived from sample rate in prepare().
    float envUp = 0.0f, envDown = 0.0f;
    float fastUp = 0.0f, fastDown = 0.0f;
    float slowUp = 0.0f, slowDown = 0.0f;
    float refDecay = 0.0f;
    float transientDecay = 0.0f;

    // FFT workspace (preallocated, fed by a mono ring buffer).
    std::array<float, kFftSize> ring {};
    int ringWrite = 0;
    int samplesSinceFft = 0;
    std::array<float, kFftSize> window {};
    std::array<float, kFftSize> fftRe {};
    std::array<float, kFftSize> fftIm {};

    AudioReactiveFrame frame;
};
} // namespace lumi
