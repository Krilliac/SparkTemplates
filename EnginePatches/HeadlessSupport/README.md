# Engine Patch: Headless / Dedicated Server Support

This patch adds `-headless` and `-dedicated` command-line flags to SparkEngine,
enabling true headless dedicated server operation without a window, GPU, or audio device.

## Usage

```bash
# Launch as a dedicated server (no window, no GPU required)
SparkEngine.exe -headless -game MyServerModule.dll

# Or equivalently:
SparkEngine.exe -dedicated -game MyServerModule.dll

# Standard windowed mode (unchanged):
SparkEngine.exe -game MyGameModule.dll
```

## What runs in headless mode

| System | Status |
|--------|--------|
| ECS (World, entities, components) | Active |
| Physics (Bullet, rigid bodies, colliders) | Active |
| AI (behavior trees, NavMesh, perception) | Active |
| Networking (NetworkManager, replication) | Active |
| Events (EventBus, all event types) | Active |
| Save System | Active |
| Coroutines | Active |
| Day/Night Cycle, Weather | Active |
| Timer | Active |
| Console (stdin commands) | Active |
| **Graphics (rendering, shaders, textures)** | **Disabled** |
| **Input (keyboard, mouse)** | **Disabled** |
| **Audio (XAudio2, 3D sound)** | **Disabled** |

## Files modified

| File | Change |
|------|--------|
| `SparkEngine/Source/Core/SparkEngine.cpp` | Command-line parsing, headless init path, 60 Hz server loop, console allocation, Ctrl+C handler, Linux headless support |
| `SparkEngine/Source/Core/SparkEngine.h` | Export `g_headlessMode` global |
| `SparkEngine/Source/Core/EngineContext.h` | Add `IsHeadless()` method |
| `SparkEngine/Source/Core/IGameModule.h` | Make `Render()` non-pure-virtual (default empty body) |
| `SparkEngine/Source/Core/ModuleManager.h` | Updated docs |
| `SparkEngine/Source/Core/ModuleManager.cpp` | Skip `RenderAll()` in headless mode |
| `CMakeLists.txt` | Add `SPARK_HEADLESS_SUPPORT` compile option (ON by default) |

## Build

```bash
# With headless support (default):
cmake -B build -DSPARK_HEADLESS_SUPPORT=ON

# Without headless support (strip all headless code):
cmake -B build -DSPARK_HEADLESS_SUPPORT=OFF
```

## Architecture

The headless path is a complete early-return in `wWinMain`:

1. Parse `-headless` / `-dedicated` before any window creation
2. Allocate a Win32 console for stdout logging
3. Install Ctrl+C handler for graceful shutdown
4. Initialize only: Timer, EventBus, Physics, SaveSystem
5. Skip: Window, Graphics, Input, Audio
6. Load and initialize game modules normally (they receive `nullptr` for graphics/input/audio via `EngineContext`)
7. Run a fixed 60 Hz tick loop calling `ModuleManager::UpdateAll(dt)`
8. On Ctrl+C or `server_stop` console command: clean shutdown

On Linux, the same flag works via `main(argc, argv)` with `SIGINT`/`SIGTERM` handlers.

## Game module compatibility

Existing game modules work unchanged in windowed mode. For headless mode,
modules should either:

1. Check `context->IsHeadless()` in `OnLoad()` and skip graphics setup
2. Null-check `context->GetGraphics()` / `context->GetInput()` before use
3. Be purpose-built as server modules that never touch graphics APIs

The `IGameModule::Render()` method is now non-pure-virtual with a default
empty implementation, so legacy modules that don't override it will compile
without changes.
