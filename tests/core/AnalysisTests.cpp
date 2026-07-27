// Audio analysis tests: RMS, peak, band energies, transients, centroid,
// stereo width, silence, NaN robustness.
#include "TestFramework.h"

#include <array>
#include <vector>

#include "analysis/AnalysisEngine.h"
#include "core/LumiMath.h"

using namespace lumi;

namespace
{
constexpr double kSr = 48000.0;
constexpr int kBlock = 512;

struct SignalFeeder
{
    AnalysisEngine engine;
    std::vector<float> left, right;
    double phaseL = 0.0;

    SignalFeeder()
    {
        engine.prepare (kSr, kBlock);
        left.resize (kBlock);
        right.resize (kBlock);
    }

    // Feed `seconds` of a sine at `freq` with amplitude `amp` (stereo copy).
    const AudioReactiveFrame& feedSine (float freq, float amp, double seconds,
                                        bool invertRight = false)
    {
        const int blocks = int (seconds * kSr / kBlock);
        const AudioReactiveFrame* out = &engine.latestFrame();
        for (int b = 0; b < blocks; ++b)
        {
            for (int i = 0; i < kBlock; ++i)
            {
                left[size_t (i)] = amp * float (std::sin (phaseL));
                right[size_t (i)] = invertRight ? -left[size_t (i)] : left[size_t (i)];
                phaseL += kTwoPi * freq / kSr;
            }
            const float* chans[2] = { left.data(), right.data() };
            out = &engine.process (chans, 2, kBlock);
        }
        return *out;
    }

    const AudioReactiveFrame& feedSilence (double seconds)
    {
        return feedSine (100.0f, 0.0f, seconds);
    }
};
} // namespace

LD_TEST (analysis_rms_and_peak_track_level)
{
    SignalFeeder f;
    const auto& frame = f.feedSine (440.0f, 0.5f, 1.0);

    // Sine RMS = amp / sqrt(2) ≈ 0.354.
    LD_NEAR (frame.rms, 0.354f, 0.06f);
    LD_NEAR (frame.peak, 0.5f, 0.08f);
    LD_CHECK (! frame.silence);
}

LD_TEST (analysis_band_energies_separate_frequencies)
{
    {
        SignalFeeder f;
        const auto& lo = f.feedSine (60.0f, 0.5f, 1.0);
        LD_GT (lo.lowEnergy, 0.5f);
        LD_GT (lo.lowEnergy, lo.highEnergy + 0.3f);
    }
    {
        SignalFeeder f;
        const auto& mid = f.feedSine (800.0f, 0.5f, 1.0);
        LD_GT (mid.midEnergy, 0.5f);
        LD_GT (mid.midEnergy, mid.lowEnergy + 0.2f);
        LD_GT (mid.midEnergy, mid.highEnergy + 0.2f);
    }
    {
        SignalFeeder f;
        const auto& hi = f.feedSine (8000.0f, 0.5f, 1.0);
        LD_GT (hi.highEnergy, 0.5f);
        LD_GT (hi.highEnergy, hi.lowEnergy + 0.3f);
    }
}

LD_TEST (analysis_spectral_centroid_orders_bright_vs_dark)
{
    SignalFeeder dark, bright;
    const float darkCentroid   = dark.feedSine (100.0f, 0.5f, 1.0).spectralCentroid;
    const float brightCentroid = bright.feedSine (10000.0f, 0.5f, 1.0).spectralCentroid;
    LD_GT (brightCentroid, darkCentroid + 0.15f);
    LD_GE (darkCentroid, 0.0f);
    LD_LE (brightCentroid, 1.0f);
}

LD_TEST (analysis_transients_fire_on_bursts_not_steady_tone)
{
    SignalFeeder f;
    f.feedSine (60.0f, 0.4f, 1.5);
    const float steady = f.engine.latestFrame().lowTransient;

    // Silence, then a sudden loud low burst = kick-like onset.
    f.feedSilence (0.3);
    const auto& hit = f.feedSine (60.0f, 0.9f, 0.03);
    LD_GT (hit.lowTransient, steady + 0.2f);
    LD_GT (hit.transientProbability, 0.2f);
}

LD_TEST (analysis_silence_detected_and_recovers)
{
    SignalFeeder f;
    const auto& quiet = f.feedSilence (1.0);
    LD_CHECK (quiet.silence);
    LD_NEAR (quiet.rms, 0.0f, 1e-4);

    const auto& loud = f.feedSine (440.0f, 0.3f, 0.5);
    LD_CHECK (! loud.silence);
}

LD_TEST (analysis_stereo_width_mono_vs_inverted)
{
    SignalFeeder mono, wide;
    const float monoWidth = mono.feedSine (440.0f, 0.4f, 1.0, false).stereoWidth;
    const float wideWidth = wide.feedSine (440.0f, 0.4f, 1.0, true).stereoWidth;
    LD_NEAR (monoWidth, 0.0f, 0.05f);
    LD_GT (wideWidth, 0.6f);   // fully inverted = maximally decorrelated
}

LD_TEST (analysis_survives_nan_and_inf_input)
{
    SignalFeeder f;
    std::array<float, kBlock> bad {};
    bad.fill (std::numeric_limits<float>::quiet_NaN());
    bad[10] = std::numeric_limits<float>::infinity();
    const float* chans[2] = { bad.data(), bad.data() };

    const auto& frame = f.engine.process (chans, 2, kBlock);
    LD_CHECK (std::isfinite (frame.rms));
    LD_CHECK (std::isfinite (frame.peak));
    LD_CHECK (std::isfinite (frame.lowEnergy));
    LD_CHECK (std::isfinite (frame.spectralCentroid));

    // And it keeps working on good audio afterwards.
    const auto& after = f.feedSine (440.0f, 0.4f, 0.5);
    LD_GT (after.rms, 0.1f);
}

LD_TEST (analysis_mono_input_supported)
{
    AnalysisEngine engine;
    engine.prepare (kSr, kBlock);
    std::vector<float> buf (kBlock);
    double phase = 0.0;
    const AudioReactiveFrame* out = nullptr;
    for (int b = 0; b < 90; ++b)
    {
        for (int i = 0; i < kBlock; ++i)
        {
            buf[size_t (i)] = 0.5f * float (std::sin (phase));
            phase += kTwoPi * 440.0 / kSr;
        }
        const float* chans[1] = { buf.data() };
        out = &engine.process (chans, 1, kBlock);
    }
    LD_CHECK (out != nullptr);
    LD_GT (out->rms, 0.2f);
    LD_NEAR (out->stereoWidth, 0.0f, 0.05f);
}

LD_TEST (analysis_adaptive_reference_keeps_quiet_mixes_alive)
{
    // A quiet mix should still reach solid normalised band energy once the
    // adaptive reference settles.
    SignalFeeder f;
    const auto& frame = f.feedSine (60.0f, 0.05f, 3.0);
    LD_GT (frame.lowEnergy, 0.4f);
}
