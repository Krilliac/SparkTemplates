# 18 — Shadow & Lighting  (Advanced)

A comprehensive lighting and shadow showcase demonstrating SparkEngine's
rendering features including cascaded shadow maps, multiple light types, IBL,
screen-space reflections, ambient occlusion, and time-of-day transitions.

## What you will learn

| Concept | Engine API |
|---------|-----------|
| Cascaded Shadow Maps | `GraphicsEngine::SetCSMSplits()`, debug overlay |
| Point light shadows | `LightComponent::Type::Point`, omnidirectional |
| Spot light shadows | `LightComponent::Type::Spot`, cookie textures |
| Area lights | `LightComponent::Type::Area`, soft shadows |
| Shadow quality | Resolution, PCF filter size, bias configuration |
| Color temperature | Kelvin to RGB conversion for realistic lighting |
| Image-Based Lighting | `GraphicsEngine::SetEnvironmentMap()`, `ToggleIBL()` |
| Screen-Space Reflections | `GraphicsEngine::ToggleSSR()` |
| Ambient occlusion | `GraphicsEngine::SetSSAO()` |
| Day-night cycle | Animated directional light rotation |
| Light probes | `GraphicsEngine::PlaceLightProbe()` |

## Controls

| Key | Action |
|-----|--------|
| 1 | Toggle directional light |
| 2 | Toggle point lights |
| 3 | Toggle spot lights |
| 4 | Toggle area lights |
| S | Cycle shadow quality (Low / Medium / High / Ultra) |
| C | Toggle CSM debug visualisation |
| I | Toggle Image-Based Lighting |
| R | Toggle Screen-Space Reflections |
| A | Toggle Ambient Occlusion (SSAO) |
| + / - | Increase / decrease light intensity |
| T | Toggle time-of-day animation |
| W / A / S / D | Move camera |

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

This produces `ShadowLighting.dll` (Windows) or `libShadowLighting.so` (Linux).
Run it with the engine:

```bash
# Windows
SparkEngine.exe -game ShadowLighting.dll

# Linux
./SparkEngine -game libShadowLighting.so
```
