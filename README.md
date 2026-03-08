# SparkTemplates

A collection of example projects and templates for **SparkEngine**, progressing from basic "Hello World" samples to production-grade systems like multiplayer networking and dedicated servers.

## Templates

| # | Name | Difficulty | Description |
|---|------|------------|-------------|
| 01 | **HelloCube** | Basic | ECS fundamentals — entity creation, components, input handling, and scene setup |
| 02 | **AmbientScene** | Basic | Dynamic day/night cycles, weather systems, multiple light types, and EventBus usage |
| 03 | **PhysicsPlayground** | Intermediate | Bullet Physics integration — rigid bodies, colliders, collision events, and runtime spawning |
| 04 | **RPGDemo** | Intermediate | Health, inventory, quests, save/load systems, and gameplay event pipelines |
| 05 | **AIArena** | Intermediate | Behavior trees, NavMesh pathfinding, perception systems, and steering behaviors |
| 06 | **ParticleShowcase** | Intermediate | Particle emitters, decals, fog modes, and post-processing effects pipeline |
| 07 | **ProceduralWorld** | Advanced | Noise-based terrain generation, erosion simulation, and Wave Function Collapse dungeons |
| 08 | **MultiplayerArena** | Advanced | UDP client/server networking, entity replication, input prediction, and lag compensation |
| 09 | **CinematicDemo** | Intermediate | Sequencer-driven cutscenes, camera paths, subtitles, audio cues, and screen fades |
| 10 | **ScriptingPlayground** | Intermediate | AngelScript integration with hot-reload, lifecycle callbacks, and entity scripting |
| 11 | **StressTest** | Advanced | Simultaneous stress test of every engine system at extreme load |
| 12 | **DedicatedServer** | Advanced | Headless authoritative game server with bot AI, match lifecycle, and admin console |

## Prerequisites

These templates require a **built and installed** copy of SparkEngine on your
system. "Installed" means you have run CMake's install step — pointing at the
raw build tree will not work because the exported CMake config files
(`SparkEngineTargets.cmake`, `SparkGameModule.cmake`, etc.) are only generated
during installation.

### Installing SparkEngine

```bash
# 1. Build SparkEngine (if you haven't already)
cd /path/to/SparkEngine
cmake -B build
cmake --build build --config Release

# 2. Install to a local prefix (e.g. ~/SparkEngine-install)
cmake --install build --prefix ~/SparkEngine-install
```

After this, `~/SparkEngine-install` will contain the engine executable, headers,
libraries, and — critically — the CMake package config files that
`find_package(SparkEngine)` needs.

> **Common mistake:** Passing the build directory or the `bin/` subdirectory as
> `CMAKE_PREFIX_PATH`. This will fail because the build tree does not contain
> the installed config files. Always point at the **install prefix** (the path
> you passed to `--prefix`).

## Getting Started

Each template is a standalone CMake project located under `Templates/`:

```
Templates/
├── 01_HelloCube/
│   ├── CMakeLists.txt
│   └── Source/
├── 02_AmbientScene/
│   ├── CMakeLists.txt
│   └── Source/
└── ...
```

To build a template:

```bash
cd Templates/01_HelloCube
cmake -B build -DCMAKE_PREFIX_PATH=~/SparkEngine-install
cmake --build build --config Release
```

## How templates relate to the engine

Each template compiles into a **game module** — a shared library (`.dll` on
Windows, `.so` on Linux) — not a standalone executable. The SparkEngine
executable loads the game module at startup:

```bash
# Windows
SparkEngine.exe -game HelloCube.dll

# Linux
./SparkEngine -game libHelloCube.so
```

Because templates are separate shared libraries, they are **built independently
from the engine**. You do not need the engine source tree at build time — only
the installed SDK (headers + CMake config). However, a game module cannot be
hot-swapped while the engine is running; you must restart the engine to pick up
a rebuilt module. (AngelScript scripts in Template 10 *can* be hot-reloaded at
runtime, but compiled C++ modules cannot.)

## Template Details

### 01 — HelloCube

The "Hello World" of SparkEngine. Creates an ECS world, attaches mesh and transform components to a cube entity, adds a point light, and handles keyboard input for movement, rotation, and scaling.

**Controls:** WASD move, Q/E rotate, R pitch, Space scale

### 02 — AmbientScene

Demonstrates environmental systems with a sun that follows a day/night arc, lanterns and accent spotlights, and a weather system that cycles through Clear, Cloudy, Rain, and Snow. Uses EventBus subscriptions for time-of-day and weather change events.

**Controls:** T toggle time scale (1x/60x), C cycle weather

### 03 — PhysicsPlayground

An interactive sandbox built on Bullet Physics. Spawns static floors/walls, dynamic boxes and bouncy spheres, and supports launching projectiles. Demonstrates rigid body configuration (mass, friction, restitution), collider shapes, and collision event handling.

**Controls:** 1-3 spawn objects, 4 fire projectile, 5 clear all, R reset

### 04 — RPGDemo

A complete gameplay loop with health/damage mechanics, a slot-based inventory with weight tracking, a four-quest progression system, and a full save/load pipeline including auto-save. Events drive damage, kills, item pickups, and quest completion.

**Controls:** WASD move, E pick up items, X damage, H heal, Q advance quest, F5/F9 quick save/load

### 05 — AIArena

Showcases AI with behavior trees (Sequence, Selector, Action, Condition nodes), blackboard data sharing, NavMesh pathfinding, and a perception system with sight cones and hearing radius. Three agent archetypes: Patroller (wanders and chases), Guard (holds position), and Coward (flees on approach).

**Controls:** WASD move player, 1/2 spawn agents, R reset

### 06 — ParticleShowcase

A visual effects gallery featuring five particle emitter stations (fire, smoke, sparks, magic, rain), a decal system with surface-type mapping, four fog modes (Linear, Exponential, Height, Volumetric), and a post-processing pipeline with Bloom, SSAO, ToneMapping, ColorGrading, and FXAA.

**Controls:** 1-4 trigger effects, F cycle fog, B toggle bloom

### 07 — ProceduralWorld

Procedurally generates landscapes using noise functions (Perlin, Simplex, Worley, FBM, Ridged, Warped), applies thermal and hydraulic erosion, scatters objects based on slope/height rules, and generates tile-based dungeons with Wave Function Collapse. Includes LOD management for terrain meshes.

**Controls:** G regenerate world, E toggle erosion, +/- adjust octaves

### 08 — MultiplayerArena

A networked deathmatch template with UDP client/server architecture, entity replication, client-side input prediction, server reconciliation, and lag compensation with snapshot rewinding. Supports reliable, unreliable, and ordered message channels plus a chat system.

**Controls:** H host, J join, WASD move, T chat

### 09 — CinematicDemo

Cutscene authoring with a Sequencer system, camera path interpolation (CatmullRom, CubicBezier, Linear), timed subtitles, audio cues, screen fades, and entity animation within timelines. Includes a CoroutineScheduler and AudioEngine integration with reverb zones.

**Controls:** 1/2/3 play sequences, Escape stop

### 10 — ScriptingPlayground

Integrates AngelScript for runtime scripting. Attach scripts to entities with lifecycle callbacks (Start, Update, OnCollision), compile from files or strings, and hot-reload without restarting. Provides script-accessible engine APIs for entity creation, transform access, and input queries.

**Controls:** F5 hot-reload scripts, 1/2 spawn scripted entities

### 11 — StressTest

Exercises every major engine system simultaneously: 500+ ECS entities, 300 physics bodies, 20 AI agents, 10 particle emitters, 30+ decals, cycling fog and weather, full post-processing, networking with 4 replicated entities, animation, procedural terrain, scripting, cinematics, audio, and auto-save. Designed for performance profiling and system validation.

**Controls:** WASD move, 1-5 stress triggers, 3 cycle render paths, F5/F9 save/load

### 12 — DedicatedServer

A headless authoritative game server for FPS deathmatch. Runs without a window, GPU, or audio. Features server-side physics validation, AI bots that fill empty player slots, a full match lifecycle (WaitingForPlayers → Warmup → Playing → Cooldown → MapRotation), and a console command interface for live administration.

**Server Commands:** `sv_status`, `sv_players`, `sv_kick`, `sv_bot_add`, `sv_bot_remove`, `sv_restart_round`, `sv_end_round`, `sv_save`, `sv_set`

**Launch:** `./DedicatedServer -dedicated -port 27015 -maxplayers 16`

## Engine Systems Covered

- **ECS** — Entity creation, component attachment, world management
- **Physics** — Bullet integration, rigid bodies, colliders, collision events
- **AI** — Behavior trees, NavMesh, pathfinding, perception, steering
- **Networking** — UDP client/server, replication, prediction, lag compensation
- **Rendering** — Mesh rendering, materials, lighting, shadows, post-processing
- **Particles** — Emitter shapes, blend modes, preset effects
- **Audio** — 3D positional sound, music management, reverb zones
- **Scripting** — AngelScript with hot-reload and lifecycle callbacks
- **Procedural Generation** — Noise functions, erosion, Wave Function Collapse
- **Cinematics** — Sequencer, camera paths, interpolation, subtitles
- **Save System** — Save, Load, QuickSave, QuickLoad, AutoSave
- **Events** — EventBus pub/sub for decoupled system communication
- **Day/Night & Weather** — Time-of-day cycles, weather state transitions

## Learning Path

1. **Start here:** Templates 01-02 for ECS and engine basics
2. **Core systems:** Templates 03-06 for physics, AI, particles, and RPG mechanics
3. **Content tools:** Templates 09-10 for cinematics and scripting
4. **Advanced:** Templates 07-08, 11-12 for procedural generation, networking, stress testing, and dedicated servers

## License

This project is licensed under the MIT License — see [LICENSE](LICENSE) for details.
