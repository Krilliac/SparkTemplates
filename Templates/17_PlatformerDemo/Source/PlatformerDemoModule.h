#pragma once
/**
 * @file PlatformerDemoModule.h
 * @brief INTERMEDIATE — 2.5D platformer with physics, coins, enemies, and checkpoints.
 *
 * Demonstrates:
 *   - Player character with gravity, jumping, and double-jump
 *   - Moving platforms (kinematic bodies with sine-wave motion)
 *   - Collectible coins with spinning animation and pickup events
 *   - Patrol enemies that walk back-and-forth on platforms
 *   - Death zones (fall off map triggers respawn)
 *   - Checkpoint system using TagComponent
 *   - Score tracking and lives system
 *   - Smooth camera follow with damping
 *   - Level geometry from positioned cube entities
 *   - Spring-loaded jump pads
 *   - Custom events: CoinCollectedEvent, PlayerDiedEvent, CheckpointReachedEvent
 *
 * Spark Engine systems used:
 *   ECS        — World, Transform, MeshRenderer, NameComponent, HealthComponent
 *   Physics    — RigidBodyComponent, ColliderComponent
 *   Events     — EventBus (custom game events)
 *   AI         — AIComponent for enemy patrol
 *   Input      — InputManager
 *   Rendering  — LightComponent, Camera
 */

#include <Core/ModuleManager.h>
#include <Core/EngineContext.h>
#include <Engine/ECS/Components.h>
#include <Engine/Events/EventSystem.h>
#include <vector>
#include <string>

// ---------------------------------------------------------------------------
// Custom game events
// ---------------------------------------------------------------------------

struct CoinCollectedEvent
{
    entt::entity player;
    entt::entity coin;
    int          coinValue;
};

struct PlayerDiedEvent
{
    entt::entity player;
    int          livesRemaining;
};

struct CheckpointReachedEvent
{
    entt::entity player;
    entt::entity checkpoint;
    int          checkpointIndex;
};

// ---------------------------------------------------------------------------
// Moving platform descriptor
// ---------------------------------------------------------------------------

struct MovingPlatformData
{
    entt::entity entity   = entt::null;
    float        baseY    = 0.0f;       // centre position Y
    float        baseX    = 0.0f;       // centre position X
    float        ampX     = 0.0f;       // horizontal amplitude
    float        ampY     = 0.0f;       // vertical amplitude
    float        freq     = 1.0f;       // oscillation frequency (Hz)
    float        phase    = 0.0f;       // phase offset (radians)
};

// ---------------------------------------------------------------------------
// Enemy patrol descriptor
// ---------------------------------------------------------------------------

struct PatrolEnemyData
{
    entt::entity entity    = entt::null;
    float        leftBound  = 0.0f;
    float        rightBound = 0.0f;
    float        speed      = 2.0f;
    bool         movingRight = true;
};

// ---------------------------------------------------------------------------
// Module
// ---------------------------------------------------------------------------

class PlatformerDemoModule final : public Spark::IModule
{
public:
    Spark::ModuleInfo GetModuleInfo() const override
    {
        return { "PlatformerDemo", "0.2.0", SPARK_SDK_VERSION, 1000 };
    }

    void OnLoad(Spark::IEngineContext* ctx) override;
    void OnUnload() override;
    void OnUpdate(float dt) override;
    void OnRender() override;
    void OnResize(int w, int h) override;

private:
    // World building
    void BuildLevel();
    void SpawnPlatform(const char* name, float x, float y, float z,
                       float sx, float sy, float sz);
    void SpawnMovingPlatform(const char* name, float x, float y, float z,
                             float sx, float sy, float sz,
                             float ampX, float ampY, float freq, float phase);
    void SpawnCoin(float x, float y, float z);
    void SpawnEnemy(const char* name, float x, float y,
                    float leftBound, float rightBound, float speed);
    void SpawnJumpPad(float x, float y, float z, float launchForce);
    void SpawnCheckpoint(float x, float y, float z, int index);
    void SpawnDeathZone(float y);

    // Gameplay
    void UpdatePlayerPhysics(float dt);
    void UpdateMovingPlatforms(float dt);
    void UpdateEnemyPatrol(float dt);
    void UpdateCoinSpin(float dt);
    void UpdateCameraFollow(float dt);
    void CheckCoinPickup();
    void CheckDeathZone();
    void CheckCheckpoints();
    void CheckJumpPads();
    void RespawnPlayer();
    void RestartLevel();

    Spark::IEngineContext* m_ctx = nullptr;

    // ECS handles — core entities
    entt::entity m_player    = entt::null;
    entt::entity m_camera    = entt::null;
    entt::entity m_sunLight  = entt::null;
    entt::entity m_deathZone = entt::null;

    // Level entities
    std::vector<entt::entity>       m_platforms;
    std::vector<MovingPlatformData> m_movingPlatforms;
    std::vector<entt::entity>       m_coins;
    std::vector<PatrolEnemyData>    m_enemies;
    std::vector<entt::entity>       m_jumpPads;
    std::vector<entt::entity>       m_checkpoints;

    // Player physics state
    float m_velocityY        = 0.0f;
    bool  m_grounded         = false;
    bool  m_hasDoubleJump    = true;
    float m_coyoteTimer      = 0.0f;   // brief grace period after leaving edge

    // Checkpoint / respawn
    float m_spawnX = 0.0f;
    float m_spawnY = 3.0f;
    float m_spawnZ = 0.0f;
    int   m_lastCheckpoint = -1;

    // Scoring
    int   m_score      = 0;
    int   m_lives      = 3;
    int   m_coinsTotal = 0;

    // Time accumulator (for platform oscillation & coin spin)
    float m_time = 0.0f;

    // Camera damping
    static constexpr float kCamDamping   = 4.0f;
    static constexpr float kCamOffsetY   = 4.0f;
    static constexpr float kCamOffsetZ   = -12.0f;

    // Physics constants
    static constexpr float kGravity      = -20.0f;
    static constexpr float kJumpForce    =  12.0f;
    static constexpr float kMoveSpeed    =  8.0f;
    static constexpr float kCoyoteTime   =  0.1f;
    static constexpr float kDeathY       = -20.0f;
    static constexpr float kPickupRadius =  1.5f;
    static constexpr float kPadRadius    =  1.2f;
    static constexpr float kCheckRadius  =  2.0f;

    // Event subscriptions
    Spark::SubscriptionID m_coinSub       = 0;
    Spark::SubscriptionID m_diedSub       = 0;
    Spark::SubscriptionID m_checkpointSub = 0;
};
