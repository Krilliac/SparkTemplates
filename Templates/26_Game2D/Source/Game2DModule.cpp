/**
 * @file Game2DModule.cpp
 * @brief INTERMEDIATE — 2D engine features demo.
 */

#include "Game2DModule.h"
#include <Input/InputManager.h>
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <cstring>

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void Game2DModule::OnLoad(Spark::IEngineContext* ctx)
{
    m_ctx = ctx;
    BuildScene();
}

void Game2DModule::OnUnload()
{
    auto* world = m_ctx ? m_ctx->GetWorld() : nullptr;
    if (!world) { m_ctx = nullptr; return; }

    for (auto e : m_allEntities)
        if (e != entt::null) world->DestroyEntity(e);

    m_allEntities.clear();
    m_coins.clear();
    m_ctx = nullptr;
}

// ---------------------------------------------------------------------------
// Scene construction
// ---------------------------------------------------------------------------

void Game2DModule::BuildScene()
{
    auto* world = m_ctx->GetWorld();

    // --- Camera2D -----------------------------------------------------------
    m_cameraEntity = world->CreateEntity();
    world->AddComponent<NameComponent>(m_cameraEntity, NameComponent{ "MainCamera2D" });
    world->AddComponent<Transform>(m_cameraEntity, Transform{
        { 10.0f, 4.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }
    });

    Camera2D cam2d;
    cam2d.zoom = m_cameraZoom;
    cam2d.isActive = true;
    cam2d.boundsMin = { 0.0f, 0.0f };
    cam2d.boundsMax = { static_cast<float>(kMapWidth), static_cast<float>(kMapHeight) };
    world->AddComponent<Camera2D>(m_cameraEntity, cam2d);

    PixelPerfectComponent ppc;
    world->AddComponent<PixelPerfectComponent>(m_cameraEntity, ppc);
    m_allEntities.push_back(m_cameraEntity);

    CreateParallaxLayers();
    CreateTilemap();
    CreatePlayer();
    CreateCoins();
    CreateUIPanel();
}

// ---------------------------------------------------------------------------
// Parallax backgrounds
// ---------------------------------------------------------------------------

void Game2DModule::CreateParallaxLayers()
{
    auto* world = m_ctx->GetWorld();

    // Layer 0: Sky (slowest)
    m_bgSky = world->CreateEntity();
    world->AddComponent<NameComponent>(m_bgSky, NameComponent{ "BG_Sky" });
    world->AddComponent<Transform>(m_bgSky, Transform{
        { 0.0f, 0.0f, 10.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }
    });
    world->AddComponent<SpriteRenderer>(m_bgSky, SpriteRenderer{
        "Assets/Sprites/sky.png",
        { 0.5f, 0.6f, 0.9f, 1.0f }, // tint
        false, false, 0, 0, 0.5f, 0.5f
    });
    world->AddComponent<ParallaxBackground>(m_bgSky, ParallaxBackground{
        "Assets/Sprites/sky.png", 0.05f, 0.02f, true, true
    });
    m_allEntities.push_back(m_bgSky);

    // Layer 1: Mountains (medium)
    m_bgMountains = world->CreateEntity();
    world->AddComponent<NameComponent>(m_bgMountains, NameComponent{ "BG_Mountains" });
    world->AddComponent<Transform>(m_bgMountains, Transform{
        { 0.0f, 0.0f, 5.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }
    });
    world->AddComponent<SpriteRenderer>(m_bgMountains, SpriteRenderer{
        "Assets/Sprites/mountains.png",
        { 0.6f, 0.5f, 0.7f, 1.0f },
        false, false, 1, 0, 0.5f, 0.5f
    });
    world->AddComponent<ParallaxBackground>(m_bgMountains, ParallaxBackground{
        "Assets/Sprites/mountains.png", 0.2f, 0.05f, true, false
    });
    m_allEntities.push_back(m_bgMountains);

    // Layer 2: Trees (fastest parallax)
    m_bgTrees = world->CreateEntity();
    world->AddComponent<NameComponent>(m_bgTrees, NameComponent{ "BG_Trees" });
    world->AddComponent<Transform>(m_bgTrees, Transform{
        { 0.0f, 0.0f, 2.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }
    });
    world->AddComponent<SpriteRenderer>(m_bgTrees, SpriteRenderer{
        "Assets/Sprites/trees.png",
        { 0.2f, 0.6f, 0.3f, 1.0f },
        false, false, 2, 0, 0.5f, 0.5f
    });
    world->AddComponent<ParallaxBackground>(m_bgTrees, ParallaxBackground{
        "Assets/Sprites/trees.png", 0.5f, 0.1f, true, false
    });
    m_allEntities.push_back(m_bgTrees);
}

// ---------------------------------------------------------------------------
// Tilemap
// ---------------------------------------------------------------------------

void Game2DModule::CreateTilemap()
{
    auto* world = m_ctx->GetWorld();

    // Initialize tile data — 0=empty, 1=ground, 2=platform, 3=wall
    std::memset(m_tileData, 0, sizeof(m_tileData));

    // Ground floor (row 0)
    for (int x = 0; x < kMapWidth; ++x)
        m_tileData[0][x] = 1;

    // Some platforms
    // Platform at row 3, x=5..9
    for (int x = 5; x <= 9; ++x)
        m_tileData[3][x] = 2;

    // Platform at row 5, x=12..17
    for (int x = 12; x <= 17; ++x)
        m_tileData[5][x] = 2;

    // Platform at row 3, x=20..24
    for (int x = 20; x <= 24; ++x)
        m_tileData[3][x] = 2;

    // Platform at row 7, x=8..13
    for (int x = 8; x <= 13; ++x)
        m_tileData[7][x] = 2;

    // Platform at row 4, x=28..33
    for (int x = 28; x <= 33; ++x)
        m_tileData[4][x] = 2;

    // Walls at edges
    for (int y = 0; y < kMapHeight; ++y)
    {
        m_tileData[y][0] = 3;
        m_tileData[y][kMapWidth - 1] = 3;
    }

    // Some scattered ground bumps
    m_tileData[1][15] = 1;
    m_tileData[1][16] = 1;
    m_tileData[1][25] = 1;
    m_tileData[1][26] = 1;
    m_tileData[1][27] = 1;

    // Create tilemap entity
    m_tilemap = world->CreateEntity();
    world->AddComponent<NameComponent>(m_tilemap, NameComponent{ "Tilemap" });
    world->AddComponent<Transform>(m_tilemap, Transform{
        { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }
    });

    TilemapComponent tilemap;
    tilemap.tilesetPath = "Assets/Sprites/tileset.png";
    tilemap.tileSize = kTileSize;
    tilemap.width = kMapWidth;
    tilemap.height = kMapHeight;
    tilemap.tiles.resize(kMapHeight);
    for (int y = 0; y < kMapHeight; ++y)
    {
        tilemap.tiles[y].resize(kMapWidth);
        for (int x = 0; x < kMapWidth; ++x)
            tilemap.tiles[y][x] = m_tileData[y][x];
    }
    world->AddComponent<TilemapComponent>(m_tilemap, tilemap);
    m_allEntities.push_back(m_tilemap);

    // Create static colliders for all solid tiles
    for (int y = 0; y < kMapHeight; ++y)
    {
        for (int x = 0; x < kMapWidth; ++x)
        {
            if (m_tileData[y][x] == 0) continue;

            auto tile = world->CreateEntity();
            world->AddComponent<Transform>(tile, Transform{
                { static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f, 0.0f },
                { 0.0f, 0.0f, 0.0f },
                { 1.0f, 1.0f, 1.0f }
            });

            RigidBody2D rb;
            rb.type = RigidBody2D::BodyType::Static;
            rb.mass = 0.0f;
            rb.gravityScale = 0.0f;
            rb.linearDamping = 0.0f;
            world->AddComponent<RigidBody2D>(tile, rb);

            Collider2D col;
            col.shape = Collider2D::Shape::Box;
            col.size = { 1.0f, 1.0f };
            col.radius = 0.0f;
            col.isTrigger = false;
            world->AddComponent<Collider2D>(tile, col);

            m_allEntities.push_back(tile);
        }
    }
}

// ---------------------------------------------------------------------------
// Player
// ---------------------------------------------------------------------------

void Game2DModule::CreatePlayer()
{
    auto* world = m_ctx->GetWorld();

    m_player = world->CreateEntity();
    world->AddComponent<NameComponent>(m_player, NameComponent{ "Player" });
    world->AddComponent<Transform>(m_player, Transform{
        { 3.0f, 1.5f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }
    });

    // Sprite renderer
    SpriteRenderer sr;
    sr.texturePath = "Assets/Sprites/player_idle_0.png";
    sr.color = { 1.0f, 1.0f, 1.0f, 1.0f };
    sr.flipX = false;
    sr.flipY = false;
    sr.sortingLayer = 5;
    sr.orderInLayer = 0;
    sr.pivotX = 0.5f;
    sr.pivotY = 0.0f;
    world->AddComponent<SpriteRenderer>(m_player, sr);

    // Sprite animator — idle animation (2 frames)
    SpriteAnimator sa;
    sa.frames = {
        "Assets/Sprites/player_idle_0.png",
        "Assets/Sprites/player_idle_1.png"
    };
    sa.fps = 4.0f;
    sa.loop = true;
    sa.currentFrame = 0;
    sa.playing = true;
    world->AddComponent<SpriteAnimator>(m_player, sa);

    // 2D physics
    RigidBody2D rb;
    rb.type = RigidBody2D::BodyType::Dynamic;
    rb.mass = 1.0f;
    rb.gravityScale = 1.0f;
    rb.linearDamping = 0.5f;
    world->AddComponent<RigidBody2D>(m_player, rb);

    Collider2D col;
    col.shape = Collider2D::Shape::Box;
    col.size = { kPlayerHalfW * 2.0f, kPlayerHalfH * 2.0f };
    col.radius = 0.0f;
    col.isTrigger = false;
    world->AddComponent<Collider2D>(m_player, col);

    m_allEntities.push_back(m_player);
}

// ---------------------------------------------------------------------------
// Coins
// ---------------------------------------------------------------------------

void Game2DModule::CreateCoins()
{
    auto* world = m_ctx->GetWorld();

    struct CoinDef { float x, y; };
    CoinDef coinDefs[] = {
        { 6.5f,  4.5f },
        { 7.5f,  4.5f },
        { 8.5f,  4.5f },
        { 13.5f, 6.5f },
        { 14.5f, 6.5f },
        { 15.5f, 6.5f },
        { 21.5f, 4.5f },
        { 22.5f, 4.5f },
        { 10.5f, 8.5f },
        { 11.5f, 8.5f },
        { 30.5f, 5.5f },
        { 31.5f, 5.5f },
    };

    for (auto& def : coinDefs)
    {
        auto coin = world->CreateEntity();
        char name[32];
        std::snprintf(name, sizeof(name), "Coin_%d", m_totalCoins);
        world->AddComponent<NameComponent>(coin, NameComponent{ name });
        world->AddComponent<Transform>(coin, Transform{
            { def.x, def.y, 0.0f },
            { 0.0f, 0.0f, 0.0f },
            { 0.5f, 0.5f, 1.0f }
        });

        SpriteRenderer sr;
        sr.texturePath = "Assets/Sprites/coin_0.png";
        sr.color = { 1.0f, 0.85f, 0.2f, 1.0f };
        sr.flipX = false;
        sr.flipY = false;
        sr.sortingLayer = 4;
        sr.orderInLayer = 0;
        sr.pivotX = 0.5f;
        sr.pivotY = 0.5f;
        world->AddComponent<SpriteRenderer>(coin, sr);

        SpriteAnimator sa;
        sa.frames = {
            "Assets/Sprites/coin_0.png",
            "Assets/Sprites/coin_1.png",
            "Assets/Sprites/coin_2.png",
            "Assets/Sprites/coin_3.png"
        };
        sa.fps = 8.0f;
        sa.loop = true;
        sa.currentFrame = 0;
        sa.playing = true;
        world->AddComponent<SpriteAnimator>(coin, sa);

        Collider2D col;
        col.shape = Collider2D::Shape::Circle;
        col.size = { 0.5f, 0.5f };
        col.radius = 0.3f;
        col.isTrigger = true;
        world->AddComponent<Collider2D>(coin, col);

        m_allEntities.push_back(coin);
        m_coins.push_back({ coin, false });
        m_totalCoins++;
    }
}

// ---------------------------------------------------------------------------
// UI Panel (NineSlice)
// ---------------------------------------------------------------------------

void Game2DModule::CreateUIPanel()
{
    auto* world = m_ctx->GetWorld();

    m_uiPanel = world->CreateEntity();
    world->AddComponent<NameComponent>(m_uiPanel, NameComponent{ "ScorePanel" });
    world->AddComponent<Transform>(m_uiPanel, Transform{
        { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }
    });

    NineSliceSprite nss;
    nss.texturePath = "Assets/Sprites/panel_9slice.png";
    nss.borderLeft   = 8;
    nss.borderRight  = 8;
    nss.borderTop    = 8;
    nss.borderBottom = 8;
    world->AddComponent<NineSliceSprite>(m_uiPanel, nss);

    m_allEntities.push_back(m_uiPanel);
}

// ---------------------------------------------------------------------------
// Tile collision helpers
// ---------------------------------------------------------------------------

bool Game2DModule::IsTileSolid(int tileX, int tileY) const
{
    if (tileX < 0 || tileX >= kMapWidth || tileY < 0 || tileY >= kMapHeight)
        return true; // out of bounds is solid
    return m_tileData[tileY][tileX] != 0;
}

bool Game2DModule::CheckCollisionAt(float x, float y, float halfW, float halfH) const
{
    int minTX = static_cast<int>(std::floor(x - halfW));
    int maxTX = static_cast<int>(std::floor(x + halfW));
    int minTY = static_cast<int>(std::floor(y - halfH));
    int maxTY = static_cast<int>(std::floor(y + halfH));

    for (int ty = minTY; ty <= maxTY; ++ty)
        for (int tx = minTX; tx <= maxTX; ++tx)
            if (IsTileSolid(tx, ty))
                return true;

    return false;
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

void Game2DModule::OnUpdate(float dt)
{
    auto* input = m_ctx->GetInput();
    auto* world = m_ctx->GetWorld();
    if (!input || !world) return;

    m_time += dt;

    // Zoom controls
    if (input->IsKeyDown(VK_OEM_PLUS) || input->IsKeyDown(VK_ADD))
        m_cameraZoom = std::min(5.0f, m_cameraZoom + 1.0f * dt);
    if (input->IsKeyDown(VK_OEM_MINUS) || input->IsKeyDown(VK_SUBTRACT))
        m_cameraZoom = std::max(0.5f, m_cameraZoom - 1.0f * dt);

    auto& cam2d = world->GetComponent<Camera2D>(m_cameraEntity);
    cam2d.zoom = m_cameraZoom;

    UpdatePlayerMovement(dt);
    UpdatePhysics(dt);
    UpdatePlayerAnimation();
    UpdateCoinCollection();
    UpdateCamera(dt);
}

// ---------------------------------------------------------------------------
// Player movement input
// ---------------------------------------------------------------------------

void Game2DModule::UpdatePlayerMovement(float dt)
{
    auto* input = m_ctx->GetInput();

    // Horizontal movement
    m_playerVelX = 0.0f;
    if (input->IsKeyDown('A'))
    {
        m_playerVelX = -kMoveSpeed;
        m_facingRight = false;
    }
    if (input->IsKeyDown('D'))
    {
        m_playerVelX = kMoveSpeed;
        m_facingRight = true;
    }

    // Jump
    if (input->WasKeyPressed(VK_SPACE) && m_onGround)
    {
        m_playerVelY = kJumpForce;
        m_onGround = false;
    }
}

// ---------------------------------------------------------------------------
// Simple 2D physics (gravity + tile collision)
// ---------------------------------------------------------------------------

void Game2DModule::UpdatePhysics(float dt)
{
    auto* world = m_ctx->GetWorld();
    auto& xf = world->GetComponent<Transform>(m_player);

    // Apply gravity
    m_playerVelY += kGravity * dt;

    // Move horizontal
    float newX = xf.position.x + m_playerVelX * dt;
    if (!CheckCollisionAt(newX, xf.position.y, kPlayerHalfW, kPlayerHalfH))
    {
        xf.position.x = newX;
    }
    else
    {
        m_playerVelX = 0.0f;
    }

    // Move vertical
    float newY = xf.position.y + m_playerVelY * dt;
    if (!CheckCollisionAt(xf.position.x, newY, kPlayerHalfW, kPlayerHalfH))
    {
        xf.position.y = newY;
        m_onGround = false;
    }
    else
    {
        if (m_playerVelY < 0.0f)
            m_onGround = true;
        m_playerVelY = 0.0f;
    }

    // Update sprite flip
    auto& sr = world->GetComponent<SpriteRenderer>(m_player);
    sr.flipX = !m_facingRight;
}

// ---------------------------------------------------------------------------
// Player animation state machine
// ---------------------------------------------------------------------------

void Game2DModule::UpdatePlayerAnimation()
{
    auto* world = m_ctx->GetWorld();
    auto& sa = world->GetComponent<SpriteAnimator>(m_player);

    AnimState newState = AnimState::Idle;

    if (!m_onGround)
        newState = AnimState::Jump;
    else if (std::abs(m_playerVelX) > 0.1f)
        newState = AnimState::Walk;

    if (newState != m_animState)
    {
        m_animState = newState;
        sa.currentFrame = 0;

        switch (m_animState)
        {
        case AnimState::Idle:
            sa.frames = {
                "Assets/Sprites/player_idle_0.png",
                "Assets/Sprites/player_idle_1.png"
            };
            sa.fps = 4.0f;
            sa.loop = true;
            break;

        case AnimState::Walk:
            sa.frames = {
                "Assets/Sprites/player_walk_0.png",
                "Assets/Sprites/player_walk_1.png",
                "Assets/Sprites/player_walk_2.png",
                "Assets/Sprites/player_walk_3.png"
            };
            sa.fps = 10.0f;
            sa.loop = true;
            break;

        case AnimState::Jump:
            sa.frames = {
                "Assets/Sprites/player_jump_0.png"
            };
            sa.fps = 1.0f;
            sa.loop = false;
            break;
        }
    }
}

// ---------------------------------------------------------------------------
// Coin collection
// ---------------------------------------------------------------------------

void Game2DModule::UpdateCoinCollection()
{
    auto* world = m_ctx->GetWorld();
    auto& playerXf = world->GetComponent<Transform>(m_player);

    float collectRadius = 0.8f;

    for (auto& coin : m_coins)
    {
        if (coin.collected || coin.entity == entt::null) continue;

        auto& coinXf = world->GetComponent<Transform>(coin.entity);
        float dx = coinXf.position.x - playerXf.position.x;
        float dy = coinXf.position.y - playerXf.position.y;
        float distSq = dx * dx + dy * dy;

        if (distSq < collectRadius * collectRadius)
        {
            coin.collected = true;
            m_score += 10;

            // Hide the coin (move it far away)
            coinXf.position.x = -1000.0f;
            coinXf.position.y = -1000.0f;

            auto& coinSr = world->GetComponent<SpriteRenderer>(coin.entity);
            coinSr.color.w = 0.0f; // invisible
        }
    }
}

// ---------------------------------------------------------------------------
// Camera follow
// ---------------------------------------------------------------------------

void Game2DModule::UpdateCamera(float dt)
{
    auto* world = m_ctx->GetWorld();
    auto& playerXf = world->GetComponent<Transform>(m_player);
    auto& camXf    = world->GetComponent<Transform>(m_cameraEntity);
    auto& cam2d    = world->GetComponent<Camera2D>(m_cameraEntity);

    // Smooth follow
    float t = 1.0f - std::exp(-5.0f * dt);
    camXf.position.x += (playerXf.position.x - camXf.position.x) * t;
    camXf.position.y += (playerXf.position.y + 2.0f - camXf.position.y) * t;

    // Clamp to bounds
    float halfViewW = (m_screenW / (m_cameraZoom * static_cast<float>(kTileSize))) * 0.5f;
    float halfViewH = (m_screenH / (m_cameraZoom * static_cast<float>(kTileSize))) * 0.5f;

    camXf.position.x = std::clamp(camXf.position.x, cam2d.boundsMin.x + halfViewW,
                                    cam2d.boundsMax.x - halfViewW);
    camXf.position.y = std::clamp(camXf.position.y, cam2d.boundsMin.y + halfViewH,
                                    cam2d.boundsMax.y - halfViewH);
}

// ---------------------------------------------------------------------------
// Render
// ---------------------------------------------------------------------------

void Game2DModule::OnRender()
{
    auto* dd = Spark::DebugDraw::GetInstance();
    if (!dd) return;

    DrawScorePanel();
    DrawHUD();
}

// ---------------------------------------------------------------------------
// Score panel (NineSlice-style drawn with DebugDraw)
// ---------------------------------------------------------------------------

void Game2DModule::DrawScorePanel()
{
    auto* dd = Spark::DebugDraw::GetInstance();
    if (!dd) return;

    float panelX = 10.0f;
    float panelY = 10.0f;
    float panelW = 180.0f;
    float panelH = 60.0f;

    // Panel background
    dd->ScreenRect({ panelX, panelY }, { panelX + panelW, panelY + panelH },
                    { 0.1f, 0.1f, 0.15f, 0.85f });

    // Panel border
    dd->ScreenLine({ panelX, panelY }, { panelX + panelW, panelY },
                    { 0.5f, 0.4f, 0.2f, 1.0f });
    dd->ScreenLine({ panelX + panelW, panelY }, { panelX + panelW, panelY + panelH },
                    { 0.5f, 0.4f, 0.2f, 1.0f });
    dd->ScreenLine({ panelX + panelW, panelY + panelH }, { panelX, panelY + panelH },
                    { 0.5f, 0.4f, 0.2f, 1.0f });
    dd->ScreenLine({ panelX, panelY + panelH }, { panelX, panelY },
                    { 0.5f, 0.4f, 0.2f, 1.0f });

    // Score text
    char buf[64];
    std::snprintf(buf, sizeof(buf), "Score: %d", m_score);
    dd->ScreenText({ panelX + 10.0f, panelY + 10.0f }, buf,
                    { 1.0f, 0.85f, 0.2f, 1.0f });

    // Coins collected
    int collected = 0;
    for (auto& c : m_coins)
        if (c.collected) collected++;

    std::snprintf(buf, sizeof(buf), "Coins: %d / %d", collected, m_totalCoins);
    dd->ScreenText({ panelX + 10.0f, panelY + 32.0f }, buf,
                    { 1.0f, 1.0f, 1.0f, 0.9f });
}

// ---------------------------------------------------------------------------
// HUD overlay
// ---------------------------------------------------------------------------

void Game2DModule::DrawHUD()
{
    auto* dd = Spark::DebugDraw::GetInstance();
    auto* world = m_ctx->GetWorld();
    if (!dd || !world) return;

    // Player position info
    auto& playerXf = world->GetComponent<Transform>(m_player);
    char buf[128];

    float infoY = m_screenH - 60.0f;
    std::snprintf(buf, sizeof(buf), "Pos: (%.1f, %.1f)  Vel: (%.1f, %.1f)",
                  playerXf.position.x, playerXf.position.y,
                  m_playerVelX, m_playerVelY);
    dd->ScreenText({ 10.0f, infoY }, buf, { 0.6f, 0.6f, 0.6f, 0.8f });

    std::snprintf(buf, sizeof(buf), "Ground: %s  Zoom: %.1fx",
                  m_onGround ? "Yes" : "No", m_cameraZoom);
    dd->ScreenText({ 10.0f, infoY + 16.0f }, buf, { 0.6f, 0.6f, 0.6f, 0.8f });

    // Controls
    dd->ScreenText({ m_screenW - 350.0f, m_screenH - 30.0f },
                    "A/D: Move | Space: Jump | +/-: Zoom",
                    { 0.5f, 0.5f, 0.5f, 0.7f });

    // Animation state
    const char* animName = "Idle";
    if (m_animState == AnimState::Walk) animName = "Walk";
    else if (m_animState == AnimState::Jump) animName = "Jump";
    std::snprintf(buf, sizeof(buf), "Anim: %s", animName);
    dd->ScreenText({ 10.0f, infoY + 32.0f }, buf, { 0.6f, 0.6f, 0.6f, 0.8f });
}

// ---------------------------------------------------------------------------
// Resize
// ---------------------------------------------------------------------------

void Game2DModule::OnResize(int w, int h)
{
    m_screenW = static_cast<float>(w);
    m_screenH = static_cast<float>(h);
}

// ---------------------------------------------------------------------------
// DLL exports
// ---------------------------------------------------------------------------

SPARK_IMPLEMENT_MODULE(Game2DModule)
