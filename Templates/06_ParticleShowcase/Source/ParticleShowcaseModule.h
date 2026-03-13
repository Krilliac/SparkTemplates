#pragma once
/**
 * @file ParticleShowcaseModule.h
 * @brief INTERMEDIATE — Visual effects with particles, decals, fog, and post-processing.
 *
 * Demonstrates:
 *   - ParticleSystem emitter shapes (Point, Sphere, Cone, Box, Circle)
 *   - Particle blend modes (Additive, AlphaBlend, Multiply, Premultiplied)
 *   - Color/size curves over lifetime, gravity, drag
 *   - Built-in presets: Explosion, MuzzleFlash, Sparks, Smoke, Trail
 *   - DecalSystem with surface-type mappings
 *   - FogSystem modes (Linear, Exponential, Height, Volumetric)
 *   - PostProcessingSystem (Bloom, ToneMapping, ColorGrading, FXAA, SSAO)
 *
 * Spark Engine systems used:
 *   Particles      — ParticleSystem, ParticleEmitterComponent
 *   Decals         — DecalSystem
 *   Fog            — FogSystem
 *   PostProcessing — PostProcessingSystem (Bloom, SSAO, ColorGrading)
 *   ECS            — World, Transform, MeshRenderer, NameComponent
 *   Input          — InputManager
 *   Rendering      — LightComponent, GraphicsEngine
 */

#include <Core/ModuleManager.h>
#include <Core/EngineContext.h>
#include <Engine/ECS/Components.h>
#include <vector>

class ParticleShowcaseModule final : public Spark::IModule
{
public:
    Spark::ModuleInfo GetModuleInfo() const override
    {
        return { "ParticleShowcase", "0.2.0", SPARK_SDK_VERSION, 1000 };
    }

    void OnLoad(Spark::IEngineContext* ctx) override;
    void OnUnload() override;
    void OnUpdate(float dt) override;
    void OnRender() override;
    void OnResize(int w, int h) override;

private:
    void BuildScene();
    void SetupPostProcessing();
    void CycleWeatherFog();

    Spark::IEngineContext* m_ctx = nullptr;

    // Environment
    entt::entity m_floor = entt::null;
    entt::entity m_light = entt::null;

    // Particle emitter entities
    std::vector<entt::entity> m_emitters;

    // Fog mode cycling
    int m_fogMode = 0;

    // Timer for auto-effects
    float m_effectTimer = 0.0f;
};
