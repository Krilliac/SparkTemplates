# 07 — Procedural World  (Advanced)

A fully procedurally generated landscape with noise-based terrain, erosion,
rule-based vegetation/rock scattering, and a Wave Function Collapse dungeon.

## What you will learn

| Concept | Engine API |
|---------|-----------|
| Noise functions | `NoiseGenerator` — Perlin, Simplex, Worley, FBM, Ridged, Warped |
| Heightmap terrain | `HeightmapGenerator::Generate()`, `ThermalErosion()`, `HydraulicErosion()` |
| Procedural meshes | `ProceduralMesh::CreateTerrainFromHeightmap()`, `CreateRock()`, `CreateTree()` |
| Object scattering | `ObjectPlacer::Scatter()` with slope/height/density rules |
| WFC generation | `WaveFunctionCollapse` — tiles, sockets, rotation, collapse |
| LOD system | `LODManager::GenerateLODChain()`, distance-based LOD selection |
| Regeneration | Destroy + recreate entities with new seeds |

## Controls

| Key | Action |
|-----|--------|
| G | Regenerate the entire world (new seed) |
| E | Toggle erosion on/off and regenerate terrain |
| + | Increase noise octaves (more detail) |
| - | Decrease noise octaves (smoother) |

## Generation pipeline

1. **Heightmap** — FBM noise with configurable octaves, frequency, amplitude
2. **Erosion** — 30 thermal + 50 hydraulic iterations for realistic valleys/ridges
3. **Object scatter** — Trees on gentle slopes (0-30 deg), rocks on steep slopes (20-70 deg)
4. **WFC dungeon** — 16x16 tile grid with floor/wall/corner/pillar tiles, placed alongside terrain

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

This produces `ProceduralWorld.dll` (Windows) or `libProceduralWorld.so`
(Linux). Run it with the engine:

```bash
SparkEngine.exe -game ProceduralWorld.dll
```
