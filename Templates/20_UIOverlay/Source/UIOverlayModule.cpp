/**
 * @file UIOverlayModule.cpp
 * @brief ADVANCED — UI/HUD rendering showcase.
 */

#include "UIOverlayModule.h"
#include <Input/InputManager.h>
#include <cmath>
#include <algorithm>
#include <cstdio>

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void UIOverlayModule::OnLoad(Spark::IEngineContext* ctx)
{
    m_ctx = ctx;

    // Initialise inventory with some items
    for (int r = 0; r < kInvRows; ++r)
        for (int c = 0; c < kInvCols; ++c)
            m_inventory[r][c] = { "", 0, false };

    m_inventory[0][0] = { "Sword",      1, true };
    m_inventory[0][1] = { "Shield",     1, true };
    m_inventory[0][2] = { "Potion",     5, true };
    m_inventory[1][0] = { "Arrow",     25, true };
    m_inventory[1][1] = { "Bomb",       3, true };
    m_inventory[2][3] = { "Key",        1, true };

    BuildScene();

    AddNotification("Welcome to the UI Overlay Demo!", 5.0f, 1.0f, 1.0f, 1.0f);
    AddNotification("Press H to toggle HUD", 4.0f, 0.8f, 0.8f, 0.8f);
}

void UIOverlayModule::OnUnload()
{
    auto* world = m_ctx ? m_ctx->GetWorld() : nullptr;
    if (!world) { m_ctx = nullptr; return; }

    for (auto e : { m_player, m_camera, m_light, m_boss })
        if (e != entt::null) world->DestroyEntity(e);

    for (auto e : m_enemies)
        if (e != entt::null) world->DestroyEntity(e);
    for (auto e : m_sceneEntities)
        if (e != entt::null) world->DestroyEntity(e);

    m_enemies.clear();
    m_sceneEntities.clear();
    m_ctx = nullptr;
}

// ---------------------------------------------------------------------------
// Scene construction
// ---------------------------------------------------------------------------

void UIOverlayModule::BuildScene()
{
    auto* world = m_ctx->GetWorld();

    // --- Player entity ------------------------------------------------------
    m_player = world->CreateEntity();
    world->AddComponent<NameComponent>(m_player, NameComponent{ "Player" });
    world->AddComponent<Transform>(m_player, Transform{
        { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 2.0f, 1.0f }
    });
    world->AddComponent<MeshRenderer>(m_player, MeshRenderer{
        "Assets/Models/cube.obj", "Assets/Materials/player.mat",
        true, true, true
    });
    world->AddComponent<HealthComponent>(m_player, HealthComponent{
        100.0f, 100.0f, false, false
    });

    // --- Camera -------------------------------------------------------------
    m_camera = world->CreateEntity();
    world->AddComponent<NameComponent>(m_camera, NameComponent{ "MainCamera" });
    world->AddComponent<Transform>(m_camera, Transform{
        { 0.0f, 8.0f, -15.0f },
        { -20.0f, 0.0f, 0.0f },
        { 1.0f, 1.0f, 1.0f }
    });
    world->AddComponent<Camera>(m_camera, Camera{
        60.0f, 0.1f, 500.0f, true
    });

    // --- Light --------------------------------------------------------------
    m_light = world->CreateEntity();
    world->AddComponent<NameComponent>(m_light, NameComponent{ "SunLight" });
    world->AddComponent<Transform>(m_light, Transform{
        { 0.0f, 20.0f, 0.0f },
        { -50.0f, 30.0f, 0.0f },
        { 1.0f, 1.0f, 1.0f }
    });
    world->AddComponent<LightComponent>(m_light, LightComponent{
        LightComponent::Type::Directional,
        { 1.0f, 0.95f, 0.85f }, 2.0f, 0.0f,
        0.0f, 0.0f, true, 2048
    });

    // --- Ground -------------------------------------------------------------
    auto ground = world->CreateEntity();
    world->AddComponent<NameComponent>(ground, NameComponent{ "Ground" });
    world->AddComponent<Transform>(ground, Transform{
        { 0.0f, -0.1f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 40.0f, 0.2f, 40.0f }
    });
    world->AddComponent<MeshRenderer>(ground, MeshRenderer{
        "Assets/Models/cube.obj", "Assets/Materials/concrete.mat",
        false, true, true
    });
    m_sceneEntities.push_back(ground);

    // --- Enemies (for minimap) ----------------------------------------------
    struct EnemyDef { const char* name; float x, z; };
    EnemyDef enemyDefs[] = {
        { "Enemy_A",  8.0f,  5.0f },
        { "Enemy_B", -6.0f,  8.0f },
        { "Enemy_C", 12.0f, -3.0f },
        { "Enemy_D", -4.0f, -7.0f },
    };

    for (auto& def : enemyDefs)
    {
        auto e = world->CreateEntity();
        world->AddComponent<NameComponent>(e, NameComponent{ def.name });
        world->AddComponent<Transform>(e, Transform{
            { def.x, 1.0f, def.z },
            { 0.0f, 0.0f, 0.0f },
            { 1.0f, 2.0f, 1.0f }
        });
        world->AddComponent<MeshRenderer>(e, MeshRenderer{
            "Assets/Models/cube.obj", "Assets/Materials/enemy.mat",
            true, true, true
        });
        world->AddComponent<TagComponent>(e);
        world->GetComponent<TagComponent>(e).AddTag("enemy");
        m_enemies.push_back(e);
    }

    // --- Boss entity --------------------------------------------------------
    m_boss = world->CreateEntity();
    world->AddComponent<NameComponent>(m_boss, NameComponent{ "BossGolem" });
    world->AddComponent<Transform>(m_boss, Transform{
        { 0.0f, 2.5f, 15.0f }, { 0.0f, 0.0f, 0.0f }, { 3.0f, 5.0f, 3.0f }
    });
    world->AddComponent<MeshRenderer>(m_boss, MeshRenderer{
        "Assets/Models/cube.obj", "Assets/Materials/boss.mat",
        true, true, true
    });
    world->AddComponent<HealthComponent>(m_boss, HealthComponent{
        500.0f, 500.0f, false, false
    });
    world->AddComponent<TagComponent>(m_boss);
    world->GetComponent<TagComponent>(m_boss).AddTag("boss");

    // --- Pickup item (for interaction prompt) --------------------------------
    auto pickup = world->CreateEntity();
    world->AddComponent<NameComponent>(pickup, NameComponent{ "HealthPack" });
    world->AddComponent<Transform>(pickup, Transform{
        { 3.0f, 0.5f, -2.0f }, { 0.0f, 0.0f, 0.0f }, { 0.5f, 0.5f, 0.5f }
    });
    world->AddComponent<MeshRenderer>(pickup, MeshRenderer{
        "Assets/Models/sphere.obj", "Assets/Materials/green.mat",
        false, false, true
    });
    world->AddComponent<TagComponent>(pickup);
    world->GetComponent<TagComponent>(pickup).AddTag("pickup");
    m_sceneEntities.push_back(pickup);
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

void UIOverlayModule::OnUpdate(float dt)
{
    auto* input = m_ctx->GetInput();
    auto* world = m_ctx->GetWorld();
    if (!input || !world) return;

    // --- Pause (Escape) -----------------------------------------------------
    if (input->WasKeyPressed(VK_ESCAPE))
    {
        m_paused = !m_paused;
        if (m_paused) m_pauseSelection = 0;
    }

    // While paused, handle menu navigation only
    if (m_paused)
    {
        if (input->WasKeyPressed('W') || input->WasKeyPressed(VK_UP))
            m_pauseSelection = std::max(0, m_pauseSelection - 1);
        if (input->WasKeyPressed('S') || input->WasKeyPressed(VK_DOWN))
            m_pauseSelection = std::min(2, m_pauseSelection + 1);
        if (input->WasKeyPressed(VK_RETURN))
        {
            if (m_pauseSelection == 0) m_paused = false; // Resume
            // 1 = Options (placeholder)
            // 2 = Quit (placeholder)
        }
        return;
    }

    // --- Toggle keys --------------------------------------------------------
    if (input->WasKeyPressed(VK_TAB))
        m_inventoryOpen = !m_inventoryOpen;

    if (input->WasKeyPressed('H'))
        m_hudVisible = !m_hudVisible;

    if (input->WasKeyPressed('M'))
        m_minimapOn = !m_minimapOn;

    // --- Test notifications (1-4) -------------------------------------------
    if (input->WasKeyPressed('1'))
        AddNotification("Enemy spotted nearby!", 3.0f, 1.0f, 0.3f, 0.3f);
    if (input->WasKeyPressed('2'))
        AddNotification("Objective updated", 3.0f, 0.3f, 0.8f, 1.0f);
    if (input->WasKeyPressed('3'))
        AddNotification("Ammo refilled", 2.5f, 0.3f, 1.0f, 0.3f);
    if (input->WasKeyPressed('4'))
    {
        AddNotification("Achievement unlocked: First Blood", 4.0f, 1.0f, 0.85f, 0.0f);
        AddKillFeedEntry("Player [Sword] Enemy_A");
        m_kills++;
        m_score += 100;
    }

    // --- Simulate damage (X) ------------------------------------------------
    if (input->WasKeyPressed('X'))
    {
        float damage = 15.0f;
        m_health = std::max(0.0f, m_health - damage);
        m_healthFlash = 0.5f;

        // Random direction for damage indicator
        float angle = static_cast<float>(std::rand() % 360);
        SimulateDamageFrom(angle);

        // Also damage boss for demo
        m_bossHealth = std::max(0.0f, m_bossHealth - 25.0f);
    }

    // --- Player movement (WASD) ---------------------------------------------
    float moveX = 0.0f, moveZ = 0.0f;
    if (input->IsKeyDown('W')) moveZ += 1.0f;
    if (input->IsKeyDown('S')) moveZ -= 1.0f;
    if (input->IsKeyDown('A')) moveX -= 1.0f;
    if (input->IsKeyDown('D')) moveX += 1.0f;

    m_moveSpeed = std::sqrt(moveX * moveX + moveZ * moveZ);

    if (m_player != entt::null)
    {
        auto& xf = world->GetComponent<Transform>(m_player);
        float speed = 6.0f;
        xf.position.x += moveX * speed * dt;
        xf.position.z += moveZ * speed * dt;

        // Update yaw for compass
        if (m_moveSpeed > 0.01f)
            m_playerYaw = std::atan2(moveX, moveZ) * (180.0f / 3.14159f);
    }

    // --- Smooth health bar interpolation ------------------------------------
    float interpSpeed = 5.0f;
    if (m_displayHealth > m_health)
        m_displayHealth -= interpSpeed * dt * (m_displayHealth - m_health);
    else if (m_displayHealth < m_health)
        m_displayHealth += interpSpeed * dt * (m_health - m_displayHealth);

    // Clamp to actual value when close
    if (std::abs(m_displayHealth - m_health) < 0.5f)
        m_displayHealth = m_health;

    // --- Flash / glow timers ------------------------------------------------
    if (m_healthFlash > 0.0f) m_healthFlash -= dt;
    if (m_healGlow > 0.0f)    m_healGlow -= dt;

    // --- Damage indicators decay --------------------------------------------
    for (auto it = m_damageIndicators.begin(); it != m_damageIndicators.end(); )
    {
        it->timeRemaining -= dt;
        it->intensity = it->timeRemaining / 1.0f; // fade over 1 second
        if (it->timeRemaining <= 0.0f)
            it = m_damageIndicators.erase(it);
        else
            ++it;
    }

    // --- Notifications decay ------------------------------------------------
    for (auto it = m_notifications.begin(); it != m_notifications.end(); )
    {
        it->timeRemaining -= dt;
        if (it->timeRemaining <= 0.0f)
            it = m_notifications.erase(it);
        else
            ++it;
    }

    // --- Kill feed decay ----------------------------------------------------
    for (auto it = m_killFeed.begin(); it != m_killFeed.end(); )
    {
        it->timeRemaining -= dt;
        if (it->timeRemaining <= 0.0f)
            it = m_killFeed.erase(it);
        else
            ++it;
    }

    // --- Timer countdown ----------------------------------------------------
    if (m_timerActive && m_timerSeconds > 0.0f)
        m_timerSeconds -= dt;

    // --- Interaction prompt check -------------------------------------------
    m_showInteract = false;
    if (m_player != entt::null)
    {
        auto& playerXf = world->GetComponent<Transform>(m_player);
        for (auto e : m_sceneEntities)
        {
            if (e == entt::null) continue;
            if (!world->HasComponent<TagComponent>(e)) continue;
            auto& tags = world->GetComponent<TagComponent>(e);
            if (!tags.HasTag("pickup")) continue;

            auto& itemXf = world->GetComponent<Transform>(e);
            float dx = itemXf.position.x - playerXf.position.x;
            float dz = itemXf.position.z - playerXf.position.z;
            float distSq = dx * dx + dz * dz;
            if (distSq <= 4.0f) // within 2 units
            {
                m_showInteract = true;
                m_interactText = "Press E to pick up";
                break;
            }
        }
    }

    // --- Camera follow ------------------------------------------------------
    if (m_player != entt::null)
    {
        auto& playerXf = world->GetComponent<Transform>(m_player);
        auto& camXf    = world->GetComponent<Transform>(m_camera);

        float t = 1.0f - std::exp(-4.0f * dt);
        camXf.position.x += (playerXf.position.x - camXf.position.x) * t;
        camXf.position.y += (playerXf.position.y + 8.0f - camXf.position.y) * t;
        camXf.position.z += (playerXf.position.z - 15.0f - camXf.position.z) * t;
    }
}

// ---------------------------------------------------------------------------
// Render (all UI drawing)
// ---------------------------------------------------------------------------

void UIOverlayModule::OnRender()
{
    if (!m_hudVisible) return;

    auto* dd = Spark::DebugDraw::GetInstance();
    if (!dd) return;

    // Always-visible HUD elements
    DrawHealthBar();
    DrawAmmoCounter();
    DrawCrosshair();
    DrawScoreDisplay();
    DrawCompass();
    DrawTimer();

    if (m_minimapOn)
        DrawMinimap();

    if (m_bossActive && m_bossHealth > 0.0f)
        DrawBossHealthBar();

    // Damage direction indicators
    DrawDamageIndicators();

    // Notifications (top-right)
    DrawNotifications();

    // Kill feed (top-right, below notifications)
    DrawKillFeed();

    // Interaction prompt
    if (m_showInteract)
        DrawInteractionPrompt();

    // Overlay screens
    if (m_inventoryOpen)
        DrawInventory();

    if (m_paused)
        DrawPauseMenu();
}

// ---------------------------------------------------------------------------
// Health bar (bottom-left)
// ---------------------------------------------------------------------------

void UIOverlayModule::DrawHealthBar()
{
    auto* dd = Spark::DebugDraw::GetInstance();
    if (!dd) return;

    float barX = 20.0f;
    float barY = m_screenH - 60.0f;
    float barW = 250.0f;
    float barH = 24.0f;

    // Background
    dd->ScreenRect({ barX, barY }, { barX + barW, barY + barH },
                    { 0.1f, 0.1f, 0.1f, 0.8f });

    // Health fill (smooth interpolated)
    float fillRatio = m_displayHealth / m_maxHealth;
    float fillW = barW * fillRatio;

    // Color: green → yellow → red based on health %
    float r = (fillRatio < 0.5f) ? 1.0f : 1.0f - (fillRatio - 0.5f) * 2.0f;
    float g = (fillRatio > 0.5f) ? 1.0f : fillRatio * 2.0f;
    float b = 0.0f;

    dd->ScreenRect({ barX, barY }, { barX + fillW, barY + barH },
                    { r, g, b, 0.9f });

    // Damage flash overlay
    if (m_healthFlash > 0.0f)
    {
        dd->ScreenRect({ barX, barY }, { barX + barW, barY + barH },
                        { 1.0f, 0.0f, 0.0f, m_healthFlash });
    }

    // Heal glow overlay
    if (m_healGlow > 0.0f)
    {
        dd->ScreenRect({ barX, barY }, { barX + barW, barY + barH },
                        { 0.0f, 1.0f, 0.3f, m_healGlow * 0.5f });
    }

    // Border
    dd->ScreenLine({ barX, barY }, { barX + barW, barY }, { 0.6f, 0.6f, 0.6f, 1.0f });
    dd->ScreenLine({ barX + barW, barY }, { barX + barW, barY + barH }, { 0.6f, 0.6f, 0.6f, 1.0f });
    dd->ScreenLine({ barX + barW, barY + barH }, { barX, barY + barH }, { 0.6f, 0.6f, 0.6f, 1.0f });
    dd->ScreenLine({ barX, barY + barH }, { barX, barY }, { 0.6f, 0.6f, 0.6f, 1.0f });

    // Text
    char buf[64];
    std::snprintf(buf, sizeof(buf), "HP: %.0f / %.0f", m_health, m_maxHealth);
    dd->ScreenText({ barX + 8.0f, barY + 4.0f }, buf, { 1.0f, 1.0f, 1.0f, 1.0f });
}

// ---------------------------------------------------------------------------
// Ammo counter (bottom-right)
// ---------------------------------------------------------------------------

void UIOverlayModule::DrawAmmoCounter()
{
    auto* dd = Spark::DebugDraw::GetInstance();
    if (!dd) return;

    float x = m_screenW - 200.0f;
    float y = m_screenH - 60.0f;

    char buf[64];
    std::snprintf(buf, sizeof(buf), "%d / %d", m_ammo, m_maxAmmo);
    dd->ScreenText({ x, y }, buf, { 1.0f, 1.0f, 1.0f, 1.0f });

    std::snprintf(buf, sizeof(buf), "Reserve: %d", m_ammoReserve);
    dd->ScreenText({ x, y + 20.0f }, buf, { 0.7f, 0.7f, 0.7f, 0.8f });
}

// ---------------------------------------------------------------------------
// Minimap (top-right corner)
// ---------------------------------------------------------------------------

void UIOverlayModule::DrawMinimap()
{
    auto* dd = Spark::DebugDraw::GetInstance();
    auto* world = m_ctx->GetWorld();
    if (!dd || !world) return;

    float mapSize = 150.0f;
    float mapX = m_screenW - mapSize - 20.0f;
    float mapY = 20.0f;
    float mapScale = mapSize / 40.0f; // 40 world units mapped to minimap

    // Background
    dd->ScreenRect({ mapX, mapY }, { mapX + mapSize, mapY + mapSize },
                    { 0.0f, 0.0f, 0.0f, 0.6f });

    // Border
    dd->ScreenLine({ mapX, mapY }, { mapX + mapSize, mapY }, { 0.4f, 0.4f, 0.4f, 1.0f });
    dd->ScreenLine({ mapX + mapSize, mapY }, { mapX + mapSize, mapY + mapSize }, { 0.4f, 0.4f, 0.4f, 1.0f });
    dd->ScreenLine({ mapX + mapSize, mapY + mapSize }, { mapX, mapY + mapSize }, { 0.4f, 0.4f, 0.4f, 1.0f });
    dd->ScreenLine({ mapX, mapY + mapSize }, { mapX, mapY }, { 0.4f, 0.4f, 0.4f, 1.0f });

    float centerX = mapX + mapSize * 0.5f;
    float centerY = mapY + mapSize * 0.5f;

    // Player dot (green, centred)
    float playerWorldX = 0.0f, playerWorldZ = 0.0f;
    if (m_player != entt::null)
    {
        auto& pXf = world->GetComponent<Transform>(m_player);
        playerWorldX = pXf.position.x;
        playerWorldZ = pXf.position.z;
    }

    // Player marker (green dot at centre)
    dd->ScreenRect(
        { centerX - 3.0f, centerY - 3.0f },
        { centerX + 3.0f, centerY + 3.0f },
        { 0.0f, 1.0f, 0.0f, 1.0f });

    // Enemy dots (red)
    for (auto e : m_enemies)
    {
        if (e == entt::null) continue;
        auto& eXf = world->GetComponent<Transform>(e);

        float relX = (eXf.position.x - playerWorldX) * mapScale;
        float relZ = (eXf.position.z - playerWorldZ) * mapScale;

        float dotX = centerX + relX;
        float dotY = centerY - relZ; // flip Z for screen coords

        // Clamp to minimap bounds
        if (dotX < mapX || dotX > mapX + mapSize ||
            dotY < mapY || dotY > mapY + mapSize)
            continue;

        dd->ScreenRect(
            { dotX - 2.0f, dotY - 2.0f },
            { dotX + 2.0f, dotY + 2.0f },
            { 1.0f, 0.0f, 0.0f, 1.0f });
    }

    // Boss marker (large yellow dot)
    if (m_boss != entt::null && m_bossActive)
    {
        auto& bXf = world->GetComponent<Transform>(m_boss);
        float relX = (bXf.position.x - playerWorldX) * mapScale;
        float relZ = (bXf.position.z - playerWorldZ) * mapScale;
        float dotX = centerX + relX;
        float dotY = centerY - relZ;

        if (dotX >= mapX && dotX <= mapX + mapSize &&
            dotY >= mapY && dotY <= mapY + mapSize)
        {
            dd->ScreenRect(
                { dotX - 4.0f, dotY - 4.0f },
                { dotX + 4.0f, dotY + 4.0f },
                { 1.0f, 0.85f, 0.0f, 1.0f });
        }
    }

    // Label
    dd->ScreenText({ mapX + 4.0f, mapY + mapSize + 4.0f }, "MINIMAP",
                    { 0.6f, 0.6f, 0.6f, 0.8f });
}

// ---------------------------------------------------------------------------
// Crosshair (centre of screen, dynamic spread)
// ---------------------------------------------------------------------------

void UIOverlayModule::DrawCrosshair()
{
    auto* dd = Spark::DebugDraw::GetInstance();
    if (!dd) return;

    float cx = m_screenW * 0.5f;
    float cy = m_screenH * 0.5f;

    // Dynamic spread: increases with movement
    float spread = m_crosshairSpread + m_moveSpeed * 15.0f;
    float gap = 4.0f;
    float lineLen = 10.0f;

    // Four lines (top, bottom, left, right)
    dd->ScreenLine({ cx, cy - spread - lineLen }, { cx, cy - spread + gap },
                    { 1.0f, 1.0f, 1.0f, 0.9f });
    dd->ScreenLine({ cx, cy + spread - gap }, { cx, cy + spread + lineLen },
                    { 1.0f, 1.0f, 1.0f, 0.9f });
    dd->ScreenLine({ cx - spread - lineLen, cy }, { cx - spread + gap, cy },
                    { 1.0f, 1.0f, 1.0f, 0.9f });
    dd->ScreenLine({ cx + spread - gap, cy }, { cx + spread + lineLen, cy },
                    { 1.0f, 1.0f, 1.0f, 0.9f });

    // Centre dot
    dd->ScreenRect({ cx - 1.0f, cy - 1.0f }, { cx + 1.0f, cy + 1.0f },
                    { 1.0f, 1.0f, 1.0f, 1.0f });
}

// ---------------------------------------------------------------------------
// Damage direction indicators (red arcs around screen edge)
// ---------------------------------------------------------------------------

void UIOverlayModule::DrawDamageIndicators()
{
    auto* dd = Spark::DebugDraw::GetInstance();
    if (!dd) return;

    float cx = m_screenW * 0.5f;
    float cy = m_screenH * 0.5f;
    float radius = std::min(cx, cy) * 0.8f;

    for (auto& di : m_damageIndicators)
    {
        float rad = di.angle * (3.14159f / 180.0f);
        float ix = cx + std::sin(rad) * radius;
        float iy = cy - std::cos(rad) * radius;

        float alpha = std::clamp(di.intensity, 0.0f, 1.0f);

        // Draw a small red indicator mark
        float markSize = 20.0f;
        float perpX = std::cos(rad) * markSize * 0.5f;
        float perpY = std::sin(rad) * markSize * 0.5f;

        dd->ScreenLine(
            { ix - perpX, iy - perpY },
            { ix + perpX, iy + perpY },
            { 1.0f, 0.0f, 0.0f, alpha });

        // Thicker line (offset)
        dd->ScreenLine(
            { ix - perpX + std::sin(rad) * 2.0f, iy - perpY - std::cos(rad) * 2.0f },
            { ix + perpX + std::sin(rad) * 2.0f, iy + perpY - std::cos(rad) * 2.0f },
            { 1.0f, 0.2f, 0.0f, alpha * 0.6f });
    }
}

// ---------------------------------------------------------------------------
// Score display (top-left)
// ---------------------------------------------------------------------------

void UIOverlayModule::DrawScoreDisplay()
{
    auto* dd = Spark::DebugDraw::GetInstance();
    if (!dd) return;

    char buf[64];
    std::snprintf(buf, sizeof(buf), "Score: %d", m_score);
    dd->ScreenText({ 20.0f, 10.0f }, buf, { 1.0f, 1.0f, 1.0f, 1.0f });

    std::snprintf(buf, sizeof(buf), "Kills: %d", m_kills);
    dd->ScreenText({ 20.0f, 30.0f }, buf, { 0.8f, 0.8f, 0.8f, 0.9f });
}

// ---------------------------------------------------------------------------
// Kill feed (top-right, below minimap)
// ---------------------------------------------------------------------------

void UIOverlayModule::DrawKillFeed()
{
    auto* dd = Spark::DebugDraw::GetInstance();
    if (!dd) return;

    float x = m_screenW - 300.0f;
    float y = 190.0f;

    for (auto& entry : m_killFeed)
    {
        float alpha = std::clamp(entry.timeRemaining / 1.0f, 0.0f, 1.0f);
        dd->ScreenText({ x, y }, entry.text.c_str(),
                        { 1.0f, 0.9f, 0.7f, alpha });
        y += 18.0f;
    }
}

// ---------------------------------------------------------------------------
// Notifications (right side)
// ---------------------------------------------------------------------------

void UIOverlayModule::DrawNotifications()
{
    auto* dd = Spark::DebugDraw::GetInstance();
    if (!dd) return;

    float x = m_screenW - 350.0f;
    float y = m_screenH - 200.0f;

    for (auto& n : m_notifications)
    {
        float fadeAlpha = std::min(1.0f, n.timeRemaining / 0.5f); // fade out in last 0.5s

        // Background
        float textW = static_cast<float>(n.text.size()) * 8.0f + 16.0f;
        dd->ScreenRect({ x, y }, { x + textW, y + 24.0f },
                        { 0.0f, 0.0f, 0.0f, 0.5f * fadeAlpha });

        dd->ScreenText({ x + 8.0f, y + 4.0f }, n.text.c_str(),
                        { n.r, n.g, n.b, fadeAlpha });
        y -= 30.0f;
    }
}

// ---------------------------------------------------------------------------
// Pause menu (centred overlay)
// ---------------------------------------------------------------------------

void UIOverlayModule::DrawPauseMenu()
{
    auto* dd = Spark::DebugDraw::GetInstance();
    if (!dd) return;

    // Dim background
    dd->ScreenRect({ 0.0f, 0.0f }, { m_screenW, m_screenH },
                    { 0.0f, 0.0f, 0.0f, 0.6f });

    float cx = m_screenW * 0.5f;
    float cy = m_screenH * 0.5f;

    // Menu box
    float boxW = 300.0f;
    float boxH = 200.0f;
    dd->ScreenRect({ cx - boxW * 0.5f, cy - boxH * 0.5f },
                    { cx + boxW * 0.5f, cy + boxH * 0.5f },
                    { 0.1f, 0.1f, 0.15f, 0.9f });

    // Title
    dd->ScreenText({ cx - 30.0f, cy - 70.0f }, "PAUSED",
                    { 1.0f, 1.0f, 1.0f, 1.0f });

    // Menu items
    const char* items[] = { "Resume", "Options", "Quit" };
    for (int i = 0; i < 3; ++i)
    {
        float itemY = cy - 20.0f + static_cast<float>(i) * 35.0f;
        bool selected = (i == m_pauseSelection);

        if (selected)
        {
            dd->ScreenRect({ cx - 80.0f, itemY - 2.0f },
                            { cx + 80.0f, itemY + 22.0f },
                            { 0.2f, 0.4f, 0.8f, 0.7f });
        }

        dd->ScreenText({ cx - 30.0f, itemY }, items[i],
                        { selected ? 1.0f : 0.7f,
                          selected ? 1.0f : 0.7f,
                          selected ? 1.0f : 0.7f,
                          1.0f });
    }
}

// ---------------------------------------------------------------------------
// Inventory grid (centred overlay)
// ---------------------------------------------------------------------------

void UIOverlayModule::DrawInventory()
{
    auto* dd = Spark::DebugDraw::GetInstance();
    if (!dd) return;

    // Semi-transparent background
    dd->ScreenRect({ 0.0f, 0.0f }, { m_screenW, m_screenH },
                    { 0.0f, 0.0f, 0.0f, 0.4f });

    float cellSize = 64.0f;
    float gridW = kInvCols * cellSize;
    float gridH = kInvRows * cellSize;
    float startX = (m_screenW - gridW) * 0.5f;
    float startY = (m_screenH - gridH) * 0.5f;

    // Title
    dd->ScreenText({ startX, startY - 25.0f }, "INVENTORY",
                    { 1.0f, 1.0f, 1.0f, 1.0f });

    for (int r = 0; r < kInvRows; ++r)
    {
        for (int c = 0; c < kInvCols; ++c)
        {
            float x = startX + static_cast<float>(c) * cellSize;
            float y = startY + static_cast<float>(r) * cellSize;

            // Cell background
            float bgAlpha = m_inventory[r][c].occupied ? 0.5f : 0.2f;
            dd->ScreenRect({ x, y }, { x + cellSize - 2.0f, y + cellSize - 2.0f },
                            { 0.15f, 0.15f, 0.2f, bgAlpha });

            // Cell border
            dd->ScreenLine({ x, y }, { x + cellSize - 2.0f, y },
                            { 0.4f, 0.4f, 0.5f, 0.8f });
            dd->ScreenLine({ x + cellSize - 2.0f, y },
                            { x + cellSize - 2.0f, y + cellSize - 2.0f },
                            { 0.4f, 0.4f, 0.5f, 0.8f });
            dd->ScreenLine({ x + cellSize - 2.0f, y + cellSize - 2.0f },
                            { x, y + cellSize - 2.0f },
                            { 0.4f, 0.4f, 0.5f, 0.8f });
            dd->ScreenLine({ x, y + cellSize - 2.0f }, { x, y },
                            { 0.4f, 0.4f, 0.5f, 0.8f });

            // Item name and count
            if (m_inventory[r][c].occupied)
            {
                dd->ScreenText({ x + 4.0f, y + 8.0f },
                                m_inventory[r][c].name.c_str(),
                                { 1.0f, 1.0f, 0.8f, 1.0f });

                if (m_inventory[r][c].count > 1)
                {
                    char buf[16];
                    std::snprintf(buf, sizeof(buf), "x%d", m_inventory[r][c].count);
                    dd->ScreenText({ x + 40.0f, y + 44.0f }, buf,
                                    { 0.7f, 0.7f, 0.7f, 1.0f });
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Compass / bearing indicator (top centre)
// ---------------------------------------------------------------------------

void UIOverlayModule::DrawCompass()
{
    auto* dd = Spark::DebugDraw::GetInstance();
    if (!dd) return;

    float cx = m_screenW * 0.5f;
    float y = 12.0f;
    float compassW = 300.0f;

    // Background bar
    dd->ScreenRect({ cx - compassW * 0.5f, y },
                    { cx + compassW * 0.5f, y + 20.0f },
                    { 0.0f, 0.0f, 0.0f, 0.5f });

    // Cardinal directions based on player yaw
    const char* cardinals[] = { "N", "NE", "E", "SE", "S", "SW", "W", "NW" };
    float cardinalAngles[] = { 0, 45, 90, 135, 180, 225, 270, 315 };

    for (int i = 0; i < 8; ++i)
    {
        float diff = cardinalAngles[i] - m_playerYaw;

        // Normalise to -180..180
        while (diff > 180.0f)  diff -= 360.0f;
        while (diff < -180.0f) diff += 360.0f;

        float screenOffset = diff * (compassW / 180.0f);
        float posX = cx + screenOffset;

        if (posX >= cx - compassW * 0.5f && posX <= cx + compassW * 0.5f)
        {
            bool isCardinal = (i % 2 == 0);
            dd->ScreenText({ posX - 4.0f, y + 3.0f }, cardinals[i],
                            { isCardinal ? 1.0f : 0.6f,
                              isCardinal ? 1.0f : 0.6f,
                              isCardinal ? 1.0f : 0.6f,
                              1.0f });
        }
    }

    // Centre marker
    dd->ScreenLine({ cx, y + 18.0f }, { cx, y + 24.0f },
                    { 1.0f, 0.3f, 0.3f, 1.0f });
}

// ---------------------------------------------------------------------------
// Interaction prompt (bottom centre)
// ---------------------------------------------------------------------------

void UIOverlayModule::DrawInteractionPrompt()
{
    auto* dd = Spark::DebugDraw::GetInstance();
    if (!dd) return;

    float cx = m_screenW * 0.5f;
    float y = m_screenH - 120.0f;

    float textW = static_cast<float>(m_interactText.size()) * 8.0f + 20.0f;

    dd->ScreenRect({ cx - textW * 0.5f, y },
                    { cx + textW * 0.5f, y + 28.0f },
                    { 0.0f, 0.0f, 0.0f, 0.6f });

    dd->ScreenText({ cx - textW * 0.5f + 10.0f, y + 6.0f },
                    m_interactText.c_str(),
                    { 1.0f, 1.0f, 0.5f, 1.0f });
}

// ---------------------------------------------------------------------------
// Boss health bar (top centre, large)
// ---------------------------------------------------------------------------

void UIOverlayModule::DrawBossHealthBar()
{
    auto* dd = Spark::DebugDraw::GetInstance();
    if (!dd) return;

    float cx = m_screenW * 0.5f;
    float barW = 400.0f;
    float barH = 20.0f;
    float barY = 50.0f;

    // Background
    dd->ScreenRect({ cx - barW * 0.5f, barY },
                    { cx + barW * 0.5f, barY + barH },
                    { 0.1f, 0.1f, 0.1f, 0.8f });

    // Fill
    float fillRatio = m_bossHealth / m_bossMaxHealth;
    float fillW = barW * fillRatio;
    dd->ScreenRect({ cx - barW * 0.5f, barY },
                    { cx - barW * 0.5f + fillW, barY + barH },
                    { 0.8f, 0.0f, 0.2f, 0.9f });

    // Boss name
    dd->ScreenText({ cx - 30.0f, barY - 18.0f }, "BOSS: Golem",
                    { 1.0f, 0.3f, 0.3f, 1.0f });

    // HP text
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.0f / %.0f", m_bossHealth, m_bossMaxHealth);
    dd->ScreenText({ cx - 40.0f, barY + 2.0f }, buf,
                    { 1.0f, 1.0f, 1.0f, 1.0f });
}

// ---------------------------------------------------------------------------
// Timer countdown (top-left, below score)
// ---------------------------------------------------------------------------

void UIOverlayModule::DrawTimer()
{
    auto* dd = Spark::DebugDraw::GetInstance();
    if (!dd) return;

    if (!m_timerActive) return;

    int totalSec = static_cast<int>(std::max(0.0f, m_timerSeconds));
    int min = totalSec / 60;
    int sec = totalSec % 60;

    char buf[32];
    std::snprintf(buf, sizeof(buf), "%02d:%02d", min, sec);

    // Flash red when time is low
    float r = (m_timerSeconds < 30.0f) ? 1.0f : 1.0f;
    float g = (m_timerSeconds < 30.0f) ? 0.3f : 1.0f;
    float b = (m_timerSeconds < 30.0f) ? 0.3f : 1.0f;

    dd->ScreenText({ 20.0f, 55.0f }, buf, { r, g, b, 1.0f });
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

void UIOverlayModule::AddNotification(const std::string& text, float duration,
                                       float r, float g, float b)
{
    if (static_cast<int>(m_notifications.size()) >= kMaxNotifications)
        m_notifications.pop_front();

    m_notifications.push_back({ text, duration, duration, r, g, b });
}

void UIOverlayModule::AddKillFeedEntry(const std::string& text)
{
    if (static_cast<int>(m_killFeed.size()) >= kMaxKillFeed)
        m_killFeed.pop_front();

    m_killFeed.push_back({ text, 5.0f });
}

void UIOverlayModule::SimulateDamageFrom(float worldAngle)
{
    m_damageIndicators.push_back({ worldAngle, 1.0f, 1.0f });
}

// ---------------------------------------------------------------------------

void UIOverlayModule::OnResize(int w, int h)
{
    m_screenW = static_cast<float>(w);
    m_screenH = static_cast<float>(h);
}

// ---------------------------------------------------------------------------
// DLL exports
// ---------------------------------------------------------------------------

SPARK_IMPLEMENT_MODULE(UIOverlayModule)
