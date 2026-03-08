# 10 — Scripting Playground  (Intermediate)

An AngelScript sandbox demonstrating runtime scripting, lifecycle callbacks,
hot-reload, and multiple script modules attached to ECS entities.

## What you will learn

| Concept | Engine API |
|---------|-----------|
| Script compilation | `AngelScriptEngine::CompileScriptFile()`, `CompileScriptFromString()` |
| Script attachment | `AngelScriptEngine::AttachScript(entity, className, moduleName)` |
| Lifecycle callbacks | `CallStart(entity)`, `CallUpdate(entity, dt)`, `CallOnCollision(entity, other)` |
| Hot-reload | `DetachScript()` → recompile → `AttachScript()` → `CallStart()` |
| Script component | `Script { scriptPath, className, moduleName, enabled }` |
| Script globals | `ASPrint()`, `ASCreateEntity()`, `ASGetTransform()`, `ASGetKeyDown()` |

## Controls

| Key | Action |
|-----|--------|
| F5 | Hot-reload all scripts (recompile from files) |
| 1 | Spawn a spinning cube with Spinner script |
| 2 | Spawn an orbiting sphere with Orbiter script |

## Scripts included

| Script | Behavior |
|--------|----------|
| `Spinner` | Rotates entity, bobs up/down with sine wave |
| `Orbiter` | Moves entity in a circle, faces movement direction |
| `SpinnerController.as` | File-based script with input handling (Space reverses rotation) |

## Hot-reload workflow

1. Edit `Assets/Scripts/SpinnerController.as` in any text editor
2. Press F5 in-game
3. Scripts are recompiled and re-attached without restarting

## Building

```bash
cmake -B build -DCMAKE_PREFIX_PATH=<SparkEngine install>
cmake --build build --config Release
```
