# 17 — Platformer Demo  (Intermediate)

A 2.5D platformer game demonstrating full gameplay integration with the
SparkEngine ECS.  Features player physics, moving platforms, collectible coins,
patrol enemies, checkpoints, death zones, and a smooth follow camera.

## What you will learn

| Concept | Engine API |
|---------|-----------|
| Platform physics | `RigidBodyComponent`, `ColliderComponent`, gravity simulation |
| Moving platforms | Kinematic bodies, sine-wave motion |
| Collectibles | `TagComponent`, `EventBus` (CoinCollectedEvent) |
| Patrol AI | `AIComponent` state machine, waypoint movement |
| Death / respawn | `EventBus` (PlayerDiedEvent), checkpoint system |
| Camera follow | Smooth damping via `Transform` lerp |
| Health & lives | `HealthComponent`, score tracking |
| Jump pads | Spring-loaded launch via velocity override |

## Controls

| Key | Action |
|-----|--------|
| A / D | Move left / right |
| Space | Jump |
| Space (in air) | Double-jump |
| R | Restart level |

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

This produces `PlatformerDemo.dll` (Windows) or `libPlatformerDemo.so` (Linux).
Run it with the engine:

```bash
# Windows
SparkEngine.exe -game PlatformerDemo.dll

# Linux
./SparkEngine -game libPlatformerDemo.so
```
