# 11 — Stress Test  (Advanced)

The ultimate SparkEngine torture test. Every major engine system runs
simultaneously at extreme load to validate stability and performance.

## Systems under stress

| System | Load |
|--------|------|
| **ECS** | 500+ entities, continuous creation/destruction |
| **Physics** | 100-300 active rigid bodies (grows over time), 4 ramps, walls |
| **AI** | 20 agents (10 patrollers + 6 guards + 4 fleers), NavMesh, behavior trees |
| **Particles** | 10 concurrent emitters (fire, smoke, sparks, magic, rain, snow, dust) |
| **Decals** | 30+ floor decals + periodic explosion scorch marks |
| **Fog** | Auto-cycling every 20s (Volumetric → Height → Exponential → Linear) |
| **Post-processing** | Full pipeline: Bloom, SSAO, SSR, TAA, FXAA, ColorGrading, ToneMapping |
| **Lighting** | 1 directional (4K shadows) + 8 point lights + 4 spotlights |
| **Day/Night** | 60x time scale (1 real second = 1 game minute) |
| **Weather** | Auto-cycling every 15s (Clear → Cloudy → Rain → Snow) |
| **Networking** | Local server + 4 replicated entities + lag compensation snapshots |
| **Animation** | 6 animated characters with different animation states |
| **Procedural** | Noise terrain (64x64, erosion) + 12x12 WFC dungeon + rock scatter |
| **Scripting** | 5 AngelScript-driven spinning cubes |
| **Cinematic** | 20-second 5-keyframe flyover sequence with subtitles + fades |
| **Coroutines** | Periodic timed sound chains |
| **Audio** | 6 loaded sounds, 3D positional, music buses, reverb zone |
| **Save System** | Auto-save every 30s, quick-save/load |
| **Events** | 7 subscribed event types, high-frequency collision + damage publishing |
| **Materials** | PBR pipeline with multiple material types |
| **Render Paths** | Switchable: Deferred, ForwardPlus, Clustered |
| **LOD** | Auto-generated LOD chains for procedural terrain |
| **Frustum Culling** | Active under 500+ entity load |
| **Gameplay** | Health, inventory (30 slots), quests, 20 collectables, tag system |

## Controls

| Key | Action |
|-----|--------|
| WASD | Move the player (AI agents react) |
| 1 | Spawn 50 more physics bodies |
| 2 | Trigger explosion cluster at random position |
| 3 | Cycle render path (Deferred → ForwardPlus → Clustered) |
| 4 | Play cinematic flyover sequence |
| 5 | Force auto-save |
| F5 | Quick save |
| F9 | Quick load |

## Automatic stress events

| Event | Interval |
|-------|----------|
| Spawn 10 physics bodies | Every 2 seconds (caps at 300) |
| Damage random AI agent | Every 0.5 seconds |
| Explosion cluster | Every 4 seconds |
| Auto-save | Every 30 seconds |
| Weather cycle | Every 15 seconds |
| Fog mode cycle | Every 20 seconds |

## Building

```bash
cmake -B build -DCMAKE_PREFIX_PATH=<SparkEngine install>
cmake --build build --config Release
```
