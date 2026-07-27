// LUMI//DANCER — the audio processor.
//
// A transparent visualizer: the audio buffer is never written to. The only
// audio-thread work is the preallocated analysis tap plus two lock-free
// snapshot publishes. Everything visual happens on the message thread.
#pragma once

#include <atomic>

#include <juce_audio_processors/juce_audio_processors.h>

#include "analysis/AnalysisEngine.h"
#include "plugin/AnalysisBus.h"
#include "state/Settings.h"
#include "timing/BeatClock.h"

class LumiDancerProcessor : public juce::AudioProcessor
{
public:
    LumiDancerProcessor();
    ~LumiDancerProcessor() override;

    // ------------------------------------------------------- AudioProcessor
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "LUMI//DANCER"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorParameter* getBypassParameter() const override;

    // ----------------------------------------------------------- LUMI state
    juce::AudioProcessorValueTreeState apvts;

    // Audio → UI hand-off (lock-free).
    lumi::AtomicSnapshot<lumi::AudioReactiveFrame> frameBus;
    lumi::AtomicSnapshot<lumi::HostTimingSnapshot> timingBus;

    // Non-automatable persistent settings (message thread only).
    lumi::LumiSettings getSettings() const;
    void setSettings (const lumi::LumiSettings& newSettings, bool notify = true);

    // Applies a factory preset: settings + matching parameter values.
    void applyPreset (int factoryPresetIndex);

    // Fired on setStateInformation / applyPreset so open editors can rebuild.
    juce::ChangeBroadcaster settingsBroadcaster;

    // Distinguishes instances in the overlay arbitration UI.
    const int instanceNumber;

private:
    void pushSettingsToParameters (const lumi::LumiSettings& s);

    static std::atomic<int> instanceCounter;

    lumi::AnalysisEngine analysis;
    lumi::LumiSettings settings;
    mutable juce::CriticalSection settingsLock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LumiDancerProcessor)
};
