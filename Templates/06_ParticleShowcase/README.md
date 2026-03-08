# 06 — Particle Showcase  (Intermediate)

A visual effects gallery featuring particle emitters, decals, fog, and
post-processing effects side by side.

## What you will learn

| Concept | Engine API |
|---------|-----------|
| Particle emitters | `ParticleEmitterComponent` — effectName, emissionRate, lifetime, startColor |
| Emitter shapes | Point, Sphere, Cone, Box, Circle |
| Preset effects | `SpawnExplosion()`, `SpawnMuzzleFlash()`, `SpawnSparks()`, `SpawnSmoke()` |
| Decal system | `DecalSystem::SpawnDecal()` — BulletHole, ScorchMark, Crack |
| Fog modes | `FogSystem` — Linear, Exponential, Height, Volumetric |
| Post-processing | Bloom, ToneMapping, FXAA, SSAO, ColorGrading |
| Graphics settings | `GraphicsEngine::SetSSAOSettings()`, quality presets |

## Controls

| Key | Action |
|-----|--------|
| 1 | Trigger explosion effect |
| 2 | Trigger muzzle flash effect |
| 3 | Spawn smoke puff |
| 4 | Spawn sparks burst |
| F | Cycle fog modes (Height → Linear → Exponential → Volumetric → Off) |
| B | Toggle bloom on/off |

## Emitter stations

| Station | Effect | Emission Rate | Blend Mode |
|---------|--------|--------------|------------|
| Fire (left) | Orange flames | 50/sec | Additive |
| Smoke | Gray wisps | 20/sec | AlphaBlend |
| Sparks (centre) | Yellow sparks | 80/sec | Additive |
| Magic | Blue orbs | 40/sec | Premultiplied |
| Rain (right) | Downpour | 200/sec | AlphaBlend |

## Building

```bash
cmake -B build -DCMAKE_PREFIX_PATH=<SparkEngine install>
cmake --build build --config Release
```
