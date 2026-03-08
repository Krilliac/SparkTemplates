# 05 — AI Arena  (Intermediate)

An arena populated with AI agents that patrol, guard, and flee using the
engine's behavior tree, NavMesh, and perception systems.

## What you will learn

| Concept | Engine API |
|---------|-----------|
| Behavior trees | `BehaviorTree`, `SequenceNode`, `SelectorNode`, `ActionNode`, `ConditionNode` |
| Pre-built behaviors | `CreatePatrolBehavior()`, `CreateGuardBehavior()`, `CreateFleeBehavior()` |
| Blackboard data | `Blackboard::SetFloat()`, `SetBool()`, `SetFloat3()` |
| NavMesh pathfinding | `NavMeshBuilder::Build()`, `NavMeshQuery::FindPath()` |
| AI state machine | `AIComponent::State` — Idle, Patrolling, Alert, Combat, Fleeing |
| Perception | `PerceptionSystem::CanSee()`, `CanHear()` (cone + radius checks) |
| Steering | `SteeringBehaviors` — Seek, Flee, Wander, Arrive, Separation |
| AI events | `EntityDamagedEvent`, `EntityKilledEvent` |

## Controls

| Key | Action |
|-----|--------|
| WASD | Move the player entity |
| 1 | Spawn an extra patrolling agent |
| 2 | Spawn an extra guard agent |
| R | Reset all AI agents |

## AI agent types

| Agent | Behavior | Detection | Speed |
|-------|----------|-----------|-------|
| Patroller (blue) | Wanders in a radius, chases player on sight | 15 m | 4 m/s |
| Guard (red) | Holds position, attacks if player enters zone | 20 m | 3 m/s |
| Coward (green) | Flees when player approaches | 25 m | 6 m/s |

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

This produces `AIArena.dll` (Windows) or `libAIArena.so` (Linux). Run it with
the engine:

```bash
SparkEngine.exe -game AIArena.dll
```
