/**
 * @file HelloCubeModule.cpp
 * @brief BASIC — Spinning cube with keyboard controls.
 */

#include "HelloCubeModule.h"
#include <Input/InputManager.h>

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
}

void HelloCubeModule::OnUnload()
{
    if (auto* world = m_ctx ? m_ctx->GetWorld() : nullptr)
    {
        if (m_cube != entt::null)  world->DestroyEntity(m_cube);
        if (m_light != entt::null) world->DestroyEntity(m_light);
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

    // --- Rotation: arrow keys or auto-spin ----------------------------------
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
