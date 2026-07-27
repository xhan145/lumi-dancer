# Changelog

All notable changes to LUMI//DANCER are documented here.

## [0.2.0] - 2026-07-27

- NEW: Painted (Anime) art style — 16 hand-painted sprite poses (original
  project artwork, embedded) beat-stepped per dance style with bounce/tilt/
  squash riding the rig pose; sunglasses "Cool" frame on sustained drops;
  sitting/shy frames while idle. Selectable per instance (Painted default,
  Vector still available with full customisation).
- FIX: detached overlay is now a true per-pixel-alpha layered window —
  transparent desktop-pet background instead of a black box (regression
  test asserts WS_EX_LAYERED on the live overlay).
- Vector Lumi rebuilt from welded silhouette paths (~half the primitives,
  cleaner look).

## [0.1.0-alpha] - 2026-07-27

Initial alpha. Portable package: `dist/LUMI-DANCER-v0.1.0-win64.zip`
(built by `scripts/package.ps1`; not committed).

- Transparent audio pass-through (mono/stereo/multichannel) with a
  real-time-safe analysis tap.
- Audio analysis: RMS, peak, low/mid/high band energy, spectral centroid,
  per-band transient detection, stereo width, silence detection.
- Host synchronisation: BPM, PPQ, transport state, seek/loop recovery,
  FREE MODE fallback when the host provides no timeline.
- Procedural Lumi mascot: layered vector character with a 2D bone rig,
  pose interpolation, spring-damped hair/accessory secondary motion.
- Ten dance styles: Bounce, Kawaii Pop, Orbit, Groove, Hyper, Chill,
  Breakcore, Drum & Bass, Trance, Freestyle.
- Expression system (9 expressions), blinking, idle behaviour with sleep.
- Constellation choreography: dance-move stars, compatibility graph,
  deterministic seeded routine generation, routine playback modes.
- Fixed-pool audio-reactive particles (stars, sparkles, hearts, rings,
  trails, moons).
- Customisation: hair palettes, outfits, gold accents, accessories,
  backgrounds.
- Factory presets (general, music-style and visual banks).
- Detachable transparent always-on-top overlay window with click-through
  and multi-instance arbitration.
- Accessibility: Reduced Motion, flash suppression, particle reduction,
  UI scaling, high contrast.
- Full state persistence with schema versioning and corrupt-state fallback.
