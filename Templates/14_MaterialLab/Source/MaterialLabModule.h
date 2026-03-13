#pragma once
/**
 * @file MaterialLabModule.h
 * @brief INTERMEDIATE — Material and rendering system showcase.
 *
 * Demonstrates:
 *   - MaterialComponent with PBR parameters (albedo, roughness, metallic, normal, AO)
 *   - Material property animation (pulsing emissive, color cycling)
 *   - Transparency modes (Opaque, AlphaTest, AlphaBlend)
 *   - Emissive materials with bloom interaction
 *   - Subsurface scattering (SSS) materials for skin/wax
 *   - Material instances — cloning base materials with parameter overrides
 *   - Multiple render queues (opaque, transparent, overlay)
 *   - Reflective / mirror materials
 *   - UV tiling and offset animation
 *   - Sphere grid showing roughness / metallic combinations
 *   - GraphicsEngine render-path switching (Forward, Deferred, ForwardPlus)
 *
 * Spark Engine systems used:
 *   Materials  — MaterialComponent, MaterialInstance
 *   Rendering  — GraphicsEngine, PostProcessingSystem, RenderPath
 *   ECS        — World, NameComponent, Transform, MeshRenderer
 *   Input      — InputManager
 *   Rendering  — LightComponent, Camera
 */

#include <Core/ModuleManager.h>
#include <Core/EngineContext.h>
#include <Engine/ECS/Components.h>
#include <Graphics/GraphicsEngine.h>
#include <Graphics/PostProcessingSystem.h>
#include <vector>

class MaterialLabModule final : public Spark::IModule
{
public:
    Spark::ModuleInfo GetModuleInfo() const override
    {
        return { "MaterialLab", "0.2.0", SPARK_SDK_VERSION, 1000 };
    }

    void OnLoad(Spark::IEngineContext* ctx) override;
    void OnUnload() override;
    void OnUpdate(float dt) override;
    void OnRender() override;
    void OnResize(int w, int h) override;

private:
    void BuildScene();
    void CreateSphereGrid();
    void CreateShowcasePedestals();
    void SetupPostProcessing();
    void ApplyMaterialPreset(int preset);
    void CycleRenderPath();
    void UpdateAnimatedMaterials(float dt);

    Spark::IEngineContext* m_ctx = nullptr;

    // Scene entities
    entt::entity m_floor     = entt::null;
    entt::entity m_sunLight  = entt::null;
    entt::entity m_fillLight = entt::null;
    entt::entity m_camera    = entt::null;

    // Sphere grid (roughness x metallic matrix)
    static constexpr int kGridRows = 5;  // roughness steps
    static constexpr int kGridCols = 5;  // metallic steps
    std::vector<entt::entity> m_gridSpheres;

    // Showcase pedestals
    entt::entity m_emissiveSphere  = entt::null;
    entt::entity m_transparentOrb  = entt::null;
    entt::entity m_sssSphere       = entt::null;
    entt::entity m_mirrorSphere    = entt::null;
    entt::entity m_uvAnimCube      = entt::null;

    // State
    int   m_currentPreset   = 0;
    int   m_renderPathIndex = 0;
    float m_roughnessOffset = 0.0f;
    bool  m_emissiveOn      = true;
    float m_animTimer       = 0.0f;

    // Camera
    float m_camYaw   = 0.0f;
    float m_camPitch = -15.0f;
    float m_camDist  = 20.0f;
};
