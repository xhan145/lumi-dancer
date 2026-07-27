// LUMI//DANCER — the stage: runs the animation loop on the message thread,
// draws background, particles and Lumi. Shared by the plugin editor and the
// detached overlay window (each owns its own stage; both read the same
// lock-free buses from the processor).
#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

#include "dance/Choreographer.h"
#include "fx/ParticleSystem.h"
#include "plugin/PluginProcessor.h"
#include "ui/LumiRenderer.h"

class StageComponent : public juce::Component,
                       private juce::Timer
{
public:
    explicit StageComponent (LumiDancerProcessor& processorIn);
    ~StageComponent() override;

    void paint (juce::Graphics& g) override;

    // Overlay mode: transparent background, no status text.
    void setOverlayMode (bool overlay) { overlayMode = overlay; }

    // Freeze pose (bottom-bar control).
    void setFrozen (bool shouldFreeze) { frozen = shouldFreeze; }
    bool isFrozen() const { return frozen; }

    // Rebuilds cached settings-derived state; call after settings change.
    void refreshFromSettings();

    lumi::SyncStatus syncStatus() const { return clock.status(); }
    double hostBpm() const { return clock.bpm(); }
    float activityLevel() const { return choreographer.activityLevel(); }

private:
    void timerCallback() override;
    void updateFrameRate();
    void drawBackground (juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawStatusHints (juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawParticles (juce::Graphics& g, juce::Rectangle<float> bounds);
    lumi::ChoreographerParams gatherParams (const lumi::LumiSettings& s) const;

    LumiDancerProcessor& processor;
    lumi::BeatClock clock;
    lumi::Choreographer choreographer;
    lumi::ParticleSystem particles;

    lumi::CharacterPose currentPose;
    lumi::LumiSettings cachedSettings;
    juce::Image userBackground;         // loaded lazily off the audio thread
    juce::String userBackgroundPath;
    bool userBackgroundFailed = false;

    double lastTickSeconds = 0.0;
    bool overlayMode = false;
    bool frozen = false;

    // Adaptive frame rate: drop to 30 fps when paints run long.
    float paintMsAverage = 0.0f;
    int currentFps = 60;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StageComponent)
};
