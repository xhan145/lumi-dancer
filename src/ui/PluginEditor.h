// LUMI//DANCER — the plugin editor: top bar, centre stage, dance panel,
// reaction panel, look bar, compact mode.
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "plugin/PluginProcessor.h"
#include "ui/StageComponent.h"
#include "ui/Theme.h"

class LumiDancerEditor : public juce::AudioProcessorEditor,
                         private juce::ChangeListener
{
public:
    explicit LumiDancerEditor (LumiDancerProcessor& processorIn);
    ~LumiDancerEditor() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void changeListenerCallback (juce::ChangeBroadcaster* source) override;
    void buildPresetMenu();
    void refreshControlsFromSettings();
    void mutateSettings (const std::function<void (lumi::LumiSettings&)>& mutate);
    void updateCompactMode();
    void regenerateRoutine();

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    LumiDancerProcessor& processor;
    lumi::theme::LumiLookAndFeel lookAndFeel;
    juce::TooltipWindow tooltips { this, 600 };

    StageComponent stage { processor };

    // ------------------------------------------------------------- top bar
    juce::Label titleLabel, bpmLabel, syncLabel;
    juce::ComboBox presetBox;
    juce::TextButton detachButton { "Detach" };
    juce::TextButton overlayMenuButton { "v" };

    // ----------------------------------------------------- left (dance)
    juce::Label danceHeader, moodLabel, seedLabel, routineHeader;
    juce::ComboBox styleBox, moodBox;
    juce::TextButton newSeedButton { "New Seed" };
    juce::ToggleButton seedLockToggle { "Lock Seed" };
    juce::ToggleButton useRoutineToggle { "Use Routine" };
    juce::ComboBox routineBarsBox, playbackModeBox;
    juce::TextButton newRoutineButton { "New Routine" };
    juce::Slider energySlider, cutenessSlider;
    juce::Label energyLabel, cutenessLabel;

    // -------------------------------------------------- right (reaction)
    juce::Label reactionHeader, fxHeader;
    juce::Slider intensitySlider, reactionSlider, lowSlider, midSlider, highSlider,
                 transientSlider, smoothingSlider, particleSlider;
    juce::Label intensityLabel, reactionLabel, lowLabel, midLabel, highLabel,
                transientLabel, smoothingLabel, particleLabel;
    juce::ToggleButton beatLockToggle { "Beat Lock" };
    juce::ToggleButton reducedMotionToggle { "Reduced Motion" };
    juce::ToggleButton noFlashToggle { "No Flashes" };
    juce::ToggleButton highContrastToggle { "High Contrast" };

    // ------------------------------------------------------ bottom (look)
    juce::ComboBox outfitBox, hairBox, accentBox, backgroundBox, cameraBox, fpsBox;
    juce::TextButton accessoriesButton { "Accessories" };
    juce::ToggleButton freezeToggle { "Freeze" };
    juce::TextButton randomizeButton { "Randomize" };
    juce::Slider scaleSlider;
    juce::ToggleButton bypassToggle { "Bypass" };

    // Parameter attachments.
    std::unique_ptr<ComboAttachment> styleAttachment, moodAttachment;
    std::unique_ptr<SliderAttachment> intensityAttachment, reactionAttachment,
        lowAttachment, midAttachment, highAttachment, transientAttachment,
        smoothingAttachment, particleAttachment, scaleAttachment;
    std::unique_ptr<ButtonAttachment> beatLockAttachment, reducedMotionAttachment,
        bypassAttachment;

    // Periodic BPM/sync/detach status refresh (see constructor).
    std::unique_ptr<juce::Timer> statusTimer;

    // Panel plate rectangles for paint().
    juce::Rectangle<int> topBarBounds, leftPanelBounds, rightPanelBounds, bottomBarBounds;

    bool compactMode = false;
    bool refreshing = false;    // guards control callbacks during rebuilds

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LumiDancerEditor)
};
