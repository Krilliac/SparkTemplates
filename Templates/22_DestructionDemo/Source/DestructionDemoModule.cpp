/**
 * @file DestructionDemoModule.cpp
 * @brief ADVANCED — Destruction system showcase.
 */

#include "DestructionDemoModule.h"
#include <Input/InputManager.h>
#include <cmath>
#include <algorithm>
#include <cstdio>

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void DestructionDemoModule::OnLoad(Spark::IEngineContext* ctx)
{
    m_ctx = ctx;

    // Subscribe to destruction events
    auto* bus = m_ctx->GetEventBus();
    if (bus)
    {
        m_destructionSubId = bus->Subscribe<DestructionEvent>(
            [this](const DestructionEvent& evt)
            {
                m_destroyedCount++;
            });

        m_damageSubId = bus->Subscribe<DamageAppliedEvent>(
            [this](const DamageAppliedEvent& evt)
            {
                m_totalDamageApplied += static_cast<int>(evt.damageAmount);
            });
    }

    // Register debris callback with engine destruction system
    auto* destructionSys = DestructionSystem::GetInstance();
    if (destructionSys)
    {
        destructionSys->SetDebrisCallback(
            [](entt::entity debris, float lifetime)
            {
                // Engine handles basic debris physics; we track lifetime ourselves
            });
    }

    BuildScene();
}

void DestructionDemoModule::OnUnload()
{
    auto* bus = m_ctx ? m_ctx->GetEventBus() : nullptr;
    if (bus)
    {
        bus->Unsubscribe<DestructionEvent>(m_destructionSubId);
        bus->Unsubscribe<DamageAppliedEvent>(m_damageSubId);
    }

    auto* world = m_ctx ? m_ctx->GetWorld() : nullptr;
    if (!world) { m_ctx = nullptr; return; }

    for (auto e : { m_player, m_camera, m_light })
        if (e != entt::null) world->DestroyEntity(e);

    for (auto& di : m_destructibles)
        if (di.entity != entt::null) world->DestroyEntity(di.entity);

    for (auto& db : m_debris)
        if (db.entity != entt::null) world->DestroyEntity(db.entity);

    for (auto e : m_sceneEntities)
        if (e != entt::null) world->DestroyEntity(e);

    m_destructibles.clear();
    m_debris.clear();
    m_sceneEntities.clear();
    m_ctx = nullptr;
}

// ---------------------------------------------------------------------------
// Scene construction
// ---------------------------------------------------------------------------

void DestructionDemoModule::BuildScene()
{
    auto* world = m_ctx->GetWorld();

    // --- Player entity ------------------------------------------------------
    m_player = world->CreateEntity();
    world->AddComponent<NameComponent>(m_player, NameComponent{ "Player" });
    world->AddComponent<Transform>(m_player, Transform{
        { m_playerX, m_playerY, m_playerZ },
        { 0.0f, 0.0f, 0.0f },
        { 1.0f, 1.0f, 1.0f }
    });

    // --- Camera -------------------------------------------------------------
    m_camera = world->CreateEntity();
    world->AddComponent<NameComponent>(m_camera, NameComponent{ "MainCamera" });
    world->AddComponent<Transform>(m_camera, Transform{
        { m_playerX, m_playerY, m_playerZ },
        { m_camPitch, m_camYaw, 0.0f },
        { 1.0f, 1.0f, 1.0f }
    });
    world->AddComponent<Camera>(m_camera, Camera{
        65.0f, 0.1f, 500.0f, true
    });

    // --- Light --------------------------------------------------------------
    m_light = world->CreateEntity();
    world->AddComponent<NameComponent>(m_light, NameComponent{ "SunLight" });
    world->AddComponent<Transform>(m_light, Transform{
        { 0.0f, 30.0f, 0.0f },
        { -45.0f, 20.0f, 0.0f },
        { 1.0f, 1.0f, 1.0f }
    });
    world->AddComponent<LightComponent>(m_light, LightComponent{
        LightComponent::Type::Directional,
        { 1.0f, 0.95f, 0.85f }, 2.2f, 0.0f,
        0.0f, 0.0f, true, 2048
    });

    // --- Ground -------------------------------------------------------------
    auto ground = world->CreateEntity();
    world->AddComponent<NameComponent>(ground, NameComponent{ "Ground" });
    world->AddComponent<Transform>(ground, Transform{
        { 0.0f, -0.1f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 60.0f, 0.2f, 60.0f }
    });
    world->AddComponent<MeshRenderer>(ground, MeshRenderer{
        "Assets/Models/cube.obj", "Assets/Materials/concrete.mat",
        false, true, true
    });
    m_sceneEntities.push_back(ground);

    // --- Destructible objects -----------------------------------------------

    // Walls (Voronoi fracture)
    SpawnDestructible("Wall_Left",  -8.0f, 2.5f, 5.0f, 6.0f, 5.0f, 0.8f,
                      FracturePattern::Voronoi, 100.0f, "Assets/Materials/brick.mat");
    SpawnDestructible("Wall_Right",  8.0f, 2.5f, 5.0f, 6.0f, 5.0f, 0.8f,
                      FracturePattern::Voronoi, 100.0f, "Assets/Materials/brick.mat");
    SpawnDestructible("Wall_Center", 0.0f, 2.5f, 12.0f, 8.0f, 5.0f, 0.8f,
                      FracturePattern::Voronoi, 150.0f, "Assets/Materials/brick.mat");

    // Pillars (Radial fracture)
    SpawnDestructible("Pillar_A", -4.0f, 3.0f, 0.0f, 1.2f, 6.0f, 1.2f,
                      FracturePattern::Radial, 120.0f, "Assets/Materials/stone.mat");
    SpawnDestructible("Pillar_B",  4.0f, 3.0f, 0.0f, 1.2f, 6.0f, 1.2f,
                      FracturePattern::Radial, 120.0f, "Assets/Materials/stone.mat");

    // Floor tiles (Slice fracture)
    for (int i = 0; i < 4; ++i)
    {
        float tx = -6.0f + static_cast<float>(i) * 4.0f;
        char name[32];
        std::snprintf(name, sizeof(name), "FloorTile_%d", i);
        SpawnDestructible(name, tx, 0.15f, 8.0f, 3.5f, 0.3f, 3.5f,
                          FracturePattern::Slice, 60.0f, "Assets/Materials/tile.mat");
    }

    // Crates (Uniform fracture)
    SpawnDestructible("Crate_A", -3.0f, 1.0f, -3.0f, 2.0f, 2.0f, 2.0f,
                      FracturePattern::Uniform, 50.0f, "Assets/Materials/wood.mat");
    SpawnDestructible("Crate_B",  3.0f, 1.0f, -3.0f, 2.0f, 2.0f, 2.0f,
                      FracturePattern::Uniform, 50.0f, "Assets/Materials/wood.mat");
    SpawnDestructible("Crate_C",  0.0f, 1.0f, -5.0f, 1.5f, 1.5f, 1.5f,
                      FracturePattern::Uniform, 40.0f, "Assets/Materials/wood.mat");
    SpawnDestructible("Crate_D",  6.0f, 0.75f, 3.0f, 1.5f, 1.5f, 1.5f,
                      FracturePattern::Uniform, 40.0f, "Assets/Materials/wood.mat");
}

void DestructionDemoModule::SpawnDestructible(const std::string& name,
                                              float x, float y, float z,
                                              float sx, float sy, float sz,
                                              FracturePattern pattern, float health,
                                              const char* material)
{
    auto* world = m_ctx->GetWorld();

    auto e = world->CreateEntity();
    world->AddComponent<NameComponent>(e, NameComponent{ name.c_str() });
    world->AddComponent<Transform>(e, Transform{
        { x, y, z }, { 0.0f, 0.0f, 0.0f }, { sx, sy, sz }
    });
    world->AddComponent<MeshRenderer>(e, MeshRenderer{
        "Assets/Models/cube.obj", material,
        true, true, true
    });
    world->AddComponent<ColliderComponent>(e, ColliderComponent{
        ColliderComponent::Shape::Box, { sx, sy, sz }, false
    });

    // Register with destruction system
    DestructibleComponent dc;
    dc.maxHealth      = health;
    dc.fracturePattern = pattern;
    dc.debrisLifetime = 4.0f;
    dc.debrisPhysics  = true;
    dc.damageStages   = {
        { 0.75f, "crack" },
        { 0.40f, "partial_destroy" },
        { 0.0f,  "full_destroy" }
    };

    world->AddComponent<DestructibleComponent>(e, dc);

    auto* destructionSys = DestructionSystem::GetInstance();
    if (destructionSys)
        destructionSys->RegisterDestructible(e);

    DestructibleInfo info;
    info.entity       = e;
    info.name         = name;
    info.maxHealth    = health;
    info.currentHealth = health;
    info.pattern      = pattern;
    info.currentStage = 0;
    info.origX = x;  info.origY = y;  info.origZ = z;
    info.origScaleX = sx; info.origScaleY = sy; info.origScaleZ = sz;
    info.destroyed    = false;

    m_destructibles.push_back(info);
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

void DestructionDemoModule::OnUpdate(float dt)
{
    HandleInput(dt);
    UpdateDebris(dt);

    // Update camera transform
    auto* world = m_ctx->GetWorld();
    if (world && m_camera != entt::null)
    {
        auto& camXf = world->GetComponent<Transform>(m_camera);
        camXf.position = { m_playerX, m_playerY, m_playerZ };
        camXf.rotation = { m_camPitch, m_camYaw, 0.0f };
    }

    if (world && m_player != entt::null)
    {
        auto& xf = world->GetComponent<Transform>(m_player);
        xf.position = { m_playerX, m_playerY, m_playerZ };
    }

    // --- Determine target under crosshair -----------------------------------
    m_targetIndex = -1;
    if (world)
    {
        float yawRad   = m_camYaw * (3.14159f / 180.0f);
        float pitchRad = m_camPitch * (3.14159f / 180.0f);

        float dirX = std::sin(yawRad) * std::cos(pitchRad);
        float dirY = std::sin(pitchRad);
        float dirZ = std::cos(yawRad) * std::cos(pitchRad);

        float closestDist = 1000.0f;

        for (int i = 0; i < static_cast<int>(m_destructibles.size()); ++i)
        {
            auto& info = m_destructibles[i];
            if (info.destroyed || info.entity == entt::null) continue;

            auto& tXf = world->GetComponent<Transform>(info.entity);
            float toX = tXf.position.x - m_playerX;
            float toY = tXf.position.y - m_playerY;
            float toZ = tXf.position.z - m_playerZ;
            float dist = std::sqrt(toX * toX + toY * toY + toZ * toZ);
            if (dist < 0.5f) continue;

            float invDist = 1.0f / dist;
            float dot = dirX * toX * invDist + dirY * toY * invDist + dirZ * toZ * invDist;
            float angle = std::acos(std::clamp(dot, -1.0f, 1.0f));

            float maxDim = std::max({ tXf.scale.x, tXf.scale.y, tXf.scale.z });
            float hitAngle = (maxDim * 0.5f) / dist;

            if (angle < hitAngle && dist < closestDist)
            {
                closestDist = dist;
                m_targetIndex = i;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Input handling
// ---------------------------------------------------------------------------

void DestructionDemoModule::HandleInput(float dt)
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

    float speed = 8.0f;
    float yawRad = m_camYaw * (3.14159f / 180.0f);

    float forwardX = std::sin(yawRad);
    float forwardZ = std::cos(yawRad);
    float rightX   = std::cos(yawRad);
    float rightZ   = -std::sin(yawRad);

    m_playerX += (forwardX * moveZ + rightX * moveX) * speed * dt;
    m_playerZ += (forwardZ * moveZ + rightZ * moveX) * speed * dt;

    // --- LMB: Apply damage at crosshair -------------------------------------
    if (input->WasMouseButtonPressed(0))
        ApplyDamageAtCrosshair(25.0f);

    // --- Space: Explosion at player position --------------------------------
    if (input->WasKeyPressed(VK_SPACE))
        ApplyExplosionAtPlayer(12.0f, 60.0f);

    // --- R: Reset all destructibles -----------------------------------------
    if (input->WasKeyPressed('R'))
        ResetAllDestructibles();
}

// ---------------------------------------------------------------------------
// Damage application
// ---------------------------------------------------------------------------

void DestructionDemoModule::ApplyDamageAtCrosshair(float amount)
{
    if (m_targetIndex < 0 || m_targetIndex >= static_cast<int>(m_destructibles.size()))
        return;

    auto& info = m_destructibles[m_targetIndex];
    if (info.destroyed) return;

    info.currentHealth -= amount;
    if (info.currentHealth < 0.0f)
        info.currentHealth = 0.0f;

    // Publish damage event
    auto* bus = m_ctx->GetEventBus();
    if (bus)
        bus->Publish(DamageAppliedEvent{ info.entity, amount, info.currentHealth });

    // Also notify engine destruction system
    auto* destructionSys = DestructionSystem::GetInstance();
    if (destructionSys)
        destructionSys->ApplyDamage(info.entity, amount);

    // Check for stage advancement
    AdvanceDamageStage(info);
}

void DestructionDemoModule::ApplyExplosionAtPlayer(float radius, float damage)
{
    auto* world = m_ctx->GetWorld();
    if (!world) return;

    // Notify engine destruction system
    auto* destructionSys = DestructionSystem::GetInstance();
    if (destructionSys)
        destructionSys->ApplyExplosion({ m_playerX, m_playerY, m_playerZ }, radius, damage);

    for (auto& info : m_destructibles)
    {
        if (info.destroyed || info.entity == entt::null) continue;

        auto& xf = world->GetComponent<Transform>(info.entity);
        float dx = xf.position.x - m_playerX;
        float dy = xf.position.y - m_playerY;
        float dz = xf.position.z - m_playerZ;
        float dist = std::sqrt(dx * dx + dy * dy + dz * dz);

        if (dist < radius)
        {
            float falloff = 1.0f - (dist / radius);
            float actualDamage = damage * falloff;

            info.currentHealth -= actualDamage;
            if (info.currentHealth < 0.0f)
                info.currentHealth = 0.0f;

            auto* bus = m_ctx->GetEventBus();
            if (bus)
                bus->Publish(DamageAppliedEvent{ info.entity, actualDamage, info.currentHealth });

            AdvanceDamageStage(info);
        }
    }
}

// ---------------------------------------------------------------------------
// Damage stages
// ---------------------------------------------------------------------------

void DestructionDemoModule::AdvanceDamageStage(DestructibleInfo& info)
{
    auto* world = m_ctx->GetWorld();
    if (!world || info.entity == entt::null) return;

    float healthRatio = info.currentHealth / info.maxHealth;
    int newStage = 0;

    if (healthRatio <= 0.0f)
        newStage = 3; // fully destroyed
    else if (healthRatio <= 0.40f)
        newStage = 2; // partially destroyed
    else if (healthRatio <= 0.75f)
        newStage = 1; // cracked

    if (newStage <= info.currentStage) return;

    info.currentStage = newStage;

    auto& xf = world->GetComponent<Transform>(info.entity);
    auto& mr = world->GetComponent<MeshRenderer>(info.entity);

    switch (newStage)
    {
    case 1: // Cracked — tint darker, slight scale reduction
        mr.material = "Assets/Materials/cracked.mat";
        xf.scale.x *= 0.98f;
        xf.scale.y *= 0.98f;
        xf.scale.z *= 0.98f;
        break;

    case 2: // Partially destroyed — reduced scale, spawn some debris
        mr.material = "Assets/Materials/damaged.mat";
        xf.scale.x *= 0.85f;
        xf.scale.y *= 0.70f;
        xf.scale.z *= 0.85f;
        SpawnDebris(info, 4);
        break;

    case 3: // Fully destroyed — hide entity, spawn lots of debris
    {
        SpawnDebris(info, 8);

        // Mark as destroyed and hide
        info.destroyed = true;
        mr.visible = false;
        xf.scale = { 0.0f, 0.0f, 0.0f };

        auto* bus = m_ctx->GetEventBus();
        if (bus)
            bus->Publish(DestructionEvent{ info.entity, info.name, 3 });
        break;
    }
    }
}

// ---------------------------------------------------------------------------
// Debris spawning and management
// ---------------------------------------------------------------------------

void DestructionDemoModule::SpawnDebris(const DestructibleInfo& info, int count)
{
    auto* world = m_ctx->GetWorld();
    if (!world || info.entity == entt::null) return;

    auto& srcXf = world->GetComponent<Transform>(info.entity);

    for (int i = 0; i < count; ++i)
    {
        auto debris = world->CreateEntity();

        // Pseudo-random offset based on index
        float angle = static_cast<float>(i) * (6.28318f / static_cast<float>(count));
        float offsetX = std::cos(angle) * (info.origScaleX * 0.4f);
        float offsetZ = std::sin(angle) * (info.origScaleZ * 0.4f);
        float offsetY = static_cast<float>(i % 3) * 0.3f;

        float debrisScale = std::max(0.2f, std::min({ info.origScaleX, info.origScaleY, info.origScaleZ }) * 0.25f);

        char name[32];
        std::snprintf(name, sizeof(name), "Debris_%d", static_cast<int>(m_debris.size()));
        world->AddComponent<NameComponent>(debris, NameComponent{ name });
        world->AddComponent<Transform>(debris, Transform{
            { srcXf.position.x + offsetX,
              srcXf.position.y + offsetY,
              srcXf.position.z + offsetZ },
            { static_cast<float>(i * 37 % 360),
              static_cast<float>(i * 73 % 360),
              static_cast<float>(i * 19 % 360) },
            { debrisScale, debrisScale, debrisScale }
        });
        world->AddComponent<MeshRenderer>(debris, MeshRenderer{
            "Assets/Models/cube.obj", "Assets/Materials/debris.mat",
            false, false, true
        });
        world->AddComponent<RigidBodyComponent>(debris, RigidBodyComponent{
            1.0f, 0.5f, 0.3f, false, false
        });
        world->AddComponent<ColliderComponent>(debris, ColliderComponent{
            ColliderComponent::Shape::Box,
            { debrisScale, debrisScale, debrisScale },
            false
        });

        DebrisInfo di;
        di.entity   = debris;
        di.lifetime = 4.0f;
        di.elapsed  = 0.0f;
        m_debris.push_back(di);
    }
}

void DestructionDemoModule::UpdateDebris(float dt)
{
    auto* world = m_ctx->GetWorld();
    if (!world) return;

    for (auto it = m_debris.begin(); it != m_debris.end(); )
    {
        it->elapsed += dt;

        if (it->elapsed >= it->lifetime)
        {
            if (it->entity != entt::null)
                world->DestroyEntity(it->entity);
            it = m_debris.erase(it);
        }
        else
        {
            // Fade debris by shrinking it in the last second
            float remaining = it->lifetime - it->elapsed;
            if (remaining < 1.0f && it->entity != entt::null)
            {
                auto& xf = world->GetComponent<Transform>(it->entity);
                float fade = remaining; // 1.0 → 0.0
                xf.scale.x *= (0.95f + 0.05f * fade);
                xf.scale.y *= (0.95f + 0.05f * fade);
                xf.scale.z *= (0.95f + 0.05f * fade);
            }
            ++it;
        }
    }
}

// ---------------------------------------------------------------------------
// Reset
// ---------------------------------------------------------------------------

void DestructionDemoModule::ResetAllDestructibles()
{
    auto* world = m_ctx->GetWorld();
    if (!world) return;

    // Destroy remaining debris
    for (auto& db : m_debris)
        if (db.entity != entt::null) world->DestroyEntity(db.entity);
    m_debris.clear();

    // Destroy old destructible entities
    for (auto& info : m_destructibles)
        if (info.entity != entt::null) world->DestroyEntity(info.entity);
    m_destructibles.clear();

    // Reset score
    m_destroyedCount = 0;
    m_totalDamageApplied = 0;
    m_targetIndex = -1;

    // Rebuild — reuses BuildScene's destructible spawn calls
    // Walls (Voronoi)
    SpawnDestructible("Wall_Left",  -8.0f, 2.5f, 5.0f, 6.0f, 5.0f, 0.8f,
                      FracturePattern::Voronoi, 100.0f, "Assets/Materials/brick.mat");
    SpawnDestructible("Wall_Right",  8.0f, 2.5f, 5.0f, 6.0f, 5.0f, 0.8f,
                      FracturePattern::Voronoi, 100.0f, "Assets/Materials/brick.mat");
    SpawnDestructible("Wall_Center", 0.0f, 2.5f, 12.0f, 8.0f, 5.0f, 0.8f,
                      FracturePattern::Voronoi, 150.0f, "Assets/Materials/brick.mat");

    // Pillars (Radial)
    SpawnDestructible("Pillar_A", -4.0f, 3.0f, 0.0f, 1.2f, 6.0f, 1.2f,
                      FracturePattern::Radial, 120.0f, "Assets/Materials/stone.mat");
    SpawnDestructible("Pillar_B",  4.0f, 3.0f, 0.0f, 1.2f, 6.0f, 1.2f,
                      FracturePattern::Radial, 120.0f, "Assets/Materials/stone.mat");

    // Floor tiles (Slice)
    for (int i = 0; i < 4; ++i)
    {
        float tx = -6.0f + static_cast<float>(i) * 4.0f;
        char name[32];
        std::snprintf(name, sizeof(name), "FloorTile_%d", i);
        SpawnDestructible(name, tx, 0.15f, 8.0f, 3.5f, 0.3f, 3.5f,
                          FracturePattern::Slice, 60.0f, "Assets/Materials/tile.mat");
    }

    // Crates (Uniform)
    SpawnDestructible("Crate_A", -3.0f, 1.0f, -3.0f, 2.0f, 2.0f, 2.0f,
                      FracturePattern::Uniform, 50.0f, "Assets/Materials/wood.mat");
    SpawnDestructible("Crate_B",  3.0f, 1.0f, -3.0f, 2.0f, 2.0f, 2.0f,
                      FracturePattern::Uniform, 50.0f, "Assets/Materials/wood.mat");
    SpawnDestructible("Crate_C",  0.0f, 1.0f, -5.0f, 1.5f, 1.5f, 1.5f,
                      FracturePattern::Uniform, 40.0f, "Assets/Materials/wood.mat");
    SpawnDestructible("Crate_D",  6.0f, 0.75f, 3.0f, 1.5f, 1.5f, 1.5f,
                      FracturePattern::Uniform, 40.0f, "Assets/Materials/wood.mat");
}

// ---------------------------------------------------------------------------
// Render
// ---------------------------------------------------------------------------

void DestructionDemoModule::OnRender()
{
    DrawCrosshair();
    DrawHUD();
    DrawTargetInfo();
}

// ---------------------------------------------------------------------------
// Crosshair
// ---------------------------------------------------------------------------

void DestructionDemoModule::DrawCrosshair()
{
    auto* dd = Spark::DebugDraw::GetInstance();
    if (!dd) return;

    float cx = m_screenW * 0.5f;
    float cy = m_screenH * 0.5f;
    float size = 12.0f;
    float gap = 4.0f;

    float r = (m_targetIndex >= 0) ? 1.0f : 1.0f;
    float g = (m_targetIndex >= 0) ? 0.3f : 1.0f;
    float b = (m_targetIndex >= 0) ? 0.3f : 1.0f;

    dd->ScreenLine({ cx, cy - size }, { cx, cy - gap }, { r, g, b, 0.9f });
    dd->ScreenLine({ cx, cy + gap }, { cx, cy + size }, { r, g, b, 0.9f });
    dd->ScreenLine({ cx - size, cy }, { cx - gap, cy }, { r, g, b, 0.9f });
    dd->ScreenLine({ cx + gap, cy }, { cx + size, cy }, { r, g, b, 0.9f });

    dd->ScreenRect({ cx - 1.0f, cy - 1.0f }, { cx + 1.0f, cy + 1.0f },
                    { r, g, b, 1.0f });
}

// ---------------------------------------------------------------------------
// HUD
// ---------------------------------------------------------------------------

void DestructionDemoModule::DrawHUD()
{
    auto* dd = Spark::DebugDraw::GetInstance();
    if (!dd) return;

    // Controls (top-left)
    dd->ScreenText({ 20.0f, 10.0f }, "LMB: Damage  SPACE: Explosion  R: Reset",
                    { 0.6f, 0.6f, 0.6f, 0.7f });

    // Score (top-left below controls)
    char buf[64];
    std::snprintf(buf, sizeof(buf), "Destroyed: %d", m_destroyedCount);
    dd->ScreenText({ 20.0f, 35.0f }, buf, { 1.0f, 0.8f, 0.3f, 1.0f });

    std::snprintf(buf, sizeof(buf), "Total Damage: %d", m_totalDamageApplied);
    dd->ScreenText({ 20.0f, 55.0f }, buf, { 0.8f, 0.8f, 0.8f, 0.9f });

    // Active debris count
    std::snprintf(buf, sizeof(buf), "Active Debris: %d", static_cast<int>(m_debris.size()));
    dd->ScreenText({ 20.0f, 75.0f }, buf, { 0.6f, 0.6f, 0.8f, 0.8f });
}

// ---------------------------------------------------------------------------
// Target info (bottom-centre, shows damage state of aimed object)
// ---------------------------------------------------------------------------

void DestructionDemoModule::DrawTargetInfo()
{
    auto* dd = Spark::DebugDraw::GetInstance();
    if (!dd) return;

    if (m_targetIndex < 0 || m_targetIndex >= static_cast<int>(m_destructibles.size()))
        return;

    auto& info = m_destructibles[m_targetIndex];
    if (info.destroyed) return;

    float cx = m_screenW * 0.5f;
    float y  = m_screenH - 130.0f;

    // Background
    dd->ScreenRect({ cx - 160.0f, y }, { cx + 160.0f, y + 80.0f },
                    { 0.0f, 0.0f, 0.0f, 0.6f });

    // Name
    dd->ScreenText({ cx - 145.0f, y + 5.0f }, info.name.c_str(),
                    { 1.0f, 1.0f, 1.0f, 1.0f });

    // Health bar
    float barX = cx - 145.0f;
    float barY = y + 28.0f;
    float barW = 290.0f;
    float barH = 14.0f;

    dd->ScreenRect({ barX, barY }, { barX + barW, barY + barH },
                    { 0.15f, 0.15f, 0.15f, 0.8f });

    float healthRatio = info.currentHealth / info.maxHealth;
    float fillW = barW * healthRatio;

    float hr = (healthRatio < 0.5f) ? 1.0f : 1.0f - (healthRatio - 0.5f) * 2.0f;
    float hg = (healthRatio > 0.5f) ? 1.0f : healthRatio * 2.0f;

    dd->ScreenRect({ barX, barY }, { barX + fillW, barY + barH },
                    { hr, hg, 0.0f, 0.9f });

    // HP text
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.0f / %.0f", info.currentHealth, info.maxHealth);
    dd->ScreenText({ barX + 4.0f, barY + 1.0f }, buf,
                    { 1.0f, 1.0f, 1.0f, 1.0f });

    // Damage stage
    const char* stages[] = { "Clean", "Cracked", "Partially Destroyed", "Destroyed" };
    std::snprintf(buf, sizeof(buf), "Stage: %s", stages[info.currentStage]);
    dd->ScreenText({ cx - 145.0f, y + 50.0f }, buf,
                    { 0.8f, 0.8f, 0.5f, 1.0f });

    // Fracture pattern
    const char* patterns[] = { "Voronoi", "Radial", "Slice", "Uniform" };
    int patIdx = static_cast<int>(info.pattern);
    if (patIdx >= 0 && patIdx < 4)
    {
        std::snprintf(buf, sizeof(buf), "Fracture: %s", patterns[patIdx]);
        dd->ScreenText({ cx + 20.0f, y + 50.0f }, buf,
                        { 0.5f, 0.7f, 0.8f, 0.9f });
    }
}

// ---------------------------------------------------------------------------

void DestructionDemoModule::OnResize(int w, int h)
{
    m_screenW = static_cast<float>(w);
    m_screenH = static_cast<float>(h);
}

// ---------------------------------------------------------------------------
// DLL exports
// ---------------------------------------------------------------------------

SPARK_IMPLEMENT_MODULE(DestructionDemoModule)
