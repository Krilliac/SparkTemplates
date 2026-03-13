# 14 — Material Lab  (Intermediate)

An interactive material/rendering laboratory demonstrating PBR workflows,
transparency modes, emissive bloom, subsurface scattering, material instancing,
render queues, and render-path switching.

## What you will learn

| Concept | Engine API |
|---------|-----------|
| PBR materials | `MaterialComponent` (albedo, roughness, metallic, normal, AO) |
| Transparency | `MaterialComponent::TransparencyMode` (Opaque, AlphaTest, AlphaBlend) |
| Emissive / bloom | Emissive color + `PostProcessingSystem` bloom |
| Subsurface scattering | SSS parameters on `MaterialComponent` |
| Material instancing | `MaterialComponent::CloneFrom()` with overrides |
| Render queues | `MaterialComponent::RenderQueue` (Opaque, Transparent, Overlay) |
| Render paths | `GraphicsEngine::SetRenderPath()` (Forward, Deferred, ForwardPlus) |
| UV animation | Tiling and offset manipulation per frame |

## Controls

| Key | Action |
|-----|--------|
| 1 / 2 / 3 / 4 | Cycle material presets (Metal, Plastic, Glass, Skin) |
| M | Toggle metallic on selected row |
| T | Toggle transparency mode |
| E | Toggle emissive |
| R | Cycle render paths (Forward / Deferred / ForwardPlus) |
| + / - | Adjust roughness |
| Mouse wheel | Zoom camera |
| Right-click drag | Orbit camera |

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

This produces `MaterialLab.dll` (Windows) or `libMaterialLab.so` (Linux).
Run it with the engine:

```bash
# Windows
SparkEngine.exe -game MaterialLab.dll

# Linux
./SparkEngine -game libMaterialLab.so
```
