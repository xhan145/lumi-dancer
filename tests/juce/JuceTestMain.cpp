// LUMI//DANCER — JUCE-linked integration tests.
//
// Covers the audio contract (bit-exact pass-through), host-sync plumbing,
// parameters, state round-trips, presets, editor lifecycle and overlay
// arbitration. Console app; returns nonzero on any failure.
#include <cstring>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>

#include "JuceTestFramework.h"
#include "constellation/RoutineEngine.h"
#include "plugin/Parameters.h"
#include "plugin/PluginProcessor.h"
#include "state/Presets.h"
#include "ui/OverlayWindow.h"
#include "ui/PluginEditor.h"

// -------------------------------------------------------------- utilities
namespace
{
class FakePlayHead : public juce::AudioPlayHead
{
public:
    juce::Optional<juce::AudioPlayHead::PositionInfo> getPosition() const override
    {
        juce::AudioPlayHead::PositionInfo info;
        info.setBpm (bpm);
        info.setPpqPosition (ppq);
        info.setIsPlaying (playing);
        info.setIsRecording (false);
        info.setTimeSignature (juce::AudioPlayHead::TimeSignature { 4, 4 });
        info.setTimeInSamples (samplePos);
        return info;
    }

    double bpm = 120.0, ppq = 0.0;
    bool playing = false;
    int64_t samplePos = 0;
};

enum class Signal { Silence, Sine, Noise, Impulse };

void fillSignal (juce::AudioBuffer<float>& buffer, Signal type, double sampleRate,
                 double& phase, juce::Random& random)
{
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        double p = phase;
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float v = 0.0f;
            switch (type)
            {
                case Signal::Sine:
                    v = 0.5f * float (std::sin (p));
                    p += 2.0 * juce::MathConstants<double>::pi * 220.0 / sampleRate;
                    break;
                case Signal::Noise:
                    v = random.nextFloat() * 0.8f - 0.4f;
                    break;
                case Signal::Impulse:
                    v = (i % 1000 == 0) ? 0.9f : 0.0f;
                    break;
                case Signal::Silence:
                default:
                    break;
            }
            buffer.setSample (ch, i, v);
        }
        if (ch == buffer.getNumChannels() - 1)
            phase = p;
    }
}

// Runs `blocks` blocks through a fresh processor; verifies bit-exact
// pass-through and returns the last analysis frame.
lumi::AudioReactiveFrame runPassThrough (jt::Ctx& _ctx, Signal type, int numChannels,
                                         double sampleRate, int blockSize,
                                         int blocks = 40)
{
    LumiDancerProcessor processor;
    FakePlayHead playHead;
    playHead.playing = true;
    processor.setPlayHead (&playHead);
    processor.setPlayConfigDetails (numChannels, numChannels, sampleRate, blockSize);
    processor.prepareToPlay (sampleRate, blockSize);

    juce::AudioBuffer<float> buffer (numChannels, blockSize);
    juce::AudioBuffer<float> reference (numChannels, blockSize);
    juce::MidiBuffer midi;
    juce::Random random (42);
    double phase = 0.0;

    bool bitExact = true;
    bool finite = true;
    for (int b = 0; b < blocks; ++b)
    {
        fillSignal (buffer, type, sampleRate, phase, random);
        for (int ch = 0; ch < numChannels; ++ch)
            reference.copyFrom (ch, 0, buffer, ch, 0, blockSize);

        playHead.ppq += double (blockSize) / sampleRate * 2.0;
        playHead.samplePos += blockSize;
        processor.processBlock (buffer, midi);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float* out = buffer.getReadPointer (ch);
            const float* ref = reference.getReadPointer (ch);
            for (int i = 0; i < blockSize; ++i)
            {
                if (out[i] != ref[i])
                    bitExact = false;                 // bit-exact, zero tolerance
                if (! std::isfinite (out[i]))
                    finite = false;
            }
        }
    }
    JT_CHECK (bitExact);
    JT_CHECK (finite);
    processor.setPlayHead (nullptr);
    return processor.frameBus.read();
}
} // namespace

// ------------------------------------------------------------ audio tests
JT_TEST (audio_pass_through_is_bit_exact_all_signals)
{
    for (const Signal type : { Signal::Silence, Signal::Sine, Signal::Noise, Signal::Impulse })
        for (const int channels : { 1, 2 })
            runPassThrough (_ctx, type, channels, 48000.0, 512);
}

JT_TEST (audio_pass_through_across_rates_and_block_sizes)
{
    for (const double rate : { 44100.0, 48000.0, 96000.0 })
        runPassThrough (_ctx, Signal::Sine, 2, rate, 512, 20);
    for (const int blockSize : { 64, 128, 256, 512, 1024 })
        runPassThrough (_ctx, Signal::Noise, 2, 48000.0, blockSize, 20);
}

JT_TEST (audio_silence_stays_silent_and_flags_silence)
{
    const auto frame = runPassThrough (_ctx, Signal::Silence, 2, 48000.0, 512, 60);
    JT_CHECK (frame.silence);
    JT_NEAR (frame.rms, 0.0f, 1e-6);
}

JT_TEST (audio_analysis_frame_reacts_and_stays_finite)
{
    const auto frame = runPassThrough (_ctx, Signal::Sine, 2, 48000.0, 512, 90);
    JT_CHECK (! frame.silence);
    JT_GT (frame.rms, 0.1f);
    JT_GT (frame.midEnergy, 0.3f);   // 220 Hz sine lands in the mid band
    JT_CHECK (std::isfinite (frame.spectralCentroid));
    JT_CHECK (std::isfinite (frame.transientProbability));
}

JT_TEST (audio_nan_input_does_not_poison_analysis)
{
    LumiDancerProcessor processor;
    processor.setPlayConfigDetails (2, 2, 48000.0, 256);
    processor.prepareToPlay (48000.0, 256);

    juce::AudioBuffer<float> buffer (2, 256);
    juce::MidiBuffer midi;
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 256; ++i)
            buffer.setSample (ch, i, std::numeric_limits<float>::quiet_NaN());

    processor.processBlock (buffer, midi);
    const auto frame = processor.frameBus.read();
    JT_CHECK (std::isfinite (frame.rms));
    JT_CHECK (std::isfinite (frame.lowEnergy));
}

JT_TEST (audio_rapid_bypass_toggling_is_stable)
{
    LumiDancerProcessor processor;
    processor.setPlayConfigDetails (2, 2, 48000.0, 256);
    processor.prepareToPlay (48000.0, 256);
    auto* bypass = processor.apvts.getParameter (lumi::params::bypass);

    juce::AudioBuffer<float> buffer (2, 256);
    juce::AudioBuffer<float> reference (2, 256);
    juce::MidiBuffer midi;
    juce::Random random (7);
    double phase = 0.0;

    bool bitExact = true;
    for (int b = 0; b < 100; ++b)
    {
        bypass->setValueNotifyingHost ((b % 2 == 0) ? 1.0f : 0.0f);
        fillSignal (buffer, Signal::Noise, 48000.0, phase, random);
        for (int ch = 0; ch < 2; ++ch)
            reference.copyFrom (ch, 0, buffer, ch, 0, 256);
        processor.processBlock (buffer, midi);
        for (int ch = 0; ch < 2; ++ch)
            if (std::memcmp (buffer.getReadPointer (ch), reference.getReadPointer (ch),
                             sizeof (float) * 256) != 0)
                bitExact = false;
    }
    JT_CHECK (bitExact);
}

// ------------------------------------------------------------- host sync
JT_TEST (host_sync_timing_reaches_the_bus)
{
    LumiDancerProcessor processor;
    FakePlayHead playHead;
    playHead.bpm = 174.0;
    playHead.ppq = 32.5;
    playHead.playing = true;
    processor.setPlayHead (&playHead);
    processor.setPlayConfigDetails (2, 2, 48000.0, 256);
    processor.prepareToPlay (48000.0, 256);

    juce::AudioBuffer<float> buffer (2, 256);
    buffer.clear();
    juce::MidiBuffer midi;
    processor.processBlock (buffer, midi);

    const auto timing = processor.timingBus.read();
    JT_CHECK (timing.hasBpm);
    JT_NEAR (timing.bpm, 174.0, 1e-9);
    JT_CHECK (timing.hasPpq);
    JT_NEAR (timing.ppq, 32.5, 1e-9);
    JT_CHECK (timing.isPlaying);
    JT_EQ (timing.timeSigNumerator, 4);
    processor.setPlayHead (nullptr);
}

JT_TEST (host_sync_missing_playhead_is_safe)
{
    LumiDancerProcessor processor;
    processor.setPlayConfigDetails (2, 2, 48000.0, 256);
    processor.prepareToPlay (48000.0, 256);
    juce::AudioBuffer<float> buffer (2, 256);
    buffer.clear();
    juce::MidiBuffer midi;
    processor.processBlock (buffer, midi);

    const auto timing = processor.timingBus.read();
    JT_CHECK (! timing.hasPpq);   // triggers FREE MODE downstream
    JT_CHECK (! timing.isPlaying);
}

// ------------------------------------------------------------ parameters
JT_TEST (parameters_stable_ids_exist_with_sane_ranges)
{
    LumiDancerProcessor processor;
    const char* ids[] = {
        lumi::params::danceStyle, lumi::params::intensity, lumi::params::reactionAmount,
        lumi::params::lowSens, lumi::params::midSens, lumi::params::highSens,
        lumi::params::transientSens, lumi::params::smoothing, lumi::params::particleAmount,
        lumi::params::mood, lumi::params::routinePosition, lumi::params::visualScale,
        lumi::params::mirror, lumi::params::visualOpacity, lumi::params::beatLock,
        lumi::params::reducedMotion, lumi::params::bypass,
    };
    for (const char* id : ids)
    {
        auto* param = processor.apvts.getParameter (id);
        _ctx.report (param != nullptr, (std::string ("param exists: ") + id).c_str(),
                     __FILE__, __LINE__);
    }

    // Choice ranges.
    auto* style = dynamic_cast<juce::AudioParameterChoice*> (
        processor.apvts.getParameter (lumi::params::danceStyle));
    JT_CHECK (style != nullptr && style->choices.size() == 10);

    JT_CHECK (processor.getBypassParameter() != nullptr);
}

// ----------------------------------------------------------------- state
JT_TEST (state_full_round_trip_via_binary_chunk)
{
    juce::MemoryBlock chunk;
    lumi::LumiSettings savedSettings;

    {
        LumiDancerProcessor source;
        source.apvts.getParameter (lumi::params::danceStyle)
            ->setValueNotifyingHost (6.0f / 9.0f);   // Breakcore
        source.apvts.getParameter (lumi::params::intensity)->setValueNotifyingHost (0.75f);

        lumi::LumiSettings s = source.getSettings();
        s.outfit = 3;
        s.hairPalette = 2;
        s.accessories = lumi::accHeadphones;
        s.useRoutine = true;
        lumi::ChoreoParams cp;
        s.routineData = lumi::serializeRoutine (lumi::generateRoutine (4, 4, 99, cp));
        s.overlay.x = 222;
        s.overlay.y = 111;
        s.accessibility.reducedMotion = true;
        source.setSettings (s, false);
        savedSettings = s;

        source.getStateInformation (chunk);
        JT_GT (chunk.getSize(), size_t (100));
    }

    LumiDancerProcessor target;
    target.setStateInformation (chunk.getData(), int (chunk.getSize()));

    const auto restored = target.getSettings();
    JT_EQ (restored.outfit, savedSettings.outfit);
    JT_EQ (restored.hairPalette, savedSettings.hairPalette);
    JT_EQ (restored.accessories, savedSettings.accessories);
    JT_EQ (restored.useRoutine, savedSettings.useRoutine);
    JT_EQ (restored.overlay.x, savedSettings.overlay.x);
    JT_EQ (restored.overlay.y, savedSettings.overlay.y);
    JT_EQ (restored.accessibility.reducedMotion, true);
    JT_CHECK (restored.routineData == savedSettings.routineData);

    const float styleValue = target.apvts.getRawParameterValue (lumi::params::danceStyle)->load();
    JT_NEAR (styleValue, 6.0f, 0.01f);
    JT_NEAR (target.apvts.getRawParameterValue (lumi::params::intensity)->load(), 1.5f, 0.01f);
}

JT_TEST (state_corrupt_chunk_falls_back_safely)
{
    LumiDancerProcessor processor;
    const char garbage[] = "this is definitely not a valid plugin state chunk";
    processor.setStateInformation (garbage, int (sizeof (garbage)));

    const auto settings = processor.getSettings();
    JT_EQ (settings.danceStyle, lumi::LumiSettings {}.danceStyle);

    // And audio still works afterwards.
    processor.setPlayConfigDetails (2, 2, 48000.0, 256);
    processor.prepareToPlay (48000.0, 256);
    juce::AudioBuffer<float> buffer (2, 256);
    buffer.clear();
    juce::MidiBuffer midi;
    processor.processBlock (buffer, midi);
    JT_CHECK (std::isfinite (processor.frameBus.read().rms));
}

JT_TEST (state_presets_apply_meaningful_values)
{
    LumiDancerProcessor processor;
    const int index = lumi::findFactoryPreset ("Breakcore Sprite");
    JT_CHECK (index >= 0);
    processor.applyPreset (index);

    JT_NEAR (processor.apvts.getRawParameterValue (lumi::params::danceStyle)->load(),
             6.0f, 0.01f);   // Breakcore
    JT_GT (processor.apvts.getRawParameterValue (lumi::params::transientSens)->load(), 1.4f);
    JT_EQ (processor.getSettings().outfit, int (lumi::Outfit::CosmicStreetwear));
}

// -------------------------------------------------------- gui + overlay
JT_TEST (editor_creates_resizes_and_destroys)
{
    LumiDancerProcessor processor;
    std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());
    JT_CHECK (editor != nullptr);
    editor->setSize (900, 650);
    JT_EQ (editor->getWidth(), 900);

    // Compact mode: shrink below the threshold and relayout.
    editor->setSize (500, 340);
    editor->setSize (1100, 760);

    // Editor size persists into settings.
    JT_EQ (processor.getSettings().editorWidth, 1100);
}

JT_TEST (overlay_controller_arbitration)
{
    auto& controller = OverlayController::instance();

    {
        LumiDancerProcessor a, b;
        JT_CHECK (! controller.hasOverlay());

        controller.claim (a);
        JT_CHECK (controller.hasOverlay());
        JT_CHECK (controller.isOwnedBy (&a));
        JT_CHECK (! controller.isOwnedBy (&b));
        JT_CHECK (a.getSettings().overlay.enabled);

        // Second instance steals politely.
        controller.claim (b);
        JT_CHECK (controller.isOwnedBy (&b));
        JT_CHECK (! controller.isOwnedBy (&a));

        // Releasing from a non-owner is a no-op.
        controller.release (a);
        JT_CHECK (controller.isOwnedBy (&b));

        // Owner release closes.
        controller.release (b);
        JT_CHECK (! controller.hasOverlay());
        JT_CHECK (! b.getSettings().overlay.enabled);

        // Toggle claims, processor death tears down synchronously.
        controller.toggle (b);
        JT_CHECK (controller.isOwnedBy (&b));
    }   // b's destructor runs notifyProcessorDying

    JT_CHECK (! controller.hasOverlay());
}

JT_TEST (overlay_geometry_persists)
{
    auto& controller = OverlayController::instance();
    LumiDancerProcessor processor;

    lumi::LumiSettings s = processor.getSettings();
    s.overlay.x = 300;
    s.overlay.y = 200;
    s.overlay.w = 500;
    s.overlay.h = 480;
    processor.setSettings (s, false);

    controller.claim (processor);
    const auto after = processor.getSettings();
    JT_EQ (after.overlay.w, 500);
    JT_EQ (after.overlay.h, 480);
    controller.release (processor);
}

JT_TEST (multiple_instances_coexist)
{
    std::vector<std::unique_ptr<LumiDancerProcessor>> instances;
    for (int i = 0; i < 4; ++i)
        instances.push_back (std::make_unique<LumiDancerProcessor>());

    juce::AudioBuffer<float> buffer (2, 256);
    juce::MidiBuffer midi;
    for (auto& instance : instances)
    {
        instance->setPlayConfigDetails (2, 2, 48000.0, 256);
        instance->prepareToPlay (48000.0, 256);
        buffer.clear();
        instance->processBlock (buffer, midi);
    }

    // Distinct instance numbers.
    JT_CHECK (instances[0]->instanceNumber != instances[1]->instanceNumber);
    instances.clear();   // clean destruction
    JT_CHECK (true);
}

// ------------------------------------------------------------------ main
int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    jt::Ctx ctx;
    int caseFails = 0;
    std::printf ("LUMI//DANCER JUCE tests\n=======================\n");
    for (auto& testCase : jt::registry())
    {
        ctx.current = testCase.name;
        const int before = ctx.fails;
        testCase.fn (ctx);
        const bool ok = ctx.fails == before;
        if (! ok)
            ++caseFails;
        std::printf ("  [%s] %s\n", ok ? "PASS" : "FAIL", testCase.name.c_str());
    }
    std::printf ("=======================\n%zu cases, %d checks, %d failures\n",
                 jt::registry().size(), ctx.checks, ctx.fails);
    return caseFails == 0 ? 0 : 1;
}
