#pragma once
/**
 * @file SplineRunnerModule.h
 * @brief INTERMEDIATE — Spline paths and followers demo.
 *
 * Demonstrates:
 *   - CatmullRom, CubicBezier, and Linear spline paths
 *   - SplineFollowerComponent with Loop, PingPong, Once modes
 *   - Camera attachment to spline paths
 *   - DebugDraw visualization of spline curves and control points
 *   - Speed adjustment and pause/resume controls
 *   - Free camera vs spline-following camera toggle
 *
 * Spark Engine systems used:
 *   ECS        — World, Transform, MeshRenderer, Camera, NameComponent, LightComponent
 *   Splines    — SplineComponent, SplineFollowerComponent, SplinePath, SplineMath
 *   Input      — InputManager
 *   Debug      — DebugDraw
 *   Events     — EventBus
 */

#include <Core/ModuleManager.h>
#include <Core/EngineContext.h>
#include <Engine/ECS/Components.h>
#include <Engine/ECS/Components/SplineComponents.h>
#include <Engine/Debug/DebugDraw.h>
#include <Engine/Events/EventSystem.h>
#include <vector>
#include <string>

// ---------------------------------------------------------------------------
// Spline path descriptor (groups a spline entity with its followers)
// ---------------------------------------------------------------------------

struct SplinePathInfo
{
    std::string   name;
    entt::entity  splineEntity = entt::null;
    std::vector<entt::entity> followers;
    float         displayR = 1.0f;
    float         displayG = 1.0f;
    float         displayB = 1.0f;
};

// ---------------------------------------------------------------------------
// Module
// ---------------------------------------------------------------------------

class SplineRunnerModule final : public Spark::IModule
{
public:
    Spark::ModuleInfo GetModuleInfo() const override
    {
        return { "SplineRunner", "0.2.0", SPARK_SDK_VERSION, 1000 };
    }

    void OnLoad(Spark::IEngineContext* ctx) override;
    void OnUnload() override;
    void OnUpdate(float dt) override;
    void OnRender() override;
    void OnResize(int w, int h) override;

private:
    // Scene setup
    void BuildScene();
    void CreateCircularCatmullRomPath();
    void CreateFigure8BezierPath();
    void CreateLinearPath();
    void CreateFollower(int pathIndex, float startProgress, float r, float g, float b,
                        const char* name);

    // Spline evaluation helpers
    DirectX::XMFLOAT3 EvaluateSpline(entt::entity splineEntity, float t);
    DirectX::XMFLOAT3 EvaluateSplineTangent(entt::entity splineEntity, float t);

    // Drawing
    void DrawSplinePath(const SplinePathInfo& pathInfo);
    void DrawControlPoints(const SplinePathInfo& pathInfo);
    void DrawOverlay();

    // Camera
    void UpdateFreeCamera(float dt);
    void UpdateSplineCamera(float dt);

    Spark::IEngineContext* m_ctx = nullptr;

    // Screen dimensions
    float m_screenW = 1920.0f;
    float m_screenH = 1080.0f;

    // Core entities
    entt::entity m_camera    = entt::null;
    entt::entity m_light     = entt::null;
    entt::entity m_ground    = entt::null;

    // Spline paths
    std::vector<SplinePathInfo> m_paths;

    // All tracked entities for cleanup
    std::vector<entt::entity> m_allEntities;

    // State
    bool  m_paused             = false;
    bool  m_showControlPoints  = false;
    bool  m_freeCamera         = true;
    int   m_cameraSplineIndex  = 0;
    float m_cameraSplineProgress = 0.0f;
    float m_cameraSplineSpeed  = 0.3f;
    float m_followerSpeedMult  = 1.0f;

    // Free camera state
    float m_camYaw   = 0.0f;
    float m_camPitch = -30.0f;
    float m_camDist  = 25.0f;
    float m_camFocusX = 0.0f;
    float m_camFocusZ = 0.0f;

    // Timer for animation
    float m_time = 0.0f;
};
