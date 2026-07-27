# LUMI//DANCER — Testing

Two suites, both dependency-free micro-harnesses (`LD_TEST` / `JT_TEST`)
that print per-case PASS/FAIL and return nonzero on any failure.

## Core suite (JUCE-free, seconds to build)

```bash
cmake --preset core && cmake --build --preset core && ctest --preset core
```

72 cases / 2395 checks across: maths + RNG determinism, analysis (RMS,
bands, centroid ordering, transients, silence, stereo width, NaN input,
mono, adaptive reference), BeatClock (tracking, tempo change, seek, loop
wrap, stop/restart, stopped-cursor, free mode, invalid input, bar phase),
pose maths, all ten dance styles (constructibility, determinism, pairwise
distinctness, bounds, audio reactivity, signatures, freestyle seeding),
expressions + idle, Choreographer integration (blending, crossfades,
reduced motion, routine mode, NaN), constellation (library integrity,
choose-next determinism/complexity/repeat-avoidance, routine generation/
serialisation/playback modes), particles (capacity, spawn rules, decay,
reduced), state (full round trip, corrupt fallback, unknown keys, future
schema, clamps, presets).

## JUCE suite

```bash
cmake --preset windows
cmake --build --preset windows-release --target LumiTests
ctest --preset windows-tests
```

21 cases / 127 checks: bit-exact pass-through (silence/sine/noise/impulse
× mono/stereo × 44.1/48/96 kHz × 64–1024 blocks), silence flag, analysis
integration, NaN input, rapid bypass, host-sync bus plumbing, missing
playhead, parameter ID stability, binary state round trip, corrupt chunk,
preset application, editor lifecycle + size persistence, overlay
arbitration + geometry, multi-instance coexistence, renderer identity
palette / per-style bounds / customisation pixels / expression + mirror.

Set `LUMI_SNAPSHOT_DIR=<dir>` before running `LumiTests` to dump PNG
snapshots of Lumi for visual inspection.

## Sanitizers

`cmake --preset windows-asan` enables MSVC AddressSanitizer. MSVC has no
UBSan; the `LUMI_ENABLE_UBSAN` option applies on Clang/GCC only.

## Manual host testing

See [manual-test-plan.md](manual-test-plan.md) — required before any
non-alpha release; not yet executed for v0.1.0-alpha.
