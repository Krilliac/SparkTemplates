#pragma once
/**
 * @file WeaponShowcaseModule.h
 * @brief INTERMEDIATE — FPS weapon system showcase.
 *
 * Demonstrates:
 *   - Three weapon types: Pistol (Semi), Rifle (FullAuto), Shotgun (Semi)
 *   - Fire modes: Semi, Burst, FullAuto
 *   - Recoil pattern affecting camera pitch
 *   - ADS (Aim Down Sights) with FOV zoom
 *   - Ammo management and reload
 *   - Muzzle flash via brief point light
 *   - Shooting targets (cube entities at distance)
 *   - Event-driven fire/reload sounds
 *   - HUD: ammo counter, weapon name, crosshair
 *
 * Spark Engine systems used:
 *   Gameplay  — WeaponSystem, WeaponRegistry, WeaponDefinition
 *   ECS       — World, Transform, MeshRenderer, Camera, LightComponent, NameComponent
 *   Input     — InputManager
 *   Audio     — AudioEngine
 *   Events    — EventBus
 *   Debug     — DebugDraw (ScreenText, ScreenRect, ScreenLine)
 */

#include <Core/ModuleManager.h>
#include <Core/EngineContext.h>
#include <Engine/ECS/Components.h>
#include <Engine/Debug/DebugDraw.h>
#include <Engine/Events/EventSystem.h>
#include <Engine/Gameplay/WeaponManager.h>
#include <Engine/Audio/AudioEngine.h>
#include <vector>
#include <string>

// ---------------------------------------------------------------------------
// Events
// ---------------------------------------------------------------------------

struct WeaponFiredEvent
{
    std::string weaponName;
    int         remainingAmmo;
};

struct ReloadStartedEvent
{
    std::string weaponName;
};

struct ReloadFinishedEvent
{
    std::string weaponName;
    int         newAmmo;
};

// ---------------------------------------------------------------------------
// Module
// ---------------------------------------------------------------------------

class WeaponShowcaseModule final : public Spark::IModule
{
public:
    Spark::ModuleInfo GetModuleInfo() const override
    {
        return { "WeaponShowcase", "0.2.0", SPARK_SDK_VERSION, 1000 };
    }

    void OnLoad(Spark::IEngineContext* ctx) override;
    void OnUnload() override;
    void OnUpdate(float dt) override;
    void OnRender() override;
    void OnResize(int w, int h) override;

private:
    // Scene setup
    void BuildScene();
    void RegisterWeapons();

    // Gameplay
    void HandleInput(float dt);
    void UpdateRecoilRecovery(float dt);
    void UpdateReload(float dt);
    void UpdateMuzzleFlash(float dt);
    void FireWeapon();
    void StartReload();
    void SwitchWeapon(int index);

    // Drawing
    void DrawCrosshair();
    void DrawAmmoHUD();
    void DrawWeaponInfo();
    void DrawReloadBar();

    Spark::IEngineContext* m_ctx = nullptr;

    // Screen dimensions
    float m_screenW = 1920.0f;
    float m_screenH = 1080.0f;

    // Core entities
    entt::entity m_player       = entt::null;
    entt::entity m_camera       = entt::null;
    entt::entity m_light        = entt::null;
    entt::entity m_muzzleLight  = entt::null;
    std::vector<entt::entity> m_targets;
    std::vector<entt::entity> m_sceneEntities;

    // Camera control
    float m_camYaw   = 0.0f;
    float m_camPitch = 0.0f;
    float m_baseFOV  = 70.0f;
    float m_currentFOV = 70.0f;

    // Player position
    float m_playerX = 0.0f;
    float m_playerY = 1.7f;
    float m_playerZ = 0.0f;

    // Weapon state
    int   m_currentWeaponIndex = 0;
    int   m_ammo[3]            = { 12, 30, 8 };
    int   m_maxAmmo[3]         = { 12, 30, 8 };
    int   m_reserveAmmo[3]     = { 48, 120, 32 };
    float m_fireTimer          = 0.0f;
    bool  m_isReloading        = false;
    float m_reloadTimer        = 0.0f;
    float m_reloadDuration     = 0.0f;
    bool  m_isADS              = false;
    float m_adsBlend           = 0.0f;

    // Recoil
    float m_recoilPitch       = 0.0f;
    float m_recoilRecoveryRate = 5.0f;

    // Muzzle flash
    float m_muzzleFlashTimer = 0.0f;
    bool  m_muzzleFlashOn    = false;

    // Fire hold (for full-auto)
    bool m_fireHeld = false;

    // Weapon definitions (cached for quick access)
    struct WeaponInfo
    {
        std::string name;
        FireMode    fireMode;
        float       fireRate;
        float       damage;
        int         magazineSize;
        float       reloadTime;
        float       recoilAmount;
        float       spreadAngle;
        float       adsZoomFOV;
    };

    WeaponInfo m_weapons[3];

    // Score
    int m_targetsHit = 0;

    // Event subscription IDs
    uint64_t m_firedSubId   = 0;
    uint64_t m_reloadStartSubId = 0;
    uint64_t m_reloadEndSubId   = 0;
};
