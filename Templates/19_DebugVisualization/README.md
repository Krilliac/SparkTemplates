# 19 — Debug Visualization  (Advanced)

Demonstrates SparkEngine's debug and development tools including the DebugDraw
system, wireframe rendering, physics overlays, NavMesh visualisation, performance
statistics, entity inspection, raycasting, and an in-game console.

## What you will learn

| Concept | Engine API |
|---------|-----------|
| Debug drawing | `DebugDraw::Line()`, `Box()`, `Sphere()`, `Arrow()`, `Text()` |
| Wireframe mode | `GraphicsEngine::SetWireframe()` |
| Physics debug | `GraphicsEngine::TogglePhysicsDebug()` |
| NavMesh overlay | `GraphicsEngine::ToggleNavMeshDebug()` |
| AI perception | `DebugDraw::Cone()`, `DebugDraw::Circle()` |
| Performance stats | `GraphicsEngine::GetRenderStats()` |
| Entity inspector | Component iteration, transform gizmo |
| Frustum debug | Camera frustum line drawing |
| Bounding boxes | Per-mesh AABB visualisation |
| Raycasting | `World::Raycast()`, hit-point visualisation |
| World grid | `DebugDraw` line grid |
| Console | `SimpleConsole` with custom commands |

## Controls

| Key | Action |
|-----|--------|
| F1 | Toggle debug overlay (axes, origins) |
| F2 | Toggle wireframe rendering |
| F3 | Toggle physics debug (colliders, contacts) |
| F4 | Toggle NavMesh debug overlay |
| F5 | Toggle performance statistics |
| G | Toggle world grid |
| Left Click | Raycast / select entity |
| C | Open / close console |
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

This produces `DebugVisualization.dll` (Windows) or `libDebugVisualization.so`
(Linux). Run it with the engine:

```bash
# Windows
SparkEngine.exe -game DebugVisualization.dll

# Linux
./SparkEngine -game libDebugVisualization.so
```
