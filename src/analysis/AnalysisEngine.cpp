#include "analysis/AnalysisEngine.h"

#include <cmath>

#include "core/LumiMath.h"

namespace lumi
{
void Biquad::makeLowpass (float freq, float sampleRate, float q)
{
    const float w  = kTwoPi * freq / sampleRate;
    const float cw = std::cos (w);
    const float sw = std::sin (w);
    const float alpha = sw / (2.0f * q);
    const float a0 = 1.0f + alpha;

    b0 = ((1.0f - cw) * 0.5f) / a0;
    b1 = (1.0f - cw) / a0;
    b2 = b0;
    a1 = (-2.0f * cw) / a0;
    a2 = (1.0f - alpha) / a0;
    reset();
}

void Biquad::makeHighpass (float freq, float sampleRate, float q)
{
    const float w  = kTwoPi * freq / sampleRate;
    const float cw = std::cos (w);
    const float sw = std::sin (w);
    const float alpha = sw / (2.0f * q);
    const float a0 = 1.0f + alpha;

    b0 = ((1.0f + cw) * 0.5f) / a0;
    b1 = -(1.0f + cw) / a0;
    b2 = b0;
    a1 = (-2.0f * cw) / a0;
    a2 = (1.0f - alpha) / a0;
    reset();
}

AnalysisEngine::AnalysisEngine()
{
    prepare (48000.0, 512);
}

void AnalysisEngine::prepare (double sampleRate, int /*maxBlockSize*/)
{
    sr = (sampleRate > 1000.0 && sampleRate < 1000000.0) ? sampleRate : 48000.0;
    const float fs = float (sr);

    low1.makeLowpass  (150.0f,  fs);
    low2.makeLowpass  (150.0f,  fs);
    midHp1.makeHighpass (150.0f,  fs);
    midHp2.makeHighpass (150.0f,  fs);
    midLp1.makeLowpass  (2000.0f, fs);
    midLp2.makeLowpass  (2000.0f, fs);
    high1.makeHighpass (2000.0f, fs);
    high2.makeHighpass (2000.0f, fs);

    // Per-sample envelope coefficients.
    envUp    = 1.0f - onePoleCoeff (0.010f, fs);   // 10 ms attack
    envDown  = 1.0f - onePoleCoeff (0.180f, fs);   // 180 ms release
    fastUp   = 1.0f - onePoleCoeff (0.001f, fs);
    fastDown = 1.0f - onePoleCoeff (0.060f, fs);
    slowUp   = 1.0f - onePoleCoeff (0.080f, fs);
    slowDown = 1.0f - onePoleCoeff (0.400f, fs);
    refDecay = 1.0f - onePoleCoeff (6.0f,   fs);   // adaptive reference falls over ~6 s
    transientDecay = 1.0f - onePoleCoeff (0.120f, fs);

    for (int i = 0; i < kFftSize; ++i)
        window[size_t (i)] = 0.5f - 0.5f * std::cos (kTwoPi * float (i) / float (kFftSize));

    reset();
}

void AnalysisEngine::reset()
{
    low1.reset(); low2.reset();
    midHp1.reset(); midHp2.reset(); midLp1.reset(); midLp2.reset();
    high1.reset(); high2.reset();
    bands = {};
    rmsSmooth = peakHold = widthSmooth = 0.0f;
    centroidSmooth = 0.0f;
    silenceSeconds = 1.0f;
    ring.fill (0.0f);
    ringWrite = 0;
    samplesSinceFft = 0;
    samplePos = 0;
    frame = {};
}

const AudioReactiveFrame& AnalysisEngine::process (const float* const* channels,
                                                   int numChannels, int numSamples)
{
    if (channels == nullptr || numChannels <= 0 || numSamples <= 0)
        return frame;

    const float* left  = channels[0];
    const float* right = numChannels > 1 ? channels[1] : nullptr;
    if (left == nullptr)
        return frame;

    double sumSq = 0.0;
    float blockPeak = 0.0f;
    double sumLL = 0.0, sumRR = 0.0, sumLR = 0.0;

    for (int i = 0; i < numSamples; ++i)
    {
        const float l = sanitize (left[i]);
        const float r = right != nullptr ? sanitize (right[i]) : l;
        const float mono = 0.5f * (l + r);

        sumSq += double (mono) * double (mono);
        blockPeak = std::max (blockPeak, std::max (std::fabs (l), std::fabs (r)));
        sumLL += double (l) * double (l);
        sumRR += double (r) * double (r);
        sumLR += double (l) * double (r);

        // Band split (cascaded biquads = 24 dB/oct crossovers).
        const float lo = low2.process (low1.process (mono));
        const float md = midLp2.process (midLp1.process (midHp2.process (midHp1.process (mono))));
        const float hi = high2.process (high1.process (mono));

        const float rect[3] = { std::fabs (lo), std::fabs (md), std::fabs (hi) };
        for (int b = 0; b < 3; ++b)
        {
            BandState& s = bands[size_t (b)];
            const float v = rect[b];
            s.env     += (v > s.env     ? envUp   : envDown)  * (v - s.env);
            s.fastEnv += (v > s.fastEnv ? fastUp  : fastDown) * (v - s.fastEnv);
            s.slowEnv += (v > s.slowEnv ? slowUp  : slowDown) * (v - s.slowEnv);

            // Onset: fast envelope exceeding the slow envelope by a margin.
            const float excess = s.fastEnv - 1.8f * s.slowEnv - 1.0e-4f;
            if (excess > 0.0f)
            {
                const float strength = clamp01 (excess / (s.slowEnv + 1.0e-3f));
                s.transient = std::max (s.transient, strength);
            }
            s.transient -= transientDecay * s.transient;

            // Adaptive normalisation reference: tracks recent loud level.
            if (s.env > s.slowRef)
                s.slowRef = s.env;
            else
                s.slowRef = std::max (1.0e-4f, s.slowRef - refDecay * s.slowRef);
        }

        // FFT feed (mono).
        ring[size_t (ringWrite)] = mono;
        ringWrite = (ringWrite + 1) & (kFftSize - 1);
    }

    samplesSinceFft += numSamples;
    if (samplesSinceFft >= kFftSize / 2)
    {
        samplesSinceFft = 0;
        computeSpectrum();
    }

    samplePos += numSamples;

    // ------------------------------------------------------------ block stats
    const float blockRms = float (std::sqrt (sumSq / double (numSamples)));
    const float blockSeconds = float (double (numSamples) / sr);
    const float rmsCoeff = blockRms > rmsSmooth ? 0.5f : 0.2f;
    rmsSmooth += rmsCoeff * (blockRms - rmsSmooth);

    peakHold = std::max (blockPeak, peakHold * (1.0f - 2.5f * blockSeconds));

    // Stereo width from normalised correlation: 0 = mono, 1 = fully decorrelated.
    float width = 0.0f;
    if (right != nullptr && sumLL > 1.0e-12 && sumRR > 1.0e-12)
    {
        const float corr = float (sumLR / std::sqrt (sumLL * sumRR));
        width = clamp01 (0.5f * (1.0f - corr));
    }
    widthSmooth += 0.1f * (width - widthSmooth);

    // Silence detection: sustained level under -70 dBFS.
    if (blockRms < dbToGain (-70.0f))
        silenceSeconds += blockSeconds;
    else
        silenceSeconds = 0.0f;

    // ------------------------------------------------------------- the frame
    frame.absoluteSample = samplePos;
    frame.rms  = sanitize (rmsSmooth);
    frame.peak = sanitize (peakHold);

    // Normalise every band by one shared adaptive reference so relative band
    // balance survives (a bright hat stays "high", a kick stays "low") while
    // the overall scale still adapts to quiet mixes.
    const float globalRef = std::max ({ bands[0].slowRef, bands[1].slowRef,
                                        bands[2].slowRef, 1.0e-4f });
    const auto normBand = [globalRef] (const BandState& s)
    {
        return clamp01 (s.env / globalRef);
    };
    frame.lowEnergy  = normBand (bands[0]);
    frame.midEnergy  = normBand (bands[1]);
    frame.highEnergy = normBand (bands[2]);

    frame.lowTransient  = sanitize (clamp01 (bands[0].transient));
    frame.midTransient  = sanitize (clamp01 (bands[1].transient));
    frame.highTransient = sanitize (clamp01 (bands[2].transient));
    frame.transientProbability = std::max ({ frame.lowTransient, frame.midTransient, frame.highTransient });

    frame.spectralCentroid = sanitize (centroidSmooth);
    frame.stereoWidth      = sanitize (widthSmooth);
    frame.silence          = silenceSeconds > 0.25f;

    return frame;
}

void AnalysisEngine::computeSpectrum()
{
    // Copy the ring buffer (oldest first) into the windowed FFT input.
    for (int i = 0; i < kFftSize; ++i)
    {
        const int idx = (ringWrite + i) & (kFftSize - 1);
        fftRe[size_t (i)] = ring[size_t (idx)] * window[size_t (i)];
        fftIm[size_t (i)] = 0.0f;
    }

    // Iterative in-place radix-2 Cooley-Tukey (256 points, preallocated).
    int j = 0;
    for (int i = 0; i < kFftSize - 1; ++i)
    {
        if (i < j)
        {
            std::swap (fftRe[size_t (i)], fftRe[size_t (j)]);
            std::swap (fftIm[size_t (i)], fftIm[size_t (j)]);
        }
        int k = kFftSize >> 1;
        while (k <= j) { j -= k; k >>= 1; }
        j += k;
    }
    for (int len = 2; len <= kFftSize; len <<= 1)
    {
        const float ang = -kTwoPi / float (len);
        const float wr = std::cos (ang), wi = std::sin (ang);
        for (int i = 0; i < kFftSize; i += len)
        {
            float cr = 1.0f, ci = 0.0f;
            for (int k = 0; k < len / 2; ++k)
            {
                const int a = i + k, b = i + k + len / 2;
                const float tr = cr * fftRe[size_t (b)] - ci * fftIm[size_t (b)];
                const float ti = cr * fftIm[size_t (b)] + ci * fftRe[size_t (b)];
                fftRe[size_t (b)] = fftRe[size_t (a)] - tr;
                fftIm[size_t (b)] = fftIm[size_t (a)] - ti;
                fftRe[size_t (a)] += tr;
                fftIm[size_t (a)] += ti;
                const float ncr = cr * wr - ci * wi;
                ci = cr * wi + ci * wr;
                cr = ncr;
            }
        }
    }

    // Spectral centroid over bins 1..N/2, normalised by Nyquist.
    double num = 0.0, den = 0.0;
    for (int bin = 1; bin < kFftSize / 2; ++bin)
    {
        const float re = fftRe[size_t (bin)], im = fftIm[size_t (bin)];
        const double mag = std::sqrt (double (re) * re + double (im) * im);
        num += mag * bin;
        den += mag;
    }
    if (den > 1.0e-9)
    {
        const float centroid = clamp01 (float (num / den) / float (kFftSize / 2));
        centroidSmooth += 0.25f * (centroid - centroidSmooth);
    }
}
} // namespace lumi
