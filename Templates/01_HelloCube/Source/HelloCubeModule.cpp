/**
 * @file HelloCubeModule.cpp
 * @brief BASIC — Spinning cube with keyboard and mouse controls.
 */

#include "HelloCubeModule.h"
#include <Input/InputManager.h>
#include <cmath>

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void HelloCubeModule::OnLoad(Spark::IEngineContext* ctx)
{
    m_ctx = ctx;
    auto* world = m_ctx->GetWorld();

    // -- Create the cube entity ----------------------------------------------
    m_cube = world->CreateEntity();
    world->AddComponent<NameComponent>(m_cube, NameComponent{ "SpinningCube" });
    world->AddComponent<Transform>(m_cube, Transform{
        { 0.0f, 1.0f, 0.0f },   // position — slightly above the origin
        { 0.0f, 0.0f, 0.0f },   // rotation (degrees)
        { 1.0f, 1.0f, 1.0f }    // scale
    });
    world->AddComponent<MeshRenderer>(m_cube, MeshRenderer{
        "Assets/Models/cube.obj",          // meshPath (engine built-in)
        "Assets/Materials/default.mat",    // materialPath
        true,  // castShadows
        true,  // receiveShadows
        true   // visible
    });

    // -- Ground plane so the cube has visual context -------------------------
    m_ground = world->CreateEntity();
    world->AddComponent<NameComponent>(m_ground, NameComponent{ "Ground" });
    world->AddComponent<Transform>(m_ground, Transform{
        { 0.0f, -0.05f, 0.0f },
        { 0.0f, 0.0f, 0.0f },
        { 20.0f, 0.1f, 20.0f }
    });
    world->AddComponent<MeshRenderer>(m_ground, MeshRenderer{
        "Assets/Models/cube.obj", "Assets/Materials/concrete.mat",
        false, true, true
    });

    // -- Create a point light so we can actually see the cube ----------------
    m_light = world->CreateEntity();
    world->AddComponent<NameComponent>(m_light, NameComponent{ "KeyLight" });
    world->AddComponent<Transform>(m_light, Transform{
        { 3.0f, 5.0f, -3.0f },
        { 0.0f, 0.0f, 0.0f },
        { 1.0f, 1.0f, 1.0f }
    });
    world->AddComponent<LightComponent>(m_light, LightComponent{
        LightComponent::Type::Point,
        { 1.0f, 0.95f, 0.8f },  // warm white colour
        2.0f,                    // intensity
        20.0f,                   // range
        45.0f,                   // spotAngle  (unused for point)
        30.0f,                   // spotInnerAngle
        true,                    // castShadows
        1024                     // shadowMapResolution
    });

    // -- Camera entity for player view ---------------------------------------
    m_camera = world->CreateEntity();
    world->AddComponent<NameComponent>(m_camera, NameComponent{ "MainCamera" });
    world->AddComponent<Transform>(m_camera, Transform{
        { 0.0f, 5.0f, -10.0f },
        { -20.0f, 0.0f, 0.0f },
        { 1.0f, 1.0f, 1.0f }
    });
    world->AddComponent<Camera>(m_camera, Camera{
        60.0f,   // fov
        0.1f,    // nearPlane
        500.0f,  // farPlane
        true     // isActive
    });
}

void HelloCubeModule::OnUnload()
{
    if (auto* world = m_ctx ? m_ctx->GetWorld() : nullptr)
    {
        for (auto e : { m_cube, m_light, m_camera, m_ground })
            if (e != entt::null) world->DestroyEntity(e);
    }
    m_ctx = nullptr;
}

// ---------------------------------------------------------------------------
// Per-frame update
// ---------------------------------------------------------------------------

void HelloCubeModule::OnUpdate(float dt)
{
    auto* input = m_ctx->GetInput();
    auto* world = m_ctx->GetWorld();
    if (!input || !world) return;

    auto& xf = world->GetComponent<Transform>(m_cube);

    // --- Rotation: Q/E keys or auto-spin ------------------------------------
    if (input->IsKeyDown('Q'))
        m_yaw -= kRotateSpeed * dt;
    else if (input->IsKeyDown('E'))
        m_yaw += kRotateSpeed * dt;
    else
        m_yaw += 45.0f * dt; // gentle auto-spin when no key held

    if (input->IsKeyDown('R'))
        m_pitch += kRotateSpeed * dt;

    xf.rotation = { m_pitch, m_yaw, 0.0f };

    // --- Translation: WASD --------------------------------------------------
    if (input->IsKeyDown('W')) xf.position.z += kMoveSpeed * dt;
    if (input->IsKeyDown('S')) xf.position.z -= kMoveSpeed * dt;
    if (input->IsKeyDown('A')) xf.position.x -= kMoveSpeed * dt;
    if (input->IsKeyDown('D')) xf.position.x += kMoveSpeed * dt;

    // --- Scale toggle: Space ------------------------------------------------
    if (input->WasKeyPressed(VK_SPACE))
    {
        bool big = (xf.scale.x > 1.5f);
        float s = big ? 1.0f : 2.0f;
        xf.scale = { s, s, s };
    }

    // --- Mouse orbit camera -------------------------------------------------
    if (input->IsMouseButtonDown(1)) // right-click drag to orbit
    {
        auto mouseDelta = input->GetMouseDelta();
        m_camYaw   += mouseDelta.x * 0.3f;
        m_camPitch -= mouseDelta.y * 0.3f;
        m_camPitch  = std::max(-80.0f, std::min(10.0f, m_camPitch));
    }

    // Mouse wheel to zoom
    float scroll = input->GetMouseScrollDelta();
    m_camDist -= scroll * 1.0f;
    m_camDist  = std::max(3.0f, std::min(30.0f, m_camDist));

    // Compute camera orbit position around the cube
    float radYaw   = m_camYaw   * (3.14159f / 180.0f);
    float radPitch = m_camPitch * (3.14159f / 180.0f);

    auto& camXf = world->GetComponent<Transform>(m_camera);
    camXf.position = {
        xf.position.x + std::cos(radPitch) * std::sin(radYaw) * m_camDist,
        xf.position.y - std::sin(radPitch) * m_camDist,
        xf.position.z + std::cos(radPitch) * std::cos(radYaw) * m_camDist
    };
    camXf.rotation = { m_camPitch, m_camYaw + 180.0f, 0.0f };
}

// ---------------------------------------------------------------------------
// Render — the engine's RenderSystem handles drawing; nothing custom needed.
// ---------------------------------------------------------------------------

void HelloCubeModule::OnRender() { /* RenderSystem auto-draws MeshRenderers */ }

void HelloCubeModule::OnResize(int /*w*/, int /*h*/) { }

// ---------------------------------------------------------------------------
// DLL exports
// ---------------------------------------------------------------------------

SPARK_IMPLEMENT_MODULE(HelloCubeModule)
