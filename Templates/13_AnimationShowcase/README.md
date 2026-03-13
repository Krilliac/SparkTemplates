# 13 — Animation Showcase  (Intermediate)

A comprehensive demonstration of SparkEngine's skeletal animation system
including state machines, blend trees, animation layers, inverse kinematics,
and animation events.

## What you will learn

| Concept | Engine API |
|---------|-----------|
| Animation clips | `AnimationComponent` with named clips |
| State machines | `AnimationStateMachine` with states and transitions |
| Animation blending | Crossfade, blend weights, transition durations |
| Animation layers | `AnimationLayer` (base body + upper body override) |
| Inverse kinematics | `IKComponent`, `IKTarget` for feet/hand placement |
| Animation events | Keyframe callbacks (footstep sounds, VFX triggers) |
| Runtime control | Speed, weight, pause/resume manipulation |
| NPC spawning | Multiple animated entities with independent state |

## Controls

| Key | Action |
|-----|--------|
| W / A / S / D | Move player (triggers walk/run blending) |
| Space | Jump animation |
| LMB (Left Mouse) | Attack animation |
| 1 | Spawn patrol NPC |
| 2 | Spawn dance NPC |
| I | Toggle IK (foot/hand placement) |
| P | Pause / resume all animations |

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

This produces `AnimationShowcase.dll` (Windows) or `libAnimationShowcase.so`
(Linux). Run it with the engine:

```bash
# Windows
SparkEngine.exe -game AnimationShowcase.dll

# Linux
./SparkEngine -game libAnimationShowcase.so
```
