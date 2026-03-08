# 03 — Physics Playground  (Intermediate)

An interactive physics sandbox inside a walled arena.  Spawn boxes, bouncy
spheres, and pyramids, then launch projectiles to knock them down.

## What you will learn

| Concept | Engine API |
|---------|-----------|
| Rigid body types | `RigidBodyComponent` — Static, Kinematic, Dynamic |
| Collider shapes | `ColliderComponent` — Box, Sphere |
| Physics properties | mass, friction, restitution, linearVelocity |
| Runtime entity spawning | `World::CreateEntity()` + component attachment in loops |
| Entity destruction | `World::DestroyEntity()`, batch cleanup |
| Collision events | `EventBus::Subscribe<CollisionEvent>` |
| Scene architecture | Static arena + dynamic spawned objects |

## Controls

| Key | Action |
|-----|--------|
| 1 | Drop a box from above |
| 2 | Drop a bouncy sphere from above |
| 3 | Build a 4-high pyramid stack |
| 4 | Launch a projectile sphere into the scene |
| 5 | Clear all dynamic objects |
| R | Reset to initial state |

## Key physics settings

| Object | Mass | Friction | Restitution |
|--------|------|----------|-------------|
| Floor / Walls | 0 (static) | 0.8 | 0.2 |
| Box | 5 kg | 0.6 | 0.3 |
| Sphere | 3 kg | 0.4 | 0.7 (bouncy) |
| Projectile | 3 kg | 0.4 | 0.7 |

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

This produces `PhysicsPlayground.dll` (Windows) or `libPhysicsPlayground.so`
(Linux). Run it with the engine:

```bash
SparkEngine.exe -game PhysicsPlayground.dll
```
