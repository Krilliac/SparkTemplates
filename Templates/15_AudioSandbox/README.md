# 15 — Audio Sandbox  (Intermediate)

An interactive audio playground demonstrating 3D positional audio, reverb
zones, Doppler effect, music management, sound occlusion, channel pooling,
and EventBus-driven sound triggers.

## What you will learn

| Concept | Engine API |
|---------|-----------|
| Loading / playing sounds | `AudioEngine::LoadSound()`, `PlaySound()`, `Play3DSound()` |
| Music management | `MusicManager` playlists, crossfade, bus volumes |
| Audio buses | `AudioBus::Music`, `SFX`, `Ambient`, `Voice` |
| 3D positional audio | Distance attenuation, listener position |
| Reverb zones | `ReverbZone` (None, SmallRoom, Hall, Cave, Arena, Outdoor) |
| Doppler effect | Moving sound sources with velocity |
| Sound occlusion | Ray-based wall occlusion factor |
| Audio emitter entities | ECS-based `AudioEmitterComponent` |
| EventBus integration | Sound triggers from game events |

## Controls

| Key | Action |
|-----|--------|
| 1 / 2 / 3 / 4 / 5 | Play different sounds (gunshot, explosion, bell, alarm, footstep) |
| M | Toggle background music |
| R | Cycle reverb zones |
| + / - | Master volume up / down |
| W / A / S / D | Move audio listener |
| Space | Spawn a moving sound source (Doppler demo) |

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

This produces `AudioSandbox.dll` (Windows) or `libAudioSandbox.so` (Linux).
Run it with the engine:

```bash
# Windows
SparkEngine.exe -game AudioSandbox.dll

# Linux
./SparkEngine -game libAudioSandbox.so
```
