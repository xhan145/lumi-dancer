// LUMI//DANCER — overlay transparency verification.
//
// The desktop-pet contract: the detached overlay must be a per-pixel-alpha
// layered window (transparent background) and draggable. The first test pins
// the OS-level property that makes transparency work; the second is an
// env-gated manual visual check that holds a live, animating overlay open on
// the desktop so an external tool (or human) can capture and inspect it.
#include <juce_gui_extra/juce_gui_extra.h>

#include "JuceTestFramework.h"
#include "plugin/PluginProcessor.h"
#include "ui/OverlayWindow.h"

#if JUCE_WINDOWS
 #define WIN32_LEAN_AND_MEAN
 #define NOMINMAX
 #include <windows.h>
#endif

JT_TEST (overlay_window_is_layered_for_transparency)
{
    LumiDancerProcessor processor;
    auto& controller = OverlayController::instance();
    controller.claim (processor);

    auto* overlay = controller.overlayComponent();
    JT_CHECK (overlay != nullptr);
    JT_CHECK (overlay->getPeer() != nullptr);
    JT_CHECK (! overlay->isOpaque());

    if (overlay != nullptr && overlay->getPeer() != nullptr)
    {
        const int styleFlags = overlay->getPeer()->getStyleFlags();
        JT_CHECK ((styleFlags & juce::ComponentPeer::windowIsSemiTransparent) != 0);

#if JUCE_WINDOWS
        // WS_EX_LAYERED is what actually gives the window per-pixel alpha on
        // Windows; without it the "transparent" stage paints as a black box.
        const HWND hwnd = (HWND) overlay->getPeer()->getNativeHandle();
        const LONG_PTR exStyle = GetWindowLongPtr (hwnd, GWL_EXSTYLE);
        JT_CHECK ((exStyle & WS_EX_LAYERED) != 0);
#endif
    }

    controller.release (processor);
}

// Manual/visual: set LUMI_OVERLAY_VISUAL=<seconds> to hold a live overlay on
// the desktop with the message loop pumping (animation + painting active),
// e.g. for a desktop screen capture. Skipped by default so CI stays fast.
JT_TEST (overlay_visual_check_manual)
{
    const auto secondsText = juce::SystemStats::getEnvironmentVariable ("LUMI_OVERLAY_VISUAL", {});
    if (secondsText.isEmpty())
    {
        JT_CHECK (true);   // explicitly skipped
        return;
    }

    const int milliseconds = juce::jlimit (1000, 30000, secondsText.getIntValue() * 1000);

    LumiDancerProcessor processor;
    auto& controller = OverlayController::instance();
    controller.claim (processor);
    JT_CHECK (controller.hasOverlay());

    juce::MessageManager::getInstance()->runDispatchLoopUntil (milliseconds);

    controller.release (processor);
    JT_CHECK (! controller.hasOverlay());
}
