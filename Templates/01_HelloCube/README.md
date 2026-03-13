# 01 — Hello Cube  (Basic)

A minimal Spark Engine project that creates a spinning cube you can move with
the keyboard and orbit with the mouse.  This is the "Hello World" of Spark and
the best place to start.

## What you will learn

| Concept | Engine API |
|---------|-----------|
| Creating a game module | `Spark::IModule`, `SPARK_IMPLEMENT_MODULE` |
| Accessing engine subsystems | `Spark::IEngineContext` |
| Creating entities | `World::CreateEntity()` |
| Adding components | `World::AddComponent<T>()` |
| Core components | `NameComponent`, `Transform`, `MeshRenderer` |
| Camera setup | `Camera` component (FOV, near/far, active) |
| Lighting | `LightComponent` (Point light) |
| Keyboard input | `InputManager::IsKeyDown()`, `WasKeyPressed()` |
| Mouse input | `GetMouseDelta()`, `GetMouseScrollDelta()`, `IsMouseButtonDown()` |

## Controls

| Key | Action |
|-----|--------|
| W / A / S / D | Move cube |
| Q / E | Rotate left / right |
| R | Pitch up |
| Space | Toggle 1x / 2x scale |
| Right-click drag | Orbit camera around cube |
| Mouse wheel | Zoom in / out |

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

This produces `HelloCube.dll` (Windows) or `libHelloCube.so` (Linux). Run it
with the engine:

```bash
# Windows
SparkEngine.exe -game HelloCube.dll

# Linux
./SparkEngine -game libHelloCube.so
```
