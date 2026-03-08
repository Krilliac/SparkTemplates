#pragma once
/**
 * @file ProceduralWorldModule.h
 * @brief ADVANCED — Procedural terrain, noise, erosion, WFC, and object scattering.
 *
 * Demonstrates:
 *   - NoiseGenerator (Perlin, Simplex, Worley, FBM, Ridged, Warped)
 *   - HeightmapGenerator with thermal/hydraulic erosion
 *   - ProceduralMesh creation (terrain, rocks, trees)
 *   - ObjectPlacer rule-based scattering with slope/height filters
 *   - WaveFunctionCollapse tile-based level generation
 *   - Mesh LOD for procedural objects
 *
 * Spark Engine systems used:
 *   Procedural — NoiseGenerator, HeightmapGenerator, ProceduralMesh, ObjectPlacer, WFC
 *   Graphics   — MeshLOD, LODManager, FrustumCulling
 *   ECS        — World, Transform, MeshRenderer, NameComponent
 *   Input      — InputManager
 *   Rendering  — LightComponent
 */

#include <Core/ModuleManager.h>
#include <Core/EngineContext.h>
#include <Engine/ECS/Components.h>
#include <Engine/Procedural/ProceduralGeneration.h>
#include <vector>

class ProceduralWorldModule final : public Spark::IModule
{
public:
    Spark::ModuleInfo GetModuleInfo() const override
    {
        return { "ProceduralWorld", "0.1.0", SPARK_SDK_VERSION, 1000 };
    }

    void OnLoad(Spark::IEngineContext* ctx) override;
    void OnUnload() override;
    void OnUpdate(float dt) override;
    void OnRender() override;
    void OnResize(int w, int h) override;

private:
    void GenerateTerrain();
    void ScatterObjects();
    void GenerateWFCDungeon();
    void RegenerateAll();

    Spark::IEngineContext* m_ctx = nullptr;

    entt::entity m_terrain = entt::null;
    entt::entity m_light   = entt::null;

    std::vector<entt::entity> m_scatteredObjects;
    std::vector<entt::entity> m_wfcTiles;

    // Generation parameters
    int    m_seed       = 42;
    int    m_octaves    = 6;
    float  m_frequency  = 0.01f;
    float  m_amplitude  = 30.0f;
    bool   m_eroded     = true;
};
