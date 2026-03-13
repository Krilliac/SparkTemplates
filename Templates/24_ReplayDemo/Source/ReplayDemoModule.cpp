/**
 * @file ReplayDemoModule.cpp
 * @brief ADVANCED — Replay system showcase.
 */

#include "ReplayDemoModule.h"
#include <Input/InputManager.h>
#include <cmath>
#include <algorithm>
#include <cstdio>

// Static member definition
constexpr float ReplayDemoModule::kSpeedOptions[];

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void ReplayDemoModule::OnLoad(Spark::IEngineContext* ctx)
{
    m_ctx = ctx;

    // Subscribe to replay events
    auto* bus = m_ctx->GetEventBus();
    if (bus)
    {
        m_replayStartSubId = bus->Subscribe<ReplayStartedEvent>(
            [](const ReplayStartedEvent&) {});

        m_replayEndSubId = bus->Subscribe<ReplayEndedEvent>(
            [](const ReplayEndedEvent&) {});
    }

    BuildScene();

    // Start recording automatically
    auto* replaySys = ReplaySystem::GetInstance();
    if (replaySys)
        replaySys->StartRecording();

    m_isRecording = true;
    m_recordTime  = 0.0f;
}

void ReplayDemoModule::OnUnload()
{
    auto* bus = m_ctx ? m_ctx->GetEventBus() : nullptr;
    if (bus)
    {
        bus->Unsubscribe<ReplayStartedEvent>(m_replayStartSubId);
        bus->Unsubscribe<ReplayEndedEvent>(m_replayEndSubId);
    }

    // Stop any active recording/playback
    auto* replaySys = ReplaySystem::GetInstance();
    if (replaySys)
    {
        if (replaySys->IsRecording()) replaySys->StopRecording();
        if (replaySys->IsPlaying())   replaySys->StopPlayback();
    }

    auto* world = m_ctx ? m_ctx->GetWorld() : nullptr;
    if (!world) { m_ctx = nullptr; return; }

    for (auto e : { m_player, m_camera, m_light })
        if (e != entt::null) world->DestroyEntity(e);

    for (auto& t : m_targets)
        if (t.entity != entt::null) world->DestroyEntity(t.entity);

    for (auto e : m_sceneEntities)
        if (e != entt::null) world->DestroyEntity(e);

    m_targets.clear();
    m_sceneEntities.clear();
    m_ctx = nullptr;
}

// ---------------------------------------------------------------------------
// Scene construction
// ---------------------------------------------------------------------------

void ReplayDemoModule::BuildScene()
{
    auto* world = m_ctx->GetWorld();

    // --- Player entity ------------------------------------------------------
    m_player = world->CreateEntity();
    world->AddComponent<NameComponent>(m_player, NameComponent{ "Player" });
    world->AddComponent<Transform>(m_player, Transform{
        { 0.0f, 1.7f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.8f, 1.8f, 0.8f }
    });
    world->AddComponent<MeshRenderer>(m_player, MeshRenderer{
        "Assets/Models/cube.obj", "Assets/Materials/player.mat",
        true, true, true
    });

    // --- Camera (FPS) -------------------------------------------------------
    m_camera = world->CreateEntity();
    world->AddComponent<NameComponent>(m_camera, NameComponent{ "FPSCamera" });
    world->AddComponent<Transform>(m_camera, Transform{
        { 0.0f, 1.7f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }
    });
    world->AddComponent<Camera>(m_camera, Camera{
        70.0f, 0.1f, 500.0f, true
    });

    // --- Light --------------------------------------------------------------
    m_light = world->CreateEntity();
    world->AddComponent<NameComponent>(m_light, NameComponent{ "SunLight" });
    world->AddComponent<Transform>(m_light, Transform{
        { 0.0f, 25.0f, 0.0f },
        { -45.0f, 30.0f, 0.0f },
        { 1.0f, 1.0f, 1.0f }
    });
    world->AddComponent<LightComponent>(m_light, LightComponent{
        LightComponent::Type::Directional,
        { 1.0f, 0.95f, 0.9f }, 2.0f, 0.0f,
        0.0f, 0.0f, true, 2048
    });

    // --- Arena ground -------------------------------------------------------
    auto ground = world->CreateEntity();
    world->AddComponent<NameComponent>(ground, NameComponent{ "Ground" });
    world->AddComponent<Transform>(ground, Transform{
        { 0.0f, -0.1f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 40.0f, 0.2f, 40.0f }
    });
    world->AddComponent<MeshRenderer>(ground, MeshRenderer{
        "Assets/Models/cube.obj", "Assets/Materials/concrete.mat",
        false, true, true
    });
    world->AddComponent<ColliderComponent>(ground, ColliderComponent{
        ColliderComponent::Shape::Box, { 40.0f, 0.2f, 40.0f }, true
    });
    m_sceneEntities.push_back(ground);

    // --- Arena walls --------------------------------------------------------
    struct WallDef { float x, y, z, sx, sy, sz; const char* name; };
    WallDef walls[] = {
        {  0.0f, 2.0f,  20.0f, 40.0f, 4.0f, 0.5f, "NorthWall" },
        {  0.0f, 2.0f, -20.0f, 40.0f, 4.0f, 0.5f, "SouthWall" },
        {  20.0f, 2.0f,  0.0f, 0.5f, 4.0f, 40.0f, "EastWall" },
        { -20.0f, 2.0f,  0.0f, 0.5f, 4.0f, 40.0f, "WestWall" },
    };
    for (auto& w : walls)
    {
        auto wall = world->CreateEntity();
        world->AddComponent<NameComponent>(wall, NameComponent{ w.name });
        world->AddComponent<Transform>(wall, Transform{
            { w.x, w.y, w.z }, { 0.0f, 0.0f, 0.0f }, { w.sx, w.sy, w.sz }
        });
        world->AddComponent<MeshRenderer>(wall, MeshRenderer{
            "Assets/Models/cube.obj", "Assets/Materials/concrete.mat",
            false, true, true
        });
        world->AddComponent<ColliderComponent>(wall, ColliderComponent{
            ColliderComponent::Shape::Box, { w.sx, w.sy, w.sz }, true
        });
        m_sceneEntities.push_back(wall);
    }

    // --- Cover objects ------------------------------------------------------
    struct CoverDef { float x, z, sx, sz; };
    CoverDef covers[] = {
        { -8.0f,  6.0f, 3.0f, 1.0f },
        {  8.0f, -4.0f, 1.0f, 3.0f },
        {  0.0f, 10.0f, 4.0f, 1.0f },
        { -12.0f, -8.0f, 2.0f, 2.0f },
    };
    int coverIdx = 0;
    for (auto& c : covers)
    {
        auto cover = world->CreateEntity();
        char name[32];
        std::snprintf(name, sizeof(name), "Cover_%d", coverIdx++);
        world->AddComponent<NameComponent>(cover, NameComponent{ name });
        world->AddComponent<Transform>(cover, Transform{
            { c.x, 1.0f, c.z }, { 0.0f, 0.0f, 0.0f }, { c.sx, 2.0f, c.sz }
        });
        world->AddComponent<MeshRenderer>(cover, MeshRenderer{
            "Assets/Models/cube.obj", "Assets/Materials/stone.mat",
            false, true, true
        });
        world->AddComponent<ColliderComponent>(cover, ColliderComponent{
            ColliderComponent::Shape::Box, { c.sx, 2.0f, c.sz }, true
        });
        m_sceneEntities.push_back(cover);
    }

    // --- AI Targets ---------------------------------------------------------
    struct TargetSpawn { float x, z; const char* name; };
    TargetSpawn spawns[] = {
        {  5.0f,  8.0f, "Target_A" },
        { -6.0f, 12.0f, "Target_B" },
        { 10.0f, -6.0f, "Target_C" },
        { -10.0f, 3.0f, "Target_D" },
        {  3.0f, -12.0f, "Target_E" },
    };

    for (auto& sp : spawns)
    {
        auto e = world->CreateEntity();
        world->AddComponent<NameComponent>(e, NameComponent{ sp.name });
        world->AddComponent<Transform>(e, Transform{
            { sp.x, 1.0f, sp.z },
            { 0.0f, 0.0f, 0.0f },
            { 1.0f, 2.0f, 1.0f }
        });
        world->AddComponent<MeshRenderer>(e, MeshRenderer{
            "Assets/Models/cube.obj", "Assets/Materials/enemy.mat",
            true, true, true
        });
        world->AddComponent<RigidBodyComponent>(e, RigidBodyComponent{
            5.0f, 0.5f, 0.3f, false, false
        });
        world->AddComponent<ColliderComponent>(e, ColliderComponent{
            ColliderComponent::Shape::Box, { 1.0f, 2.0f, 1.0f }, false
        });

        AITarget ai;
        ai.entity    = e;
        ai.name      = sp.name;
        ai.alive     = true;
        ai.moveTimer = 0.0f;
        ai.moveDirX  = 0.0f;
        ai.moveDirZ  = 1.0f;
        ai.moveSpeed = 2.0f + static_cast<float>(m_targets.size()) * 0.5f;
        ai.nextDirChange = 2.0f;
        ai.spawnX = sp.x;
        ai.spawnY = 1.0f;
        ai.spawnZ = sp.z;
        m_targets.push_back(ai);
    }
}

void ReplayDemoModule::ResetTargets()
{
    auto* world = m_ctx->GetWorld();
    if (!world) return;

    for (auto& t : m_targets)
    {
        t.alive     = true;
        t.moveTimer = 0.0f;
        t.moveDirX  = 0.0f;
        t.moveDirZ  = 1.0f;
        t.nextDirChange = 2.0f;

        if (t.entity != entt::null)
        {
            auto& xf = world->GetComponent<Transform>(t.entity);
            xf.position = { t.spawnX, t.spawnY, t.spawnZ };
            xf.scale    = { 1.0f, 2.0f, 1.0f };

            auto& mr = world->GetComponent<MeshRenderer>(t.entity);
            mr.visible = true;
        }
    }

    m_killCount = 0;
    m_kills.clear();
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

void ReplayDemoModule::OnUpdate(float dt)
{
    if (m_inKillCam)
    {
        UpdateKillCam(dt);
        return;
    }

    if (m_isPlaying)
    {
        HandlePlaybackInput(dt);

        // Update playback time from ReplaySystem
        auto* replaySys = ReplaySystem::GetInstance();
        if (replaySys)
        {
            // Check if playback ended
            if (!replaySys->IsPlaying())
            {
                m_isPlaying = false;
                m_isPaused  = false;

                auto* bus = m_ctx->GetEventBus();
                if (bus)
                    bus->Publish(ReplayEndedEvent{ replaySys->GetDuration() });
            }
        }
    }
    else
    {
        HandleLiveInput(dt);
        UpdateAI(dt);

        // Track recording time
        if (m_isRecording)
            m_recordTime += dt;

        // Update player entity
        auto* world = m_ctx->GetWorld();
        if (world && m_player != entt::null)
        {
            auto& xf = world->GetComponent<Transform>(m_player);
            xf.position = { m_playerX, m_playerY, m_playerZ };
            xf.rotation = { 0.0f, m_camYaw, 0.0f };
        }

        // Update camera
        if (world && m_camera != entt::null)
        {
            auto& camXf = world->GetComponent<Transform>(m_camera);
            camXf.position = { m_playerX, m_playerY, m_playerZ };
            camXf.rotation = { m_camPitch, m_camYaw, 0.0f };
        }
    }
}

// ---------------------------------------------------------------------------
// AI movement
// ---------------------------------------------------------------------------

void ReplayDemoModule::UpdateAI(float dt)
{
    auto* world = m_ctx->GetWorld();
    if (!world) return;

    for (auto& t : m_targets)
    {
        if (!t.alive || t.entity == entt::null) continue;

        t.moveTimer += dt;

        // Change direction periodically
        if (t.moveTimer >= t.nextDirChange)
        {
            t.moveTimer = 0.0f;
            t.nextDirChange = 1.5f + static_cast<float>(static_cast<int>(t.moveTimer * 1000.0f) % 30) * 0.1f;

            // Pseudo-random direction change
            float angle = static_cast<float>(static_cast<int>(t.moveTimer * 7919.0f) % 628) * 0.01f;
            t.moveDirX = std::sin(angle);
            t.moveDirZ = std::cos(angle);
        }

        auto& xf = world->GetComponent<Transform>(t.entity);
        xf.position.x += t.moveDirX * t.moveSpeed * dt;
        xf.position.z += t.moveDirZ * t.moveSpeed * dt;

        // Keep within arena bounds
        xf.position.x = std::clamp(xf.position.x, -18.0f, 18.0f);
        xf.position.z = std::clamp(xf.position.z, -18.0f, 18.0f);

        // Bounce off walls
        if (std::abs(xf.position.x) > 17.0f) t.moveDirX = -t.moveDirX;
        if (std::abs(xf.position.z) > 17.0f) t.moveDirZ = -t.moveDirZ;

        // Face movement direction
        xf.rotation.y = std::atan2(t.moveDirX, t.moveDirZ) * (180.0f / 3.14159f);
    }
}

// ---------------------------------------------------------------------------
// Live input (recording mode)
// ---------------------------------------------------------------------------

void ReplayDemoModule::HandleLiveInput(float dt)
{
    auto* input = m_ctx->GetInput();
    if (!input) return;

    // --- Mouse look ---------------------------------------------------------
    float mouseX = input->GetMouseDeltaX();
    float mouseY = input->GetMouseDeltaY();
    float sensitivity = 0.15f;

    m_camYaw   += mouseX * sensitivity;
    m_camPitch -= mouseY * sensitivity;
    m_camPitch  = std::clamp(m_camPitch, -89.0f, 89.0f);

    // --- WASD movement ------------------------------------------------------
    float moveX = 0.0f, moveZ = 0.0f;
    if (input->IsKeyDown('W')) moveZ += 1.0f;
    if (input->IsKeyDown('S')) moveZ -= 1.0f;
    if (input->IsKeyDown('A')) moveX -= 1.0f;
    if (input->IsKeyDown('D')) moveX += 1.0f;

    float speed = 7.0f;
    float yawRad = m_camYaw * (3.14159f / 180.0f);

    float forwardX = std::sin(yawRad);
    float forwardZ = std::cos(yawRad);
    float rightX   = std::cos(yawRad);
    float rightZ   = -std::sin(yawRad);

    m_playerX += (forwardX * moveZ + rightX * moveX) * speed * dt;
    m_playerZ += (forwardZ * moveZ + rightZ * moveX) * speed * dt;

    // Clamp to arena
    m_playerX = std::clamp(m_playerX, -18.0f, 18.0f);
    m_playerZ = std::clamp(m_playerZ, -18.0f, 18.0f);

    // --- LMB: Shoot ---------------------------------------------------------
    if (input->WasMouseButtonPressed(0))
        ShootRaycast();

    // --- F5: Stop recording, start playback ---------------------------------
    if (input->WasKeyPressed(VK_F5))
        StartPlayback();

    // --- F6: Save replay to file --------------------------------------------
    if (input->WasKeyPressed(VK_F6))
    {
        auto* replaySys = ReplaySystem::GetInstance();
        if (replaySys)
            replaySys->SaveToFile("replay.dat");
    }

    // --- F7: Load replay from file ------------------------------------------
    if (input->WasKeyPressed(VK_F7))
    {
        auto* replaySys = ReplaySystem::GetInstance();
        if (replaySys)
        {
            replaySys->LoadFromFile("replay.dat");
            StartPlayback();
        }
    }
}

// ---------------------------------------------------------------------------
// Playback input
// ---------------------------------------------------------------------------

void ReplayDemoModule::HandlePlaybackInput(float dt)
{
    auto* input = m_ctx->GetInput();
    if (!input) return;

    auto* replaySys = ReplaySystem::GetInstance();
    if (!replaySys) return;

    // --- Space: Pause / Resume ----------------------------------------------
    if (input->WasKeyPressed(VK_SPACE))
    {
        m_isPaused = !m_isPaused;
        if (m_isPaused)
            replaySys->Pause();
        else
            replaySys->Resume();
    }

    // --- Left/Right: Seek ---------------------------------------------------
    if (input->WasKeyPressed(VK_LEFT))
    {
        float curTime = replaySys->GetPlaybackTime();
        float seekTo = std::max(0.0f, curTime - 2.0f);
        replaySys->Seek(seekTo);
    }
    if (input->WasKeyPressed(VK_RIGHT))
    {
        float curTime  = replaySys->GetPlaybackTime();
        float duration = replaySys->GetDuration();
        float seekTo   = std::min(duration, curTime + 2.0f);
        replaySys->Seek(seekTo);
    }

    // --- +/-: Playback speed ------------------------------------------------
    if (input->WasKeyPressed(VK_OEM_PLUS) || input->WasKeyPressed(VK_ADD))
    {
        if (m_speedIndex < 4)
        {
            m_speedIndex++;
            m_playbackSpeed = kSpeedOptions[m_speedIndex];
            replaySys->SetPlaybackSpeed(m_playbackSpeed);
        }
    }
    if (input->WasKeyPressed(VK_OEM_MINUS) || input->WasKeyPressed(VK_SUBTRACT))
    {
        if (m_speedIndex > 0)
        {
            m_speedIndex--;
            m_playbackSpeed = kSpeedOptions[m_speedIndex];
            replaySys->SetPlaybackSpeed(m_playbackSpeed);
        }
    }

    // --- K: Kill cam --------------------------------------------------------
    if (input->WasKeyPressed('K'))
        TriggerKillCam();

    // --- F8: Return to live mode --------------------------------------------
    if (input->WasKeyPressed(VK_F8))
        StopPlaybackAndRecord();

    // --- F6: Save current replay --------------------------------------------
    if (input->WasKeyPressed(VK_F6))
        replaySys->SaveToFile("replay.dat");
}

// ---------------------------------------------------------------------------
// Shooting
// ---------------------------------------------------------------------------

void ReplayDemoModule::ShootRaycast()
{
    auto* world = m_ctx->GetWorld();
    if (!world) return;

    float yawRad   = m_camYaw * (3.14159f / 180.0f);
    float pitchRad = m_camPitch * (3.14159f / 180.0f);

    float dirX = std::sin(yawRad) * std::cos(pitchRad);
    float dirY = std::sin(pitchRad);
    float dirZ = std::cos(yawRad) * std::cos(pitchRad);

    float closestDist = 1000.0f;
    int   closestIdx  = -1;

    for (int i = 0; i < static_cast<int>(m_targets.size()); ++i)
    {
        auto& t = m_targets[i];
        if (!t.alive || t.entity == entt::null) continue;

        auto& xf = world->GetComponent<Transform>(t.entity);
        float toX = xf.position.x - m_playerX;
        float toY = xf.position.y - m_playerY;
        float toZ = xf.position.z - m_playerZ;
        float dist = std::sqrt(toX * toX + toY * toY + toZ * toZ);
        if (dist < 0.5f) continue;

        float invDist = 1.0f / dist;
        float dot = dirX * toX * invDist + dirY * toY * invDist + dirZ * toZ * invDist;
        float angle = std::acos(std::clamp(dot, -1.0f, 1.0f));

        float hitAngle = 1.0f / dist; // approximate target angular size
        if (angle < hitAngle && dist < closestDist)
        {
            closestDist = dist;
            closestIdx  = i;
        }
    }

    if (closestIdx >= 0)
    {
        auto& t = m_targets[closestIdx];
        t.alive = false;
        m_killCount++;

        // Record kill for kill cam
        auto& xf = world->GetComponent<Transform>(t.entity);
        KillRecord kr;
        kr.time    = m_recordTime;
        kr.victimX = xf.position.x;
        kr.victimY = xf.position.y;
        kr.victimZ = xf.position.z;
        kr.playerX = m_playerX;
        kr.playerY = m_playerY;
        kr.playerZ = m_playerZ;
        kr.playerYaw   = m_camYaw;
        kr.playerPitch = m_camPitch;
        m_kills.push_back(kr);

        // Visual: hide target
        auto& mr = world->GetComponent<MeshRenderer>(t.entity);
        mr.visible = false;
        xf.scale = { 0.0f, 0.0f, 0.0f };
    }
}

// ---------------------------------------------------------------------------
// Playback control
// ---------------------------------------------------------------------------

void ReplayDemoModule::StartPlayback()
{
    auto* replaySys = ReplaySystem::GetInstance();
    if (!replaySys) return;

    if (m_isRecording)
    {
        replaySys->StopRecording();
        m_isRecording = false;
    }

    replaySys->StartPlayback();
    m_isPlaying     = true;
    m_isPaused      = false;
    m_speedIndex    = 2;
    m_playbackSpeed = 1.0f;
    replaySys->SetPlaybackSpeed(m_playbackSpeed);

    auto* bus = m_ctx->GetEventBus();
    if (bus)
        bus->Publish(ReplayStartedEvent{ false });
}

void ReplayDemoModule::StopPlaybackAndRecord()
{
    auto* replaySys = ReplaySystem::GetInstance();
    if (!replaySys) return;

    if (m_isPlaying)
    {
        replaySys->StopPlayback();
        m_isPlaying = false;
        m_isPaused  = false;
    }

    if (m_inKillCam)
    {
        m_inKillCam = false;
    }

    // Reset and start new recording
    ResetTargets();
    m_playerX = 0.0f;
    m_playerY = 1.7f;
    m_playerZ = 0.0f;
    m_camYaw  = 0.0f;
    m_camPitch = 0.0f;
    m_recordTime = 0.0f;

    replaySys->StartRecording();
    m_isRecording = true;
}

// ---------------------------------------------------------------------------
// Kill cam
// ---------------------------------------------------------------------------

void ReplayDemoModule::TriggerKillCam()
{
    if (m_kills.empty()) return;

    auto* replaySys = ReplaySystem::GetInstance();
    if (!replaySys) return;

    // Use the last kill
    m_killCamTarget = m_kills.back();
    m_inKillCam     = true;
    m_killCamTimer  = 0.0f;

    // Seek replay to slightly before the kill
    float seekTime = std::max(0.0f, m_killCamTarget.time - 1.0f);
    replaySys->Seek(seekTime);
    replaySys->SetPlaybackSpeed(0.25f); // slow motion
    replaySys->StartKillCam(m_killCamTarget.victimX,
                            m_killCamTarget.victimY,
                            m_killCamTarget.victimZ);

    if (m_isPaused)
    {
        replaySys->Resume();
        m_isPaused = false;
    }
}

void ReplayDemoModule::UpdateKillCam(float dt)
{
    m_killCamTimer += dt;

    auto* world = m_ctx->GetWorld();
    auto* replaySys = ReplaySystem::GetInstance();

    // Orbit camera around the kill position
    if (world && m_camera != entt::null)
    {
        float orbitAngle = m_killCamTimer * 60.0f; // degrees per second
        float orbitDist  = 6.0f;
        float orbitHeight = 3.0f;

        float rad = orbitAngle * (3.14159f / 180.0f);

        auto& camXf = world->GetComponent<Transform>(m_camera);
        camXf.position = {
            m_killCamTarget.victimX + std::sin(rad) * orbitDist,
            m_killCamTarget.victimY + orbitHeight,
            m_killCamTarget.victimZ + std::cos(rad) * orbitDist
        };

        // Look at victim
        float lookX = m_killCamTarget.victimX - camXf.position.x;
        float lookY = m_killCamTarget.victimY - camXf.position.y;
        float lookZ = m_killCamTarget.victimZ - camXf.position.z;

        float yaw   = std::atan2(lookX, lookZ) * (180.0f / 3.14159f);
        float hDist = std::sqrt(lookX * lookX + lookZ * lookZ);
        float pitch = std::atan2(lookY, hDist) * (180.0f / 3.14159f);

        camXf.rotation = { pitch, yaw, 0.0f };
    }

    // End kill cam after duration
    if (m_killCamTimer >= m_killCamDuration)
    {
        m_inKillCam = false;

        if (replaySys)
        {
            replaySys->SetPlaybackSpeed(m_playbackSpeed);
        }
    }
}

// ---------------------------------------------------------------------------
// Render
// ---------------------------------------------------------------------------

void ReplayDemoModule::OnRender()
{
    if (!m_inKillCam && !m_isPlaying)
        DrawCrosshair();

    DrawStatusOverlay();

    if (m_isPlaying || m_inKillCam)
        DrawTimeline();

    if (m_inKillCam)
        DrawKillCamOverlay();
}

// ---------------------------------------------------------------------------
// Crosshair
// ---------------------------------------------------------------------------

void ReplayDemoModule::DrawCrosshair()
{
    auto* dd = Spark::DebugDraw::GetInstance();
    if (!dd) return;

    float cx = m_screenW * 0.5f;
    float cy = m_screenH * 0.5f;
    float size = 10.0f;
    float gap = 3.0f;

    dd->ScreenLine({ cx, cy - size }, { cx, cy - gap }, { 1.0f, 1.0f, 1.0f, 0.8f });
    dd->ScreenLine({ cx, cy + gap }, { cx, cy + size }, { 1.0f, 1.0f, 1.0f, 0.8f });
    dd->ScreenLine({ cx - size, cy }, { cx - gap, cy }, { 1.0f, 1.0f, 1.0f, 0.8f });
    dd->ScreenLine({ cx + gap, cy }, { cx + size, cy }, { 1.0f, 1.0f, 1.0f, 0.8f });

    dd->ScreenRect({ cx - 1.0f, cy - 1.0f }, { cx + 1.0f, cy + 1.0f },
                    { 1.0f, 1.0f, 1.0f, 1.0f });
}

// ---------------------------------------------------------------------------
// Status overlay
// ---------------------------------------------------------------------------

void ReplayDemoModule::DrawStatusOverlay()
{
    auto* dd = Spark::DebugDraw::GetInstance();
    if (!dd) return;

    auto* replaySys = ReplaySystem::GetInstance();

    // Mode indicator (top-left)
    if (m_isRecording)
    {
        dd->ScreenRect({ 15.0f, 8.0f }, { 28.0f, 21.0f },
                        { 1.0f, 0.0f, 0.0f, 1.0f }); // red dot
        dd->ScreenText({ 35.0f, 10.0f }, "RECORDING",
                        { 1.0f, 0.3f, 0.3f, 1.0f });
    }
    else if (m_isPlaying)
    {
        const char* playStr = m_isPaused ? "PAUSED" : "PLAYING";
        float pr = m_isPaused ? 1.0f : 0.3f;
        float pg = m_isPaused ? 0.8f : 1.0f;
        float pb = m_isPaused ? 0.0f : 0.3f;

        dd->ScreenText({ 20.0f, 10.0f }, playStr, { pr, pg, pb, 1.0f });
    }

    // Time display
    char buf[64];
    if (m_isRecording)
    {
        int sec = static_cast<int>(m_recordTime);
        int min = sec / 60;
        sec %= 60;
        std::snprintf(buf, sizeof(buf), "Time: %02d:%02d", min, sec);
        dd->ScreenText({ 20.0f, 35.0f }, buf, { 0.8f, 0.8f, 0.8f, 0.9f });
    }
    else if (m_isPlaying && replaySys)
    {
        float cur = replaySys->GetPlaybackTime();
        float dur = replaySys->GetDuration();
        std::snprintf(buf, sizeof(buf), "%.1f / %.1f s", cur, dur);
        dd->ScreenText({ 20.0f, 35.0f }, buf, { 0.8f, 0.8f, 0.8f, 0.9f });
    }

    // Speed display (during playback)
    if (m_isPlaying)
    {
        std::snprintf(buf, sizeof(buf), "Speed: %.2fx", m_playbackSpeed);
        dd->ScreenText({ 20.0f, 55.0f }, buf, { 0.6f, 0.8f, 1.0f, 0.9f });
    }

    // Kill count
    std::snprintf(buf, sizeof(buf), "Kills: %d / %d",
                  m_killCount, static_cast<int>(m_targets.size()));
    dd->ScreenText({ m_screenW - 160.0f, 10.0f }, buf,
                    { 1.0f, 0.8f, 0.3f, 1.0f });

    // Controls help (bottom-left)
    float helpY = m_screenH - 25.0f;
    if (m_isRecording)
    {
        dd->ScreenText({ 20.0f, helpY },
                        "WASD: Move  LMB: Shoot  F5: Playback  F6: Save  F7: Load",
                        { 0.5f, 0.5f, 0.5f, 0.7f });
    }
    else if (m_isPlaying)
    {
        dd->ScreenText({ 20.0f, helpY },
                        "Space: Pause  Arrows: Seek  +/-: Speed  K: Kill Cam  F6: Save  F8: Live",
                        { 0.5f, 0.5f, 0.5f, 0.7f });
    }
}

// ---------------------------------------------------------------------------
// Timeline bar (during playback)
// ---------------------------------------------------------------------------

void ReplayDemoModule::DrawTimeline()
{
    auto* dd = Spark::DebugDraw::GetInstance();
    if (!dd) return;

    auto* replaySys = ReplaySystem::GetInstance();
    if (!replaySys) return;

    float barX = m_screenW * 0.1f;
    float barY = m_screenH - 60.0f;
    float barW = m_screenW * 0.8f;
    float barH = 12.0f;

    // Background
    dd->ScreenRect({ barX, barY }, { barX + barW, barY + barH },
                    { 0.15f, 0.15f, 0.15f, 0.7f });

    // Progress fill
    float duration = replaySys->GetDuration();
    float current  = replaySys->GetPlaybackTime();
    float progress = (duration > 0.0f) ? (current / duration) : 0.0f;
    float fillW    = barW * progress;

    dd->ScreenRect({ barX, barY }, { barX + fillW, barY + barH },
                    { 0.3f, 0.6f, 1.0f, 0.8f });

    // Playhead indicator
    float headX = barX + fillW;
    dd->ScreenRect({ headX - 2.0f, barY - 3.0f },
                    { headX + 2.0f, barY + barH + 3.0f },
                    { 1.0f, 1.0f, 1.0f, 1.0f });

    // Kill markers on timeline
    for (auto& kr : m_kills)
    {
        if (duration <= 0.0f) break;
        float killPos = barX + (kr.time / duration) * barW;
        dd->ScreenRect({ killPos - 1.0f, barY - 2.0f },
                        { killPos + 1.0f, barY + barH + 2.0f },
                        { 1.0f, 0.2f, 0.2f, 0.9f });
    }

    // Border
    dd->ScreenLine({ barX, barY }, { barX + barW, barY }, { 0.4f, 0.4f, 0.4f, 0.8f });
    dd->ScreenLine({ barX + barW, barY }, { barX + barW, barY + barH }, { 0.4f, 0.4f, 0.4f, 0.8f });
    dd->ScreenLine({ barX + barW, barY + barH }, { barX, barY + barH }, { 0.4f, 0.4f, 0.4f, 0.8f });
    dd->ScreenLine({ barX, barY + barH }, { barX, barY }, { 0.4f, 0.4f, 0.4f, 0.8f });
}

// ---------------------------------------------------------------------------
// Kill cam overlay
// ---------------------------------------------------------------------------

void ReplayDemoModule::DrawKillCamOverlay()
{
    auto* dd = Spark::DebugDraw::GetInstance();
    if (!dd) return;

    // Cinematic bars (top and bottom)
    float barH = 60.0f;
    dd->ScreenRect({ 0.0f, 0.0f }, { m_screenW, barH },
                    { 0.0f, 0.0f, 0.0f, 0.9f });
    dd->ScreenRect({ 0.0f, m_screenH - barH }, { m_screenW, m_screenH },
                    { 0.0f, 0.0f, 0.0f, 0.9f });

    // "KILL CAM" text
    dd->ScreenText({ m_screenW * 0.5f - 40.0f, barH - 25.0f },
                    "KILL CAM", { 1.0f, 0.2f, 0.2f, 1.0f });

    // Speed indicator
    dd->ScreenText({ m_screenW * 0.5f - 20.0f, m_screenH - barH + 8.0f },
                    "0.25x", { 0.8f, 0.8f, 0.8f, 0.8f });
}

// ---------------------------------------------------------------------------

void ReplayDemoModule::OnResize(int w, int h)
{
    m_screenW = static_cast<float>(w);
    m_screenH = static_cast<float>(h);
}

// ---------------------------------------------------------------------------
// DLL exports
// ---------------------------------------------------------------------------

SPARK_IMPLEMENT_MODULE(ReplayDemoModule)
