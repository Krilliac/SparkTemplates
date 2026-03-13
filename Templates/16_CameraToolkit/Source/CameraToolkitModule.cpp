/**
 * @file CameraToolkitModule.cpp
 * @brief INTERMEDIATE — Camera system capabilities showcase.
 */

#include "CameraToolkitModule.h"
#include <Graphics/GraphicsEngine.h>
#include <Graphics/PostProcessingSystem.h>
#include <Input/InputManager.h>
#include <cmath>

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void CameraToolkitModule::OnLoad(Spark::IEngineContext* ctx)
{
    m_ctx = ctx;
    BuildScene();
    CreateCinematicPath();
}

void CameraToolkitModule::OnUnload()
{
    auto* seq = Spark::SequencerManager::GetInstance();
    if (seq) seq->StopAll();

    // Restore full viewport if split-screen was active
    auto* gfx = m_ctx ? m_ctx->GetGraphics() : nullptr;
    if (gfx)
    {
        gfx->SetLetterbox(false, 0.0f);
        gfx->RemoveViewport(1);
    }

    auto* world = m_ctx ? m_ctx->GetWorld() : nullptr;
    if (!world) { m_ctx = nullptr; return; }

    for (auto e : { m_floor, m_light, m_mainCamera, m_splitCamera, m_player })
        if (e != entt::null) world->DestroyEntity(e);

    for (auto e : m_props)
        if (e != entt::null) world->DestroyEntity(e);
    m_props.clear();

    m_ctx = nullptr;
}

// ---------------------------------------------------------------------------
// Scene construction
// ---------------------------------------------------------------------------

void CameraToolkitModule::BuildScene()
{
    auto* world = m_ctx->GetWorld();

    // Floor
    m_floor = world->CreateEntity();
    world->AddComponent<NameComponent>(m_floor, NameComponent{ "Floor" });
    world->AddComponent<Transform>(m_floor, Transform{
        { 0.f, -0.5f, 0.f }, { 0.f, 0.f, 0.f }, { 80.f, 1.f, 80.f }
    });
    world->AddComponent<MeshRenderer>(m_floor, MeshRenderer{
        "Assets/Models/cube.obj", "Assets/Materials/grid.mat",
        false, true, true
    });

    // Directional light
    m_light = world->CreateEntity();
    world->AddComponent<NameComponent>(m_light, NameComponent{ "SunLight" });
    world->AddComponent<Transform>(m_light, Transform{
        { 0.f, 25.f, 0.f }, { -45.f, 30.f, 0.f }, { 1.f, 1.f, 1.f }
    });
    world->AddComponent<LightComponent>(m_light, LightComponent{
        LightComponent::Type::Directional,
        { 1.f, 0.95f, 0.85f }, 2.0f, 0.f,
        0.f, 0.f, true, 2048
    });

    // Main camera
    m_mainCamera = world->CreateEntity();
    world->AddComponent<NameComponent>(m_mainCamera, NameComponent{ "MainCamera" });
    world->AddComponent<Transform>(m_mainCamera, Transform{
        { 0.f, 2.f, 0.f }, { 0.f, 0.f, 0.f }, { 1.f, 1.f, 1.f }
    });
    world->AddComponent<Camera>(m_mainCamera, Camera{
        60.0f, 0.1f, 1000.0f, true
    });

    // Split-screen secondary camera (inactive by default)
    m_splitCamera = world->CreateEntity();
    world->AddComponent<NameComponent>(m_splitCamera, NameComponent{ "SplitCamera" });
    world->AddComponent<Transform>(m_splitCamera, Transform{
        { 0.f, 20.f, 0.f }, { -90.f, 0.f, 0.f }, { 1.f, 1.f, 1.f }
    });
    world->AddComponent<Camera>(m_splitCamera, Camera{
        60.0f, 0.1f, 1000.0f, false   // starts inactive
    });

    // Player entity (target for third-person camera)
    m_player = world->CreateEntity();
    world->AddComponent<NameComponent>(m_player, NameComponent{ "Player" });
    world->AddComponent<Transform>(m_player, Transform{
        { 0.f, 1.f, 0.f }, { 0.f, 0.f, 0.f }, { 1.f, 2.f, 1.f }
    });
    world->AddComponent<MeshRenderer>(m_player, MeshRenderer{
        "Assets/Models/cube.obj", "Assets/Materials/character.mat",
        true, true, true
    });

    // Scatter props around the scene for visual reference
    auto spawnProp = [&](const char* name, const char* mesh,
                         DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 scale) {
        auto e = world->CreateEntity();
        world->AddComponent<NameComponent>(e, NameComponent{ name });
        world->AddComponent<Transform>(e, Transform{
            pos, { 0.f, 0.f, 0.f }, scale
        });
        world->AddComponent<MeshRenderer>(e, MeshRenderer{
            mesh, "Assets/Materials/concrete.mat", true, true, true
        });
        m_props.push_back(e);
    };

    spawnProp("Tower",     "Assets/Models/watchtower.obj",     { 10.f, 0.f, 10.f },  { 2.f, 2.f, 2.f });
    spawnProp("Building",  "Assets/Models/building_small.obj", { -15.f, 0.f, 8.f },  { 1.5f, 1.5f, 1.5f });
    spawnProp("Barrier1",  "Assets/Models/barrier.obj",        { 5.f, 0.f, -5.f },   { 1.f, 1.f, 1.f });
    spawnProp("Barrier2",  "Assets/Models/barrier.obj",        { -5.f, 0.f, -10.f },  { 1.f, 1.f, 1.f });
    spawnProp("Column1",   "Assets/Models/cube.obj",           { 8.f, 3.f, -8.f },   { 1.f, 6.f, 1.f });
    spawnProp("Column2",   "Assets/Models/cube.obj",           { -8.f, 3.f, -8.f },  { 1.f, 6.f, 1.f });

    // DOF target sphere (something to focus on)
    auto focusBall = world->CreateEntity();
    world->AddComponent<NameComponent>(focusBall, NameComponent{ "FocusBall" });
    world->AddComponent<Transform>(focusBall, Transform{
        { 0.f, 1.5f, 10.f }, { 0.f, 0.f, 0.f }, { 1.5f, 1.5f, 1.5f }
    });
    world->AddComponent<MeshRenderer>(focusBall, MeshRenderer{
        "Assets/Models/sphere.obj", "Assets/Materials/emissive_blue.mat",
        true, true, true
    });
    m_props.push_back(focusBall);
}

// ---------------------------------------------------------------------------
// Cinematic camera path
// ---------------------------------------------------------------------------

void CameraToolkitModule::CreateCinematicPath()
{
    auto* seq = Spark::SequencerManager::GetInstance();
    if (!seq) return;

    m_pathSequenceID = seq->CreateSequence("CameraShowcasePath");

    // Sweeping path around the scene
    seq->AddCameraPathKey(m_pathSequenceID, 0.0f,
        { -20.f, 10.f, -20.f }, { -20.f, 45.f, 0.f }, 70.0f, 0.0f,
        Spark::Interpolation::CatmullRom);

    seq->AddCameraPathKey(m_pathSequenceID, 3.0f,
        { 20.f, 8.f, -10.f }, { -15.f, -30.f, 0.f }, 60.0f, 0.0f,
        Spark::Interpolation::CatmullRom);

    seq->AddCameraPathKey(m_pathSequenceID, 6.0f,
        { 15.f, 5.f, 15.f }, { -5.f, -120.f, 0.f }, 55.0f, 5.0f,
        Spark::Interpolation::CubicBezier);

    seq->AddCameraPathKey(m_pathSequenceID, 9.0f,
        { -10.f, 12.f, 10.f }, { -25.f, 150.f, 0.f }, 65.0f, -3.0f,
        Spark::Interpolation::CatmullRom);

    seq->AddCameraPathKey(m_pathSequenceID, 12.0f,
        { 0.f, 5.f, -15.f }, { -10.f, 0.f, 0.f }, 60.0f, 0.0f,
        Spark::Interpolation::Linear);
}

// ---------------------------------------------------------------------------
// Camera mode: First Person
// ---------------------------------------------------------------------------

void CameraToolkitModule::UpdateFirstPerson(float dt)
{
    auto* input = m_ctx->GetInput();
    auto* world = m_ctx->GetWorld();

    // Mouse look
    auto md = input->GetMouseDelta();
    m_fpsYaw   += md.x * kMouseSens;
    m_fpsPitch -= md.y * kMouseSens;
    m_fpsPitch  = std::max(-89.0f, std::min(89.0f, m_fpsPitch));

    // Direction vectors
    float radYaw   = m_fpsYaw   * (3.14159f / 180.0f);
    float radPitch = m_fpsPitch * (3.14159f / 180.0f);

    float fwdX = std::cos(radPitch) * std::sin(radYaw);
    float fwdZ = std::cos(radPitch) * std::cos(radYaw);
    float rightX = std::cos(radYaw);
    float rightZ = -std::sin(radYaw);

    // WASD movement
    float speed = kMoveSpeed * dt;
    if (input->IsKeyDown('W')) { m_fpsPos.x += fwdX * speed;   m_fpsPos.z += fwdZ * speed; }
    if (input->IsKeyDown('S')) { m_fpsPos.x -= fwdX * speed;   m_fpsPos.z -= fwdZ * speed; }
    if (input->IsKeyDown('A')) { m_fpsPos.x -= rightX * speed;  m_fpsPos.z -= rightZ * speed; }
    if (input->IsKeyDown('D')) { m_fpsPos.x += rightX * speed;  m_fpsPos.z += rightZ * speed; }

    auto& camXf = world->GetComponent<Transform>(m_mainCamera);
    camXf.position = m_fpsPos;
    camXf.rotation = { m_fpsPitch, m_fpsYaw, 0.0f };

    // Hide player model in first person
    if (world->HasComponent<MeshRenderer>(m_player))
        world->GetComponent<MeshRenderer>(m_player).visible = false;
}

// ---------------------------------------------------------------------------
// Camera mode: Third Person (spring chase)
// ---------------------------------------------------------------------------

void CameraToolkitModule::UpdateThirdPerson(float dt)
{
    auto* input = m_ctx->GetInput();
    auto* world = m_ctx->GetWorld();

    // Show player in third person
    if (world->HasComponent<MeshRenderer>(m_player))
        world->GetComponent<MeshRenderer>(m_player).visible = true;

    auto& playerXf = world->GetComponent<Transform>(m_player);

    // Move player with WASD
    float speed = kMoveSpeed * dt;
    if (input->IsKeyDown('W')) playerXf.position.z += speed;
    if (input->IsKeyDown('S')) playerXf.position.z -= speed;
    if (input->IsKeyDown('A')) playerXf.position.x -= speed;
    if (input->IsKeyDown('D')) playerXf.position.x += speed;

    // Mouse orbit around player
    if (input->IsMouseButtonDown(1))
    {
        auto md = input->GetMouseDelta();
        m_tpsYaw   += md.x * kMouseSens;
        m_tpsPitch -= md.y * kMouseSens;
        m_tpsPitch  = std::max(-80.0f, std::min(10.0f, m_tpsPitch));
    }

    float scroll = input->GetMouseScrollDelta();
    m_tpsDist -= scroll * 1.0f;
    m_tpsDist  = std::max(3.0f, std::min(20.0f, m_tpsDist));

    // Compute desired camera position
    float radYaw   = m_tpsYaw   * (3.14159f / 180.0f);
    float radPitch = m_tpsPitch * (3.14159f / 180.0f);

    DirectX::XMFLOAT3 desiredPos = {
        playerXf.position.x + std::cos(radPitch) * std::sin(radYaw) * m_tpsDist,
        playerXf.position.y + 2.0f - std::sin(radPitch) * m_tpsDist,
        playerXf.position.z + std::cos(radPitch) * std::cos(radYaw) * m_tpsDist
    };

    // Spring-arm interpolation for smooth follow
    auto& camXf = world->GetComponent<Transform>(m_mainCamera);

    float springForceX = (desiredPos.x - camXf.position.x) * m_tpsSpringK;
    float springForceY = (desiredPos.y - camXf.position.y) * m_tpsSpringK;
    float springForceZ = (desiredPos.z - camXf.position.z) * m_tpsSpringK;

    m_tpsVelocity.x = (m_tpsVelocity.x + springForceX * dt) * m_tpsDamping;
    m_tpsVelocity.y = (m_tpsVelocity.y + springForceY * dt) * m_tpsDamping;
    m_tpsVelocity.z = (m_tpsVelocity.z + springForceZ * dt) * m_tpsDamping;

    camXf.position.x += m_tpsVelocity.x * dt;
    camXf.position.y += m_tpsVelocity.y * dt;
    camXf.position.z += m_tpsVelocity.z * dt;

    camXf.rotation = { m_tpsPitch, m_tpsYaw + 180.0f, 0.0f };
}

// ---------------------------------------------------------------------------
// Camera mode: Orbit
// ---------------------------------------------------------------------------

void CameraToolkitModule::UpdateOrbit(float dt)
{
    auto* input = m_ctx->GetInput();
    auto* world = m_ctx->GetWorld();

    // Show player as orbit target
    if (world->HasComponent<MeshRenderer>(m_player))
        world->GetComponent<MeshRenderer>(m_player).visible = true;

    // WASD moves the orbit pivot
    float speed = kMoveSpeed * 0.5f * dt;
    if (input->IsKeyDown('W')) m_orbitPivot.z += speed;
    if (input->IsKeyDown('S')) m_orbitPivot.z -= speed;
    if (input->IsKeyDown('A')) m_orbitPivot.x -= speed;
    if (input->IsKeyDown('D')) m_orbitPivot.x += speed;

    // Mouse drag to orbit
    auto md = input->GetMouseDelta();
    if (input->IsMouseButtonDown(0) || input->IsMouseButtonDown(1))
    {
        m_orbitYaw   += md.x * kMouseSens;
        m_orbitPitch -= md.y * kMouseSens;
        m_orbitPitch  = std::max(-89.0f, std::min(89.0f, m_orbitPitch));
    }

    // Scroll to zoom
    float scroll = input->GetMouseScrollDelta();
    m_orbitDist -= scroll * 1.0f;
    m_orbitDist  = std::max(2.0f, std::min(40.0f, m_orbitDist));

    float radYaw   = m_orbitYaw   * (3.14159f / 180.0f);
    float radPitch = m_orbitPitch * (3.14159f / 180.0f);

    auto& camXf = world->GetComponent<Transform>(m_mainCamera);
    camXf.position = {
        m_orbitPivot.x + std::cos(radPitch) * std::sin(radYaw) * m_orbitDist,
        m_orbitPivot.y - std::sin(radPitch) * m_orbitDist,
        m_orbitPivot.z + std::cos(radPitch) * std::cos(radYaw) * m_orbitDist
    };
    camXf.rotation = { m_orbitPitch, m_orbitYaw + 180.0f, 0.0f };
}

// ---------------------------------------------------------------------------
// Camera mode: Free Fly (6DOF)
// ---------------------------------------------------------------------------

void CameraToolkitModule::UpdateFreeFly(float dt)
{
    auto* input = m_ctx->GetInput();
    auto* world = m_ctx->GetWorld();

    // Hide player in free-fly
    if (world->HasComponent<MeshRenderer>(m_player))
        world->GetComponent<MeshRenderer>(m_player).visible = false;

    // Mouse look
    auto md = input->GetMouseDelta();
    m_flyYaw   += md.x * kMouseSens;
    m_flyPitch -= md.y * kMouseSens;
    m_flyPitch  = std::max(-89.0f, std::min(89.0f, m_flyPitch));

    float radYaw   = m_flyYaw   * (3.14159f / 180.0f);
    float radPitch = m_flyPitch * (3.14159f / 180.0f);

    // Forward / right / up vectors
    float fwdX = std::cos(radPitch) * std::sin(radYaw);
    float fwdY = std::sin(radPitch);
    float fwdZ = std::cos(radPitch) * std::cos(radYaw);

    float rightX = std::cos(radYaw);
    float rightZ = -std::sin(radYaw);

    float speed = kFlySpeed * dt;

    // Shift for fast movement
    if (input->IsKeyDown(VK_SHIFT))
        speed *= 3.0f;

    // 6DOF: WASD + Q/E for vertical
    if (input->IsKeyDown('W')) { m_flyPos.x += fwdX * speed; m_flyPos.y += fwdY * speed; m_flyPos.z += fwdZ * speed; }
    if (input->IsKeyDown('S')) { m_flyPos.x -= fwdX * speed; m_flyPos.y -= fwdY * speed; m_flyPos.z -= fwdZ * speed; }
    if (input->IsKeyDown('A')) { m_flyPos.x -= rightX * speed; m_flyPos.z -= rightZ * speed; }
    if (input->IsKeyDown('D')) { m_flyPos.x += rightX * speed; m_flyPos.z += rightZ * speed; }
    if (input->IsKeyDown('Q')) m_flyPos.y -= speed;
    if (input->IsKeyDown('E')) m_flyPos.y += speed;

    auto& camXf = world->GetComponent<Transform>(m_mainCamera);
    camXf.position = m_flyPos;
    camXf.rotation = { m_flyPitch, m_flyYaw, 0.0f };
}

// ---------------------------------------------------------------------------
// Camera shake
// ---------------------------------------------------------------------------

void CameraToolkitModule::UpdateShake(float dt)
{
    if (m_shakeTrauma <= 0.0f) return;

    auto* world = m_ctx->GetWorld();
    auto& camXf = world->GetComponent<Transform>(m_mainCamera);

    m_shakeTimer += dt;

    // Shake intensity = trauma^2 (quadratic falloff feels more natural)
    float intensity = m_shakeTrauma * m_shakeTrauma;

    // Perlin-like oscillation using sin at different frequencies
    float offsetX = std::sin(m_shakeTimer * m_shakeFreq)        * intensity * 0.3f;
    float offsetY = std::sin(m_shakeTimer * m_shakeFreq * 1.3f) * intensity * 0.3f;
    float rollDeg = std::sin(m_shakeTimer * m_shakeFreq * 0.7f) * intensity * 3.0f;

    camXf.position.x += offsetX;
    camXf.position.y += offsetY;
    camXf.rotation.z  = rollDeg;

    // Decay trauma over time
    m_shakeTrauma -= m_shakeDecay * dt;
    if (m_shakeTrauma < 0.0f) m_shakeTrauma = 0.0f;
}

// ---------------------------------------------------------------------------
// Depth of Field
// ---------------------------------------------------------------------------

void CameraToolkitModule::ApplyDOF()
{
    auto* gfx = m_ctx->GetGraphics();
    if (!gfx) return;

    auto* post = gfx->GetPostProcessingSystem();
    if (!post) return;

    post->SetDOFEnabled(m_dofEnabled);
    if (m_dofEnabled)
    {
        post->SetDOFFocusDistance(m_dofFocusDist);
        post->SetDOFAperture(m_dofAperture);
    }
}

// ---------------------------------------------------------------------------
// FOV animation
// ---------------------------------------------------------------------------

void CameraToolkitModule::UpdateFOVAnimation(float dt)
{
    if (std::abs(m_currentFOV - m_targetFOV) < 0.01f)
    {
        m_currentFOV = m_targetFOV;
        return;
    }

    float dir = (m_targetFOV > m_currentFOV) ? 1.0f : -1.0f;
    m_currentFOV += dir * kFOVSpeed * dt;

    // Clamp to not overshoot
    if (dir > 0.0f && m_currentFOV > m_targetFOV) m_currentFOV = m_targetFOV;
    if (dir < 0.0f && m_currentFOV < m_targetFOV) m_currentFOV = m_targetFOV;

    auto* world = m_ctx->GetWorld();
    if (world && world->HasComponent<Camera>(m_mainCamera))
    {
        auto& cam = world->GetComponent<Camera>(m_mainCamera);
        cam.fov = m_currentFOV;
    }
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

void CameraToolkitModule::OnUpdate(float dt)
{
    auto* input = m_ctx->GetInput();
    auto* world = m_ctx->GetWorld();
    if (!input || !world) return;

    // Tab — cycle camera modes
    if (input->WasKeyPressed(VK_TAB))
    {
        int next = (static_cast<int>(m_cameraMode) + 1) % static_cast<int>(CameraMode::Count);
        m_cameraMode = static_cast<CameraMode>(next);

        // Sync state from current camera position when switching
        auto& camXf = world->GetComponent<Transform>(m_mainCamera);
        switch (m_cameraMode)
        {
        case CameraMode::FirstPerson:
            m_fpsPos   = camXf.position;
            m_fpsYaw   = camXf.rotation.y;
            m_fpsPitch = camXf.rotation.x;
            break;
        case CameraMode::ThirdPerson:
            // Reset to behind player
            m_tpsYaw   = 0.0f;
            m_tpsPitch = -15.0f;
            m_tpsVelocity = { 0.f, 0.f, 0.f };
            break;
        case CameraMode::Orbit:
            m_orbitYaw   = camXf.rotation.y - 180.0f;
            m_orbitPitch = camXf.rotation.x;
            break;
        case CameraMode::FreeFly:
            m_flyPos   = camXf.position;
            m_flyYaw   = camXf.rotation.y;
            m_flyPitch = camXf.rotation.x;
            break;
        default: break;
        }
    }

    // Update current camera mode
    switch (m_cameraMode)
    {
    case CameraMode::FirstPerson: UpdateFirstPerson(dt); break;
    case CameraMode::ThirdPerson: UpdateThirdPerson(dt); break;
    case CameraMode::Orbit:       UpdateOrbit(dt);       break;
    case CameraMode::FreeFly:     UpdateFreeFly(dt);     break;
    default: break;
    }

    // K — Trigger camera shake
    if (input->WasKeyPressed('K'))
    {
        m_shakeTrauma = 1.0f;
        m_shakeTimer  = 0.0f;
    }

    // F — Toggle DOF
    if (input->WasKeyPressed('F'))
    {
        m_dofEnabled = !m_dofEnabled;
        ApplyDOF();
    }

    // L — Toggle letterbox
    if (input->WasKeyPressed('L'))
    {
        m_letterbox = !m_letterbox;
        auto* gfx = m_ctx->GetGraphics();
        if (gfx)
            gfx->SetLetterbox(m_letterbox, 2.35f);  // 2.35:1 cinemascope
    }

    // V — Toggle split-screen
    if (input->WasKeyPressed('V'))
    {
        m_splitScreen = !m_splitScreen;
        auto* gfx = m_ctx->GetGraphics();
        if (gfx && world)
        {
            if (m_splitScreen)
            {
                // Main camera on left half
                gfx->SetViewport(0, { 0.0f, 0.0f, 0.5f, 1.0f });

                // Secondary camera on right half (top-down view)
                auto& splitCam = world->GetComponent<Camera>(m_splitCamera);
                splitCam.isActive = true;
                gfx->SetViewport(1, { 0.5f, 0.0f, 0.5f, 1.0f });
                gfx->AssignCameraToViewport(1, m_splitCamera);
            }
            else
            {
                // Restore full viewport
                gfx->SetViewport(0, { 0.0f, 0.0f, 1.0f, 1.0f });
                gfx->RemoveViewport(1);

                auto& splitCam = world->GetComponent<Camera>(m_splitCamera);
                splitCam.isActive = false;
            }
        }
    }

    // +/- — FOV adjustment
    if (input->WasKeyPressed(VK_OEM_PLUS) || input->WasKeyPressed(VK_ADD))
    {
        m_targetFOV = std::min(120.0f, m_targetFOV + 5.0f);
    }
    if (input->WasKeyPressed(VK_OEM_MINUS) || input->WasKeyPressed(VK_SUBTRACT))
    {
        m_targetFOV = std::max(20.0f, m_targetFOV - 5.0f);
    }

    // Update cinematic path sequencer
    auto* seq = Spark::SequencerManager::GetInstance();
    if (seq) seq->Update(dt);

    // Update split-screen camera (follow main camera from top-down)
    if (m_splitScreen)
    {
        auto& mainXf  = world->GetComponent<Transform>(m_mainCamera);
        auto& splitXf = world->GetComponent<Transform>(m_splitCamera);
        splitXf.position = {
            mainXf.position.x,
            30.0f,
            mainXf.position.z
        };
        splitXf.rotation = { -90.0f, 0.0f, 0.0f };
    }

    // Apply shake after camera mode update
    UpdateShake(dt);

    // Animate FOV
    UpdateFOVAnimation(dt);
}

// ---------------------------------------------------------------------------
void CameraToolkitModule::OnRender() { /* RenderSystem auto-draws */ }

void CameraToolkitModule::OnResize(int w, int h)
{
    // Re-apply DOF settings on resize (aspect ratio change)
    if (m_dofEnabled)
        ApplyDOF();
}

// ---------------------------------------------------------------------------
// DLL exports
// ---------------------------------------------------------------------------

SPARK_IMPLEMENT_MODULE(CameraToolkitModule)
