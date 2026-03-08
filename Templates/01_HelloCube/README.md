# 01 — Hello Cube  (Basic)

A minimal Spark Engine project that creates a spinning cube you can move with
the keyboard.  This is the "Hello World" of Spark and the best place to start.

## What you will learn

| Concept | Engine API |
|---------|-----------|
| Creating a game module | `Spark::IModule`, `SPARK_IMPLEMENT_MODULE` |
| Accessing engine subsystems | `Spark::IEngineContext` |
| Creating entities | `World::CreateEntity()` |
| Adding components | `World::AddComponent<T>()` |
| Core components | `NameComponent`, `Transform`, `MeshRenderer` |
| Lighting | `LightComponent` (Point light) |
| Input polling | `InputManager::IsKeyDown()`, `WasKeyPressed()` |

## Controls

| Key | Action |
|-----|--------|
| W / A / S / D | Move cube |
| Q / E | Rotate left / right |
| R | Pitch up |
| Space | Toggle 1x ↔ 2x scale |

## Building

```bash
cmake -B build -DCMAKE_PREFIX_PATH=<SparkEngine install>
cmake --build build --config Release
```

The engine executable will automatically load `HelloCube.dll` at startup.
