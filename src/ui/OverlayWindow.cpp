#include "ui/OverlayWindow.h"

#include "ui/Theme.h"

using namespace lumi;

// ============================================================ OverlayWindow
OverlayWindow::OverlayWindow (LumiDancerProcessor& processorIn)
    : processor (processorIn), stage (processorIn)
{
    setOpaque (false);
    stage.setOverlayMode (true);
    stage.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (stage);

    closeButton.setTooltip ("Return Lumi to the plugin window");
    closeButton.onClick = [this] { OverlayController::instance().release (processor); };
    addChildComponent (closeButton);

    pinButton.setClickingTogglesState (true);
    pinButton.setTooltip ("Always on top");
    pinButton.onClick = [this]
    {
        LumiSettings s = processor.getSettings();
        s.overlay.alwaysOnTop = pinButton.getToggleState();
        processor.setSettings (s, false);
        applyOverlaySettings();
    };
    addChildComponent (pinButton);

    constrainer.setMinimumSize (160, 160);

    resizer = std::make_unique<juce::ResizableCornerComponent> (this, &constrainer);
    addChildComponent (*resizer);

    // Geometry from settings; centre on the primary display the first time.
    const LumiSettings s = processor.getSettings();
    int w = s.overlay.w, h = s.overlay.h, x = s.overlay.x, y = s.overlay.y;
    const auto userArea = juce::Desktop::getInstance().getDisplays()
                              .getPrimaryDisplay()->userArea;
    if (x < 0 || y < 0 || ! userArea.expanded (200).contains (x, y))
    {
        x = userArea.getCentreX() - w / 2;
        y = userArea.getCentreY() - h / 2;
    }
    setBounds (x, y, w, h);

    addToDesktopWithFlags();
    setVisible (true);
    applyOverlaySettings();
}

OverlayWindow::~OverlayWindow()
{
    saveGeometry();
    removeFromDesktop();
}

void OverlayWindow::addToDesktopWithFlags()
{
    const LumiSettings s = processor.getSettings();
    int styleFlags = juce::ComponentPeer::windowIsTemporary;
    if (s.overlay.clickThrough)
        styleFlags |= juce::ComponentPeer::windowIgnoresMouseClicks;
    addToDesktop (styleFlags);
    clickThroughApplied = s.overlay.clickThrough;
}

void OverlayWindow::applyOverlaySettings()
{
    const LumiSettings s = processor.getSettings();
    setAlwaysOnTop (s.overlay.alwaysOnTop);
    setAlpha (s.overlay.opacity);
    pinButton.setToggleState (s.overlay.alwaysOnTop, juce::dontSendNotification);

    if (clickThroughApplied != s.overlay.clickThrough)
    {
        // Toggling OS-level click-through requires recreating the peer.
        removeFromDesktop();
        addToDesktopWithFlags();
        setVisible (true);
    }
    stage.refreshFromSettings();
    repaint();
}

void OverlayWindow::saveGeometry()
{
    LumiSettings s = processor.getSettings();
    s.overlay.x = getX();
    s.overlay.y = getY();
    s.overlay.w = getWidth();
    s.overlay.h = getHeight();
    processor.setSettings (s, false);
}

void OverlayWindow::paint (juce::Graphics& g)
{
    if (showChrome)
    {
        // Subtle frame so the user can find the window edges while hovering.
        g.setColour (theme::lavender().withAlpha (0.25f));
        g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (1.0f), 10.0f, 1.5f);
    }
}

void OverlayWindow::resized()
{
    stage.setBounds (getLocalBounds());
    closeButton.setBounds (getWidth() - 26, 4, 22, 20);
    pinButton.setBounds (getWidth() - 54, 4, 26, 20);
    if (resizer != nullptr)
        resizer->setBounds (getWidth() - 18, getHeight() - 18, 18, 18);
    saveGeometry();
}

void OverlayWindow::mouseEnter (const juce::MouseEvent&)
{
    showChrome = true;
    const bool locked = processor.getSettings().overlay.locked;
    closeButton.setVisible (true);
    pinButton.setVisible (true);
    resizer->setVisible (! locked);
    repaint();
}

void OverlayWindow::mouseExit (const juce::MouseEvent&)
{
    showChrome = false;
    closeButton.setVisible (false);
    pinButton.setVisible (false);
    resizer->setVisible (false);
    repaint();
}

void OverlayWindow::mouseDown (const juce::MouseEvent& e)
{
    if (! processor.getSettings().overlay.locked)
        dragger.startDraggingComponent (this, e);
}

void OverlayWindow::mouseDrag (const juce::MouseEvent& e)
{
    if (! processor.getSettings().overlay.locked)
        dragger.dragComponent (this, e, &constrainer);
}

void OverlayWindow::mouseUp (const juce::MouseEvent&)
{
    saveGeometry();
}

// ======================================================== OverlayController
OverlayController& OverlayController::instance()
{
    static OverlayController controller;
    return controller;
}

bool OverlayController::claim (LumiDancerProcessor& owner)
{
    JUCE_ASSERT_MESSAGE_THREAD

    if (ownerProcessor == &owner && window != nullptr)
        return true;

    // Polite steal: the previous owner's overlay closes cleanly first.
    window.reset();
    ownerProcessor = &owner;

    LumiSettings s = owner.getSettings();
    s.overlay.enabled = true;
    owner.setSettings (s, false);

    window = std::make_unique<OverlayWindow> (owner);
    return true;
}

void OverlayController::release (LumiDancerProcessor& owner)
{
    JUCE_ASSERT_MESSAGE_THREAD
    if (ownerProcessor != &owner)
        return;

    window.reset();
    ownerProcessor = nullptr;

    LumiSettings s = owner.getSettings();
    s.overlay.enabled = false;
    owner.setSettings (s, false);
}

void OverlayController::toggle (LumiDancerProcessor& owner)
{
    if (isOwnedBy (&owner) && window != nullptr)
        release (owner);
    else
        claim (owner);
}

void OverlayController::notifyProcessorDying (LumiDancerProcessor& processor)
{
    if (ownerProcessor != &processor)
        return;
    // Synchronous teardown: the overlay references the processor's buses, so
    // it must die before the processor does. Plugin destruction happens on
    // the message thread in supported hosts.
    window.reset();
    ownerProcessor = nullptr;
}

void OverlayController::refreshFromSettings()
{
    if (window != nullptr)
        window->applyOverlaySettings();
}
