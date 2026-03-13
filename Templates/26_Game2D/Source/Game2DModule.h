#pragma once
/**
 * @file Game2DModule.h
 * @brief INTERMEDIATE — 2D engine features demo.
 *
 * Demonstrates:
 *   - Camera2D with bounds and zoom
 *   - 3-layer parallax background
 *   - Tilemap-based level geometry
 *   - SpriteRenderer and SpriteAnimator
 *   - 2D physics (RigidBody2D + Collider2D)
 *   - Collectible animated sprites
 *   - NineSliceSprite for UI panels
 *   - PixelPerfectComponent for crisp rendering
 *
 * Spark Engine systems used:
 *   ECS        — World, Transform, NameComponent
 *   Sprite2D   — SpriteRenderer, SpriteAnimator, Camera2D, ParallaxBackground,
 *                TilemapComponent, RigidBody2D, Collider2D, NineSliceSprite,
 *                PixelPerfectComponent
 *   Input      — InputManager
 *   Events     — EventBus
 *   Debug      — DebugDraw
 */

#include <Core/ModuleManager.h>
#include <Core/EngineContext.h>
#include <Engine/ECS/Components.h>
#include <Engine/ECS/Components/Sprite2DComponents.h>
#include <Engine/Debug/DebugDraw.h>
#include <Engine/Events/EventSystem.h>
#include <vector>
#include <string>

// ---------------------------------------------------------------------------
// Coin collectible tracking
// ---------------------------------------------------------------------------

struct CoinInfo
{
    entt::entity entity = entt::null;
    bool collected = false;
};

// ---------------------------------------------------------------------------
// Module
// ---------------------------------------------------------------------------

class Game2DModule final : public Spark::IModule
{
public:
    Spark::ModuleInfo GetModuleInfo() const override
    {
        return { "Game2D", "0.2.0", SPARK_SDK_VERSION, 1000 };
    }

    void OnLoad(Spark::IEngineContext* ctx) override;
    void OnUnload() override;
    void OnUpdate(float dt) override;
    void OnRender() override;
    void OnResize(int w, int h) override;

private:
    // Scene construction
    void BuildScene();
    void CreateParallaxLayers();
    void CreateTilemap();
    void CreatePlayer();
    void CreateCoins();
    void CreateUIPanel();

    // Gameplay
    void UpdatePlayerMovement(float dt);
    void UpdatePlayerAnimation();
    void UpdatePhysics(float dt);
    void UpdateCoinCollection();
    void UpdateCamera(float dt);

    // Drawing
    void DrawScorePanel();
    void DrawHUD();

    // Tilemap collision helper
    bool IsTileSolid(int tileX, int tileY) const;
    bool CheckCollisionAt(float x, float y, float halfW, float halfH) const;

    Spark::IEngineContext* m_ctx = nullptr;

    // Screen dimensions
    float m_screenW = 1920.0f;
    float m_screenH = 1080.0f;

    // Core entities
    entt::entity m_cameraEntity = entt::null;
    entt::entity m_player       = entt::null;
    entt::entity m_tilemap      = entt::null;
    entt::entity m_uiPanel      = entt::null;

    // Parallax layers
    entt::entity m_bgSky       = entt::null;
    entt::entity m_bgMountains = entt::null;
    entt::entity m_bgTrees     = entt::null;

    // Coins
    std::vector<CoinInfo> m_coins;

    // All entities for cleanup
    std::vector<entt::entity> m_allEntities;

    // Player state
    float m_playerVelX    = 0.0f;
    float m_playerVelY    = 0.0f;
    bool  m_onGround      = false;
    bool  m_facingRight    = true;
    int   m_score          = 0;
    int   m_totalCoins     = 0;

    // Player animation state
    enum class AnimState { Idle, Walk, Jump };
    AnimState m_animState = AnimState::Idle;

    // Camera state
    float m_cameraZoom = 2.0f;

    // Physics constants
    static constexpr float kGravity       = -20.0f;
    static constexpr float kMoveSpeed     = 6.0f;
    static constexpr float kJumpForce     = 10.0f;
    static constexpr float kPlayerHalfW   = 0.4f;
    static constexpr float kPlayerHalfH   = 0.6f;

    // Tilemap data (stored locally for collision)
    static constexpr int kTileSize    = 16;
    static constexpr int kMapWidth    = 40;
    static constexpr int kMapHeight   = 12;
    int m_tileData[kMapHeight][kMapWidth] = {};

    // Timer for animation
    float m_time = 0.0f;
};
