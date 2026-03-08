# 09 — Cinematic Demo  (Intermediate)

A cutscene showcase demonstrating the sequencer system with camera paths,
subtitles, audio cues, screen fades, and coroutine-driven scripted events.

## What you will learn

| Concept | Engine API |
|---------|-----------|
| Sequencer playback | `SequencerManager::CreateSequence()`, `PlaySequence()`, `StopAll()` |
| Camera paths | `AddCameraPathKey()` — position, rotation, FOV, roll over time |
| Interpolation | CatmullRom (smooth curves), CubicBezier (eased), Linear |
| Subtitles | `AddSubtitleKey()` — text display at timed intervals |
| Audio cues | `AddAudioCueKey()` — trigger sound effects during playback |
| Screen fades | `AddFadeKey()` — fade in/out transitions |
| Entity animation | `AddEntityTransformKey()` — animate entity transforms over time |
| Event triggers | `AddEventKey()` — fire named events from sequences |
| Coroutines | `CoroutineScheduler::StartCoroutine().Do().WaitForSeconds()` chain |
| Audio system | `AudioEngine::LoadSound()`, `PlaySound()`, `MusicManager` buses |
| Reverb zones | `MusicManager::SetReverbZone()` — SmallRoom, Hall, Cave, Outdoor |

## Controls

| Key | Action |
|-----|--------|
| 1 | Play intro cinematic (9-sec sweep with subtitles) |
| 2 | Play flyover cinematic (12-sec aerial view) |
| 3 | Start coroutine demo (timed sound chain) |
| Escape | Stop all playing cinematics |

## Sequences

### Intro Cinematic (9 seconds)
- Fade-in from black
- CatmullRom camera sweep around the arena
- Three subtitle cards
- Whoosh + reveal sound effects

### Flyover Cinematic (12 seconds)
- High-altitude CatmullRom camera orbit
- Entity animation (turret rotates 360 degrees)
- Camera roll effects for dramatic tilt

## Building

Requires an **installed** SparkEngine SDK (see the repository root
[README](../../README.md#prerequisites) for install instructions).

```bash
cmake -B build -DCMAKE_PREFIX_PATH=<path-to-SparkEngine-install-prefix>
cmake --build build --config Release
```

> `CMAKE_PREFIX_PATH` must point at the SparkEngine **install prefix** (the
> directory you passed to `cmake --install --prefix`), **not** the build tree or
> its `bin/` subdirectory.

This produces `CinematicDemo.dll` (Windows) or `libCinematicDemo.so` (Linux).
Run it with the engine:

```bash
SparkEngine.exe -game CinematicDemo.dll
```
