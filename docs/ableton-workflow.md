# LUMI//DANCER — Ableton Live workflow

## Install

1. Build (`./scripts/build-windows.ps1`) or unzip the portable package.
2. Copy `LUMI DANCER.vst3` into `C:\Program Files\Common Files\VST3\`.
3. In Live: Options → Preferences → Plug-Ins → rescan. LUMI//DANCER
   appears under ENAHKEM Audio.

## Everyday use

- **Where to load it**: any audio track, a return, or the Master channel.
  On Master, Lumi reacts to the whole mix. The plugin never changes the
  audio, so its position in the chain only affects what she hears.
- **Play**: Lumi dances beat-synchronised (top bar shows BPM + SYNCED).
  Stop: she idles — breathing, blinking, looking around; after a minute
  she sits, and after the sleep delay she naps.
- **Styles**: the DANCE panel picks one of ten personalities. Freestyle
  auto-walks the library; the CONSTELLATION section generates seeded
  routines instead (lock the seed to keep a routine forever — it is saved
  with the project and reproduces exactly).
- **Reaction shaping**: right panel. `Low` makes kicks hit harder, `Mid`
  drives snare arm-snaps, `High` drives sparkles, `Snap` scales all
  transient accents, `Smooth` calms everything. `Beat Lock` quantises the
  dance to a 16th grid.
- **Looks**: bottom bar — hair, outfit, gold accent, accessories,
  background (including your own PNG/JPEG), camera framing, frame rate.
- **Automation**: all reaction controls, style, mood, scale, mirror,
  opacity and bypass are host-automatable parameters with stable IDs.

## The floating overlay

Click **Detach**. Lumi leaves the plugin window and floats above Live in
a transparent window:

- drag to move, corner to resize, hover to reveal the close/pin chrome;
- the `v` menu next to Detach: Always on Top, Click Through, Lock
  Position, Overlay Background, Opacity, Reset Position;
- position/size are remembered in the project;
- with several LUMI//DANCER instances, whichever clicked Detach last owns
  the overlay — instances never fight, and closing the owning plugin
  closes the overlay safely.

## Tips

- 44.1/48/96 kHz and buffer sizes 64–1024 are all supported.
- Bypass pauses the analysis (Lumi rests); audio is untouched either way.
- Reduced Motion (right panel) + No Flashes + smaller Sparkle amount give
  a calm, accessibility-friendly stage; the Reduced Motion factory preset
  bundles these.
