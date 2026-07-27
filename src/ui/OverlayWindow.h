// LUMI//DANCER — the detached floating overlay.
//
// A transparent, optionally always-on-top, optionally click-through desktop
// window showing only Lumi. One overlay exists per process; the
// OverlayController arbitrates which plugin instance owns it, so multiple
// LUMI//DANCER instances never fight. All methods are message-thread only.
#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

#include "plugin/PluginProcessor.h"
#include "ui/StageComponent.h"

class OverlayWindow : public juce::Component
{
public:
    explicit OverlayWindow (LumiDancerProcessor& processorIn);
    ~OverlayWindow() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;
    void mouseEnter (const juce::MouseEvent& e) override;
    void mouseExit (const juce::MouseEvent& e) override;

    // Re-applies always-on-top / click-through / opacity from settings.
    void applyOverlaySettings();

private:
    void saveGeometry();
    void addToDesktopWithFlags();

    LumiDancerProcessor& processor;
    StageComponent stage;
    juce::ComponentDragger dragger;
    std::unique_ptr<juce::ResizableCornerComponent> resizer;
    juce::ComponentBoundsConstrainer constrainer;
    juce::TextButton closeButton { "x" };
    juce::TextButton pinButton { "pin" };
    bool showChrome = false;
    bool clickThroughApplied = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OverlayWindow)
};

// ------------------------------------------------------------- arbitration
class OverlayController
{
public:
    static OverlayController& instance();

    // Claim the overlay for `owner` (steals politely: the previous owner's
    // overlay closes first). Returns true when the overlay is now owned.
    bool claim (LumiDancerProcessor& owner);

    // Close the overlay if `owner` holds it.
    void release (LumiDancerProcessor& owner);

    // Toggle for the editor button.
    void toggle (LumiDancerProcessor& owner);

    bool isOwnedBy (const LumiDancerProcessor* p) const { return ownerProcessor == p; }
    bool hasOverlay() const { return window != nullptr; }

    // For tests: the live overlay component (nullptr when closed).
    juce::Component* overlayComponent() const { return window.get(); }

    // Called by the processor destructor: tears the overlay down synchronously
    // when the owning instance dies so the window never dangles.
    void notifyProcessorDying (LumiDancerProcessor& processor);

    void refreshFromSettings();

private:
    OverlayController() = default;

    LumiDancerProcessor* ownerProcessor = nullptr;
    std::unique_ptr<OverlayWindow> window;
};
