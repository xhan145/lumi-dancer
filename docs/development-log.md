# LUMI//DANCER — Development log

## 2026-07-27 — v0.1.0-alpha: all ten phases in one build session

Toolchain: Windows 11, VS 2022 Build Tools (MSVC 19.44), CMake 4.4.0,
JUCE 8.0.8 (FetchContent, pinned). Patterns reused from the ENAHKEM
sister plugins (aether-constellation et al.): JUCE-free tested core +
thin JUCE shell, VS presets, dependency-free test harness.

### Phase 1 — Buildable visualizer
- CMake project: `lumi_core` static lib (C++20, stdlib-only) + JUCE
  plugin (VST3 + Standalone) + two test targets + CI workflow + scripts.
- Decision: micro test harness (family pattern) instead of Catch2 —
  zero dependencies, second-scale rebuilds; same meaningful-assert bar.
- Build: Debug + Release VST3/Standalone green on first full compile.

### Phase 2 — Audio reaction
- `AnalysisEngine`: block RMS/peak, 24 dB/oct band split at 150 Hz/2 kHz,
  per-band fast/slow envelope transient detection, 256-pt radix-2 FFT
  (preallocated) for spectral centroid, correlation stereo width,
  −70 dBFS silence gate, adaptive normalisation.
- Fix found by test: per-band self-normalisation drove every band to 1.0
  on steady tones → switched to one shared adaptive reference.

### Phase 3 — Host-synchronised dancing
- `BeatClock`: local advance + slew onto host PPQ; > 0.5-beat error =
  seek/loop/restart → snap + discontinuity flag → Choreographer 200 ms
  crossfade. FREE MODE fallback with UI badge. 9 dedicated test cases
  (seek, loop wrap, stop/restart, stopped-cursor follow, NaN tempo).

### Phase 4 — Character rig
- Pose = 11 bones + 7 face channels; shortest-angle interpolation,
  critically damped springs (closed form), `sanitizePose` bounds clamp.

### Phase 5 — Dance styles
- Ten deterministic styles; pairwise distinctness + audio-reactivity +
  bounds enforced by tests (not just names). Freestyle hashes 4-beat
  segments with repeat avoidance; Breakcore quantises 16th-note pose
  snaps from seeded hashes.

### Phase 6 — Visual effects
- 256-slot fixed particle pool: ambient stars/moons, high-transient
  sparkles, beat rings (suppressed by No Flashes), hearts, trails.
  Capacity/bounded-spawn/decay/reduced-motion all tested.

### Phase 7 — Customisation
- 4 hair palettes × 5 outfits × 3 accents × 5 accessories × 8
  backgrounds; 22 factory presets in 3 banks (all restore-tested,
  no renamed duplicates — a test enforces meaningful diffs).

### Phase 8 — Constellation routines
- 12 move stars in Idle/Energy/Flow with compatibility edges; weighted
  deterministic `chooseNext` (complexity gate, recency penalty,
  surprise); routine generation tiles bars exactly; Loop/OneShot/
  PingPong/Shuffle playback on host beats; line serialisation with
  corrupt rejection.

### Phase 9 — Floating overlay
- Per-process `OverlayController` arbitration (polite steal, synchronous
  teardown from the owning processor's destructor). Transparent
  `windowIsTemporary` desktop window; click-through via peer recreation
  with `windowIgnoresMouseClicks`; drag/resize/lock; geometry persisted.

### Phase 10 — Hardening
- Corrupt state fallback, future-schema rejection, NaN-proof analysis
  and choreography inputs, reduced motion / no-flash / high-contrast,
  adaptive frame rate, renderer worst-case bounds budget.
- Renderer test caught buns/star escaping the canvas at max intensity →
  height budget now reserves max-root-lift + accessory extents.
- Visual snapshot inspection (PNG dumps via LUMI_SNAPSHOT_DIR) drove two
  design fixes: side-lock proportions, shoulder width/arm visibility.

### Results
- Core suite: 72 cases / 2395 checks, green.
- JUCE suite: 21 cases / 127 checks, green (bit-exact pass-through at
  44.1/48/96 kHz × 64–1024 blocks, NaN input, host-sync bus, state
  round-trip, presets, editor lifecycle, overlay arbitration, renderer
  palette/bounds).
- Release standalone smoke-launched (6 s, no crash).
- Portable package: dist/LUMI-DANCER-v0.1.0-win64.zip (6.0 MB).

### Known limitations / next steps
- Manual Ableton test plan not yet executed (no Live licence on this
  machine) — see docs/manual-test-plan.md.
- No OpenGL tier (software renderer only, by design for v0.1.0).
- Routine timeline editor + constellation map screen are generation-
  driven only; `routinePosition` automation currently minimal.
- CI workflow authored but not yet exercised (no remote configured).
