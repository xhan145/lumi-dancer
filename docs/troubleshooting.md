# LUMI//DANCER — Troubleshooting & known limitations

## Lumi doesn't dance

- **Transport stopped** → she idles by design; press play.
- **Input silent** → silence sends her to idle after ~1.5 s. Check that
  audio actually reaches the track the plugin sits on.
- **Bypass enabled** → the Bypass toggle pauses analysis.
- **FREE MODE badge showing** → the host isn't providing a timeline
  (normal in the standalone app). She still dances to audio at the last
  known tempo, but beat-perfect sync needs host PPQ.

## Overlay

- The overlay is one-per-process: with multiple LUMI//DANCER instances,
  the most recent Detach wins and the previous overlay closes. This is
  intended arbitration, not a bug.
- Click-through mode means clicks pass to whatever is underneath — use
  the plugin window's `v` menu to turn it off again (the overlay itself
  can no longer be clicked).
- If the overlay ends up off-screen after a monitor change, use
  `v` → Reset Position.
- Transparent overlays rely on the OS compositor. If a remote-desktop or
  exotic GPU setup breaks per-pixel transparency, enable "Overlay
  Background" so the window is a normal opaque stage.

## Visual performance

- The renderer is JUCE software rasterisation — deliberately dependency-
  free and driver-proof. There is no OpenGL path in v0.1.0, so there is
  nothing to "fall back" from: the software renderer IS the fallback tier
  and cannot fail like a GPU context can.
- On slow machines set Frame Rate to `30 FPS` or `Adaptive` (adaptive
  automatically halves the rate when paint times exceed ~9 ms), reduce
  Sparkle, or use the Minimal Stage preset.
- Audio can never glitch because of visuals: the audio thread only runs
  the preallocated analysis tap; painting starving just lowers the visual
  frame rate.

## State

- Corrupt or foreign project state loads safe defaults instead of
  crashing; you'd see Lumi reset to her default look.
- A missing user background image falls back to the default stage with a
  small notice — fix the path or re-pick the file.

## Known limitations (v0.1.0-alpha)

- Windows x64 only; macOS is untested (the code avoids Win32-specific
  calls, but no macOS build has been made).
- The routine builder edits happen through generation controls (bars,
  energy, cuteness, seed, playback mode); there is no drag-and-drop
  timeline editor yet.
- The constellation is played, not yet visualised as an interactive
  star-map screen.
- No MIDI-triggered dance moves yet.
- The standalone app is a test host: it listens to the default audio
  input, always in FREE MODE.
- `Routine Position` is exposed as an automatable parameter but currently
  only offsets OneShot playback; full host-clip-linked scrubbing is
  planned.
