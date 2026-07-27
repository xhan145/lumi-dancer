# LUMI//DANCER — Manual Ableton test plan

Repeatable checklist for release candidates. Automated suites cover the
audio/state/animation contracts; this plan covers what only a real host
session can prove.

Status legend: [ ] not run · [x] pass · [!] fail (file an issue).

## Setup

| # | Step | Status (v0.1.0-alpha) |
|---|------|-----------------------|
| 1 | Live scans the VST3 without warnings | [ ] not run |
| 2 | Loads on an audio track | [ ] not run |
| 3 | Loads on the Master channel | [ ] not run |

## Audio integrity

| # | Step | Status |
|---|------|--------|
| 4 | Null test: duplicate track, phase-invert one, add LUMI//DANCER to it → silence | [ ] not run |
| 23–25 | Repeat at 44.1 / 48 / 96 kHz | [ ] not run |
| 26 | Buffer sizes 64, 128, 256, 512, 1024 | [ ] not run |
| 27 | Rapid bypass toggling while playing | [ ] not run |

## Sync & dancing

| # | Step | Status |
|---|------|--------|
| 5 | Host BPM shown correctly, updates with tempo changes | [ ] not run |
| 6 | Lumi dances on play, in time | [ ] not run |
| 7 | Stop → idle behaviour (breathe/blink; sit after 60 s; sleep after delay) | [ ] not run |
| 8 | Seek in arrangement → choreography re-positions without limb snapping | [ ] not run |
| 9 | 4-bar loop for 5+ minutes → no drift vs the beat pips | [ ] not run |
| 10 | Tempo change 90→174 BPM → dance speed follows | [ ] not run |
| 11 | Kick-heavy loop → body bounce follows kicks | [ ] not run |
| 12 | Hat-heavy loop → sparkles follow hats | [ ] not run |
| 13 | Cycle all ten styles → visibly different choreography | [ ] not run |

## Features

| # | Step | Status |
|---|------|--------|
| 14 | Each factory preset loads and visibly changes behaviour/look | [ ] not run |
| 15 | Hair/outfit/accent/accessory changes apply immediately | [ ] not run |
| 16 | Detach opens the overlay; Lumi keeps dancing in it | [ ] not run |
| 17 | Always-on-top keeps the overlay above Live | [ ] not run |
| 18 | Click-through lets clicks reach Live underneath | [ ] not run |
| 19 | Close/reopen project → overlay position restored | [ ] not run |
| 20 | Two instances: Detach on each in turn → single overlay, polite hand-off; deleting the owner closes it | [ ] not run |
| 21 | Save project, reopen → style/look/routine/settings restored | [ ] not run |
| 22 | Automate Intensity + Dance Style from a clip envelope | [ ] not run |
| 28 | (n/a in v0.1.0 — no OpenGL path; software renderer only) | [x] n/a |
| 29 | Reduced Motion: slower, no flashes, fewer particles, still beat-aware | [ ] not run |
| 30 | Add/delete the plugin 10× in one session → no leaks/crashes | [ ] not run |

> v0.1.0-alpha: this plan has **not yet been executed in Ableton Live** —
> no Live licence is available on the build machine. The standalone app
> and the automated integration suite are the current evidence base. Run
> this plan on first Live install and update the status column.
