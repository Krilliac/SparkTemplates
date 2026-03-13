/**
 * @file SplineRunnerModule.cpp
 * @brief INTERMEDIATE — Spline paths and followers demo.
 */

#include "SplineRunnerModule.h"
#include <Input/InputManager.h>
#include <cmath>
#include <algorithm>
#include <cstdio>

static constexpr float kPi = 3.14159265358979f;

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void SplineRunnerModule::OnLoad(Spark::IEngineContext* ctx)
{
    m_ctx = ctx;
    BuildScene();
}

void SplineRunnerModule::OnUnload()
{
    auto* world = m_ctx ? m_ctx->GetWorld() : nullptr;
    if (!world) { m_ctx = nullptr; return; }

    for (auto e : m_allEntities)
        if (e != entt::null) world->DestroyEntity(e);

    m_allEntities.clear();
    m_paths.clear();
    m_ctx = nullptr;
}

// ---------------------------------------------------------------------------
// Scene construction
// ---------------------------------------------------------------------------

void SplineRunnerModule::BuildScene()
{
    auto* world = m_ctx->GetWorld();

    // --- Camera -------------------------------------------------------------
    m_camera = world->CreateEntity();
    world->AddComponent<NameComponent>(m_camera, NameComponent{ "MainCamera" });
    world->AddComponent<Transform>(m_camera, Transform{
        { 0.0f, 20.0f, -25.0f },
        { -30.0f, 0.0f, 0.0f },
        { 1.0f, 1.0f, 1.0f }
    });
    world->AddComponent<Camera>(m_camera, Camera{
        60.0f, 0.1f, 500.0f, true
    });
    m_allEntities.push_back(m_camera);

    // --- Light --------------------------------------------------------------
    m_light = world->CreateEntity();
    world->AddComponent<NameComponent>(m_light, NameComponent{ "SunLight" });
    world->AddComponent<Transform>(m_light, Transform{
        { 0.0f, 30.0f, 0.0f },
        { -45.0f, 30.0f, 0.0f },
        { 1.0f, 1.0f, 1.0f }
    });
    world->AddComponent<LightComponent>(m_light, LightComponent{
        LightComponent::Type::Directional,
        { 1.0f, 0.95f, 0.85f }, 2.0f, 0.0f,
        0.0f, 0.0f, true, 2048
    });
    m_allEntities.push_back(m_light);

    // --- Ground -------------------------------------------------------------
    m_ground = world->CreateEntity();
    world->AddComponent<NameComponent>(m_ground, NameComponent{ "Ground" });
    world->AddComponent<Transform>(m_ground, Transform{
        { 0.0f, -0.1f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 60.0f, 0.2f, 60.0f }
    });
    world->AddComponent<MeshRenderer>(m_ground, MeshRenderer{
        "Assets/Models/cube.obj", "Assets/Materials/concrete.mat",
        false, true, true
    });
    m_allEntities.push_back(m_ground);

    // --- Create the three spline paths --------------------------------------
    CreateCircularCatmullRomPath();
    CreateFigure8BezierPath();
    CreateLinearPath();
}

// ---------------------------------------------------------------------------
// Path 1: Circular CatmullRom loop
// ---------------------------------------------------------------------------

void SplineRunnerModule::CreateCircularCatmullRomPath()
{
    auto* world = m_ctx->GetWorld();

    SplinePathInfo info;
    info.name = "CircularOrbit";
    info.displayR = 0.2f;
    info.displayG = 0.8f;
    info.displayB = 1.0f;

    // Create spline entity
    info.splineEntity = world->CreateEntity();
    world->AddComponent<NameComponent>(info.splineEntity, NameComponent{ "Spline_CircularOrbit" });
    world->AddComponent<Transform>(info.splineEntity, Transform{
        { -15.0f, 0.0f, 10.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }
    });

    // Build circular control points (8 points around a circle, radius=8)
    SplineComponent spline;
    spline.type = SplineComponent::SplineType::CatmullRom;
    spline.closed = true;
    float radius = 8.0f;
    int numPoints = 8;
    for (int i = 0; i < numPoints; ++i)
    {
        float angle = (static_cast<float>(i) / static_cast<float>(numPoints)) * 2.0f * kPi;
        float px = std::cos(angle) * radius;
        float pz = std::sin(angle) * radius;
        float py = 1.0f + std::sin(angle * 2.0f) * 1.5f; // gentle up/down
        spline.controlPoints.push_back({ px, py, pz });
    }
    world->AddComponent<SplineComponent>(info.splineEntity, spline);
    m_allEntities.push_back(info.splineEntity);

    // Create 3 followers — Loop mode
    int pathIdx = static_cast<int>(m_paths.size());
    m_paths.push_back(info);

    CreateFollower(pathIdx, 0.0f,  1.0f, 0.3f, 0.3f, "Follower_Circle_A");
    CreateFollower(pathIdx, 0.33f, 0.3f, 1.0f, 0.3f, "Follower_Circle_B");
    CreateFollower(pathIdx, 0.66f, 0.3f, 0.3f, 1.0f, "Follower_Circle_C");
}

// ---------------------------------------------------------------------------
// Path 2: Figure-8 CubicBezier
// ---------------------------------------------------------------------------

void SplineRunnerModule::CreateFigure8BezierPath()
{
    auto* world = m_ctx->GetWorld();

    SplinePathInfo info;
    info.name = "Figure8";
    info.displayR = 1.0f;
    info.displayG = 0.6f;
    info.displayB = 0.1f;

    info.splineEntity = world->CreateEntity();
    world->AddComponent<NameComponent>(info.splineEntity, NameComponent{ "Spline_Figure8" });
    world->AddComponent<Transform>(info.splineEntity, Transform{
        { 15.0f, 0.0f, 10.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }
    });

    // Figure-8 with cubic bezier control points
    SplineComponent spline;
    spline.type = SplineComponent::SplineType::CubicBezier;
    spline.closed = true;

    // Right loop (anchor, ctrl, ctrl, anchor, ctrl, ctrl, ...)
    spline.controlPoints.push_back({  0.0f, 1.0f,  0.0f });  // P0 anchor (centre)
    spline.controlPoints.push_back({  5.0f, 2.0f,  4.0f });  // C1
    spline.controlPoints.push_back({  8.0f, 3.0f,  4.0f });  // C2
    spline.controlPoints.push_back({  8.0f, 2.0f,  0.0f });  // P1 anchor (right)
    spline.controlPoints.push_back({  8.0f, 1.0f, -4.0f });  // C3
    spline.controlPoints.push_back({  5.0f, 1.0f, -4.0f });  // C4
    spline.controlPoints.push_back({  0.0f, 1.0f,  0.0f });  // P2 anchor (centre again)
    spline.controlPoints.push_back({ -5.0f, 1.0f,  4.0f });  // C5
    spline.controlPoints.push_back({ -8.0f, 2.0f,  4.0f });  // C6
    spline.controlPoints.push_back({ -8.0f, 3.0f,  0.0f });  // P3 anchor (left)
    spline.controlPoints.push_back({ -8.0f, 2.0f, -4.0f });  // C7
    spline.controlPoints.push_back({ -5.0f, 1.0f, -4.0f });  // C8

    world->AddComponent<SplineComponent>(info.splineEntity, spline);
    m_allEntities.push_back(info.splineEntity);

    int pathIdx = static_cast<int>(m_paths.size());
    m_paths.push_back(info);

    // 2 followers — PingPong mode
    CreateFollower(pathIdx, 0.0f, 1.0f, 0.8f, 0.2f, "Follower_Fig8_A");
    CreateFollower(pathIdx, 0.5f, 0.8f, 0.4f, 1.0f, "Follower_Fig8_B");
}

// ---------------------------------------------------------------------------
// Path 3: Linear back-and-forth
// ---------------------------------------------------------------------------

void SplineRunnerModule::CreateLinearPath()
{
    auto* world = m_ctx->GetWorld();

    SplinePathInfo info;
    info.name = "LinearTrack";
    info.displayR = 0.3f;
    info.displayG = 1.0f;
    info.displayB = 0.3f;

    info.splineEntity = world->CreateEntity();
    world->AddComponent<NameComponent>(info.splineEntity, NameComponent{ "Spline_Linear" });
    world->AddComponent<Transform>(info.splineEntity, Transform{
        { 0.0f, 0.0f, -10.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }
    });

    SplineComponent spline;
    spline.type = SplineComponent::SplineType::Linear;
    spline.closed = false;

    // Straight path with waypoints
    spline.controlPoints.push_back({ -15.0f, 1.0f,  0.0f });
    spline.controlPoints.push_back({  -5.0f, 3.0f,  0.0f });
    spline.controlPoints.push_back({   5.0f, 1.5f,  0.0f });
    spline.controlPoints.push_back({  15.0f, 1.0f,  0.0f });

    world->AddComponent<SplineComponent>(info.splineEntity, spline);
    m_allEntities.push_back(info.splineEntity);

    int pathIdx = static_cast<int>(m_paths.size());
    m_paths.push_back(info);

    // 2 followers — Once mode (one will restart via R key)
    CreateFollower(pathIdx, 0.0f, 0.5f, 1.0f, 0.5f, "Follower_Linear_A");
    CreateFollower(pathIdx, 0.2f, 1.0f, 1.0f, 0.5f, "Follower_Linear_B");
}

// ---------------------------------------------------------------------------
// Create a follower entity on a given path
// ---------------------------------------------------------------------------

void SplineRunnerModule::CreateFollower(int pathIndex, float startProgress,
                                         float r, float g, float b,
                                         const char* name)
{
    auto* world = m_ctx->GetWorld();

    auto follower = world->CreateEntity();
    world->AddComponent<NameComponent>(follower, NameComponent{ name });
    world->AddComponent<Transform>(follower, Transform{
        { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.6f, 0.6f, 0.6f }
    });
    world->AddComponent<MeshRenderer>(follower, MeshRenderer{
        "Assets/Models/cube.obj", "Assets/Materials/default.mat",
        true, true, true
    });

    // Determine follower mode based on path
    SplineFollowerComponent fc;
    fc.speed = 0.15f;
    fc.progress = startProgress;
    fc.active = true;

    if (pathIndex == 0)
        fc.mode = SplineFollowerComponent::FollowMode::Loop;
    else if (pathIndex == 1)
        fc.mode = SplineFollowerComponent::FollowMode::PingPong;
    else
        fc.mode = SplineFollowerComponent::FollowMode::Once;

    world->AddComponent<SplineFollowerComponent>(follower, fc);

    m_allEntities.push_back(follower);
    m_paths[pathIndex].followers.push_back(follower);
}

// ---------------------------------------------------------------------------
// Spline evaluation
// ---------------------------------------------------------------------------

DirectX::XMFLOAT3 SplineRunnerModule::EvaluateSpline(entt::entity splineEntity, float t)
{
    auto* world = m_ctx->GetWorld();
    auto& spline = world->GetComponent<SplineComponent>(splineEntity);
    auto& splineXf = world->GetComponent<Transform>(splineEntity);

    // Clamp t
    t = std::clamp(t, 0.0f, 1.0f);

    const auto& pts = spline.controlPoints;
    if (pts.empty()) return splineXf.position;

    DirectX::XMFLOAT3 localPos = { 0.0f, 0.0f, 0.0f };

    if (spline.type == SplineComponent::SplineType::Linear)
    {
        if (pts.size() == 1) return { splineXf.position.x + pts[0].x,
                                       splineXf.position.y + pts[0].y,
                                       splineXf.position.z + pts[0].z };

        float totalSegments = static_cast<float>(pts.size() - 1);
        float scaledT = t * totalSegments;
        int seg = static_cast<int>(scaledT);
        if (seg >= static_cast<int>(pts.size()) - 1) seg = static_cast<int>(pts.size()) - 2;
        float localT = scaledT - static_cast<float>(seg);

        localPos = SplineMath::EvaluateLinear(pts[seg], pts[seg + 1], localT);
    }
    else if (spline.type == SplineComponent::SplineType::CatmullRom)
    {
        int n = static_cast<int>(pts.size());
        float scaledT = t * static_cast<float>(spline.closed ? n : n - 1);
        int seg = static_cast<int>(scaledT);
        float localT = scaledT - static_cast<float>(seg);

        if (spline.closed)
        {
            int p0 = ((seg - 1) % n + n) % n;
            int p1 = seg % n;
            int p2 = (seg + 1) % n;
            int p3 = (seg + 2) % n;
            localPos = SplineMath::EvaluateCatmullRom(pts[p0], pts[p1], pts[p2], pts[p3], localT);
        }
        else
        {
            if (seg >= n - 1) { seg = n - 2; localT = 1.0f; }
            int p0 = std::max(0, seg - 1);
            int p1 = seg;
            int p2 = std::min(n - 1, seg + 1);
            int p3 = std::min(n - 1, seg + 2);
            localPos = SplineMath::EvaluateCatmullRom(pts[p0], pts[p1], pts[p2], pts[p3], localT);
        }
    }
    else if (spline.type == SplineComponent::SplineType::CubicBezier)
    {
        // Every 3 points after the first define a segment: P0, C1, C2, P1, C3, C4, P2, ...
        int numSegments = static_cast<int>(pts.size()) / 3;
        if (numSegments < 1) return splineXf.position;

        float scaledT = t * static_cast<float>(numSegments);
        int seg = static_cast<int>(scaledT);
        if (seg >= numSegments) seg = numSegments - 1;
        float localT = scaledT - static_cast<float>(seg);

        int base = seg * 3;
        if (base + 3 < static_cast<int>(pts.size()))
        {
            localPos = SplineMath::EvaluateBezier(pts[base], pts[base + 1],
                                                   pts[base + 2], pts[base + 3], localT);
        }
        else
        {
            localPos = pts[base];
        }
    }

    return { splineXf.position.x + localPos.x,
             splineXf.position.y + localPos.y,
             splineXf.position.z + localPos.z };
}

DirectX::XMFLOAT3 SplineRunnerModule::EvaluateSplineTangent(entt::entity splineEntity, float t)
{
    float dt = 0.001f;
    auto a = EvaluateSpline(splineEntity, std::max(0.0f, t - dt));
    auto b = EvaluateSpline(splineEntity, std::min(1.0f, t + dt));
    float dx = b.x - a.x;
    float dy = b.y - a.y;
    float dz = b.z - a.z;
    float len = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (len < 0.0001f) return { 0.0f, 0.0f, 1.0f };
    return { dx / len, dy / len, dz / len };
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

void SplineRunnerModule::OnUpdate(float dt)
{
    auto* input = m_ctx->GetInput();
    auto* world = m_ctx->GetWorld();
    if (!input || !world) return;

    m_time += dt;

    // --- Pause/resume (Space) -----------------------------------------------
    if (input->WasKeyPressed(VK_SPACE))
        m_paused = !m_paused;

    // --- Toggle control points (P) ------------------------------------------
    if (input->WasKeyPressed('P'))
        m_showControlPoints = !m_showControlPoints;

    // --- Toggle free camera / spline camera (Tab) ---------------------------
    if (input->WasKeyPressed(VK_TAB))
        m_freeCamera = !m_freeCamera;

    // --- Attach camera to spline 1/2/3 --------------------------------------
    if (input->WasKeyPressed('1'))
    {
        m_cameraSplineIndex = 0;
        m_cameraSplineProgress = 0.0f;
        m_freeCamera = false;
    }
    if (input->WasKeyPressed('2'))
    {
        m_cameraSplineIndex = 1;
        m_cameraSplineProgress = 0.0f;
        m_freeCamera = false;
    }
    if (input->WasKeyPressed('3'))
    {
        m_cameraSplineIndex = 2;
        m_cameraSplineProgress = 0.0f;
        m_freeCamera = false;
    }

    // --- Speed adjustment (+/-) ---------------------------------------------
    if (input->IsKeyDown(VK_OEM_PLUS) || input->IsKeyDown(VK_ADD))
        m_followerSpeedMult += 1.0f * dt;
    if (input->IsKeyDown(VK_OEM_MINUS) || input->IsKeyDown(VK_SUBTRACT))
        m_followerSpeedMult = std::max(0.05f, m_followerSpeedMult - 1.0f * dt);

    // --- Reset all followers (R) --------------------------------------------
    if (input->WasKeyPressed('R'))
    {
        for (auto& pathInfo : m_paths)
        {
            for (auto follower : pathInfo.followers)
            {
                if (follower == entt::null) continue;
                auto& fc = world->GetComponent<SplineFollowerComponent>(follower);
                fc.progress = 0.0f;
                fc.active = true;
            }
        }
    }

    // --- Update followers ---------------------------------------------------
    if (!m_paused)
    {
        for (size_t pi = 0; pi < m_paths.size(); ++pi)
        {
            auto& pathInfo = m_paths[pi];
            auto& spline = world->GetComponent<SplineComponent>(pathInfo.splineEntity);

            for (auto follower : pathInfo.followers)
            {
                if (follower == entt::null) continue;
                auto& fc = world->GetComponent<SplineFollowerComponent>(follower);
                if (!fc.active) continue;

                float speed = fc.speed * m_followerSpeedMult;

                if (fc.mode == SplineFollowerComponent::FollowMode::Loop)
                {
                    fc.progress += speed * dt;
                    if (fc.progress > 1.0f) fc.progress -= 1.0f;
                }
                else if (fc.mode == SplineFollowerComponent::FollowMode::PingPong)
                {
                    fc.progress += speed * dt;
                    if (fc.progress > 1.0f)
                    {
                        fc.progress = 2.0f - fc.progress;
                        fc.speed = -std::abs(fc.speed);
                    }
                    else if (fc.progress < 0.0f)
                    {
                        fc.progress = -fc.progress;
                        fc.speed = std::abs(fc.speed);
                    }
                }
                else // Once
                {
                    fc.progress += speed * dt;
                    if (fc.progress >= 1.0f)
                    {
                        fc.progress = 1.0f;
                        fc.active = false;
                    }
                }

                // Position the follower on the spline
                auto pos = EvaluateSpline(pathInfo.splineEntity, fc.progress);
                auto& xf = world->GetComponent<Transform>(follower);
                xf.position = pos;

                // Orient toward tangent
                auto tan = EvaluateSplineTangent(pathInfo.splineEntity, fc.progress);
                float yaw = std::atan2(tan.x, tan.z) * (180.0f / kPi);
                xf.rotation.y = yaw;
            }
        }
    }

    // --- Camera update ------------------------------------------------------
    if (m_freeCamera)
        UpdateFreeCamera(dt);
    else
        UpdateSplineCamera(dt);
}

// ---------------------------------------------------------------------------
// Free camera (orbit around scene)
// ---------------------------------------------------------------------------

void SplineRunnerModule::UpdateFreeCamera(float dt)
{
    auto* input = m_ctx->GetInput();
    auto* world = m_ctx->GetWorld();

    // Orbit controls
    if (input->IsKeyDown('A')) m_camYaw -= 60.0f * dt;
    if (input->IsKeyDown('D')) m_camYaw += 60.0f * dt;
    if (input->IsKeyDown('W')) m_camPitch = std::max(-80.0f, m_camPitch - 40.0f * dt);
    if (input->IsKeyDown('S')) m_camPitch = std::min(-5.0f, m_camPitch + 40.0f * dt);
    if (input->IsKeyDown('Q')) m_camDist = std::max(5.0f, m_camDist - 15.0f * dt);
    if (input->IsKeyDown('E')) m_camDist = std::min(60.0f, m_camDist + 15.0f * dt);

    float yawRad   = m_camYaw * (kPi / 180.0f);
    float pitchRad = m_camPitch * (kPi / 180.0f);

    auto& camXf = world->GetComponent<Transform>(m_camera);
    camXf.position.x = m_camFocusX + std::sin(yawRad) * std::cos(pitchRad) * m_camDist;
    camXf.position.y = -std::sin(pitchRad) * m_camDist;
    camXf.position.z = m_camFocusZ + std::cos(yawRad) * std::cos(pitchRad) * m_camDist;
    camXf.rotation.x = m_camPitch;
    camXf.rotation.y = m_camYaw;
}

// ---------------------------------------------------------------------------
// Spline-following camera
// ---------------------------------------------------------------------------

void SplineRunnerModule::UpdateSplineCamera(float dt)
{
    auto* world = m_ctx->GetWorld();

    if (m_cameraSplineIndex < 0 || m_cameraSplineIndex >= static_cast<int>(m_paths.size()))
        return;

    if (!m_paused)
    {
        m_cameraSplineProgress += m_cameraSplineSpeed * dt;
        if (m_cameraSplineProgress > 1.0f) m_cameraSplineProgress -= 1.0f;
    }

    auto& pathInfo = m_paths[m_cameraSplineIndex];
    auto pos = EvaluateSpline(pathInfo.splineEntity, m_cameraSplineProgress);
    auto tan = EvaluateSplineTangent(pathInfo.splineEntity, m_cameraSplineProgress);

    auto& camXf = world->GetComponent<Transform>(m_camera);
    // Offset camera above and behind the spline point
    camXf.position.x = pos.x - tan.x * 3.0f;
    camXf.position.y = pos.y + 3.0f;
    camXf.position.z = pos.z - tan.z * 3.0f;

    float yaw = std::atan2(tan.x, tan.z) * (180.0f / kPi);
    float pitch = std::atan2(-tan.y, std::sqrt(tan.x * tan.x + tan.z * tan.z)) * (180.0f / kPi);
    camXf.rotation.x = pitch - 10.0f;
    camXf.rotation.y = yaw;
}

// ---------------------------------------------------------------------------
// Render
// ---------------------------------------------------------------------------

void SplineRunnerModule::OnRender()
{
    auto* dd = Spark::DebugDraw::GetInstance();
    if (!dd) return;

    // Draw all spline paths
    for (auto& pathInfo : m_paths)
    {
        DrawSplinePath(pathInfo);
        if (m_showControlPoints)
            DrawControlPoints(pathInfo);
    }

    // Draw follower markers (small colored indicators above each follower)
    auto* world = m_ctx->GetWorld();
    for (auto& pathInfo : m_paths)
    {
        for (auto follower : pathInfo.followers)
        {
            if (follower == entt::null) continue;
            auto& xf = world->GetComponent<Transform>(follower);
            // Small cross above follower
            dd->Line(
                { xf.position.x - 0.3f, xf.position.y + 1.0f, xf.position.z },
                { xf.position.x + 0.3f, xf.position.y + 1.0f, xf.position.z },
                { pathInfo.displayR, pathInfo.displayG, pathInfo.displayB, 1.0f });
            dd->Line(
                { xf.position.x, xf.position.y + 0.7f, xf.position.z },
                { xf.position.x, xf.position.y + 1.3f, xf.position.z },
                { pathInfo.displayR, pathInfo.displayG, pathInfo.displayB, 1.0f });
        }
    }

    DrawOverlay();
}

// ---------------------------------------------------------------------------
// Draw a spline path as segmented lines
// ---------------------------------------------------------------------------

void SplineRunnerModule::DrawSplinePath(const SplinePathInfo& pathInfo)
{
    auto* dd = Spark::DebugDraw::GetInstance();
    if (!dd) return;

    int segments = 64;
    DirectX::XMFLOAT4 color = { pathInfo.displayR, pathInfo.displayG, pathInfo.displayB, 1.0f };

    DirectX::XMFLOAT3 prev = EvaluateSpline(pathInfo.splineEntity, 0.0f);
    for (int i = 1; i <= segments; ++i)
    {
        float t = static_cast<float>(i) / static_cast<float>(segments);
        DirectX::XMFLOAT3 curr = EvaluateSpline(pathInfo.splineEntity, t);
        dd->Line(
            { prev.x, prev.y, prev.z },
            { curr.x, curr.y, curr.z },
            color);
        prev = curr;
    }
}

// ---------------------------------------------------------------------------
// Draw control points as small wireframe boxes
// ---------------------------------------------------------------------------

void SplineRunnerModule::DrawControlPoints(const SplinePathInfo& pathInfo)
{
    auto* dd = Spark::DebugDraw::GetInstance();
    auto* world = m_ctx->GetWorld();
    if (!dd || !world) return;

    auto& spline = world->GetComponent<SplineComponent>(pathInfo.splineEntity);
    auto& splineXf = world->GetComponent<Transform>(pathInfo.splineEntity);

    DirectX::XMFLOAT4 cpColor = { pathInfo.displayR * 0.7f, pathInfo.displayG * 0.7f,
                                    pathInfo.displayB * 0.7f, 1.0f };

    float s = 0.4f; // half-size of control point marker
    for (auto& cp : spline.controlPoints)
    {
        float wx = splineXf.position.x + cp.x;
        float wy = splineXf.position.y + cp.y;
        float wz = splineXf.position.z + cp.z;

        // Draw wireframe box
        dd->Line({ wx - s, wy - s, wz - s }, { wx + s, wy - s, wz - s }, cpColor);
        dd->Line({ wx + s, wy - s, wz - s }, { wx + s, wy + s, wz - s }, cpColor);
        dd->Line({ wx + s, wy + s, wz - s }, { wx - s, wy + s, wz - s }, cpColor);
        dd->Line({ wx - s, wy + s, wz - s }, { wx - s, wy - s, wz - s }, cpColor);

        dd->Line({ wx - s, wy - s, wz + s }, { wx + s, wy - s, wz + s }, cpColor);
        dd->Line({ wx + s, wy - s, wz + s }, { wx + s, wy + s, wz + s }, cpColor);
        dd->Line({ wx + s, wy + s, wz + s }, { wx - s, wy + s, wz + s }, cpColor);
        dd->Line({ wx - s, wy + s, wz + s }, { wx - s, wy - s, wz + s }, cpColor);

        dd->Line({ wx - s, wy - s, wz - s }, { wx - s, wy - s, wz + s }, cpColor);
        dd->Line({ wx + s, wy - s, wz - s }, { wx + s, wy - s, wz + s }, cpColor);
        dd->Line({ wx + s, wy + s, wz - s }, { wx + s, wy + s, wz + s }, cpColor);
        dd->Line({ wx - s, wy + s, wz - s }, { wx - s, wy + s, wz + s }, cpColor);
    }
}

// ---------------------------------------------------------------------------
// HUD overlay
// ---------------------------------------------------------------------------

void SplineRunnerModule::DrawOverlay()
{
    auto* dd = Spark::DebugDraw::GetInstance();
    auto* world = m_ctx->GetWorld();
    if (!dd || !world) return;

    float x = 10.0f;
    float y = 10.0f;
    char buf[256];

    // Title
    dd->ScreenText({ x, y }, "SPLINE RUNNER DEMO", { 1.0f, 1.0f, 1.0f, 1.0f });
    y += 20.0f;

    // Camera mode
    if (m_freeCamera)
    {
        dd->ScreenText({ x, y }, "Camera: FREE (WASD orbit, Q/E zoom)",
                        { 0.7f, 0.7f, 0.7f, 1.0f });
    }
    else
    {
        std::snprintf(buf, sizeof(buf), "Camera: Following [%s] %.0f%%",
                      m_paths[m_cameraSplineIndex].name.c_str(),
                      m_cameraSplineProgress * 100.0f);
        dd->ScreenText({ x, y }, buf, { 0.3f, 1.0f, 0.8f, 1.0f });
    }
    y += 20.0f;

    // Speed
    std::snprintf(buf, sizeof(buf), "Speed Mult: %.2fx", m_followerSpeedMult);
    dd->ScreenText({ x, y }, buf, { 0.8f, 0.8f, 0.8f, 1.0f });
    y += 20.0f;

    // Paused
    if (m_paused)
    {
        dd->ScreenText({ x, y }, "** PAUSED ** (Space to resume)",
                        { 1.0f, 0.3f, 0.3f, 1.0f });
        y += 20.0f;
    }

    // Control points toggle
    std::snprintf(buf, sizeof(buf), "Control Points: %s (P to toggle)",
                  m_showControlPoints ? "ON" : "OFF");
    dd->ScreenText({ x, y }, buf, { 0.6f, 0.6f, 0.6f, 1.0f });
    y += 25.0f;

    // Per-path info
    for (size_t pi = 0; pi < m_paths.size(); ++pi)
    {
        auto& pathInfo = m_paths[pi];
        auto& spline = world->GetComponent<SplineComponent>(pathInfo.splineEntity);

        const char* typeStr = "Unknown";
        if (spline.type == SplineComponent::SplineType::CatmullRom) typeStr = "CatmullRom";
        else if (spline.type == SplineComponent::SplineType::CubicBezier) typeStr = "CubicBezier";
        else if (spline.type == SplineComponent::SplineType::Linear) typeStr = "Linear";

        std::snprintf(buf, sizeof(buf), "[%zu] %s (%s, %zu pts)",
                      pi + 1, pathInfo.name.c_str(), typeStr,
                      spline.controlPoints.size());
        dd->ScreenText({ x, y }, buf,
                        { pathInfo.displayR, pathInfo.displayG, pathInfo.displayB, 1.0f });
        y += 16.0f;

        // Follower info
        for (auto follower : pathInfo.followers)
        {
            if (follower == entt::null) continue;
            auto& fc = world->GetComponent<SplineFollowerComponent>(follower);
            auto& nc = world->GetComponent<NameComponent>(follower);

            const char* modeStr = "Once";
            if (fc.mode == SplineFollowerComponent::FollowMode::Loop) modeStr = "Loop";
            else if (fc.mode == SplineFollowerComponent::FollowMode::PingPong) modeStr = "PingPong";

            std::snprintf(buf, sizeof(buf), "  %s: %.1f%% [%s] %s",
                          nc.name.c_str(), fc.progress * 100.0f, modeStr,
                          fc.active ? "active" : "stopped");
            dd->ScreenText({ x + 10.0f, y }, buf, { 0.7f, 0.7f, 0.7f, 0.9f });
            y += 14.0f;
        }

        y += 6.0f;
    }

    // Controls help at bottom
    float helpY = m_screenH - 80.0f;
    dd->ScreenText({ 10.0f, helpY },       "1/2/3: Camera to spline | Tab: Free/Follow | +/-: Speed",
                    { 0.5f, 0.5f, 0.5f, 0.8f });
    dd->ScreenText({ 10.0f, helpY + 16.0f }, "Space: Pause | P: Control points | R: Reset followers",
                    { 0.5f, 0.5f, 0.5f, 0.8f });
}

// ---------------------------------------------------------------------------
// Resize
// ---------------------------------------------------------------------------

void SplineRunnerModule::OnResize(int w, int h)
{
    m_screenW = static_cast<float>(w);
    m_screenH = static_cast<float>(h);
}

// ---------------------------------------------------------------------------
// DLL exports
// ---------------------------------------------------------------------------

SPARK_IMPLEMENT_MODULE(SplineRunnerModule)
