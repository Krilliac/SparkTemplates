#pragma once
/**
 * @file WorldStreamingModule.h
 * @brief ADVANCED — Seamless area streaming and world origin rebasing.
 *
 * Demonstrates:
 *   - 4 distinct areas in a 2x2 grid (Forest, Desert, Snow, City)
 *   - Seamless area loading/unloading based on player proximity
 *   - World origin rebasing when player moves far from origin
 *   - Area boundary visualization with DebugDraw
 *   - Teleportation between areas
 *   - Loading progress display
 *
 * Spark Engine systems used:
 *   ECS        — World, Transform, MeshRenderer, Camera, NameComponent, LightComponent
 *   Streaming  — SeamlessAreaManager, SceneTransitionManager, AsyncAssetLoader
 *   World      — WorldOriginSystem
 *   Input      — InputManager
 *   Debug      — DebugDraw
 */

#include <Core/ModuleManager.h>
#include <Core/EngineContext.h>
#include <Engine/ECS/Components.h>
#include <Engine/Streaming/SeamlessAreaManager.h>
#include <Engine/Streaming/SceneTransitionManager.h>
#include <Engine/Streaming/AsyncAssetLoader.h>
#include <Engine/World/WorldOriginSystem.h>
#include <Engine/Debug/DebugDraw.h>
#include <vector>
#include <string>
#include <functional>

// ---------------------------------------------------------------------------
// Area descriptor
// ---------------------------------------------------------------------------

struct AreaDef
{
    std::string name;
    float       originX;
    float       originZ;
    float       sizeX;
    float       sizeZ;
    float       colorR, colorG, colorB;  // theme color for ground/props
    bool        loaded;
    std::vector<entt::entity> entities;  // entities belonging to this area
};

// ---------------------------------------------------------------------------
// Module
// ---------------------------------------------------------------------------

class WorldStreamingModule final : public Spark::IModule
{
public:
    Spark::ModuleInfo GetModuleInfo() const override
    {
        return { "WorldStreaming", "0.2.0", SPARK_SDK_VERSION, 1000 };
    }

    void OnLoad(Spark::IEngineContext* ctx) override;
    void OnUnload() override;
    void OnUpdate(float dt) override;
    void OnRender() override;
    void OnResize(int w, int h) override;

private:
    // Scene setup
    void BuildScene();
    void RegisterAreas();
    void LoadArea(int index);
    void UnloadArea(int index);

    // Area content creation
    void PopulateForest(int index);
    void PopulateDesert(int index);
    void PopulateSnow(int index);
    void PopulateCity(int index);

    // Helpers
    entt::entity CreateProp(const char* name, float x, float y, float z,
                             float sx, float sy, float sz,
                             float r, float g, float b);
    void UpdateAreaStreaming();
    void UpdateOriginRebasing();
    void TeleportToArea(int index);

    // Drawing
    void DrawAreaBoundaries();
    void DrawOriginMarker();
    void DrawOverlay();
    void DrawLoadingBar();

    Spark::IEngineContext* m_ctx = nullptr;

    // Screen dimensions
    float m_screenW = 1920.0f;
    float m_screenH = 1080.0f;

    // Core entities
    entt::entity m_camera = entt::null;
    entt::entity m_light  = entt::null;
    entt::entity m_player = entt::null;

    // All non-area entities for cleanup
    std::vector<entt::entity> m_coreEntities;

    // Areas
    std::vector<AreaDef> m_areas;
    static constexpr float kAreaSize = 100.0f;
    static constexpr float kBorderOverlap = 10.0f;
    static constexpr float kLoadDistance = 80.0f;
    static constexpr float kUnloadDistance = 120.0f;

    // Streaming managers
    SeamlessAreaManager   m_areaManager;
    SceneTransitionManager m_transitionManager;
    WorldOriginSystem     m_originSystem;

    // Origin offset tracking
    DirectX::XMFLOAT3 m_originOffset = { 0.0f, 0.0f, 0.0f };
    static constexpr float kOriginThreshold = 500.0f;

    // UI state
    bool  m_showBoundaries   = true;
    bool  m_showOriginInfo   = false;
    bool  m_isTransitioning  = false;
    float m_transitionProgress = 0.0f;
    float m_transitionTimer  = 0.0f;
    int   m_currentAreaIndex = 0;

    // Player movement
    float m_playerSpeed = 15.0f;

    // Camera
    float m_camYaw   = 0.0f;
    float m_camPitch = -40.0f;
    float m_camDist  = 30.0f;
};
