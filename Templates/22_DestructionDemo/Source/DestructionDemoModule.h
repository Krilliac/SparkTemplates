#pragma once
/**
 * @file DestructionDemoModule.h
 * @brief ADVANCED — Destruction system showcase.
 *
 * Demonstrates:
 *   - Fracture patterns: Voronoi, Radial, Slice, Uniform
 *   - Multi-stage damage (clean, cracked, partial, full destruction)
 *   - Debris with physics and lifetime
 *   - Explosion area damage
 *   - Raycast damage via crosshair click
 *   - Score tracking for destroyed objects
 *   - Reset / rebuild all destructibles
 *   - DebugDraw UI for damage state of targeted object
 *
 * Spark Engine systems used:
 *   Destruction — DestructionSystem, FracturePattern, DestructibleComponent
 *   ECS         — World, Transform, MeshRenderer, Camera, LightComponent, NameComponent
 *   Physics     — RigidBodyComponent, ColliderComponent
 *   Input       — InputManager
 *   Events      — EventBus
 *   Debug       — DebugDraw (ScreenText, ScreenRect, ScreenLine)
 */

#include <Core/ModuleManager.h>
#include <Core/EngineContext.h>
#include <Engine/ECS/Components.h>
#include <Engine/Debug/DebugDraw.h>
#include <Engine/Events/EventSystem.h>
#include <Engine/Destruction/DestructionSystem.h>
#include <vector>
#include <string>

// ---------------------------------------------------------------------------
// Events
// ---------------------------------------------------------------------------

struct DestructionEvent
{
    entt::entity entity;
    std::string  objectName;
    int          stage; // final damage stage reached
};

struct DamageAppliedEvent
{
    entt::entity entity;
    float        damageAmount;
    float        remainingHealth;
};

// ---------------------------------------------------------------------------
// Tracked destructible info
// ---------------------------------------------------------------------------

struct DestructibleInfo
{
    entt::entity  entity;
    std::string   name;
    float         maxHealth;
    float         currentHealth;
    FracturePattern pattern;
    int           currentStage;      // 0=clean, 1=cracked, 2=partial, 3=destroyed
    float         origX, origY, origZ;
    float         origScaleX, origScaleY, origScaleZ;
    bool          destroyed;
};

// ---------------------------------------------------------------------------
// Debris tracking
// ---------------------------------------------------------------------------

struct DebrisInfo
{
    entt::entity entity;
    float        lifetime;
    float        elapsed;
};

// ---------------------------------------------------------------------------
// Module
// ---------------------------------------------------------------------------

class DestructionDemoModule final : public Spark::IModule
{
public:
    Spark::ModuleInfo GetModuleInfo() const override
    {
        return { "DestructionDemo", "0.2.0", SPARK_SDK_VERSION, 1000 };
    }

    void OnLoad(Spark::IEngineContext* ctx) override;
    void OnUnload() override;
    void OnUpdate(float dt) override;
    void OnRender() override;
    void OnResize(int w, int h) override;

private:
    // Scene setup
    void BuildScene();
    void SpawnDestructible(const std::string& name, float x, float y, float z,
                           float sx, float sy, float sz,
                           FracturePattern pattern, float health,
                           const char* material);

    // Gameplay
    void HandleInput(float dt);
    void ApplyDamageAtCrosshair(float amount);
    void ApplyExplosionAtPlayer(float radius, float damage);
    void AdvanceDamageStage(DestructibleInfo& info);
    void SpawnDebris(const DestructibleInfo& info, int count);
    void UpdateDebris(float dt);
    void ResetAllDestructibles();

    // Drawing
    void DrawCrosshair();
    void DrawHUD();
    void DrawTargetInfo();

    Spark::IEngineContext* m_ctx = nullptr;

    // Screen dimensions
    float m_screenW = 1920.0f;
    float m_screenH = 1080.0f;

    // Core entities
    entt::entity m_player = entt::null;
    entt::entity m_camera = entt::null;
    entt::entity m_light  = entt::null;
    std::vector<entt::entity> m_sceneEntities;

    // Camera control
    float m_camYaw   = 0.0f;
    float m_camPitch = -10.0f;

    // Player position
    float m_playerX = 0.0f;
    float m_playerY = 1.7f;
    float m_playerZ = -10.0f;

    // Destructibles
    std::vector<DestructibleInfo> m_destructibles;

    // Debris
    std::vector<DebrisInfo> m_debris;

    // Target under crosshair
    int m_targetIndex = -1;

    // Score
    int m_destroyedCount = 0;
    int m_totalDamageApplied = 0;

    // Event subscriptions
    uint64_t m_destructionSubId = 0;
    uint64_t m_damageSubId      = 0;
};
