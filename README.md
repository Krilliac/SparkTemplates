# SparkTemplates

A comprehensive collection of 28 example projects and templates for **SparkEngine**, progressing from basic "Hello World" samples to production-grade systems covering every engine capability.

## Templates

| # | Name | Difficulty | Description |
|---|------|------------|-------------|
| 01 | **HelloCube** | Basic | ECS fundamentals — entity creation, components, camera, mouse/keyboard input |
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
| 13 | **AnimationShowcase** | Intermediate | Skeletal animation, state machines, blending, layers, IK, and animation events |
| 14 | **MaterialLab** | Intermediate | PBR materials, emissive, transparency, subsurface scattering, and render paths |
| 15 | **AudioSandbox** | Intermediate | 3D positional audio, music playlists, bus mixing, reverb zones, and Doppler effect |
| 16 | **CameraToolkit** | Intermediate | FPS, TPS, orbit, and free-fly cameras; shake, DOF, split-screen, and letterbox |
| 17 | **PlatformerDemo** | Intermediate | 2.5D platformer with double-jump, moving platforms, enemies, coins, and checkpoints |
| 18 | **ShadowLighting** | Advanced | Cascaded shadows, area lights, IBL, SSR, light probes, and shadow quality tuning |
| 19 | **DebugVisualization** | Intermediate | Debug draw, wireframe, physics/NavMesh overlays, performance stats, and console |
| 20 | **UIOverlay** | Advanced | HUD health bars, minimap, crosshair, damage indicators, menus, inventory, and compass |
| 21 | **WeaponShowcase** | Intermediate | FPS weapon system — fire modes, recoil patterns, ADS, ammo, and reload |
| 22 | **DestructionDemo** | Advanced | Destruction system — fracture patterns, debris physics, and multi-stage damage |
| 23 | **DialogueDemo** | Intermediate | Branching dialogue trees — conditions, choices, events, and NPC conversations |
| 24 | **ReplayDemo** | Advanced | Replay recording and playback — seek, speed control, kill cam, and file save/load |
| 25 | **SplineRunner** | Intermediate | Spline paths and followers — CatmullRom, Bezier, Linear; Loop, PingPong, Once modes |
| 26 | **Game2D** | Intermediate | 2D engine — sprites, tilemaps, Camera2D, parallax backgrounds, and 2D physics |
| 27 | **WorldStreaming** | Advanced | Seamless area streaming, async asset loading, and floating-point origin rebasing |
| 28 | **ModdingPlayground** | Advanced | Mod system — scanning, loading, dependencies, enable/disable, and console commands |

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
│   ├── spark.project.json
│   ├── README.md
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

The "Hello World" of SparkEngine. Creates an ECS world, attaches mesh and transform components to a cube entity, adds a point light and camera, and handles keyboard and mouse input for movement, rotation, scaling, and camera orbit.

**Controls:** WASD move, Q/E rotate, R pitch, Space scale, Right-click drag orbit, Mouse wheel zoom

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

A headless authoritative game server for FPS deathmatch. Runs without a window, GPU, or audio. Features server-side physics validation, AI bots that fill empty player slots, a full match lifecycle (WaitingForPlayers -> Warmup -> Playing -> Cooldown -> MapRotation), and a console command interface for live administration.

**Server Commands:** `sv_status`, `sv_players`, `sv_kick`, `sv_bot_add`, `sv_bot_remove`, `sv_restart_round`, `sv_end_round`, `sv_save`, `sv_set`

**Launch:** `./DedicatedServer -dedicated -port 27015 -maxplayers 16`

### 13 — AnimationShowcase

Skeletal animation system demo with a rigged character. Features an AnimationStateMachine driving transitions between Idle, Walk, Run, Jump, and Attack states based on speed and input. Dual animation layers allow attacking while running (upper body override on base body). Foot and hand IK keeps feet planted on uneven surfaces. Animation events trigger footstep sounds at specific keyframes. Spawn NPC patrollers and dancers with their own state machines.

**Controls:** WASD move (triggers walk/run blend), Space jump, LMB attack, 1/2 spawn NPCs, I toggle IK, P pause animations

### 14 — MaterialLab

PBR material parameter playground. A grid of spheres shows all roughness/metallic combinations side by side. Demonstrates emissive materials that pulse and interact with bloom, transparent objects with alpha blending, subsurface scattering for skin/wax, material instance cloning with parameter overrides, UV tiling animation, and render path switching between Forward, Deferred, and ForwardPlus.

**Controls:** 1-4 cycle material presets, M toggle metallic, T toggle transparency, E toggle emissive, R cycle render paths, +/- roughness

### 15 — AudioSandbox

Full audio system exploration. Walk around a scene with 3D positioned sound emitters (fountain, fireplace, wind, bell, music box) that attenuate with distance. Cycle through reverb zones (None, SmallRoom, Hall, Cave, Arena, Outdoor). Control audio bus volumes independently (Music, SFX, Ambient, Voice). Trigger one-shot sounds, start/stop background music with crossfade, and spawn moving sound sources for Doppler effect demonstration.

**Controls:** WASD move listener, 1-5 play sounds, M toggle music, R cycle reverb, +/- volume, Space spawn moving source

### 16 — CameraToolkit

Camera system reference. Tab through four camera modes: First-Person (WASD + mouse look), Third-Person chase (spring-damped follow with orbit), Orbit (rotate/zoom around a point), and Free-Fly spectator (6DOF with shift for descent). Camera shake with parametric trauma/decay. Depth of Field with adjustable focus distance and aperture. Split-screen (two cameras, two viewports). Cinematic letterbox mode. FOV zoom animation (Z for sniper zoom).

**Controls:** Tab cycle modes, WASD/mouse move, K shake, F toggle DOF, V split-screen, L letterbox, Z zoom, P play camera path, +/- FOV

### 17 — PlatformerDemo

A playable 2.5D platformer game. Player character with gravity, jumping, and double-jump. Moving platforms on sine-wave paths. Spinning collectible coins with pickup events and score tracking. Patrol enemies that reverse direction at boundaries. Death zones below the map trigger respawn at the last checkpoint. Spring-loaded jump pads launch the player upward. Smooth-damped camera follow. Lives system with game-over and restart.

**Controls:** A/D move, Space jump (double-jump in air), R restart level

### 18 — ShadowLighting

Advanced lighting and shadow techniques. Cascaded Shadow Maps with configurable split distances. Point light omnidirectional shadows. Spot light shadows with cookie textures. Area lights (rectangle, disc) with soft shadows. Shadow quality cycling (resolution, PCF filter size, bias). Image-Based Lighting with environment cubemaps. Screen-Space Reflections. SSAO with tunable radius. Day-to-night transition showing shadow direction change. Interior and exterior lighting scenarios. Light probe placement for indirect lighting.

**Controls:** 1-4 toggle light types, S cycle shadow quality, C toggle CSM debug, I toggle IBL, R toggle SSR, A toggle AO, +/- intensity, T time-of-day

### 19 — DebugVisualization

Engine development and debugging tools. DebugDraw primitives (lines, wireframe boxes, spheres, arrows, screen text). Wireframe rendering mode toggle. Physics collider visualization (shapes and contact points). NavMesh walkable area overlay. AI perception cone display (sight/hearing). Performance statistics (FPS, frame time, draw calls, triangles, entity count, memory). Entity inspector (click to select, show components). Frustum and bounding box visualization. World-space grid. Raycast visualization (hit point and normal). SimpleConsole with custom commands.

**Controls:** F1 debug overlay, F2 wireframe, F3 physics debug, F4 NavMesh, F5 perf stats, G grid, Click raycast/select, C console

### 20 — UIOverlay

Complete HUD and menu system. Smooth-interpolated health bar with damage flash and heal glow. Ammo counter with reload animation. Minimap with player dot, enemy markers, and objective icons. Dynamic crosshair (spread widens with movement). Damage direction indicators (red arc from hit angle). Score display and kill feed. Toast notifications with timed fade-out. Pause menu with options. Inventory grid display. Compass/bearing indicator. Interaction prompts. Boss health bar. Countdown timer.

**Controls:** Tab inventory, Escape pause menu, 1-4 notifications, H toggle HUD, M toggle minimap, X simulate damage

### 21 — WeaponShowcase

FPS weapon system reference. Three weapons: Pistol (semi-auto, high accuracy), Rifle (full-auto, moderate spread, recoil pattern), Shotgun (semi-auto, wide spread). Demonstrates fire modes, recoil that affects camera pitch, ADS zoom with FOV change, magazine and reserve ammo tracking, reload mechanics, muzzle flash point lights, and weapon switch animations. Event-driven sound playback for fire/reload.

**Controls:** 1/2/3 switch weapons, LMB fire, RMB aim down sights, R reload

### 22 — DestructionDemo

Destruction system playground. Multiple destructible objects with different fracture patterns (Voronoi walls, Radial pillars, Slice floors, Uniform crates). Multi-stage damage progression from clean through cracked to fully destroyed with physics-driven debris. Supports point damage via raycast and area explosions. Debris has configurable lifetime and physics properties.

**Controls:** LMB shoot (raycast damage), Space explosion, R reset all, 1-4 select fracture type

### 23 — DialogueDemo

Branching dialogue tree system. Three NPCs (Merchant, Guard, Wizard) each with multi-path conversation trees. Walk near an NPC and press E to initiate dialogue. Choose responses with number keys. Dialogue outcomes affect game state (flags, score). Conditional choices appear based on prior decisions. UI rendered via DebugDraw with speaker name, text box, and numbered options.

**Controls:** WASD move, E talk to NPC, 1/2/3 choose dialogue option, Escape exit dialogue

### 24 — ReplayDemo

Replay recording and playback system. Records entity transforms and game events in real-time. Play back recordings with full timeline control: seek forward/backward, variable speed (0.25x to 4x), pause/resume. Kill cam mode replays the last kill from a cinematic angle in slow motion. Save replays to file and load them back. Status overlay shows recording/playing state, timeline position, and speed.

**Controls:** F5 stop recording and play back, Left/Right seek, +/- speed, Space pause, K kill cam, F6 save, F7 load, F8 new recording

### 25 — SplineRunner

Spline path system demo. Three visible splines drawn with DebugDraw: a CatmullRom loop, a CubicBezier figure-8, and a Linear back-and-forth path. Colored follower entities travel along each spline with different modes (Loop, PingPong, Once). Attach the camera to any spline for a rail-cam experience. Control points visualized as spheres. Adjustable speed and pause controls.

**Controls:** 1/2/3 attach camera to spline, +/- speed, Space pause, P toggle control points, Tab free camera, R reset followers

### 26 — Game2D

Full 2D engine showcase. Sprite rendering with SpriteAnimator (idle, walk, jump frame sequences). Tilemap-based level geometry. Three-layer parallax scrolling background (sky, mountains, trees). 2D physics with RigidBody2D and Collider2D for platformer mechanics. Camera2D with bounds and zoom. Collectible animated coins. NineSliceSprite UI panel for score display. PixelPerfectComponent for crisp rendering.

**Controls:** A/D move, Space jump, +/- zoom

### 27 — WorldStreaming

Large-world streaming demonstration. Four distinct areas (Forest, Desert, Snow, City) arranged in a 2x2 grid. Areas load and unload seamlessly as the player walks between them using SeamlessAreaManager. AsyncAssetLoader handles background loading with progress indication. WorldOriginSystem rebases the floating-point origin when the player travels far from the world center, preventing precision loss. Debug overlays show area boundaries and origin offset.

**Controls:** WASD move, F1 toggle area boundaries, F2 show origin info, Tab teleport between areas

### 28 — ModdingPlayground

Mod system integration demo. Three simulated mods registered programmatically: ColorMod (changes material colors), SpawnMod (adds decorative entities and NPCs), WeatherMod (adds weather effects, depends on ColorMod). Mod browser UI displays available mods with name, version, author, status, and dependency info. Dependency enforcement prevents loading mods with unsatisfied prerequisites. Console commands for mod management.

**Controls:** M toggle mod browser, 1/2/3 toggle mods, L load enabled mods, U unload all, R rescan

## Engine Systems Covered

| System | Templates |
|--------|-----------|
| **ECS** — Entity creation, components, world management | All |
| **Physics** — Rigid bodies, colliders, collision events, raycasting | 03, 05, 08, 11, 12, 17, 19, 22 |
| **AI** — Behavior trees, NavMesh, pathfinding, perception, steering | 05, 11, 12, 19 |
| **Networking** — UDP client/server, replication, prediction, lag compensation | 08, 11, 12 |
| **Rendering** — Meshes, materials, PBR, lighting, shadows, post-processing | 01, 06, 14, 18 |
| **Animation** — Skeletal, state machines, blending, layers, IK, events | 13 |
| **Particles** — Emitter shapes, blend modes, preset effects | 06, 11 |
| **Audio** — 3D positional sound, music, buses, reverb, Doppler | 09, 15, 21 |
| **Camera** — FPS, TPS, orbit, free-fly, shake, DOF, split-screen | 01, 16 |
| **Scripting** — AngelScript with hot-reload and lifecycle callbacks | 10, 11 |
| **Procedural Generation** — Noise functions, erosion, WFC | 07, 11 |
| **Cinematics** — Sequencer, camera paths, interpolation, subtitles | 09, 16 |
| **Save System** — Save, Load, QuickSave, QuickLoad, AutoSave | 04, 11, 12 |
| **Events** — EventBus pub/sub for decoupled communication | 02, 03, 04, 05, 08, 13, 17, 20, 21, 22, 23, 24 |
| **Day/Night & Weather** — Time-of-day cycles, weather transitions | 02, 11, 18 |
| **UI/HUD** — Health bars, minimap, crosshair, menus, inventory | 20 |
| **Debug Tools** — Debug draw, wireframe, perf stats, console | 19, 28 |
| **Materials** — PBR, emissive, transparency, SSS, render paths | 14 |
| **Shadows** — CSM, point/spot/area shadows, IBL, SSR, light probes | 18 |
| **Weapons** — Fire modes, recoil, ADS, ammo, reload mechanics | 21 |
| **Destruction** — Fracture patterns, debris, multi-stage damage | 22 |
| **Dialogue** — Branching trees, conditions, choices, NPC conversations | 23 |
| **Replay** — Recording, playback, seek, speed control, kill cam | 24 |
| **Splines** — Path types, followers, Loop/PingPong/Once modes | 25 |
| **2D Engine** — Sprites, tilemaps, Camera2D, parallax, 2D physics | 26 |
| **World Streaming** — Area streaming, async loading, origin rebasing | 27 |
| **Modding** — Mod scanning, loading, dependencies, console commands | 28 |

## Learning Path

1. **Start here:** Templates 01-02 for ECS and engine basics
2. **Core systems:** Templates 03-06 for physics, AI, particles, and RPG mechanics
3. **Visual & audio:** Templates 13-15 for animation, materials, and audio
4. **Camera & tools:** Templates 16, 19, 25 for camera modes, debug tools, and spline paths
5. **Content creation:** Templates 09-10, 23 for cinematics, scripting, and dialogue
6. **Gameplay:** Templates 17, 20, 21, 26 for platformer, HUD/UI, weapons, and 2D games
7. **Advanced systems:** Templates 22, 24, 27, 28 for destruction, replay, world streaming, and modding
8. **Infrastructure:** Templates 07-08, 11-12, 18 for procedural generation, networking, advanced lighting, stress testing, and dedicated servers

## License

This project is licensed under the MIT License — see [LICENSE](LICENSE) for details.
