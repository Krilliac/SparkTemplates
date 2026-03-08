# 04 — RPG Demo  (Intermediate)

A mini role-playing game framework demonstrating inventory management,
a quest system, health/damage, and the save/load pipeline.

## What you will learn

| Concept | Engine API |
|---------|-----------|
| Health system | `HealthComponent` — `TakeDamage()`, `Heal()`, `isDead` |
| Inventory | `InventoryTag` — maxSlots, maxWeight, currency |
| Quests | `QuestTrackerTag` — active/completed counts |
| Entity tags | `TagComponent` — `AddTag()`, `HasTag()` |
| Enable/disable | `ActiveComponent` |
| Save system | `SaveSystem` — `Save`, `Load`, `QuickSave`, `QuickLoad`, `AutoSave` |
| Gameplay events | `EntityDamagedEvent`, `EntityKilledEvent`, `ItemPickedUpEvent`, `QuestCompletedEvent` |
| Proximity checks | Distance-based pickup with manual range test |

## Controls

| Key | Action |
|-----|--------|
| W / A / S / D | Move player |
| E | Pick up nearby items (range = 3 units) |
| X | Take 15 damage (test) |
| H | Heal 20 HP |
| Q | Complete current quest |
| F5 | Quick-save |
| F9 | Quick-load |
| F6 | Save to slot "manual_save_1" |
| F7 | Load from slot "manual_save_1" |

## Quest line

1. Gather Supplies
2. Explore the Ruins
3. Defeat the Guardian
4. Return to Village

Press **Q** to advance through each quest sequentially.

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

This produces `RPGDemo.dll` (Windows) or `libRPGDemo.so` (Linux). Run it with
the engine:

```bash
SparkEngine.exe -game RPGDemo.dll
```
