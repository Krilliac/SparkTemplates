# 20 — UI Overlay  (Advanced)

Demonstrates HUD and UI rendering with SparkEngine, including health bars,
ammo counters, minimap, crosshair, damage indicators, menu system, inventory,
compass, notifications, and more.

## What you will learn

| Concept | Engine API |
|---------|-----------|
| Health bar | `DebugDraw::ScreenRect()`, smooth interpolation |
| Ammo display | `DebugDraw::ScreenText()` |
| Minimap | Entity-relative dot rendering |
| Crosshair | Dynamic spread based on movement |
| Damage indicator | Directional flash overlay |
| Notifications | Timed popup text queue |
| Menu system | Main menu, pause menu with options |
| Inventory grid | Grid-based item display |
| Compass | Bearing indicator at screen top |
| Interaction prompt | Context-sensitive "Press E" |
| Boss health bar | Centred large bar |
| Timer | Countdown display |
| UIWidget base | Position, size, visibility, alpha |

## Controls

| Key | Action |
|-----|--------|
| Tab | Toggle inventory |
| Escape | Pause menu |
| 1-4 | Trigger test notifications |
| H | Toggle HUD |
| M | Toggle minimap |
| X | Simulate damage (direction indicator) |
| W / A / S / D | Move player |

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

This produces `UIOverlay.dll` (Windows) or `libUIOverlay.so` (Linux). Run it
with the engine:

```bash
# Windows
SparkEngine.exe -game UIOverlay.dll

# Linux
./SparkEngine -game libUIOverlay.so
```
