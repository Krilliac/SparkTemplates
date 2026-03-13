#pragma once
/**
 * @file DebugVisualizationModule.h
 * @brief ADVANCED — Debug and development tool showcase.
 *
 * Demonstrates:
 *   - DebugDraw system (lines, boxes, spheres, arrows, text)
 *   - Wireframe rendering mode toggle
 *   - Physics debug visualisation (collider shapes, contact points)
 *   - NavMesh debug overlay (walkable areas, paths)
 *   - AI perception cone visualisation (sight / hearing radii)
 *   - Performance statistics overlay (FPS, frame time, draw calls, etc.)
 *   - Entity inspector (select entity, show components, modify transforms)
 *   - Camera frustum visualisation
 *   - Bounding-box visualisation for all meshes
 *   - World-space reference grid
 *   - Raycast visualisation (cast ray, show hit point / normal)
 *   - Memory usage tracking display
 *   - SimpleConsole with custom commands
 *
 * Spark Engine systems used:
 *   Rendering  — GraphicsEngine (wireframe, physics debug, NavMesh debug, stats)
 *   Debug      — DebugDraw, SimpleConsole
 *   ECS        — World, Transform, MeshRenderer, Camera, ColliderComponent,
 *                AIComponent, LightComponent, NameComponent
 *   AI         — NavMesh overlay
 *   Physics    — Raycast, collider shapes
 *   Input      — InputManager
 */

#include <Core/ModuleManager.h>
#include <Core/EngineContext.h>
#include <Engine/ECS/Components.h>
#include <Engine/Debug/DebugDraw.h>
#include <Engine/Debug/SimpleConsole.h>
#include <vector>
#include <string>

class DebugVisualizationModule final : public Spark::IModule
{
public:
    Spark::ModuleInfo GetModuleInfo() const override
    {
        return { "DebugVisualization", "0.2.0", SPARK_SDK_VERSION, 1000 };
    }

    void OnLoad(Spark::IEngineContext* ctx) override;
    void OnUnload() override;
    void OnUpdate(float dt) override;
    void OnRender() override;
    void OnResize(int w, int h) override;

private:
    // Scene construction
    void BuildScene();
    void SetupConsoleCommands();

    // Debug drawing helpers
    void DrawWorldGrid();
    void DrawDebugOverlay();
    void DrawBoundingBoxes();
    void DrawFrustum();
    void DrawAIPerception();
    void DrawPerformanceStats();
    void DrawEntityInspector();
    void DrawMemoryStats();
    void PerformRaycast();

    Spark::IEngineContext* m_ctx = nullptr;

    // Core entities
    entt::entity m_camera   = entt::null;
    entt::entity m_light    = entt::null;

    // Scene entities (for bounding box / inspector demo)
    std::vector<entt::entity> m_sceneEntities;

    // AI agents (for perception cone visualisation)
    std::vector<entt::entity> m_aiAgents;

    // Console
    Spark::SimpleConsole m_console;

    // Toggle states
    bool m_showDebugOverlay = true;
    bool m_wireframe        = false;
    bool m_physicsDebug     = false;
    bool m_navMeshDebug     = false;
    bool m_showPerfStats    = true;
    bool m_showGrid         = true;
    bool m_showBBoxes       = false;
    bool m_showFrustum      = false;
    bool m_consoleOpen      = false;

    // Entity inspector
    entt::entity m_selectedEntity = entt::null;
    bool m_inspectorActive = false;

    // Raycast state
    bool  m_rayHit       = false;
    float m_rayHitX      = 0.0f;
    float m_rayHitY      = 0.0f;
    float m_rayHitZ      = 0.0f;
    float m_rayNormalX   = 0.0f;
    float m_rayNormalY   = 1.0f;
    float m_rayNormalZ   = 0.0f;
    float m_rayVisTimer  = 0.0f;    // time remaining to show ray hit

    // Camera state
    float m_camYaw   = 0.0f;
    float m_camPitch = -30.0f;
    float m_camDist  = 25.0f;

    // Performance stats (cached per frame)
    float m_fps       = 0.0f;
    float m_frameTime = 0.0f;
    int   m_drawCalls = 0;
    int   m_triangles = 0;
    int   m_entityCount = 0;

    // Frame timing
    float m_fpsAccum   = 0.0f;
    int   m_frameCount = 0;
    float m_fpsTimer   = 0.0f;

    // Grid parameters
    static constexpr float kGridExtent = 30.0f;
    static constexpr float kGridStep   = 2.0f;
};
