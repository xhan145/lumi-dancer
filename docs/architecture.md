# LUMI//DANCER — Architecture

## Layering

```text
┌─────────────────────────────────────────────────────────────┐
│  JUCE layer (src/plugin, src/ui)                            │
│  Processor · Parameters · Editor · Stage · Renderer ·       │
│  Overlay window + controller                                │
├─────────────────────────────────────────────────────────────┤
│  lumi_core — JUCE-free C++20 static library                 │
│  analysis/  AnalysisEngine, AudioReactiveFrame              │
│  timing/    BeatClock, HostTimingSnapshot                   │
│  rig/       CharacterPose, interpolation, springs           │
│  dance/     10 styles, Choreographer, Expressions, Idle     │
│  constellation/ move stars, choreography engine, routines   │
│  fx/        fixed-pool ParticleSystem                       │
│  state/     LumiSettings blob, factory presets              │
│  core/      maths, deterministic RNG, palette               │
└─────────────────────────────────────────────────────────────┘
```

The core library depends only on the C++20 standard library, so the whole
product brain unit-tests in seconds (`cmake --preset core`). The JUCE layer
is intentionally thin: audio plumbing, painting, windows, parameters.

## Threading model

```text
audio thread                       message thread
────────────                       ──────────────
processBlock                       StageComponent timer (30/60 fps)
  read playhead ─┐                   read timingBus ──► BeatClock
  analysis tap ──┼─► AtomicSnapshot  read frameBus  ──► Choreographer ─► pose
  (no writes to  │   (seqlock)                          ParticleSystem
   the buffer)  ─┘                                      LumiRenderer.paint
```

- `AtomicSnapshot<T>` is a single-writer seqlock: the audio thread's
  publish is wait-free; readers retry on a torn read. Payloads are
  trivially copyable structs.
- The audio thread performs **no** allocation, locking, file I/O, JSON,
  logging or rendering. The analysis engine preallocates everything in
  `prepare()` (biquads, envelopes, a 256-point FFT workspace).
- All animation, particles and painting happen on the message thread.
  If the UI starves, audio is unaffected — the buses simply hold the last
  published snapshots.

## Determinism policy

Choreography must reproduce exactly for a locked seed across platforms:

- `SeededRng` (SplitMix64) everywhere; `std::` distributions are banned
  (their output is implementation-defined).
- Dance styles are pure functions of `(beat, bpm, audio, intensity)`;
  "memory" is derived by hashing beat segments (Freestyle, Breakcore).
- Routine generation, shuffle order and expression randomness all come
  from seeded streams.

## Host sync

`BeatClock` consumes `HostTimingSnapshot`s (published every audio block)
plus the UI frame delta. It advances locally at the host tempo and slews
onto the host PPQ; an error > 0.5 beats is treated as a seek/loop/restart,
snaps immediately, and raises a one-frame `discontinuityFlag` that the
Choreographer converts into a 200 ms pose crossfade — so seeks re-position
the dance instantly without teleporting limbs.

No host timeline (`hasPpq == false`, e.g. the standalone app) → FREE MODE:
the clock free-runs at the last known BPM (default 120) and the stage
shows a `FREE MODE` badge.

## State

`LumiSettings` is serialised as a versioned key=value text blob inside the
plugin's XML state chunk, next to the APVTS parameter tree. The parser
ignores unknown keys (forward-compatible), repairs bad values, clamps
ranges, and rejects future major schemas or garbage by falling back to
defaults — a corrupt project never crashes the plugin. Routine nodes embed
as a single escaped line; user background images are stored as external
paths only.

## Bounds guarantee

`sanitizePose` clamps every bone into legal ranges (root offset ≤ 0.35
character units) and the renderer's height budget reserves the worst-case
vertical/horizontal extent (max root lift + buns + companion star + arms
out), so no dance can push Lumi outside the visible stage. A renderer test
drives every style at maximum intensity and asserts the outermost pixel
ring stays empty.

## Overlay

One overlay per process, owned by `OverlayController` (message-thread
singleton). Claiming instance B while A owns it closes A's overlay first
(polite steal). The overlay window is a desktop `Component` with
`windowIsTemporary` (+ `windowIgnoresMouseClicks` when click-through);
toggling click-through recreates the peer. The owning processor's
destructor tears the overlay down synchronously so it can never dangle.
