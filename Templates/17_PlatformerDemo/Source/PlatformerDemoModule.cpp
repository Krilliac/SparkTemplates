/**
 * @file PlatformerDemoModule.cpp
 * @brief INTERMEDIATE — 2.5D platformer with physics, coins, enemies, and checkpoints.
 */

#include "PlatformerDemoModule.h"
#include <Input/InputManager.h>
#include <cmath>
#include <algorithm>

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void PlatformerDemoModule::OnLoad(Spark::IEngineContext* ctx)
{
    m_ctx = ctx;

    // --- Subscribe to custom game events ------------------------------------
    if (auto* bus = m_ctx->GetEventBus())
    {
        m_coinSub = bus->Subscribe<CoinCollectedEvent>(
            [this](const CoinCollectedEvent& e) {
                m_score += e.coinValue;
                m_coinsTotal++;
                // Extra life every 10 coins
                if (m_coinsTotal % 10 == 0)
                    m_lives++;
            });

        m_diedSub = bus->Subscribe<PlayerDiedEvent>(
            [this](const PlayerDiedEvent& /*e*/) {
                if (m_lives > 0)
                    RespawnPlayer();
                // else: game over state could be triggered here
            });

        m_checkpointSub = bus->Subscribe<CheckpointReachedEvent>(
            [this](const CheckpointReachedEvent& e) {
                if (e.checkpointIndex > m_lastCheckpoint)
                {
                    m_lastCheckpoint = e.checkpointIndex;
                    // Store checkpoint position as new spawn point
                    auto* world = m_ctx->GetWorld();
                    auto& cpXf = world->GetComponent<Transform>(e.checkpoint);
                    m_spawnX = cpXf.position.x;
                    m_spawnY = cpXf.position.y + 2.0f;
                    m_spawnZ = cpXf.position.z;
                }
            });
    }

    BuildLevel();
}

void PlatformerDemoModule::OnUnload()
{
    // Unsubscribe events
    if (auto* bus = m_ctx ? m_ctx->GetEventBus() : nullptr)
    {
        bus->Unsubscribe<CoinCollectedEvent>(m_coinSub);
        bus->Unsubscribe<PlayerDiedEvent>(m_diedSub);
        bus->Unsubscribe<CheckpointReachedEvent>(m_checkpointSub);
    }

    auto* world = m_ctx ? m_ctx->GetWorld() : nullptr;
    if (!world) { m_ctx = nullptr; return; }

    // Destroy all entities
    for (auto e : { m_player, m_camera, m_sunLight, m_deathZone })
        if (e != entt::null) world->DestroyEntity(e);

    for (auto e : m_platforms)
        if (e != entt::null) world->DestroyEntity(e);
    for (auto& mp : m_movingPlatforms)
        if (mp.entity != entt::null) world->DestroyEntity(mp.entity);
    for (auto e : m_coins)
        if (e != entt::null) world->DestroyEntity(e);
    for (auto& ed : m_enemies)
        if (ed.entity != entt::null) world->DestroyEntity(ed.entity);
    for (auto e : m_jumpPads)
        if (e != entt::null) world->DestroyEntity(e);
    for (auto e : m_checkpoints)
        if (e != entt::null) world->DestroyEntity(e);

    m_platforms.clear();
    m_movingPlatforms.clear();
    m_coins.clear();
    m_enemies.clear();
    m_jumpPads.clear();
    m_checkpoints.clear();

    m_ctx = nullptr;
}

// ---------------------------------------------------------------------------
// Level construction
// ---------------------------------------------------------------------------

void PlatformerDemoModule::BuildLevel()
{
    auto* world = m_ctx->GetWorld();

    // --- Player entity ------------------------------------------------------
    m_player = world->CreateEntity();
    world->AddComponent<NameComponent>(m_player, NameComponent{ "Player" });
    world->AddComponent<Transform>(m_player, Transform{
        { m_spawnX, m_spawnY, m_spawnZ },
        { 0.0f, 0.0f, 0.0f },
        { 0.8f, 1.6f, 0.8f }
    });
    world->AddComponent<MeshRenderer>(m_player, MeshRenderer{
        "Assets/Models/cube.obj", "Assets/Materials/player.mat",
        true, true, true
    });
    world->AddComponent<HealthComponent>(m_player, HealthComponent{
        3.0f, 3.0f, false, false   // lives stored as health
    });
    world->AddComponent<RigidBodyComponent>(m_player, RigidBodyComponent{
        RigidBodyComponent::Type::Dynamic, 1.0f, 0.5f, 0.0f
    });
    world->AddComponent<ColliderComponent>(m_player, ColliderComponent{
        ColliderComponent::Shape::Box, { 0.4f, 0.8f, 0.4f }
    });
    world->AddComponent<TagComponent>(m_player);
    world->GetComponent<TagComponent>(m_player).AddTag("player");

    // --- Camera -------------------------------------------------------------
    m_camera = world->CreateEntity();
    world->AddComponent<NameComponent>(m_camera, NameComponent{ "MainCamera" });
    world->AddComponent<Transform>(m_camera, Transform{
        { m_spawnX, m_spawnY + kCamOffsetY, kCamOffsetZ },
        { -15.0f, 0.0f, 0.0f },
        { 1.0f, 1.0f, 1.0f }
    });
    world->AddComponent<Camera>(m_camera, Camera{
        60.0f, 0.1f, 500.0f, true
    });

    // --- Sun light ----------------------------------------------------------
    m_sunLight = world->CreateEntity();
    world->AddComponent<NameComponent>(m_sunLight, NameComponent{ "SunLight" });
    world->AddComponent<Transform>(m_sunLight, Transform{
        { 0.0f, 30.0f, -10.0f },
        { -45.0f, 30.0f, 0.0f },
        { 1.0f, 1.0f, 1.0f }
    });
    world->AddComponent<LightComponent>(m_sunLight, LightComponent{
        LightComponent::Type::Directional,
        { 1.0f, 0.95f, 0.85f }, 2.0f, 0.0f,
        0.0f, 0.0f, true, 2048
    });

    // --- Static platforms (the level geometry) -------------------------------
    // Ground / starting area
    SpawnPlatform("Ground",      0.0f,  0.0f,  0.0f,  12.0f, 1.0f, 4.0f);
    SpawnPlatform("Platform_1",  8.0f,  2.0f,  0.0f,   4.0f, 0.5f, 4.0f);
    SpawnPlatform("Platform_2", 14.0f,  4.0f,  0.0f,   3.0f, 0.5f, 4.0f);
    SpawnPlatform("Platform_3", 20.0f,  6.0f,  0.0f,   5.0f, 0.5f, 4.0f);
    SpawnPlatform("Platform_4", 28.0f,  4.0f,  0.0f,   3.0f, 0.5f, 4.0f);
    SpawnPlatform("Platform_5", 35.0f,  7.0f,  0.0f,   6.0f, 0.5f, 4.0f);
    SpawnPlatform("Platform_6", 44.0f,  5.0f,  0.0f,   4.0f, 0.5f, 4.0f);
    SpawnPlatform("Ledge_High", 50.0f, 10.0f,  0.0f,   3.0f, 0.5f, 4.0f);
    SpawnPlatform("EndPlatform",58.0f,  8.0f,  0.0f,   6.0f, 1.0f, 4.0f);

    // --- Moving platforms ---------------------------------------------------
    SpawnMovingPlatform("MovPlat_V1", 24.0f, 2.0f, 0.0f,
                        3.0f, 0.4f, 4.0f,
                        0.0f, 4.0f, 0.5f, 0.0f);   // vertical oscillation

    SpawnMovingPlatform("MovPlat_H1", 40.0f, 8.0f, 0.0f,
                        3.0f, 0.4f, 4.0f,
                        5.0f, 0.0f, 0.3f, 1.57f);   // horizontal oscillation

    SpawnMovingPlatform("MovPlat_V2", 53.0f, 6.0f, 0.0f,
                        2.5f, 0.4f, 4.0f,
                        0.0f, 3.0f, 0.7f, 0.5f);

    // --- Coins (placed above platforms) -------------------------------------
    SpawnCoin( 4.0f,  2.0f, 0.0f);
    SpawnCoin( 8.0f,  4.0f, 0.0f);
    SpawnCoin(14.0f,  6.0f, 0.0f);
    SpawnCoin(20.0f,  8.0f, 0.0f);
    SpawnCoin(22.0f,  8.5f, 0.0f);
    SpawnCoin(35.0f,  9.0f, 0.0f);
    SpawnCoin(36.0f,  9.0f, 0.0f);
    SpawnCoin(37.0f,  9.0f, 0.0f);
    SpawnCoin(50.0f, 12.0f, 0.0f);
    SpawnCoin(58.0f, 10.0f, 0.0f);

    // --- Patrol enemies -----------------------------------------------------
    SpawnEnemy("Goomba_1", 20.0f, 7.0f, 18.0f, 24.0f, 2.0f);
    SpawnEnemy("Goomba_2", 35.0f, 8.0f, 33.0f, 40.0f, 3.0f);
    SpawnEnemy("Goomba_3", 58.0f, 9.5f, 55.0f, 63.0f, 1.5f);

    // --- Jump pads ----------------------------------------------------------
    SpawnJumpPad(28.0f, 4.5f, 0.0f, 18.0f);
    SpawnJumpPad(44.0f, 5.5f, 0.0f, 22.0f);

    // --- Checkpoints --------------------------------------------------------
    SpawnCheckpoint(20.0f, 7.0f, 0.0f, 0);
    SpawnCheckpoint(44.0f, 6.0f, 0.0f, 1);

    // --- Death zone (invisible plane below the level) -----------------------
    SpawnDeathZone(kDeathY);
}

void PlatformerDemoModule::SpawnPlatform(const char* name,
                                          float x, float y, float z,
                                          float sx, float sy, float sz)
{
    auto* world = m_ctx->GetWorld();
    auto e = world->CreateEntity();

    world->AddComponent<NameComponent>(e, NameComponent{ name });
    world->AddComponent<Transform>(e, Transform{
        { x, y, z }, { 0.0f, 0.0f, 0.0f }, { sx, sy, sz }
    });
    world->AddComponent<MeshRenderer>(e, MeshRenderer{
        "Assets/Models/cube.obj", "Assets/Materials/concrete.mat",
        true, true, true
    });
    world->AddComponent<RigidBodyComponent>(e, RigidBodyComponent{
        RigidBodyComponent::Type::Static, 0.0f, 0.8f, 0.1f
    });
    world->AddComponent<ColliderComponent>(e, ColliderComponent{
        ColliderComponent::Shape::Box, { sx * 0.5f, sy * 0.5f, sz * 0.5f }
    });

    m_platforms.push_back(e);
}

void PlatformerDemoModule::SpawnMovingPlatform(const char* name,
                                                float x, float y, float z,
                                                float sx, float sy, float sz,
                                                float ampX, float ampY,
                                                float freq, float phase)
{
    auto* world = m_ctx->GetWorld();
    auto e = world->CreateEntity();

    world->AddComponent<NameComponent>(e, NameComponent{ name });
    world->AddComponent<Transform>(e, Transform{
        { x, y, z }, { 0.0f, 0.0f, 0.0f }, { sx, sy, sz }
    });
    world->AddComponent<MeshRenderer>(e, MeshRenderer{
        "Assets/Models/cube.obj", "Assets/Materials/metal.mat",
        true, true, true
    });
    world->AddComponent<RigidBodyComponent>(e, RigidBodyComponent{
        RigidBodyComponent::Type::Kinematic, 0.0f, 1.0f, 0.0f
    });
    world->AddComponent<ColliderComponent>(e, ColliderComponent{
        ColliderComponent::Shape::Box, { sx * 0.5f, sy * 0.5f, sz * 0.5f }
    });
    world->AddComponent<TagComponent>(e);
    world->GetComponent<TagComponent>(e).AddTag("moving_platform");

    m_movingPlatforms.push_back({ e, y, x, ampX, ampY, freq, phase });
}

void PlatformerDemoModule::SpawnCoin(float x, float y, float z)
{
    auto* world = m_ctx->GetWorld();
    auto e = world->CreateEntity();

    std::string coinName = "Coin_" + std::to_string(m_coins.size());
    world->AddComponent<NameComponent>(e, NameComponent{ coinName });
    world->AddComponent<Transform>(e, Transform{
        { x, y, z }, { 0.0f, 0.0f, 0.0f }, { 0.4f, 0.4f, 0.1f }
    });
    world->AddComponent<MeshRenderer>(e, MeshRenderer{
        "Assets/Models/sphere.obj", "Assets/Materials/gold.mat",
        false, false, true
    });
    world->AddComponent<TagComponent>(e);
    world->GetComponent<TagComponent>(e).AddTag("coin");
    world->AddComponent<ActiveComponent>(e, ActiveComponent{ true });

    m_coins.push_back(e);
}

void PlatformerDemoModule::SpawnEnemy(const char* name, float x, float y,
                                       float leftBound, float rightBound,
                                       float speed)
{
    auto* world = m_ctx->GetWorld();
    auto e = world->CreateEntity();

    world->AddComponent<NameComponent>(e, NameComponent{ name });
    world->AddComponent<Transform>(e, Transform{
        { x, y, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.8f, 0.8f, 0.8f }
    });
    world->AddComponent<MeshRenderer>(e, MeshRenderer{
        "Assets/Models/cube.obj", "Assets/Materials/enemy.mat",
        true, true, true
    });
    world->AddComponent<HealthComponent>(e, HealthComponent{
        1.0f, 1.0f, false, false
    });
    world->AddComponent<RigidBodyComponent>(e, RigidBodyComponent{
        RigidBodyComponent::Type::Kinematic, 1.0f, 0.5f, 0.0f
    });
    world->AddComponent<ColliderComponent>(e, ColliderComponent{
        ColliderComponent::Shape::Box, { 0.4f, 0.4f, 0.4f }
    });
    world->AddComponent<TagComponent>(e);
    world->GetComponent<TagComponent>(e).AddTag("enemy");
    world->AddComponent<AIComponent>(e, AIComponent{
        AIComponent::State::Patrolling,
        "patrol_simple",
        m_player,
        { 0.0f, 0.0f, 0.0f },
        {},
        { 5.0f, 1.0f, speed, 0.5f, 0.5f }
    });

    m_enemies.push_back({ e, leftBound, rightBound, speed, true });
}

void PlatformerDemoModule::SpawnJumpPad(float x, float y, float z,
                                         float launchForce)
{
    auto* world = m_ctx->GetWorld();
    auto e = world->CreateEntity();

    std::string padName = "JumpPad_" + std::to_string(m_jumpPads.size());
    world->AddComponent<NameComponent>(e, NameComponent{ padName });
    world->AddComponent<Transform>(e, Transform{
        { x, y, z }, { 0.0f, 0.0f, 0.0f }, { 1.5f, 0.3f, 1.5f }
    });
    world->AddComponent<MeshRenderer>(e, MeshRenderer{
        "Assets/Models/cube.obj", "Assets/Materials/jumppad.mat",
        false, true, true
    });
    world->AddComponent<TagComponent>(e);
    world->GetComponent<TagComponent>(e).AddTag("jumppad");
    // Store launch force in the tag for retrieval
    world->GetComponent<TagComponent>(e).AddTag("force_" + std::to_string(static_cast<int>(launchForce)));
    world->AddComponent<ColliderComponent>(e, ColliderComponent{
        ColliderComponent::Shape::Box, { 0.75f, 0.15f, 0.75f }
    });

    m_jumpPads.push_back(e);
}

void PlatformerDemoModule::SpawnCheckpoint(float x, float y, float z, int index)
{
    auto* world = m_ctx->GetWorld();
    auto e = world->CreateEntity();

    std::string cpName = "Checkpoint_" + std::to_string(index);
    world->AddComponent<NameComponent>(e, NameComponent{ cpName });
    world->AddComponent<Transform>(e, Transform{
        { x, y, z }, { 0.0f, 0.0f, 0.0f }, { 0.5f, 3.0f, 0.5f }
    });
    world->AddComponent<MeshRenderer>(e, MeshRenderer{
        "Assets/Models/cube.obj", "Assets/Materials/checkpoint.mat",
        false, false, true
    });
    world->AddComponent<TagComponent>(e);
    world->GetComponent<TagComponent>(e).AddTag("checkpoint");
    world->GetComponent<TagComponent>(e).AddTag("cp_" + std::to_string(index));
    world->AddComponent<ActiveComponent>(e, ActiveComponent{ true });

    m_checkpoints.push_back(e);
}

void PlatformerDemoModule::SpawnDeathZone(float y)
{
    auto* world = m_ctx->GetWorld();
    m_deathZone = world->CreateEntity();

    world->AddComponent<NameComponent>(m_deathZone, NameComponent{ "DeathZone" });
    world->AddComponent<Transform>(m_deathZone, Transform{
        { 0.0f, y, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 200.0f, 1.0f, 200.0f }
    });
    world->AddComponent<ColliderComponent>(m_deathZone, ColliderComponent{
        ColliderComponent::Shape::Box, { 100.0f, 0.5f, 100.0f }
    });
    world->AddComponent<TagComponent>(m_deathZone);
    world->GetComponent<TagComponent>(m_deathZone).AddTag("death_zone");
}

// ---------------------------------------------------------------------------
// Per-frame update
// ---------------------------------------------------------------------------

void PlatformerDemoModule::OnUpdate(float dt)
{
    auto* input = m_ctx->GetInput();
    auto* world = m_ctx->GetWorld();
    if (!input || !world) return;

    m_time += dt;

    // --- Restart level (R) ---
    if (input->WasKeyPressed('R'))
    {
        RestartLevel();
        return;
    }

    // --- Player physics and input ---
    UpdatePlayerPhysics(dt);

    // --- Animated elements ---
    UpdateMovingPlatforms(dt);
    UpdateEnemyPatrol(dt);
    UpdateCoinSpin(dt);

    // --- Collision / trigger checks ---
    CheckCoinPickup();
    CheckDeathZone();
    CheckCheckpoints();
    CheckJumpPads();

    // --- Camera ---
    UpdateCameraFollow(dt);
}

// ---------------------------------------------------------------------------
// Player physics (gravity, jump, double-jump, horizontal movement)
// ---------------------------------------------------------------------------

void PlatformerDemoModule::UpdatePlayerPhysics(float dt)
{
    auto* input = m_ctx->GetInput();
    auto* world = m_ctx->GetWorld();
    auto& xf = world->GetComponent<Transform>(m_player);

    // Horizontal movement (A/D)
    float moveDir = 0.0f;
    if (input->IsKeyDown('A')) moveDir -= 1.0f;
    if (input->IsKeyDown('D')) moveDir += 1.0f;
    xf.position.x += moveDir * kMoveSpeed * dt;

    // --- Simple ground check: cast downward from player feet ---
    // For this template we use a naive Y threshold approach against platforms
    m_grounded = false;
    float playerBottom = xf.position.y - 0.8f; // half height

    // Check static platforms
    for (auto platE : m_platforms)
    {
        if (platE == entt::null) continue;
        auto& pXf = world->GetComponent<Transform>(platE);
        float platTop = pXf.position.y + pXf.scale.y * 0.5f;
        float halfW   = pXf.scale.x * 0.5f;

        if (xf.position.x >= pXf.position.x - halfW &&
            xf.position.x <= pXf.position.x + halfW &&
            playerBottom <= platTop + 0.15f &&
            playerBottom >= platTop - 0.3f &&
            m_velocityY <= 0.0f)
        {
            m_grounded = true;
            xf.position.y = platTop + 0.8f;
            m_velocityY = 0.0f;
            break;
        }
    }

    // Check moving platforms
    if (!m_grounded)
    {
        for (auto& mp : m_movingPlatforms)
        {
            if (mp.entity == entt::null) continue;
            auto& pXf = world->GetComponent<Transform>(mp.entity);
            float platTop = pXf.position.y + pXf.scale.y * 0.5f;
            float halfW   = pXf.scale.x * 0.5f;

            if (xf.position.x >= pXf.position.x - halfW &&
                xf.position.x <= pXf.position.x + halfW &&
                playerBottom <= platTop + 0.15f &&
                playerBottom >= platTop - 0.3f &&
                m_velocityY <= 0.0f)
            {
                m_grounded = true;
                xf.position.y = platTop + 0.8f;
                m_velocityY = 0.0f;
                break;
            }
        }
    }

    // Coyote time (brief grace after walking off an edge)
    if (m_grounded)
    {
        m_coyoteTimer = kCoyoteTime;
        m_hasDoubleJump = true;
    }
    else
    {
        m_coyoteTimer -= dt;
    }

    // Jump (Space)
    if (input->WasKeyPressed(VK_SPACE))
    {
        if (m_grounded || m_coyoteTimer > 0.0f)
        {
            // Normal jump
            m_velocityY = kJumpForce;
            m_grounded = false;
            m_coyoteTimer = 0.0f;
        }
        else if (m_hasDoubleJump)
        {
            // Double jump
            m_velocityY = kJumpForce * 0.85f;
            m_hasDoubleJump = false;
        }
    }

    // Apply gravity
    if (!m_grounded)
    {
        m_velocityY += kGravity * dt;
        xf.position.y += m_velocityY * dt;
    }
}

// ---------------------------------------------------------------------------
// Moving platforms (sine-wave oscillation)
// ---------------------------------------------------------------------------

void PlatformerDemoModule::UpdateMovingPlatforms(float dt)
{
    (void)dt;
    auto* world = m_ctx->GetWorld();

    for (auto& mp : m_movingPlatforms)
    {
        if (mp.entity == entt::null) continue;
        auto& xf = world->GetComponent<Transform>(mp.entity);

        float t = m_time * mp.freq * 2.0f * 3.14159f + mp.phase;
        xf.position.x = mp.baseX + std::sin(t) * mp.ampX;
        xf.position.y = mp.baseY + std::sin(t) * mp.ampY;
    }
}

// ---------------------------------------------------------------------------
// Enemy patrol AI (simple back-and-forth)
// ---------------------------------------------------------------------------

void PlatformerDemoModule::UpdateEnemyPatrol(float dt)
{
    auto* world = m_ctx->GetWorld();

    for (auto& ed : m_enemies)
    {
        if (ed.entity == entt::null) continue;
        auto& xf = world->GetComponent<Transform>(ed.entity);

        if (ed.movingRight)
        {
            xf.position.x += ed.speed * dt;
            if (xf.position.x >= ed.rightBound) ed.movingRight = false;
        }
        else
        {
            xf.position.x -= ed.speed * dt;
            if (xf.position.x <= ed.leftBound) ed.movingRight = true;
        }
    }
}

// ---------------------------------------------------------------------------
// Coin spin animation
// ---------------------------------------------------------------------------

void PlatformerDemoModule::UpdateCoinSpin(float dt)
{
    (void)dt;
    auto* world = m_ctx->GetWorld();

    for (auto e : m_coins)
    {
        if (e == entt::null) continue;
        if (!world->HasComponent<ActiveComponent>(e)) continue;
        auto& active = world->GetComponent<ActiveComponent>(e);
        if (!active.isActive) continue;

        auto& xf = world->GetComponent<Transform>(e);
        xf.rotation.y = std::fmod(m_time * 180.0f, 360.0f); // spin around Y
        // Gentle bob up and down
        xf.position.y += std::sin(m_time * 3.0f) * 0.003f;
    }
}

// ---------------------------------------------------------------------------
// Camera follow with smooth damping
// ---------------------------------------------------------------------------

void PlatformerDemoModule::UpdateCameraFollow(float dt)
{
    auto* world = m_ctx->GetWorld();
    auto& playerXf = world->GetComponent<Transform>(m_player);
    auto& camXf    = world->GetComponent<Transform>(m_camera);

    float targetX = playerXf.position.x;
    float targetY = playerXf.position.y + kCamOffsetY;
    float targetZ = kCamOffsetZ;

    float t = 1.0f - std::exp(-kCamDamping * dt);
    camXf.position.x += (targetX - camXf.position.x) * t;
    camXf.position.y += (targetY - camXf.position.y) * t;
    camXf.position.z += (targetZ - camXf.position.z) * t;
}

// ---------------------------------------------------------------------------
// Coin pickup check
// ---------------------------------------------------------------------------

void PlatformerDemoModule::CheckCoinPickup()
{
    auto* world = m_ctx->GetWorld();
    auto& playerXf = world->GetComponent<Transform>(m_player);

    for (auto it = m_coins.begin(); it != m_coins.end(); )
    {
        auto e = *it;
        if (e == entt::null)
        {
            it = m_coins.erase(it);
            continue;
        }

        auto& coinXf = world->GetComponent<Transform>(e);
        float dx = coinXf.position.x - playerXf.position.x;
        float dy = coinXf.position.y - playerXf.position.y;
        float distSq = dx * dx + dy * dy;

        if (distSq <= kPickupRadius * kPickupRadius)
        {
            // Publish coin event
            if (auto* bus = m_ctx->GetEventBus())
                bus->Publish(CoinCollectedEvent{ m_player, e, 100 });

            world->DestroyEntity(e);
            it = m_coins.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

// ---------------------------------------------------------------------------
// Death zone check (fell off the map)
// ---------------------------------------------------------------------------

void PlatformerDemoModule::CheckDeathZone()
{
    auto* world = m_ctx->GetWorld();
    auto& playerXf = world->GetComponent<Transform>(m_player);

    if (playerXf.position.y < kDeathY)
    {
        m_lives--;
        if (auto* bus = m_ctx->GetEventBus())
            bus->Publish(PlayerDiedEvent{ m_player, m_lives });

        if (m_lives <= 0)
            RestartLevel();
        else
            RespawnPlayer();
    }
}

// ---------------------------------------------------------------------------
// Checkpoint proximity check
// ---------------------------------------------------------------------------

void PlatformerDemoModule::CheckCheckpoints()
{
    auto* world = m_ctx->GetWorld();
    auto& playerXf = world->GetComponent<Transform>(m_player);

    for (int i = 0; i < static_cast<int>(m_checkpoints.size()); ++i)
    {
        auto e = m_checkpoints[i];
        if (e == entt::null) continue;

        auto& cpXf = world->GetComponent<Transform>(e);
        float dx = cpXf.position.x - playerXf.position.x;
        float dy = cpXf.position.y - playerXf.position.y;
        float distSq = dx * dx + dy * dy;

        if (distSq <= kCheckRadius * kCheckRadius && i > m_lastCheckpoint)
        {
            if (auto* bus = m_ctx->GetEventBus())
                bus->Publish(CheckpointReachedEvent{ m_player, e, i });
        }
    }
}

// ---------------------------------------------------------------------------
// Jump pad check
// ---------------------------------------------------------------------------

void PlatformerDemoModule::CheckJumpPads()
{
    auto* world = m_ctx->GetWorld();
    auto& playerXf = world->GetComponent<Transform>(m_player);

    for (auto e : m_jumpPads)
    {
        if (e == entt::null) continue;
        auto& padXf = world->GetComponent<Transform>(e);

        float dx = padXf.position.x - playerXf.position.x;
        float dy = padXf.position.y - playerXf.position.y;
        float distSq = dx * dx + dy * dy;

        if (distSq <= kPadRadius * kPadRadius && m_velocityY <= 0.0f)
        {
            // Extract launch force from tag (stored as "force_XX")
            float launchForce = 18.0f; // default
            if (world->HasComponent<TagComponent>(e))
            {
                auto& tags = world->GetComponent<TagComponent>(e);
                if (tags.HasTag("force_18")) launchForce = 18.0f;
                else if (tags.HasTag("force_22")) launchForce = 22.0f;
            }

            m_velocityY = launchForce;
            m_grounded = false;
            m_hasDoubleJump = true; // reset double jump after pad launch
        }
    }
}

// ---------------------------------------------------------------------------
// Respawn / restart
// ---------------------------------------------------------------------------

void PlatformerDemoModule::RespawnPlayer()
{
    auto* world = m_ctx->GetWorld();
    auto& xf = world->GetComponent<Transform>(m_player);

    xf.position = { m_spawnX, m_spawnY, m_spawnZ };
    m_velocityY = 0.0f;
    m_grounded = false;
    m_hasDoubleJump = true;
    m_coyoteTimer = 0.0f;
}

void PlatformerDemoModule::RestartLevel()
{
    // Reset all state
    m_score = 0;
    m_lives = 3;
    m_coinsTotal = 0;
    m_lastCheckpoint = -1;
    m_spawnX = 0.0f;
    m_spawnY = 3.0f;
    m_spawnZ = 0.0f;
    m_time = 0.0f;
    m_velocityY = 0.0f;

    // Destroy everything and rebuild
    auto* world = m_ctx->GetWorld();

    for (auto e : { m_player, m_camera, m_sunLight, m_deathZone })
        if (e != entt::null) world->DestroyEntity(e);
    m_player = m_camera = m_sunLight = m_deathZone = entt::null;

    for (auto e : m_platforms)
        if (e != entt::null) world->DestroyEntity(e);
    for (auto& mp : m_movingPlatforms)
        if (mp.entity != entt::null) world->DestroyEntity(mp.entity);
    for (auto e : m_coins)
        if (e != entt::null) world->DestroyEntity(e);
    for (auto& ed : m_enemies)
        if (ed.entity != entt::null) world->DestroyEntity(ed.entity);
    for (auto e : m_jumpPads)
        if (e != entt::null) world->DestroyEntity(e);
    for (auto e : m_checkpoints)
        if (e != entt::null) world->DestroyEntity(e);

    m_platforms.clear();
    m_movingPlatforms.clear();
    m_coins.clear();
    m_enemies.clear();
    m_jumpPads.clear();
    m_checkpoints.clear();

    BuildLevel();
}

// ---------------------------------------------------------------------------
// Render / Resize — engine handles drawing via RenderSystem
// ---------------------------------------------------------------------------

void PlatformerDemoModule::OnRender() { /* RenderSystem auto-draws MeshRenderers */ }

void PlatformerDemoModule::OnResize(int /*w*/, int /*h*/) { }

// ---------------------------------------------------------------------------
// DLL exports
// ---------------------------------------------------------------------------

SPARK_IMPLEMENT_MODULE(PlatformerDemoModule)
