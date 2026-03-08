# 02 — Ambient Scene  (Basic)

A serene outdoor scene showcasing Spark Engine's environmental systems.
Watch the sun track across the sky while weather shifts between clear,
cloudy, rainy, and snowy conditions.

## What you will learn

| Concept | Engine API |
|---------|-----------|
| Day/night cycle | `DayNightCycle` — `SetTime`, `Update`, `GetSunDirection/Color/Intensity` |
| Sun direction sync | Mapping cycle output → `Transform` rotation + `LightComponent` colour |
| Light types | Directional (sun), Point (lanterns), Spot (accent) via `LightComponent` |
| Weather | `WeatherComponent` — type, intensity, wind |
| Event system | `EventBus::Subscribe`, `Publish`, `Unsubscribe` |
| Built-in events | `TimeOfDayChangedEvent`, `WeatherChangedEvent` |
| Scene composition | Multiple entities with transforms, meshes, lights |

## Controls

| Key | Action |
|-----|--------|
| T | Toggle time-scale (1x real-time ↔ 60x fast-forward) |
| C | Cycle weather: Clear → Cloudy → Rain → Snow |

## Building

```bash
cmake -B build -DCMAKE_PREFIX_PATH=<SparkEngine install>
cmake --build build --config Release
```
