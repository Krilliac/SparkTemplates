#pragma once
/**
 * @file ModdingPlaygroundModule.h
 * @brief ADVANCED — Mod system demo with scanning, loading, dependencies.
 *
 * Demonstrates:
 *   - ModSystem: ScanForMods, LoadMod, UnloadMod, EnableMod, DisableMod
 *   - Mod dependency enforcement
 *   - Mod load/unload callbacks that modify the scene
 *   - Mod browser UI via DebugDraw
 *   - Console commands for mod management
 *   - 3 simulated mods: ColorMod, SpawnMod, WeatherMod
 *
 * Spark Engine systems used:
 *   ECS        — World, Transform, MeshRenderer, Camera, NameComponent, LightComponent
 *   Modding    — ModSystem, ModInfo
 *   Input      — InputManager
 *   Debug      — DebugDraw
 *   Events     — EventBus (ModLoadedEvent, ModUnloadedEvent)
 *   Console    — SimpleConsole
 */

#include <Core/ModuleManager.h>
#include <Core/EngineContext.h>
#include <Engine/ECS/Components.h>
#include <Engine/Modding/ModSystem.h>
#include <Engine/Debug/DebugDraw.h>
#include <Engine/Events/EventSystem.h>
#include <vector>
#include <string>
#include <functional>

// ---------------------------------------------------------------------------
// Simulated mod descriptor
// ---------------------------------------------------------------------------

struct SimulatedMod
{
    ModInfo info;
    bool    enabled       = false;
    bool    loaded        = false;
    float   memoryUsageMB = 0.0f;

    // Callbacks
    std::function<void()> onLoad;
    std::function<void()> onUnload;

    // Entities created by this mod (for cleanup)
    std::vector<entt::entity> entities;
};

// ---------------------------------------------------------------------------
// Console command entry
// ---------------------------------------------------------------------------

struct ConsoleEntry
{
    std::string text;
    float       r, g, b;
};

// ---------------------------------------------------------------------------
// Module
// ---------------------------------------------------------------------------

class ModdingPlaygroundModule final : public Spark::IModule
{
public:
    Spark::ModuleInfo GetModuleInfo() const override
    {
        return { "ModdingPlayground", "0.2.0", SPARK_SDK_VERSION, 1000 };
    }

    void OnLoad(Spark::IEngineContext* ctx) override;
    void OnUnload() override;
    void OnUpdate(float dt) override;
    void OnRender() override;
    void OnResize(int w, int h) override;

private:
    // Scene setup
    void BuildScene();
    void RegisterMods();

    // Mod operations
    void ToggleMod(int index);
    void LoadEnabledMods();
    void UnloadAllMods();
    void RescanMods();

    // Mod load/unload callbacks
    void LoadColorMod();
    void UnloadColorMod();
    void LoadSpawnMod();
    void UnloadSpawnMod();
    void LoadWeatherMod();
    void UnloadWeatherMod();

    // Console
    void ProcessConsoleCommand(const std::string& cmd);
    void ConsolePrint(const std::string& text, float r, float g, float b);

    // Drawing
    void DrawModBrowser();
    void DrawConsole();
    void DrawOverlay();

    Spark::IEngineContext* m_ctx = nullptr;

    // Screen dimensions
    float m_screenW = 1920.0f;
    float m_screenH = 1080.0f;

    // Core entities
    entt::entity m_camera = entt::null;
    entt::entity m_light  = entt::null;
    entt::entity m_ground = entt::null;
    std::vector<entt::entity> m_baseProps;
    std::vector<entt::entity> m_allCoreEntities;

    // Mods
    std::vector<SimulatedMod> m_mods;
    ModSystem m_modSystem;

    // UI state
    bool m_modBrowserOpen = false;
    bool m_consoleOpen    = false;
    int  m_browserSelection = 0;

    // Console state
    std::string m_consoleInput;
    std::vector<ConsoleEntry> m_consoleLog;
    static constexpr int kMaxConsoleLines = 20;

    // Camera orbit
    float m_camYaw   = 30.0f;
    float m_camPitch = -25.0f;
    float m_camDist  = 20.0f;

    // Scene state tracking for ColorMod
    struct OriginalColor
    {
        entt::entity entity;
        float r, g, b;
    };
    std::vector<OriginalColor> m_savedColors;

    // Weather state
    float m_weatherTime = 0.0f;
};
