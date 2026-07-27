#include "ui/PluginEditor.h"

#include "constellation/RoutineEngine.h"
#include "plugin/Parameters.h"
#include "state/Presets.h"
#include "ui/OverlayWindow.h"

using namespace lumi;

namespace
{
void styleSmallSlider (juce::Slider& slider, juce::Label& label, const char* text,
                       const char* tooltip)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setTooltip (tooltip);
    slider.setTitle (text);                        // screen-reader label
    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::Font (juce::FontOptions (11.0f)));
    label.setInterceptsMouseClicks (false, false);
}

void styleLinearSlider (juce::Slider& slider, const char* title, const char* tooltip)
{
    slider.setSliderStyle (juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setTooltip (tooltip);
    slider.setTitle (title);
}

void styleHeader (juce::Label& label, const char* text)
{
    label.setText (text, juce::dontSendNotification);
    label.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
    label.setColour (juce::Label::textColourId, theme::softGold());
    label.setJustificationType (juce::Justification::centredLeft);
}
} // namespace

LumiDancerEditor::LumiDancerEditor (LumiDancerProcessor& processorIn)
    : AudioProcessorEditor (processorIn), processor (processorIn)
{
    setLookAndFeel (&lookAndFeel);
    setWantsKeyboardFocus (true);

    addAndMakeVisible (stage);

    // -------------------------------------------------------------- top bar
    titleLabel.setText ("LUMI//DANCER", juce::dontSendNotification);
    titleLabel.setFont (juce::Font (juce::FontOptions (17.0f, juce::Font::bold)));
    titleLabel.setColour (juce::Label::textColourId, theme::lavender());
    addAndMakeVisible (titleLabel);

    bpmLabel.setJustificationType (juce::Justification::centredRight);
    bpmLabel.setFont (juce::Font (juce::FontOptions (12.0f)));
    addAndMakeVisible (bpmLabel);

    syncLabel.setJustificationType (juce::Justification::centredRight);
    syncLabel.setFont (juce::Font (juce::FontOptions (11.0f)));
    syncLabel.setColour (juce::Label::textColourId, theme::softGold());
    addAndMakeVisible (syncLabel);

    buildPresetMenu();
    presetBox.setTextWhenNothingSelected ("Presets");
    presetBox.setTooltip ("Load a factory preset");
    presetBox.onChange = [this]
    {
        if (refreshing)
            return;
        const int index = presetBox.getSelectedId() - 1;
        if (index >= 0)
            processor.applyPreset (index);
    };
    addAndMakeVisible (presetBox);

    detachButton.setTooltip ("Detach Lumi into a floating transparent overlay window");
    detachButton.onClick = [this]
    {
        OverlayController::instance().toggle (processor);
        detachButton.setToggleState (OverlayController::instance().isOwnedBy (&processor),
                                     juce::dontSendNotification);
    };
    addAndMakeVisible (detachButton);

    overlayMenuButton.setTooltip ("Overlay options");
    overlayMenuButton.onClick = [this]
    {
        const LumiSettings s = processor.getSettings();
        juce::PopupMenu menu;
        const auto overlayOption = [this] (auto&& mutator)
        {
            return [this, mutator]
            {
                mutateSettings (mutator);
                OverlayController::instance().refreshFromSettings();
            };
        };
        menu.addItem (juce::PopupMenu::Item ("Always on Top")
                          .setTicked (s.overlay.alwaysOnTop)
                          .setAction (overlayOption ([] (LumiSettings& st)
                                      { st.overlay.alwaysOnTop = ! st.overlay.alwaysOnTop; })));
        menu.addItem (juce::PopupMenu::Item ("Click Through")
                          .setTicked (s.overlay.clickThrough)
                          .setAction (overlayOption ([] (LumiSettings& st)
                                      { st.overlay.clickThrough = ! st.overlay.clickThrough; })));
        menu.addItem (juce::PopupMenu::Item ("Lock Position")
                          .setTicked (s.overlay.locked)
                          .setAction (overlayOption ([] (LumiSettings& st)
                                      { st.overlay.locked = ! st.overlay.locked; })));
        menu.addItem (juce::PopupMenu::Item ("Overlay Background")
                          .setTicked (s.overlay.showBackground)
                          .setAction (overlayOption ([] (LumiSettings& st)
                                      { st.overlay.showBackground = ! st.overlay.showBackground; })));
        juce::PopupMenu opacityMenu;
        for (const int percent : { 25, 50, 75, 100 })
        {
            opacityMenu.addItem (juce::PopupMenu::Item (juce::String (percent) + "%")
                                     .setTicked (std::abs (s.overlay.opacity - float (percent) / 100.0f) < 0.01f)
                                     .setAction (overlayOption ([percent] (LumiSettings& st)
                                                 { st.overlay.opacity = float (percent) / 100.0f; })));
        }
        menu.addSubMenu ("Overlay Opacity", opacityMenu);
        menu.addItem (juce::PopupMenu::Item ("Reset Position")
                          .setAction (overlayOption ([] (LumiSettings& st)
                                      { st.overlay.x = st.overlay.y = -1; })));
        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (overlayMenuButton));
    };
    addAndMakeVisible (overlayMenuButton);

    // ------------------------------------------------------------ left panel
    styleHeader (danceHeader, "DANCE");
    addAndMakeVisible (danceHeader);

    for (int i = 0; i < int (DanceStyle::Count); ++i)
        styleBox.addItem (danceStyleName (DanceStyle (i)), i + 1);
    styleBox.setTooltip ("Dance personality");
    addAndMakeVisible (styleBox);
    styleAttachment = std::make_unique<ComboAttachment> (processor.apvts,
                                                         params::danceStyle, styleBox);

    moodLabel.setText ("Mood", juce::dontSendNotification);
    moodLabel.setFont (juce::Font (juce::FontOptions (11.0f)));
    addAndMakeVisible (moodLabel);
    moodBox.addItemList ({ "Soft", "Cheerful", "Confident", "Sleepy", "Hyper" }, 1);
    moodBox.setTooltip ("Expression mood theme");
    addAndMakeVisible (moodBox);
    moodAttachment = std::make_unique<ComboAttachment> (processor.apvts, params::mood, moodBox);

    seedLabel.setFont (juce::Font (juce::FontOptions (11.0f)));
    addAndMakeVisible (seedLabel);
    newSeedButton.setTooltip ("Roll a new choreography seed");
    newSeedButton.onClick = [this]
    {
        mutateSettings ([] (LumiSettings& s)
        {
            if (! s.seedLock)
                s.seed = uint64_t (juce::Random::getSystemRandom().nextInt64());
        });
    };
    addAndMakeVisible (newSeedButton);
    seedLockToggle.setTooltip ("Lock the seed so routines reproduce exactly");
    seedLockToggle.onClick = [this]
    {
        mutateSettings ([this] (LumiSettings& s) { s.seedLock = seedLockToggle.getToggleState(); });
    };
    addAndMakeVisible (seedLockToggle);

    styleHeader (routineHeader, "CONSTELLATION");
    addAndMakeVisible (routineHeader);

    useRoutineToggle.setTooltip ("Dance a generated constellation routine instead of a single style");
    useRoutineToggle.onClick = [this]
    {
        mutateSettings ([this] (LumiSettings& s) { s.useRoutine = useRoutineToggle.getToggleState(); });
        regenerateRoutine();
    };
    addAndMakeVisible (useRoutineToggle);

    for (const int bars : { 1, 2, 4, 8, 16 })
        routineBarsBox.addItem (juce::String (bars) + (bars == 1 ? " bar" : " bars"), bars);
    routineBarsBox.setTooltip ("Routine length");
    routineBarsBox.onChange = [this]
    {
        if (refreshing)
            return;
        mutateSettings ([this] (LumiSettings& s) { s.routineBars = routineBarsBox.getSelectedId(); });
        regenerateRoutine();
    };
    addAndMakeVisible (routineBarsBox);

    for (int i = 0; i < int (PlaybackMode::Count); ++i)
        playbackModeBox.addItem (playbackModeName (PlaybackMode (i)), i + 1);
    playbackModeBox.setTooltip ("Routine playback mode");
    playbackModeBox.onChange = [this]
    {
        if (refreshing)
            return;
        mutateSettings ([this] (LumiSettings& s) { s.playbackMode = playbackModeBox.getSelectedId() - 1; });
    };
    addAndMakeVisible (playbackModeBox);

    newRoutineButton.setTooltip ("Generate a fresh routine from the constellation");
    newRoutineButton.onClick = [this]
    {
        mutateSettings ([] (LumiSettings& s)
        {
            if (! s.seedLock)
                s.seed = uint64_t (juce::Random::getSystemRandom().nextInt64());
        });
        regenerateRoutine();
    };
    addAndMakeVisible (newRoutineButton);

    styleLinearSlider (energySlider, "Routine Energy", "Preferred move energy for generated routines");
    energySlider.setRange (0.0, 1.0, 0.0);
    energySlider.onValueChange = [this]
    {
        if (refreshing)
            return;
        mutateSettings ([this] (LumiSettings& s) { s.choreoEnergy = float (energySlider.getValue()); });
    };
    addAndMakeVisible (energySlider);
    energyLabel.setText ("Energy", juce::dontSendNotification);
    energyLabel.setFont (juce::Font (juce::FontOptions (10.0f)));
    addAndMakeVisible (energyLabel);

    styleLinearSlider (cutenessSlider, "Routine Cuteness", "Preferred move cuteness for generated routines");
    cutenessSlider.setRange (0.0, 1.0, 0.0);
    cutenessSlider.onValueChange = [this]
    {
        if (refreshing)
            return;
        mutateSettings ([this] (LumiSettings& s) { s.choreoCuteness = float (cutenessSlider.getValue()); });
    };
    addAndMakeVisible (cutenessSlider);
    cutenessLabel.setText ("Cuteness", juce::dontSendNotification);
    cutenessLabel.setFont (juce::Font (juce::FontOptions (10.0f)));
    addAndMakeVisible (cutenessLabel);

    // ----------------------------------------------------------- right panel
    styleHeader (reactionHeader, "AUDIO REACTION");
    addAndMakeVisible (reactionHeader);

    struct RotaryDef { juce::Slider* slider; juce::Label* label; const char* name;
                       const char* param; const char* tip; };
    const RotaryDef rotaries[] = {
        { &intensitySlider, &intensityLabel, "Intensity", params::intensity,
          "Overall animation intensity" },
        { &reactionSlider, &reactionLabel, "Reaction", params::reactionAmount,
          "How strongly the music drives the dance" },
        { &lowSlider, &lowLabel, "Low", params::lowSens, "Low band (kick) sensitivity" },
        { &midSlider, &midLabel, "Mid", params::midSens, "Mid band (snare) sensitivity" },
        { &highSlider, &highLabel, "High", params::highSens, "High band (hats) sensitivity" },
        { &transientSlider, &transientLabel, "Snap", params::transientSens,
          "Transient (hit) sensitivity" },
        { &smoothingSlider, &smoothingLabel, "Smooth", params::smoothing,
          "Motion smoothing - higher is calmer" },
        { &particleSlider, &particleLabel, "Sparkle", params::particleAmount,
          "Particle amount" },
    };
    for (const auto& def : rotaries)
    {
        styleSmallSlider (*def.slider, *def.label, def.name, def.tip);
        addAndMakeVisible (*def.slider);
        addAndMakeVisible (*def.label);
    }
    intensityAttachment = std::make_unique<SliderAttachment> (processor.apvts, params::intensity, intensitySlider);
    reactionAttachment = std::make_unique<SliderAttachment> (processor.apvts, params::reactionAmount, reactionSlider);
    lowAttachment = std::make_unique<SliderAttachment> (processor.apvts, params::lowSens, lowSlider);
    midAttachment = std::make_unique<SliderAttachment> (processor.apvts, params::midSens, midSlider);
    highAttachment = std::make_unique<SliderAttachment> (processor.apvts, params::highSens, highSlider);
    transientAttachment = std::make_unique<SliderAttachment> (processor.apvts, params::transientSens, transientSlider);
    smoothingAttachment = std::make_unique<SliderAttachment> (processor.apvts, params::smoothing, smoothingSlider);
    particleAttachment = std::make_unique<SliderAttachment> (processor.apvts, params::particleAmount, particleSlider);

    beatLockToggle.setTooltip ("Quantise the dance to a 16th-note grid");
    addAndMakeVisible (beatLockToggle);
    beatLockAttachment = std::make_unique<ButtonAttachment> (processor.apvts, params::beatLock, beatLockToggle);

    styleHeader (fxHeader, "COMFORT");
    addAndMakeVisible (fxHeader);

    reducedMotionToggle.setTooltip ("Slower, gentler movement with no rapid cuts");
    addAndMakeVisible (reducedMotionToggle);
    reducedMotionAttachment = std::make_unique<ButtonAttachment> (processor.apvts, params::reducedMotion, reducedMotionToggle);

    noFlashToggle.setTooltip ("Disable beat flashes and bright rings");
    noFlashToggle.onClick = [this]
    {
        mutateSettings ([this] (LumiSettings& s)
        { s.accessibility.disableFlashes = noFlashToggle.getToggleState(); });
    };
    addAndMakeVisible (noFlashToggle);

    highContrastToggle.setTooltip ("Stronger outlines and brighter text");
    highContrastToggle.onClick = [this]
    {
        mutateSettings ([this] (LumiSettings& s)
        { s.accessibility.highContrast = highContrastToggle.getToggleState(); });
    };
    addAndMakeVisible (highContrastToggle);

    // ----------------------------------------------------------- bottom bar
    freezeToggle.setTooltip ("Freeze Lumi's current pose");
    freezeToggle.onClick = [this] { stage.setFrozen (freezeToggle.getToggleState()); };
    addAndMakeVisible (freezeToggle);

    randomizeButton.setTooltip ("Randomise Lumi's look");
    randomizeButton.onClick = [this]
    {
        auto& rng = juce::Random::getSystemRandom();
        mutateSettings ([&rng] (LumiSettings& s)
        {
            s.hairPalette = rng.nextInt (int (HairPalette::Count));
            s.outfit = rng.nextInt (int (Outfit::Count));
            s.goldAccent = rng.nextInt (int (GoldAccent::Count));
        });
    };
    addAndMakeVisible (randomizeButton);

    hairBox.addItemList ({ "Lavender Hair", "Light Purple Hair", "Deep Plum Hair",
                           "Cream + Lavender Tips" }, 1);
    hairBox.setTooltip ("Hair palette");
    hairBox.onChange = [this]
    {
        if (refreshing)
            return;
        mutateSettings ([this] (LumiSettings& s) { s.hairPalette = hairBox.getSelectedId() - 1; });
    };
    addAndMakeVisible (hairBox);

    outfitBox.addItemList ({ "Star Hoodie", "Orbit Jacket", "Dream Dress",
                             "Producer Outfit", "Cosmic Streetwear" }, 1);
    outfitBox.setTooltip ("Outfit");
    outfitBox.onChange = [this]
    {
        if (refreshing)
            return;
        mutateSettings ([this] (LumiSettings& s) { s.outfit = outfitBox.getSelectedId() - 1; });
    };
    addAndMakeVisible (outfitBox);

    accentBox.addItemList ({ "Soft Gold", "Rose Gold", "Pale Champagne" }, 1);
    accentBox.setTooltip ("Gold accent tone");
    accentBox.onChange = [this]
    {
        if (refreshing)
            return;
        mutateSettings ([this] (LumiSettings& s) { s.goldAccent = accentBox.getSelectedId() - 1; });
    };
    addAndMakeVisible (accentBox);

    backgroundBox.addItemList ({ "Transparent", "Solid Dark", "Lavender Gradient",
                                 "Gold Constellation", "Night Sky", "Soft Studio",
                                 "Neutral Studio", "User Image..." }, 1);
    backgroundBox.setTooltip ("Stage background");
    backgroundBox.onChange = [this]
    {
        if (refreshing)
            return;
        const int selection = backgroundBox.getSelectedId() - 1;
        if (selection == int (Background::UserImage))
        {
            auto chooser = std::make_shared<juce::FileChooser> (
                "Choose a background image", juce::File {}, "*.png;*.jpg;*.jpeg");
            chooser->launchAsync (juce::FileBrowserComponent::openMode
                                      | juce::FileBrowserComponent::canSelectFiles,
                                  [this, chooser] (const juce::FileChooser& fc)
            {
                const juce::File file = fc.getResult();
                mutateSettings ([&file] (LumiSettings& s)
                {
                    s.background = int (Background::UserImage);
                    if (file.existsAsFile())
                        s.userImagePath = file.getFullPathName().toStdString();
                });
            });
            return;
        }
        mutateSettings ([selection] (LumiSettings& s) { s.background = selection; });
    };
    addAndMakeVisible (backgroundBox);

    cameraBox.addItemList ({ "Full Body", "Waist Up", "Close-Up", "Auto Frame", "Stage" }, 1);
    cameraBox.setTooltip ("Camera framing");
    cameraBox.onChange = [this]
    {
        if (refreshing)
            return;
        mutateSettings ([this] (LumiSettings& s) { s.cameraMode = cameraBox.getSelectedId() - 1; });
    };
    addAndMakeVisible (cameraBox);

    fpsBox.addItemList ({ "30 FPS", "60 FPS", "Adaptive" }, 1);
    fpsBox.setTooltip ("Animation frame rate");
    fpsBox.onChange = [this]
    {
        if (refreshing)
            return;
        mutateSettings ([this] (LumiSettings& s) { s.frameRateMode = fpsBox.getSelectedId() - 1; });
    };
    addAndMakeVisible (fpsBox);

    accessoriesButton.setTooltip ("Toggle Lumi's accessories");
    accessoriesButton.onClick = [this]
    {
        const LumiSettings s = processor.getSettings();
        juce::PopupMenu menu;
        const std::pair<uint32_t, const char*> items[] = {
            { accStarClip, "Gold Star Clip" },
            { accHeadphones, "Headphones" },
            { accCrescentPin, "Crescent Pin" },
            { accOrbitBelt, "Orbit Belt" },
            { accCompanionStar, "Companion Star" },
        };
        for (const auto& [bit, name] : items)
        {
            const uint32_t bitCopy = bit;
            menu.addItem (juce::PopupMenu::Item (name)
                              .setTicked ((s.accessories & bit) != 0)
                              .setAction ([this, bitCopy]
            {
                mutateSettings ([bitCopy] (LumiSettings& st) { st.accessories ^= bitCopy; });
            }));
        }
        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (accessoriesButton));
    };
    addAndMakeVisible (accessoriesButton);

    scaleSlider.setSliderStyle (juce::Slider::LinearHorizontal);
    scaleSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    scaleSlider.setTooltip ("Lumi's size on stage");
    scaleSlider.setTitle ("Scale");
    addAndMakeVisible (scaleSlider);
    scaleAttachment = std::make_unique<SliderAttachment> (processor.apvts, params::visualScale, scaleSlider);

    bypassToggle.setTooltip ("Pause analysis - Lumi rests, audio is untouched either way");
    addAndMakeVisible (bypassToggle);
    bypassAttachment = std::make_unique<ButtonAttachment> (processor.apvts, params::bypass, bypassToggle);

    // BPM/sync refresher piggybacks on the stage's frame timer via a lightweight
    // component timer of our own.
    class StatusTimer : public juce::Timer
    {
    public:
        explicit StatusTimer (LumiDancerEditor& editorIn) : editor (editorIn) { startTimerHz (4); }
        void timerCallback() override
        {
            editor.bpmLabel.setText (juce::String (editor.stage.hostBpm(), 1) + " BPM",
                                     juce::dontSendNotification);
            const char* status = "FREE MODE";
            switch (editor.stage.syncStatus())
            {
                case SyncStatus::HostSync: status = "SYNCED"; break;
                case SyncStatus::Stopped:  status = "STOPPED"; break;
                default: break;
            }
            editor.syncLabel.setText (status, juce::dontSendNotification);
            editor.detachButton.setToggleState (
                OverlayController::instance().isOwnedBy (&editor.processor),
                juce::dontSendNotification);
        }
    private:
        LumiDancerEditor& editor;
    };
    statusTimer = std::make_unique<StatusTimer> (*this);

    processor.settingsBroadcaster.addChangeListener (this);
    refreshControlsFromSettings();

    const LumiSettings s = processor.getSettings();
    setResizable (true, true);
    setResizeLimits (380, 300, 3840, 2400);
    setSize (s.editorWidth, s.editorHeight);
}

LumiDancerEditor::~LumiDancerEditor()
{
    processor.settingsBroadcaster.removeChangeListener (this);
    setLookAndFeel (nullptr);
}

void LumiDancerEditor::buildPresetMenu()
{
    presetBox.clear (juce::dontSendNotification);
    const auto& presets = factoryPresets();
    juce::String currentCategory;
    for (size_t i = 0; i < presets.size(); ++i)
    {
        if (presets[i].category != currentCategory.toStdString())
        {
            currentCategory = presets[i].category;
            presetBox.addSectionHeading (currentCategory);
        }
        presetBox.addItem (presets[i].name, int (i) + 1);
    }
}

void LumiDancerEditor::mutateSettings (const std::function<void (LumiSettings&)>& mutate)
{
    LumiSettings s = processor.getSettings();
    mutate (s);
    processor.setSettings (s, false);
    stage.refreshFromSettings();
    refreshControlsFromSettings();
}

void LumiDancerEditor::regenerateRoutine()
{
    const LumiSettings before = processor.getSettings();
    if (! before.useRoutine)
        return;

    ChoreoParams cp;
    cp.complexity = before.choreoComplexity;
    cp.repeatAvoidance = before.choreoRepeatAvoidance;
    cp.surprise = before.choreoSurprise;
    cp.energy = before.choreoEnergy;
    cp.cuteness = before.choreoCuteness;
    const Routine routine = generateRoutine (before.routineBars, 4, before.seed, cp);

    mutateSettings ([&routine] (LumiSettings& s)
    {
        s.routineData = serializeRoutine (routine);
    });
}

void LumiDancerEditor::refreshControlsFromSettings()
{
    const juce::ScopedValueSetter<bool> guard (refreshing, true);
    const LumiSettings s = processor.getSettings();

    seedLabel.setText ("Seed " + juce::String (int64_t (s.seed % 1000000)),
                       juce::dontSendNotification);
    seedLockToggle.setToggleState (s.seedLock, juce::dontSendNotification);
    useRoutineToggle.setToggleState (s.useRoutine, juce::dontSendNotification);
    routineBarsBox.setSelectedId (s.routineBars, juce::dontSendNotification);
    playbackModeBox.setSelectedId (s.playbackMode + 1, juce::dontSendNotification);
    energySlider.setValue (s.choreoEnergy, juce::dontSendNotification);
    cutenessSlider.setValue (s.choreoCuteness, juce::dontSendNotification);
    hairBox.setSelectedId (s.hairPalette + 1, juce::dontSendNotification);
    outfitBox.setSelectedId (s.outfit + 1, juce::dontSendNotification);
    accentBox.setSelectedId (s.goldAccent + 1, juce::dontSendNotification);
    backgroundBox.setSelectedId (s.background + 1, juce::dontSendNotification);
    cameraBox.setSelectedId (s.cameraMode + 1, juce::dontSendNotification);
    fpsBox.setSelectedId (s.frameRateMode + 1, juce::dontSendNotification);
    noFlashToggle.setToggleState (s.accessibility.disableFlashes, juce::dontSendNotification);
    highContrastToggle.setToggleState (s.accessibility.highContrast, juce::dontSendNotification);

    stage.refreshFromSettings();
}

void LumiDancerEditor::changeListenerCallback (juce::ChangeBroadcaster*)
{
    refreshControlsFromSettings();
}

void LumiDancerEditor::updateCompactMode()
{
    const LumiSettings s = processor.getSettings();
    compactMode = getWidth() < 640 || getHeight() < 420
                  || s.uiMode == int (UiMode::Compact);

    const bool full = ! compactMode;
    for (juce::Component* c : { (juce::Component*) &danceHeader, (juce::Component*) &moodLabel,
                                (juce::Component*) &moodBox, (juce::Component*) &seedLabel,
                                (juce::Component*) &newSeedButton, (juce::Component*) &seedLockToggle,
                                (juce::Component*) &routineHeader, (juce::Component*) &useRoutineToggle,
                                (juce::Component*) &routineBarsBox, (juce::Component*) &playbackModeBox,
                                (juce::Component*) &newRoutineButton, (juce::Component*) &energySlider,
                                (juce::Component*) &cutenessSlider, (juce::Component*) &energyLabel,
                                (juce::Component*) &cutenessLabel, (juce::Component*) &reactionHeader,
                                (juce::Component*) &reactionSlider, (juce::Component*) &reactionLabel,
                                (juce::Component*) &lowSlider, (juce::Component*) &lowLabel,
                                (juce::Component*) &midSlider, (juce::Component*) &midLabel,
                                (juce::Component*) &highSlider, (juce::Component*) &highLabel,
                                (juce::Component*) &transientSlider, (juce::Component*) &transientLabel,
                                (juce::Component*) &smoothingSlider, (juce::Component*) &smoothingLabel,
                                (juce::Component*) &particleSlider, (juce::Component*) &particleLabel,
                                (juce::Component*) &beatLockToggle, (juce::Component*) &fxHeader,
                                (juce::Component*) &reducedMotionToggle, (juce::Component*) &noFlashToggle,
                                (juce::Component*) &highContrastToggle, (juce::Component*) &freezeToggle,
                                (juce::Component*) &randomizeButton, (juce::Component*) &hairBox,
                                (juce::Component*) &outfitBox, (juce::Component*) &accentBox,
                                (juce::Component*) &backgroundBox, (juce::Component*) &cameraBox,
                                (juce::Component*) &fpsBox, (juce::Component*) &accessoriesButton,
                                (juce::Component*) &scaleSlider })
        c->setVisible (full);
}

void LumiDancerEditor::paint (juce::Graphics& g)
{
    g.fillAll (theme::panelDeep());

    // Panel plates behind the side/bottom control zones.
    g.setColour (theme::panel().withAlpha (0.6f));
    if (! compactMode)
    {
        g.fillRoundedRectangle (leftPanelBounds.toFloat(), 8.0f);
        g.fillRoundedRectangle (rightPanelBounds.toFloat(), 8.0f);
        g.fillRoundedRectangle (bottomBarBounds.toFloat(), 8.0f);
    }
    g.fillRoundedRectangle (topBarBounds.toFloat(), 8.0f);
}

void LumiDancerEditor::resized()
{
    updateCompactMode();
    auto area = getLocalBounds().reduced (6);

    // ---------------------------------------------------------------- top
    topBarBounds = area.removeFromTop (40);
    auto top = topBarBounds.reduced (8, 6);
    titleLabel.setBounds (top.removeFromLeft (150));
    overlayMenuButton.setBounds (top.removeFromRight (24));
    top.removeFromRight (2);
    detachButton.setBounds (top.removeFromRight (66));
    top.removeFromRight (6);
    presetBox.setBounds (top.removeFromRight (juce::jmin (170, top.getWidth() / 2)));
    top.removeFromRight (6);
    syncLabel.setBounds (top.removeFromRight (76));
    bpmLabel.setBounds (top.removeFromRight (86));
    area.removeFromTop (6);

    if (compactMode)
    {
        leftPanelBounds = rightPanelBounds = bottomBarBounds = {};
        auto bottom = area.removeFromBottom (34);
        bypassToggle.setBounds (bottom.removeFromRight (76));
        bottom.removeFromRight (4);
        intensitySlider.setVisible (true);
        intensityLabel.setVisible (false);
        styleBox.setVisible (true);
        styleBox.setBounds (bottom.removeFromLeft (juce::jmin (150, bottom.getWidth() / 2)));
        bottom.removeFromLeft (6);
        intensitySlider.setBounds (bottom);
        stage.setBounds (area);
        return;
    }

    bypassToggle.setVisible (true);

    // --------------------------------------------------------------- bottom
    bottomBarBounds = area.removeFromBottom (66);
    auto bottom = bottomBarBounds.reduced (8, 6);
    auto bottomRow1 = bottom.removeFromTop (26);
    auto bottomRow2 = bottom.withTrimmedTop (2);

    freezeToggle.setBounds (bottomRow1.removeFromLeft (70));
    bottomRow1.removeFromLeft (4);
    randomizeButton.setBounds (bottomRow1.removeFromLeft (84));
    bottomRow1.removeFromLeft (8);
    hairBox.setBounds (bottomRow1.removeFromLeft (150));
    bottomRow1.removeFromLeft (4);
    outfitBox.setBounds (bottomRow1.removeFromLeft (130));
    bottomRow1.removeFromLeft (4);
    accentBox.setBounds (bottomRow1.removeFromLeft (110));
    bottomRow1.removeFromLeft (4);
    accessoriesButton.setBounds (bottomRow1.removeFromLeft (96));

    backgroundBox.setBounds (bottomRow2.removeFromLeft (150));
    bottomRow2.removeFromLeft (4);
    cameraBox.setBounds (bottomRow2.removeFromLeft (110));
    bottomRow2.removeFromLeft (4);
    fpsBox.setBounds (bottomRow2.removeFromLeft (90));
    bottomRow2.removeFromLeft (8);
    bypassToggle.setBounds (bottomRow2.removeFromRight (76));
    bottomRow2.removeFromRight (4);
    scaleSlider.setBounds (bottomRow2);
    area.removeFromBottom (6);

    // ----------------------------------------------------------- left panel
    leftPanelBounds = area.removeFromLeft (185);
    auto left = leftPanelBounds.reduced (10, 8);
    danceHeader.setBounds (left.removeFromTop (18));
    styleBox.setBounds (left.removeFromTop (26));
    left.removeFromTop (6);
    auto moodRow = left.removeFromTop (24);
    moodLabel.setBounds (moodRow.removeFromLeft (40));
    moodBox.setBounds (moodRow);
    left.removeFromTop (6);
    auto seedRow = left.removeFromTop (24);
    seedLabel.setBounds (seedRow.removeFromLeft (74));
    newSeedButton.setBounds (seedRow);
    seedLockToggle.setBounds (left.removeFromTop (22));
    left.removeFromTop (12);
    routineHeader.setBounds (left.removeFromTop (18));
    useRoutineToggle.setBounds (left.removeFromTop (22));
    routineBarsBox.setBounds (left.removeFromTop (24));
    left.removeFromTop (4);
    playbackModeBox.setBounds (left.removeFromTop (24));
    left.removeFromTop (4);
    newRoutineButton.setBounds (left.removeFromTop (24));
    left.removeFromTop (8);
    auto energyRow = left.removeFromTop (20);
    energyLabel.setBounds (energyRow.removeFromLeft (52));
    energySlider.setBounds (energyRow);
    auto cuteRow = left.removeFromTop (20);
    cutenessLabel.setBounds (cuteRow.removeFromLeft (52));
    cutenessSlider.setBounds (cuteRow);
    area.removeFromLeft (6);

    // ---------------------------------------------------------- right panel
    rightPanelBounds = area.removeFromRight (185);
    auto right = rightPanelBounds.reduced (10, 8);
    reactionHeader.setBounds (right.removeFromTop (18));

    // 2 x 4 rotary grid.
    juce::Slider* gridSliders[8] = { &intensitySlider, &reactionSlider, &lowSlider,
                                     &midSlider, &highSlider, &transientSlider,
                                     &smoothingSlider, &particleSlider };
    juce::Label* gridLabels[8] = { &intensityLabel, &reactionLabel, &lowLabel, &midLabel,
                                   &highLabel, &transientLabel, &smoothingLabel,
                                   &particleLabel };
    const int cellW = right.getWidth() / 2;
    const int cellH = 62;
    for (int i = 0; i < 8; i += 2)
    {
        auto row = right.removeFromTop (cellH);
        for (int c = 0; c < 2; ++c)
        {
            auto cell = c == 0 ? row.removeFromLeft (cellW) : row;
            gridLabels[size_t (i + c)]->setBounds (cell.removeFromBottom (14));
            gridSliders[size_t (i + c)]->setBounds (cell);
        }
    }
    right.removeFromTop (2);
    beatLockToggle.setBounds (right.removeFromTop (22));
    right.removeFromTop (8);
    fxHeader.setBounds (right.removeFromTop (18));
    reducedMotionToggle.setBounds (right.removeFromTop (22));
    noFlashToggle.setBounds (right.removeFromTop (22));
    highContrastToggle.setBounds (right.removeFromTop (22));
    area.removeFromRight (6);

    // ---------------------------------------------------------------- stage
    stage.setBounds (area);

    // Persist the editor size (geometry only, no broadcast).
    LumiSettings s = processor.getSettings();
    if (s.editorWidth != getWidth() || s.editorHeight != getHeight())
    {
        s.editorWidth = getWidth();
        s.editorHeight = getHeight();
        processor.setSettings (s, false);
    }
}
