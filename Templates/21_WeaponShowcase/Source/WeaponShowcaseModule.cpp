/**
 * @file WeaponShowcaseModule.cpp
 * @brief INTERMEDIATE — FPS weapon system showcase.
 */

#include "WeaponShowcaseModule.h"
#include <Input/InputManager.h>
#include <cmath>
#include <algorithm>
#include <cstdio>

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void WeaponShowcaseModule::OnLoad(Spark::IEngineContext* ctx)
{
    m_ctx = ctx;

    RegisterWeapons();
    BuildScene();

    // Subscribe to weapon events for audio feedback
    auto* bus = m_ctx->GetEventBus();
    if (bus)
    {
        m_firedSubId = bus->Subscribe<WeaponFiredEvent>(
            [this](const WeaponFiredEvent& evt)
            {
                auto* audio = m_ctx->GetAudioEngine();
                if (audio)
                    audio->PlayOneShot("Assets/Audio/weapon_fire.wav", 0.8f);
            });

        m_reloadStartSubId = bus->Subscribe<ReloadStartedEvent>(
            [this](const ReloadStartedEvent& evt)
            {
                auto* audio = m_ctx->GetAudioEngine();
                if (audio)
                    audio->PlayOneShot("Assets/Audio/reload_start.wav", 0.6f);
            });

        m_reloadEndSubId = bus->Subscribe<ReloadFinishedEvent>(
            [this](const ReloadFinishedEvent& evt)
            {
                auto* audio = m_ctx->GetAudioEngine();
                if (audio)
                    audio->PlayOneShot("Assets/Audio/reload_end.wav", 0.6f);
            });
    }
}

void WeaponShowcaseModule::OnUnload()
{
    auto* bus = m_ctx ? m_ctx->GetEventBus() : nullptr;
    if (bus)
    {
        bus->Unsubscribe<WeaponFiredEvent>(m_firedSubId);
        bus->Unsubscribe<ReloadStartedEvent>(m_reloadStartSubId);
        bus->Unsubscribe<ReloadFinishedEvent>(m_reloadEndSubId);
    }

    auto* world = m_ctx ? m_ctx->GetWorld() : nullptr;
    if (!world) { m_ctx = nullptr; return; }

    for (auto e : { m_player, m_camera, m_light, m_muzzleLight })
        if (e != entt::null) world->DestroyEntity(e);

    for (auto e : m_targets)
        if (e != entt::null) world->DestroyEntity(e);
    for (auto e : m_sceneEntities)
        if (e != entt::null) world->DestroyEntity(e);

    m_targets.clear();
    m_sceneEntities.clear();
    m_ctx = nullptr;
}

// ---------------------------------------------------------------------------
// Weapon registration
// ---------------------------------------------------------------------------

void WeaponShowcaseModule::RegisterWeapons()
{
    // Pistol — Semi, slow fire, high accuracy
    m_weapons[0] = {
        "Pistol",
        FireMode::Semi,
        /* fireRate */ 3.0f,
        /* damage */ 35.0f,
        /* magazineSize */ 12,
        /* reloadTime */ 1.5f,
        /* recoilAmount */ 2.5f,
        /* spreadAngle */ 1.0f,
        /* adsZoomFOV */ 55.0f
    };

    // Rifle — FullAuto, fast fire, moderate spread
    m_weapons[1] = {
        "Rifle",
        FireMode::FullAuto,
        /* fireRate */ 10.0f,
        /* damage */ 20.0f,
        /* magazineSize */ 30,
        /* reloadTime */ 2.2f,
        /* recoilAmount */ 1.2f,
        /* spreadAngle */ 3.5f,
        /* adsZoomFOV */ 45.0f
    };

    // Shotgun — Semi, wide spread
    m_weapons[2] = {
        "Shotgun",
        FireMode::Semi,
        /* fireRate */ 1.2f,
        /* damage */ 15.0f, // per pellet
        /* magazineSize */ 8,
        /* reloadTime */ 2.8f,
        /* recoilAmount */ 5.0f,
        /* spreadAngle */ 12.0f,
        /* adsZoomFOV */ 60.0f
    };

    // Register with engine WeaponRegistry
    auto* weaponSys = m_ctx->GetWeaponSystem();
    auto* registry  = m_ctx->GetWeaponRegistry();
    if (registry)
    {
        for (int i = 0; i < 3; ++i)
        {
            WeaponDefinition def;
            def.name         = m_weapons[i].name;
            def.fireMode     = m_weapons[i].fireMode;
            def.fireRate     = m_weapons[i].fireRate;
            def.damage       = m_weapons[i].damage;
            def.magazineSize = m_weapons[i].magazineSize;
            def.reloadTime   = m_weapons[i].reloadTime;
            def.recoilPattern = m_weapons[i].recoilAmount;
            def.spreadAngle  = m_weapons[i].spreadAngle;
            def.adsZoomFOV   = m_weapons[i].adsZoomFOV;
            registry->Register(def);
        }
    }

    // Set initial ammo
    for (int i = 0; i < 3; ++i)
    {
        m_ammo[i]    = m_weapons[i].magazineSize;
        m_maxAmmo[i] = m_weapons[i].magazineSize;
    }
    m_reserveAmmo[0] = 48;
    m_reserveAmmo[1] = 120;
    m_reserveAmmo[2] = 32;
}

// ---------------------------------------------------------------------------
// Scene construction
// ---------------------------------------------------------------------------

void WeaponShowcaseModule::BuildScene()
{
    auto* world = m_ctx->GetWorld();

    // --- Player entity ------------------------------------------------------
    m_player = world->CreateEntity();
    world->AddComponent<NameComponent>(m_player, NameComponent{ "Player" });
    world->AddComponent<Transform>(m_player, Transform{
        { 0.0f, 1.7f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }
    });

    // --- Camera (first person) ----------------------------------------------
    m_camera = world->CreateEntity();
    world->AddComponent<NameComponent>(m_camera, NameComponent{ "FPSCamera" });
    world->AddComponent<Transform>(m_camera, Transform{
        { 0.0f, 1.7f, 0.0f },
        { 0.0f, 0.0f, 0.0f },
        { 1.0f, 1.0f, 1.0f }
    });
    world->AddComponent<Camera>(m_camera, Camera{
        70.0f, 0.1f, 500.0f, true
    });

    // --- Directional light --------------------------------------------------
    m_light = world->CreateEntity();
    world->AddComponent<NameComponent>(m_light, NameComponent{ "SunLight" });
    world->AddComponent<Transform>(m_light, Transform{
        { 0.0f, 30.0f, 0.0f },
        { -45.0f, 30.0f, 0.0f },
        { 1.0f, 1.0f, 1.0f }
    });
    world->AddComponent<LightComponent>(m_light, LightComponent{
        LightComponent::Type::Directional,
        { 1.0f, 0.95f, 0.9f }, 2.0f, 0.0f,
        0.0f, 0.0f, true, 2048
    });

    // --- Muzzle flash point light (initially off) ---------------------------
    m_muzzleLight = world->CreateEntity();
    world->AddComponent<NameComponent>(m_muzzleLight, NameComponent{ "MuzzleFlash" });
    world->AddComponent<Transform>(m_muzzleLight, Transform{
        { 0.0f, 1.7f, 1.0f },
        { 0.0f, 0.0f, 0.0f },
        { 1.0f, 1.0f, 1.0f }
    });
    world->AddComponent<LightComponent>(m_muzzleLight, LightComponent{
        LightComponent::Type::Point,
        { 1.0f, 0.8f, 0.3f }, 0.0f, 8.0f,
        1.0f, 0.09f, false, 0
    });

    // --- Ground (firing range floor) ----------------------------------------
    auto ground = world->CreateEntity();
    world->AddComponent<NameComponent>(ground, NameComponent{ "Ground" });
    world->AddComponent<Transform>(ground, Transform{
        { 0.0f, -0.1f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 60.0f, 0.2f, 80.0f }
    });
    world->AddComponent<MeshRenderer>(ground, MeshRenderer{
        "Assets/Models/cube.obj", "Assets/Materials/concrete.mat",
        false, true, true
    });
    m_sceneEntities.push_back(ground);

    // --- Back wall ----------------------------------------------------------
    auto backWall = world->CreateEntity();
    world->AddComponent<NameComponent>(backWall, NameComponent{ "BackWall" });
    world->AddComponent<Transform>(backWall, Transform{
        { 0.0f, 5.0f, 50.0f }, { 0.0f, 0.0f, 0.0f }, { 60.0f, 10.0f, 0.5f }
    });
    world->AddComponent<MeshRenderer>(backWall, MeshRenderer{
        "Assets/Models/cube.obj", "Assets/Materials/concrete.mat",
        false, true, true
    });
    m_sceneEntities.push_back(backWall);

    // --- Shooting targets (cubes at various distances) ----------------------
    struct TargetDef { float x, y, z; float scale; };
    TargetDef targetDefs[] = {
        { -6.0f, 1.5f, 15.0f, 1.5f },
        {  0.0f, 1.5f, 15.0f, 1.5f },
        {  6.0f, 1.5f, 15.0f, 1.5f },
        { -4.0f, 1.0f, 25.0f, 1.0f },
        {  4.0f, 1.0f, 25.0f, 1.0f },
        {  0.0f, 2.0f, 35.0f, 2.0f },
        { -8.0f, 1.0f, 40.0f, 1.0f },
        {  8.0f, 1.0f, 40.0f, 1.0f },
    };

    int targetIdx = 0;
    for (auto& def : targetDefs)
    {
        auto t = world->CreateEntity();
        char name[32];
        std::snprintf(name, sizeof(name), "Target_%d", targetIdx++);
        world->AddComponent<NameComponent>(t, NameComponent{ name });
        world->AddComponent<Transform>(t, Transform{
            { def.x, def.y, def.z },
            { 0.0f, 0.0f, 0.0f },
            { def.scale, def.scale, def.scale }
        });
        world->AddComponent<MeshRenderer>(t, MeshRenderer{
            "Assets/Models/cube.obj", "Assets/Materials/target.mat",
            true, true, true
        });
        m_targets.push_back(t);
    }

    // --- Side walls for the range -------------------------------------------
    for (float side : { -30.0f, 30.0f })
    {
        auto wall = world->CreateEntity();
        world->AddComponent<NameComponent>(wall, NameComponent{ side < 0 ? "LeftWall" : "RightWall" });
        world->AddComponent<Transform>(wall, Transform{
            { side, 3.0f, 25.0f }, { 0.0f, 0.0f, 0.0f }, { 0.5f, 6.0f, 50.0f }
        });
        world->AddComponent<MeshRenderer>(wall, MeshRenderer{
            "Assets/Models/cube.obj", "Assets/Materials/concrete.mat",
            false, true, true
        });
        m_sceneEntities.push_back(wall);
    }
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

void WeaponShowcaseModule::OnUpdate(float dt)
{
    HandleInput(dt);
    UpdateRecoilRecovery(dt);
    UpdateReload(dt);
    UpdateMuzzleFlash(dt);

    // Decrease fire timer
    if (m_fireTimer > 0.0f)
        m_fireTimer -= dt;

    // Update ADS blend (smooth transition)
    float adsTarget = m_isADS ? 1.0f : 0.0f;
    float adsSpeed  = 6.0f;
    m_adsBlend += (adsTarget - m_adsBlend) * std::min(1.0f, adsSpeed * dt);

    // Update FOV based on ADS
    float adsFOV = m_weapons[m_currentWeaponIndex].adsZoomFOV;
    m_currentFOV = m_baseFOV + (adsFOV - m_baseFOV) * m_adsBlend;

    // Apply camera transform
    auto* world = m_ctx->GetWorld();
    if (world && m_camera != entt::null)
    {
        auto& camXf = world->GetComponent<Transform>(m_camera);
        camXf.position = { m_playerX, m_playerY, m_playerZ };
        camXf.rotation = { m_camPitch + m_recoilPitch, m_camYaw, 0.0f };

        auto& cam = world->GetComponent<Camera>(m_camera);
        cam.fov = m_currentFOV;
    }

    // Update player transform
    if (world && m_player != entt::null)
    {
        auto& xf = world->GetComponent<Transform>(m_player);
        xf.position = { m_playerX, m_playerY, m_playerZ };
        xf.rotation = { 0.0f, m_camYaw, 0.0f };
    }

    // Update muzzle flash light position (in front of camera)
    if (world && m_muzzleLight != entt::null)
    {
        float yawRad   = m_camYaw * (3.14159f / 180.0f);
        float pitchRad = m_camPitch * (3.14159f / 180.0f);
        float fwd      = 1.5f;

        auto& mlXf = world->GetComponent<Transform>(m_muzzleLight);
        mlXf.position = {
            m_playerX + std::sin(yawRad) * fwd,
            m_playerY - std::sin(pitchRad) * fwd,
            m_playerZ + std::cos(yawRad) * fwd
        };
    }
}

// ---------------------------------------------------------------------------
// Input handling
// ---------------------------------------------------------------------------

void WeaponShowcaseModule::HandleInput(float dt)
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

    float speed = 6.0f;
    float yawRad = m_camYaw * (3.14159f / 180.0f);

    float forwardX = std::sin(yawRad);
    float forwardZ = std::cos(yawRad);
    float rightX   = std::cos(yawRad);
    float rightZ   = -std::sin(yawRad);

    m_playerX += (forwardX * moveZ + rightX * moveX) * speed * dt;
    m_playerZ += (forwardZ * moveZ + rightZ * moveX) * speed * dt;

    // --- Weapon switching (1/2/3) -------------------------------------------
    if (input->WasKeyPressed('1')) SwitchWeapon(0);
    if (input->WasKeyPressed('2')) SwitchWeapon(1);
    if (input->WasKeyPressed('3')) SwitchWeapon(2);

    // --- Fire (LMB) --------------------------------------------------------
    auto& wep = m_weapons[m_currentWeaponIndex];

    if (wep.fireMode == FireMode::FullAuto)
    {
        m_fireHeld = input->IsMouseButtonDown(0);
        if (m_fireHeld && m_fireTimer <= 0.0f && !m_isReloading)
            FireWeapon();
    }
    else // Semi
    {
        if (input->WasMouseButtonPressed(0) && m_fireTimer <= 0.0f && !m_isReloading)
            FireWeapon();
    }

    // --- Reload (R) ---------------------------------------------------------
    if (input->WasKeyPressed('R') && !m_isReloading)
        StartReload();

    // --- ADS (Right mouse button) -------------------------------------------
    m_isADS = input->IsMouseButtonDown(1);
}

// ---------------------------------------------------------------------------
// Firing
// ---------------------------------------------------------------------------

void WeaponShowcaseModule::FireWeapon()
{
    auto& wep = m_weapons[m_currentWeaponIndex];

    if (m_ammo[m_currentWeaponIndex] <= 0)
    {
        // Click — out of ammo, auto-reload
        StartReload();
        return;
    }

    // Consume ammo
    m_ammo[m_currentWeaponIndex]--;

    // Set fire cooldown
    m_fireTimer = 1.0f / wep.fireRate;

    // Apply recoil (kick camera pitch up)
    m_recoilPitch += wep.recoilAmount;

    // Trigger muzzle flash
    m_muzzleFlashOn    = true;
    m_muzzleFlashTimer = 0.06f;

    auto* world = m_ctx->GetWorld();
    if (world && m_muzzleLight != entt::null)
    {
        auto& ml = world->GetComponent<LightComponent>(m_muzzleLight);
        ml.intensity = 8.0f;
        ml.castsShadow = true;
    }

    // --- Raycast hit check (simple distance + angle check) ------------------
    if (world)
    {
        float yawRad   = m_camYaw * (3.14159f / 180.0f);
        float pitchRad = (m_camPitch + m_recoilPitch) * (3.14159f / 180.0f);

        float dirX = std::sin(yawRad) * std::cos(pitchRad);
        float dirY = std::sin(pitchRad);
        float dirZ = std::cos(yawRad) * std::cos(pitchRad);

        float spreadRad = wep.spreadAngle * (3.14159f / 180.0f);
        float adsMultiplier = 1.0f - m_adsBlend * 0.6f; // ADS reduces spread

        for (auto target : m_targets)
        {
            if (target == entt::null) continue;
            auto& tXf = world->GetComponent<Transform>(target);

            float toX = tXf.position.x - m_playerX;
            float toY = tXf.position.y - m_playerY;
            float toZ = tXf.position.z - m_playerZ;
            float dist = std::sqrt(toX * toX + toY * toY + toZ * toZ);

            if (dist < 0.1f) continue;

            float invDist = 1.0f / dist;
            float normX = toX * invDist;
            float normY = toY * invDist;
            float normZ = toZ * invDist;

            // Dot product for angle check
            float dot = dirX * normX + dirY * normY + dirZ * normZ;
            float angle = std::acos(std::clamp(dot, -1.0f, 1.0f));

            // Hit threshold: target size / distance + spread
            float hitAngle = (tXf.scale.x * 0.5f) / dist + spreadRad * adsMultiplier;

            if (angle < hitAngle)
            {
                m_targetsHit++;

                // Visual feedback: move target back slightly and spin
                tXf.rotation.y += 45.0f;
                tXf.position.y += 0.3f;
                break; // Only hit one target per shot
            }
        }
    }

    // Publish fire event
    auto* bus = m_ctx->GetEventBus();
    if (bus)
        bus->Publish(WeaponFiredEvent{ wep.name, m_ammo[m_currentWeaponIndex] });
}

// ---------------------------------------------------------------------------
// Reload
// ---------------------------------------------------------------------------

void WeaponShowcaseModule::StartReload()
{
    auto& wep = m_weapons[m_currentWeaponIndex];
    int curAmmo = m_ammo[m_currentWeaponIndex];

    // Skip if magazine is full or no reserve ammo
    if (curAmmo >= m_maxAmmo[m_currentWeaponIndex]) return;
    if (m_reserveAmmo[m_currentWeaponIndex] <= 0) return;

    m_isReloading   = true;
    m_reloadTimer   = 0.0f;
    m_reloadDuration = wep.reloadTime;

    auto* bus = m_ctx->GetEventBus();
    if (bus)
        bus->Publish(ReloadStartedEvent{ wep.name });
}

void WeaponShowcaseModule::UpdateReload(float dt)
{
    if (!m_isReloading) return;

    m_reloadTimer += dt;
    if (m_reloadTimer >= m_reloadDuration)
    {
        // Reload complete
        int needed = m_maxAmmo[m_currentWeaponIndex] - m_ammo[m_currentWeaponIndex];
        int available = std::min(needed, m_reserveAmmo[m_currentWeaponIndex]);

        m_ammo[m_currentWeaponIndex] += available;
        m_reserveAmmo[m_currentWeaponIndex] -= available;

        m_isReloading = false;
        m_reloadTimer = 0.0f;

        auto* bus = m_ctx->GetEventBus();
        if (bus)
            bus->Publish(ReloadFinishedEvent{
                m_weapons[m_currentWeaponIndex].name,
                m_ammo[m_currentWeaponIndex]
            });
    }
}

// ---------------------------------------------------------------------------
// Weapon switching
// ---------------------------------------------------------------------------

void WeaponShowcaseModule::SwitchWeapon(int index)
{
    if (index < 0 || index >= 3) return;
    if (index == m_currentWeaponIndex) return;

    // Cancel any active reload
    m_isReloading = false;
    m_reloadTimer = 0.0f;
    m_fireTimer   = 0.0f;
    m_recoilPitch = 0.0f;

    m_currentWeaponIndex = index;
}

// ---------------------------------------------------------------------------
// Recoil recovery
// ---------------------------------------------------------------------------

void WeaponShowcaseModule::UpdateRecoilRecovery(float dt)
{
    if (m_recoilPitch > 0.0f)
    {
        m_recoilPitch -= m_recoilRecoveryRate * dt;
        if (m_recoilPitch < 0.0f)
            m_recoilPitch = 0.0f;
    }
}

// ---------------------------------------------------------------------------
// Muzzle flash
// ---------------------------------------------------------------------------

void WeaponShowcaseModule::UpdateMuzzleFlash(float dt)
{
    if (!m_muzzleFlashOn) return;

    m_muzzleFlashTimer -= dt;
    if (m_muzzleFlashTimer <= 0.0f)
    {
        m_muzzleFlashOn = false;

        auto* world = m_ctx->GetWorld();
        if (world && m_muzzleLight != entt::null)
        {
            auto& ml = world->GetComponent<LightComponent>(m_muzzleLight);
            ml.intensity   = 0.0f;
            ml.castsShadow = false;
        }
    }
}

// ---------------------------------------------------------------------------
// Render
// ---------------------------------------------------------------------------

void WeaponShowcaseModule::OnRender()
{
    DrawCrosshair();
    DrawAmmoHUD();
    DrawWeaponInfo();

    if (m_isReloading)
        DrawReloadBar();
}

// ---------------------------------------------------------------------------
// Crosshair
// ---------------------------------------------------------------------------

void WeaponShowcaseModule::DrawCrosshair()
{
    auto* dd = Spark::DebugDraw::GetInstance();
    if (!dd) return;

    float cx = m_screenW * 0.5f;
    float cy = m_screenH * 0.5f;

    auto& wep = m_weapons[m_currentWeaponIndex];
    float spread = wep.spreadAngle * 2.0f * (1.0f - m_adsBlend * 0.6f);
    float gap = 4.0f;
    float lineLen = 12.0f;

    float alpha = m_isADS ? 0.5f : 0.9f;

    // Four crosshair lines
    dd->ScreenLine({ cx, cy - spread - lineLen }, { cx, cy - spread + gap },
                    { 1.0f, 1.0f, 1.0f, alpha });
    dd->ScreenLine({ cx, cy + spread - gap }, { cx, cy + spread + lineLen },
                    { 1.0f, 1.0f, 1.0f, alpha });
    dd->ScreenLine({ cx - spread - lineLen, cy }, { cx - spread + gap, cy },
                    { 1.0f, 1.0f, 1.0f, alpha });
    dd->ScreenLine({ cx + spread - gap, cy }, { cx + spread + lineLen, cy },
                    { 1.0f, 1.0f, 1.0f, alpha });

    // Centre dot
    dd->ScreenRect({ cx - 1.0f, cy - 1.0f }, { cx + 1.0f, cy + 1.0f },
                    { 1.0f, 1.0f, 1.0f, 1.0f });
}

// ---------------------------------------------------------------------------
// Ammo HUD (bottom-right)
// ---------------------------------------------------------------------------

void WeaponShowcaseModule::DrawAmmoHUD()
{
    auto* dd = Spark::DebugDraw::GetInstance();
    if (!dd) return;

    float x = m_screenW - 220.0f;
    float y = m_screenH - 80.0f;

    // Background
    dd->ScreenRect({ x - 10.0f, y - 10.0f },
                    { x + 200.0f, y + 55.0f },
                    { 0.0f, 0.0f, 0.0f, 0.5f });

    // Ammo count
    char buf[64];
    int curAmmo = m_ammo[m_currentWeaponIndex];
    int maxAmmo = m_maxAmmo[m_currentWeaponIndex];
    int reserve = m_reserveAmmo[m_currentWeaponIndex];

    // Color: white normally, red when low
    float ammoRatio = static_cast<float>(curAmmo) / static_cast<float>(maxAmmo);
    float r = (ammoRatio < 0.25f) ? 1.0f : 1.0f;
    float g = (ammoRatio < 0.25f) ? 0.3f : 1.0f;
    float b = (ammoRatio < 0.25f) ? 0.3f : 1.0f;

    std::snprintf(buf, sizeof(buf), "%d / %d", curAmmo, maxAmmo);
    dd->ScreenText({ x, y }, buf, { r, g, b, 1.0f });

    std::snprintf(buf, sizeof(buf), "Reserve: %d", reserve);
    dd->ScreenText({ x, y + 22.0f }, buf, { 0.7f, 0.7f, 0.7f, 0.8f });

    // Targets hit
    std::snprintf(buf, sizeof(buf), "Hits: %d", m_targetsHit);
    dd->ScreenText({ x, y + 40.0f }, buf, { 0.5f, 1.0f, 0.5f, 0.9f });
}

// ---------------------------------------------------------------------------
// Weapon info (bottom-left)
// ---------------------------------------------------------------------------

void WeaponShowcaseModule::DrawWeaponInfo()
{
    auto* dd = Spark::DebugDraw::GetInstance();
    if (!dd) return;

    float x = 20.0f;
    float y = m_screenH - 80.0f;

    // Background
    dd->ScreenRect({ x - 5.0f, y - 10.0f },
                    { x + 240.0f, y + 55.0f },
                    { 0.0f, 0.0f, 0.0f, 0.5f });

    auto& wep = m_weapons[m_currentWeaponIndex];

    // Weapon name with number indicator
    char buf[128];
    std::snprintf(buf, sizeof(buf), "[%d] %s", m_currentWeaponIndex + 1, wep.name.c_str());
    dd->ScreenText({ x, y }, buf, { 1.0f, 0.85f, 0.3f, 1.0f });

    // Fire mode
    const char* modeStr = "Semi";
    if (wep.fireMode == FireMode::FullAuto) modeStr = "Full Auto";
    else if (wep.fireMode == FireMode::Burst) modeStr = "Burst";

    std::snprintf(buf, sizeof(buf), "Mode: %s", modeStr);
    dd->ScreenText({ x, y + 20.0f }, buf, { 0.7f, 0.7f, 0.7f, 0.9f });

    // ADS indicator
    if (m_isADS)
    {
        dd->ScreenText({ x, y + 40.0f }, "ADS", { 0.3f, 0.8f, 1.0f, 1.0f });
    }

    // Controls help (top-left)
    dd->ScreenText({ 20.0f, 10.0f }, "1/2/3: Switch  LMB: Fire  R: Reload  RMB: ADS",
                    { 0.6f, 0.6f, 0.6f, 0.7f });
}

// ---------------------------------------------------------------------------
// Reload progress bar (centre)
// ---------------------------------------------------------------------------

void WeaponShowcaseModule::DrawReloadBar()
{
    auto* dd = Spark::DebugDraw::GetInstance();
    if (!dd) return;

    float cx = m_screenW * 0.5f;
    float cy = m_screenH * 0.5f + 40.0f;
    float barW = 150.0f;
    float barH = 10.0f;

    // Background
    dd->ScreenRect({ cx - barW * 0.5f, cy },
                    { cx + barW * 0.5f, cy + barH },
                    { 0.1f, 0.1f, 0.1f, 0.7f });

    // Fill
    float progress = m_reloadTimer / m_reloadDuration;
    float fillW = barW * progress;
    dd->ScreenRect({ cx - barW * 0.5f, cy },
                    { cx - barW * 0.5f + fillW, cy + barH },
                    { 0.3f, 0.8f, 1.0f, 0.9f });

    // Label
    dd->ScreenText({ cx - 30.0f, cy - 18.0f }, "RELOADING",
                    { 1.0f, 1.0f, 1.0f, 0.9f });
}

// ---------------------------------------------------------------------------

void WeaponShowcaseModule::OnResize(int w, int h)
{
    m_screenW = static_cast<float>(w);
    m_screenH = static_cast<float>(h);
}

// ---------------------------------------------------------------------------
// DLL exports
// ---------------------------------------------------------------------------

SPARK_IMPLEMENT_MODULE(WeaponShowcaseModule)
