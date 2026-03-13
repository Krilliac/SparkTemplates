#pragma once
/**
 * @file UIOverlayModule.h
 * @brief ADVANCED — UI/HUD rendering showcase.
 *
 * Demonstrates:
 *   - Health bar with smooth interpolation (damage flash, heal glow)
 *   - Ammo counter display
 *   - Minimap with entity markers (player, enemies, objectives)
 *   - Crosshair rendering (dynamic spread based on movement)
 *   - Damage direction indicator (red flash from hit direction)
 *   - Score / kill feed display
 *   - Notification / toast messages (timed popup text)
 *   - Menu system (main menu, pause menu with options)
 *   - Inventory grid display
 *   - Compass / bearing indicator at top of screen
 *   - Interaction prompt ("Press E to pick up")
 *   - Boss health bar (large, centred)
 *   - Timer countdown display
 *   - UIWidget base class with position, size, visibility, alpha
 *
 * Spark Engine systems used:
 *   Debug      — DebugDraw (ScreenRect, ScreenText, ScreenLine)
 *   ECS        — World, Transform, MeshRenderer, Camera, NameComponent
 *   Input      — InputManager
 *   Events     — EventBus
 */

#include <Core/ModuleManager.h>
#include <Core/EngineContext.h>
#include <Engine/ECS/Components.h>
#include <Engine/Debug/DebugDraw.h>
#include <Engine/Events/EventSystem.h>
#include <vector>
#include <string>
#include <deque>

// ---------------------------------------------------------------------------
// UIWidget — base for all UI elements
// ---------------------------------------------------------------------------

struct UIWidget
{
    float x        = 0.0f;
    float y        = 0.0f;
    float width    = 100.0f;
    float height   = 30.0f;
    float alpha    = 1.0f;
    bool  visible  = true;
};

// ---------------------------------------------------------------------------
// Notification message
// ---------------------------------------------------------------------------

struct Notification
{
    std::string text;
    float       timeRemaining;
    float       totalTime;
    float       r, g, b;   // color
};

// ---------------------------------------------------------------------------
// Damage indicator (directional)
// ---------------------------------------------------------------------------

struct DamageIndicator
{
    float angle;          // direction in degrees (world-space)
    float timeRemaining;
    float intensity;
};

// ---------------------------------------------------------------------------
// Inventory item
// ---------------------------------------------------------------------------

struct InventoryItem
{
    std::string name;
    int         count;
    bool        occupied;
};

// ---------------------------------------------------------------------------
// Kill feed entry
// ---------------------------------------------------------------------------

struct KillFeedEntry
{
    std::string text;
    float       timeRemaining;
};

// ---------------------------------------------------------------------------
// Module
// ---------------------------------------------------------------------------

class UIOverlayModule final : public Spark::IModule
{
public:
    Spark::ModuleInfo GetModuleInfo() const override
    {
        return { "UIOverlay", "0.2.0", SPARK_SDK_VERSION, 1000 };
    }

    void OnLoad(Spark::IEngineContext* ctx) override;
    void OnUnload() override;
    void OnUpdate(float dt) override;
    void OnRender() override;
    void OnResize(int w, int h) override;

private:
    // Scene
    void BuildScene();

    // UI drawing
    void DrawHealthBar();
    void DrawAmmoCounter();
    void DrawMinimap();
    void DrawCrosshair();
    void DrawDamageIndicators();
    void DrawScoreDisplay();
    void DrawKillFeed();
    void DrawNotifications();
    void DrawPauseMenu();
    void DrawInventory();
    void DrawCompass();
    void DrawInteractionPrompt();
    void DrawBossHealthBar();
    void DrawTimer();

    // Helpers
    void AddNotification(const std::string& text, float duration,
                         float r, float g, float b);
    void AddKillFeedEntry(const std::string& text);
    void SimulateDamageFrom(float worldAngle);

    Spark::IEngineContext* m_ctx = nullptr;

    // Screen dimensions
    float m_screenW = 1920.0f;
    float m_screenH = 1080.0f;

    // Core entities
    entt::entity m_player  = entt::null;
    entt::entity m_camera  = entt::null;
    entt::entity m_light   = entt::null;

    // Enemy entities (for minimap markers)
    std::vector<entt::entity> m_enemies;
    std::vector<entt::entity> m_sceneEntities;

    // Boss entity
    entt::entity m_boss = entt::null;

    // --- UI State ---

    // HUD toggle
    bool m_hudVisible   = true;
    bool m_minimapOn    = true;
    bool m_paused       = false;
    bool m_inventoryOpen = false;

    // Player stats
    float m_health        = 100.0f;
    float m_maxHealth     = 100.0f;
    float m_displayHealth = 100.0f;   // smoothly interpolated
    float m_healthFlash   = 0.0f;     // damage flash timer
    float m_healGlow      = 0.0f;     // heal glow timer
    int   m_ammo          = 30;
    int   m_maxAmmo       = 30;
    int   m_ammoReserve   = 120;
    int   m_score         = 0;
    int   m_kills         = 0;

    // Boss stats
    float m_bossHealth    = 500.0f;
    float m_bossMaxHealth = 500.0f;
    bool  m_bossActive    = true;

    // Timer
    float m_timerSeconds  = 300.0f;   // 5 minute countdown
    bool  m_timerActive   = true;

    // Crosshair
    float m_crosshairSpread = 10.0f;  // base spread in pixels
    float m_moveSpeed       = 0.0f;   // current movement speed (affects spread)

    // Player heading (for compass)
    float m_playerYaw = 0.0f;

    // Notifications
    std::deque<Notification> m_notifications;
    static constexpr int kMaxNotifications = 4;

    // Kill feed
    std::deque<KillFeedEntry> m_killFeed;
    static constexpr int kMaxKillFeed = 5;

    // Damage indicators
    std::vector<DamageIndicator> m_damageIndicators;

    // Inventory (4x5 grid)
    static constexpr int kInvCols = 4;
    static constexpr int kInvRows = 5;
    InventoryItem m_inventory[kInvRows][kInvCols];

    // Interaction prompt
    bool        m_showInteract = false;
    std::string m_interactText = "Press E to pick up";

    // Pause menu selection
    int m_pauseSelection = 0;

    // Camera orbit
    float m_camYaw   = 0.0f;
    float m_camPitch = -20.0f;
    float m_camDist  = 15.0f;
};
