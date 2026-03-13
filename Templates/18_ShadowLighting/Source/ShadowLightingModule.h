#pragma once
/**
 * @file ShadowLightingModule.h
 * @brief ADVANCED — Lighting and shadow showcase with CSM, IBL, SSR, SSAO.
 *
 * Demonstrates:
 *   - Cascaded Shadow Maps (CSM) with configurable split distances
 *   - Point light shadows (omnidirectional cube-map)
 *   - Spot light shadows with cookie textures
 *   - Area lights (rectangle, disc) with soft shadows
 *   - Shadow quality settings (resolution, PCF filter size, bias)
 *   - Light color temperature (Kelvin → RGB conversion)
 *   - Image-Based Lighting (IBL) with environment maps
 *   - Screen-Space Reflections (SSR)
 *   - Screen-Space Ambient Occlusion (SSAO)
 *   - Day-to-night transition (animated sun direction)
 *   - Interior / exterior lighting scenarios
 *   - Light probe placement for indirect lighting
 *
 * Spark Engine systems used:
 *   Rendering  — GraphicsEngine (CSM, IBL, SSR, SSAO, environment maps)
 *   ECS        — World, Transform, MeshRenderer, LightComponent, Camera
 *   Input      — InputManager
 */

#include <Core/ModuleManager.h>
#include <Core/EngineContext.h>
#include <Engine/ECS/Components.h>
#include <vector>

// ---------------------------------------------------------------------------
// Shadow quality preset
// ---------------------------------------------------------------------------

enum class ShadowQuality
{
    Low,       // 512,  PCF 1
    Medium,    // 1024, PCF 3
    High,      // 2048, PCF 5
    Ultra      // 4096, PCF 7
};

// ---------------------------------------------------------------------------
// Module
// ---------------------------------------------------------------------------

class ShadowLightingModule final : public Spark::IModule
{
public:
    Spark::ModuleInfo GetModuleInfo() const override
    {
        return { "ShadowLighting", "0.2.0", SPARK_SDK_VERSION, 1000 };
    }

    void OnLoad(Spark::IEngineContext* ctx) override;
    void OnUnload() override;
    void OnUpdate(float dt) override;
    void OnRender() override;
    void OnResize(int w, int h) override;

private:
    // Scene construction
    void BuildScene();
    void BuildInterior();
    void BuildExterior();
    void CreateLights();
    void PlaceLightProbes();

    // Helpers
    void ApplyShadowQuality();
    void CycleShadowQuality();
    static void KelvinToRGB(float kelvin, float& r, float& g, float& b);
    void UpdateTimeOfDay(float dt);

    Spark::IEngineContext* m_ctx = nullptr;

    // Core entities
    entt::entity m_camera     = entt::null;
    entt::entity m_sunLight   = entt::null;

    // Light groups (toggled independently)
    std::vector<entt::entity> m_pointLights;
    std::vector<entt::entity> m_spotLights;
    std::vector<entt::entity> m_areaLights;

    // Scene geometry
    std::vector<entt::entity> m_sceneEntities;

    // Rendering state
    ShadowQuality m_shadowQuality = ShadowQuality::High;
    bool m_csmDebug       = false;
    bool m_iblEnabled     = true;
    bool m_ssrEnabled     = true;
    bool m_ssaoEnabled    = true;
    bool m_timeOfDay      = false;
    float m_todAngle      = -45.0f;   // sun angle in degrees
    float m_todSpeed      = 10.0f;    // degrees per second

    // Light toggle state
    bool m_dirLightOn     = true;
    bool m_pointLightsOn  = true;
    bool m_spotLightsOn   = true;
    bool m_areaLightsOn   = true;

    // Light intensity multiplier
    float m_intensityMul  = 1.0f;

    // SSAO parameters
    float m_ssaoRadius    = 0.5f;
    float m_ssaoIntensity = 1.0f;

    // Camera orbit
    float m_camYaw   = 0.0f;
    float m_camPitch = -25.0f;
    float m_camDist  = 20.0f;

    // CSM split distances
    float m_csmSplits[4] = { 10.0f, 25.0f, 60.0f, 150.0f };
};
