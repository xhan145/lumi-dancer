# LUMI//DANCER

A cute, audio-reactive dancing mascot for Ableton Live.

Load LUMI//DANCER on any track (or the Master channel), press play, and
**Lumi** — an original lavender-and-gold magical-tech performer — dances in
time with your music. The plugin analyses rhythm, energy, frequency balance
and transients in real time, reads the host's tempo and beat position, and
never touches your audio: the dry path is bit-exact.

![palette](https://img.shields.io/badge/lavender-%23B9A3FF-B9A3FF)
![palette](https://img.shields.io/badge/gold-%23E4B84C-E4B84C)
![palette](https://img.shields.io/badge/plum-%23302344-302344)

## Features (v0.1.0-alpha)

- **Transparent audio** — the buffer is never written; mono and stereo.
- **Host sync** — BPM, PPQ beat phase, play/stop/seek/loop recovery, and a
  clearly badged FREE MODE fallback when the host has no timeline.
- **Ten dance styles** — Bounce, Kawaii Pop, Orbit, Groove, Hyper, Chill,
  Breakcore, Drum & Bass, Trance, Freestyle. Each is real, distinct
  choreography (enforced by automated tests, not just names).
- **Two art styles** — Painted (Anime): 16 embedded hand-painted sprite
  poses beat-stepped per style, with a sunglasses moment on big drops;
  or Vector: the procedural mascot with full hair/outfit customisation.
- **Audio-reactive mapping** — kicks bounce the body, snares snap the arms
  and head, hats sparkle, RMS drives amplitude, silence sends Lumi to idle
  (breathing, blinking, waving, eventually sitting and sleeping).
- **Expressions** — nine expressions, seeded blinking, star eyes on big
  moments, mood themes.
- **Constellation routines** — dance moves are stars in three
  constellations (Idle / Energy / Flow) with compatibility edges; a seeded
  choreography engine generates routines that reproduce exactly when the
  seed is locked. Loop / One Shot / Ping Pong / Shuffle playback, all keyed
  to host beats.
- **Customisation** — 4 hair palettes, 5 outfits, 3 gold accents, 5
  accessories, 8 backgrounds (including your own image by external path).
- **Floating overlay** — detach Lumi into a transparent, always-on-top,
  optionally click-through desktop window that dances above Ableton.
  Multiple plugin instances arbitrate overlay ownership politely.
- **Accessibility** — Reduced Motion, flash suppression, particle
  reduction, high contrast, UI scaling, tooltips, screen-reader titles.
- **22 factory presets** across General / Music Styles / Visual banks.
- **Full state persistence** with schema versioning and corrupt-state
  fallback — projects always reopen safely.

## Building (Windows)

Prerequisites: CMake ≥ 3.25, Visual Studio 2022 (or Build Tools) with the
C++ workload. JUCE 8.0.8 is fetched automatically at configure time.

```bash
./scripts/build-windows.ps1
```

or manually:

```bash
cmake --preset windows
cmake --build --preset windows-release
```

Outputs land in `build/windows/LumiDancer_artefacts/<config>/`:

- `VST3/LUMI DANCER.vst3`
- `Standalone/LUMI DANCER.exe`

Run the test suites (fast JUCE-free core suite + JUCE-linked integration
suite):

```bash
./scripts/test.ps1
```

Package a portable zip (after a Release build):

```bash
./scripts/package.ps1
```

## Using in Ableton Live

1. Copy `LUMI DANCER.vst3` to `C:\Program Files\Common Files\VST3\`
   (or add the build folder to Live's VST3 custom folder).
2. Rescan plugins in Live's Preferences → Plug-Ins.
3. Drop **LUMI//DANCER** on any audio track, return, or the Master channel.
4. Press play. Lumi dances; the top bar shows the host BPM and SYNCED.
5. Pick a dance style, mood and preset; shape the reaction with the
   Low/Mid/High/Snap sensitivities and Motion Smoothing.
6. Click **Detach** to float Lumi above Live in a transparent overlay
   (the `v` button next to it holds Always-on-Top, Click-Through, Lock,
   Opacity and Reset Position options).

Everything is saved with your Live project, including the routine, look,
overlay position and accessibility settings.

See [docs/ableton-workflow.md](docs/ableton-workflow.md) for the full
workflow guide and [docs/troubleshooting.md](docs/troubleshooting.md) for
known limitations (overlay behaviour, FREE MODE, renderer fallback).

## Privacy

LUMI//DANCER runs entirely locally. No cloud APIs, no accounts, no
telemetry, no internet connectivity, no downloaded models. The only file
the plugin ever reads at your request is an optional user background image,
referenced by path (never embedded in project state).

## Licence

AGPL-3.0-only (matching the JUCE open-source tier). See [LICENSE](LICENSE)
and [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). The Lumi character
is an original design created for this project — no third-party character
art, sprites, or animation middleware are used or imitated.
