/**
 * @file ModdingPlaygroundModule.cpp
 * @brief ADVANCED — Mod system demo with scanning, loading, dependencies.
 */

#include "ModdingPlaygroundModule.h"
#include <Input/InputManager.h>
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <sstream>

static constexpr float kPi = 3.14159265358979f;

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void ModdingPlaygroundModule::OnLoad(Spark::IEngineContext* ctx)
{
    m_ctx = ctx;
    BuildScene();
    RegisterMods();

    ConsolePrint("Modding Playground initialized.", 0.5f, 1.0f, 0.5f);
    ConsolePrint("Type 'help' for available commands.", 0.7f, 0.7f, 0.7f);
}

void ModdingPlaygroundModule::OnUnload()
{
    auto* world = m_ctx ? m_ctx->GetWorld() : nullptr;
    if (!world) { m_ctx = nullptr; return; }

    // Unload all mods first
    UnloadAllMods();

    for (auto e : m_allCoreEntities)
        if (e != entt::null) world->DestroyEntity(e);

    m_allCoreEntities.clear();
    m_baseProps.clear();
    m_mods.clear();
    m_consoleLog.clear();
    m_ctx = nullptr;
}

// ---------------------------------------------------------------------------
// Scene construction
// ---------------------------------------------------------------------------

void ModdingPlaygroundModule::BuildScene()
{
    auto* world = m_ctx->GetWorld();

    // --- Camera -------------------------------------------------------------
    m_camera = world->CreateEntity();
    world->AddComponent<NameComponent>(m_camera, NameComponent{ "MainCamera" });
    world->AddComponent<Transform>(m_camera, Transform{
        { 15.0f, 12.0f, -15.0f },
        { -25.0f, 30.0f, 0.0f },
        { 1.0f, 1.0f, 1.0f }
    });
    world->AddComponent<Camera>(m_camera, Camera{
        60.0f, 0.1f, 500.0f, true
    });
    m_allCoreEntities.push_back(m_camera);

    // --- Light --------------------------------------------------------------
    m_light = world->CreateEntity();
    world->AddComponent<NameComponent>(m_light, NameComponent{ "SunLight" });
    world->AddComponent<Transform>(m_light, Transform{
        { 0.0f, 20.0f, 0.0f },
        { -45.0f, 30.0f, 0.0f },
        { 1.0f, 1.0f, 1.0f }
    });
    world->AddComponent<LightComponent>(m_light, LightComponent{
        LightComponent::Type::Directional,
        { 1.0f, 0.95f, 0.85f }, 2.0f, 0.0f,
        0.0f, 0.0f, true, 2048
    });
    m_allCoreEntities.push_back(m_light);

    // --- Ground -------------------------------------------------------------
    m_ground = world->CreateEntity();
    world->AddComponent<NameComponent>(m_ground, NameComponent{ "Ground" });
    world->AddComponent<Transform>(m_ground, Transform{
        { 0.0f, -0.1f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 30.0f, 0.2f, 30.0f }
    });
    world->AddComponent<MeshRenderer>(m_ground, MeshRenderer{
        "Assets/Models/cube.obj", "Assets/Materials/grass.mat",
        false, true, true
    });
    m_allCoreEntities.push_back(m_ground);
    m_baseProps.push_back(m_ground);

    // --- Base props (a few cubes/pillars representing scene objects) ---------
    struct PropDef { const char* name; float x, y, z, sx, sy, sz; };
    PropDef props[] = {
        { "Pillar_A",  -4.0f, 2.0f,  3.0f, 1.0f, 4.0f, 1.0f },
        { "Pillar_B",   4.0f, 2.0f,  3.0f, 1.0f, 4.0f, 1.0f },
        { "Wall_A",     0.0f, 1.5f,  6.0f, 8.0f, 3.0f, 0.5f },
        { "Box_A",     -6.0f, 0.75f,-3.0f, 1.5f, 1.5f, 1.5f },
        { "Box_B",      5.0f, 0.5f, -5.0f, 1.0f, 1.0f, 1.0f },
        { "Platform",   0.0f, 0.3f, -2.0f, 6.0f, 0.6f, 4.0f },
    };

    for (auto& def : props)
    {
        auto e = world->CreateEntity();
        world->AddComponent<NameComponent>(e, NameComponent{ def.name });
        world->AddComponent<Transform>(e, Transform{
            { def.x, def.y, def.z },
            { 0.0f, 0.0f, 0.0f },
            { def.sx, def.sy, def.sz }
        });
        world->AddComponent<MeshRenderer>(e, MeshRenderer{
            "Assets/Models/cube.obj", "Assets/Materials/concrete.mat",
            true, true, true
        });
        m_allCoreEntities.push_back(e);
        m_baseProps.push_back(e);
    }
}

// ---------------------------------------------------------------------------
// Register simulated mods
// ---------------------------------------------------------------------------

void ModdingPlaygroundModule::RegisterMods()
{
    // Mod 0: ColorMod
    {
        SimulatedMod mod;
        mod.info.name = "ColorMod";
        mod.info.version = "1.0.0";
        mod.info.author = "SparkDev";
        mod.info.description = "Changes material colors of scene objects to vibrant hues.";
        mod.info.dependencies = {};
        mod.info.enabled = false;
        mod.memoryUsageMB = 2.5f;
        mod.onLoad = [this]() { LoadColorMod(); };
        mod.onUnload = [this]() { UnloadColorMod(); };
        m_mods.push_back(mod);
    }

    // Mod 1: SpawnMod
    {
        SimulatedMod mod;
        mod.info.name = "SpawnMod";
        mod.info.version = "1.2.0";
        mod.info.author = "CommunityUser";
        mod.info.description = "Adds extra decorative objects and NPC entities to the scene.";
        mod.info.dependencies = {};
        mod.info.enabled = false;
        mod.memoryUsageMB = 8.0f;
        mod.onLoad = [this]() { LoadSpawnMod(); };
        mod.onUnload = [this]() { UnloadSpawnMod(); };
        m_mods.push_back(mod);
    }

    // Mod 2: WeatherMod (depends on ColorMod)
    {
        SimulatedMod mod;
        mod.info.name = "WeatherMod";
        mod.info.version = "0.9.0";
        mod.info.author = "WeatherTeam";
        mod.info.description = "Adds weather visual effects. Requires ColorMod for tinting.";
        mod.info.dependencies = { "ColorMod" };
        mod.info.enabled = false;
        mod.memoryUsageMB = 5.0f;
        mod.onLoad = [this]() { LoadWeatherMod(); };
        mod.onUnload = [this]() { UnloadWeatherMod(); };
        m_mods.push_back(mod);
    }

    // Register with ModSystem
    for (auto& mod : m_mods)
    {
        m_modSystem.ScanForMods();
    }
}

// ---------------------------------------------------------------------------
// Mod operations
// ---------------------------------------------------------------------------

void ModdingPlaygroundModule::ToggleMod(int index)
{
    if (index < 0 || index >= static_cast<int>(m_mods.size())) return;
    auto& mod = m_mods[index];

    if (mod.enabled)
    {
        // Disabling — check if other loaded mods depend on this one
        for (auto& otherMod : m_mods)
        {
            if (!otherMod.enabled) continue;
            for (auto& dep : otherMod.info.dependencies)
            {
                if (dep == mod.info.name)
                {
                    ConsolePrint("Cannot disable " + mod.info.name +
                                 ": " + otherMod.info.name + " depends on it.",
                                 1.0f, 0.3f, 0.3f);
                    return;
                }
            }
        }

        // If loaded, unload first
        if (mod.loaded && mod.onUnload)
        {
            mod.onUnload();
            mod.loaded = false;
        }

        mod.enabled = false;
        m_modSystem.DisableMod(mod.info.name);
        ConsolePrint("Disabled: " + mod.info.name, 1.0f, 0.6f, 0.2f);
    }
    else
    {
        // Enabling — check dependencies are enabled
        for (auto& dep : mod.info.dependencies)
        {
            bool found = false;
            for (auto& otherMod : m_mods)
            {
                if (otherMod.info.name == dep && otherMod.enabled)
                {
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                ConsolePrint("Cannot enable " + mod.info.name +
                             ": requires " + dep + " to be enabled first.",
                             1.0f, 0.3f, 0.3f);
                return;
            }
        }

        mod.enabled = true;
        m_modSystem.EnableMod(mod.info.name);
        ConsolePrint("Enabled: " + mod.info.name, 0.3f, 1.0f, 0.3f);
    }
}

void ModdingPlaygroundModule::LoadEnabledMods()
{
    // Load in dependency order: mods with no deps first
    for (auto& mod : m_mods)
    {
        if (!mod.enabled || mod.loaded) continue;

        // Check all dependencies are loaded
        bool depsOk = true;
        for (auto& dep : mod.info.dependencies)
        {
            bool depLoaded = false;
            for (auto& otherMod : m_mods)
            {
                if (otherMod.info.name == dep && otherMod.loaded)
                {
                    depLoaded = true;
                    break;
                }
            }
            if (!depLoaded) { depsOk = false; break; }
        }

        if (!depsOk)
        {
            // Try loading deps first
            for (auto& dep : mod.info.dependencies)
            {
                for (auto& otherMod : m_mods)
                {
                    if (otherMod.info.name == dep && otherMod.enabled && !otherMod.loaded)
                    {
                        if (otherMod.onLoad) otherMod.onLoad();
                        otherMod.loaded = true;
                        m_modSystem.LoadMod(otherMod.info.name);
                        ConsolePrint("Loaded: " + otherMod.info.name, 0.3f, 0.8f, 1.0f);
                    }
                }
            }
        }

        if (mod.onLoad) mod.onLoad();
        mod.loaded = true;
        m_modSystem.LoadMod(mod.info.name);
        ConsolePrint("Loaded: " + mod.info.name, 0.3f, 0.8f, 1.0f);
    }
}

void ModdingPlaygroundModule::UnloadAllMods()
{
    // Unload in reverse order (dependents first)
    for (int i = static_cast<int>(m_mods.size()) - 1; i >= 0; --i)
    {
        auto& mod = m_mods[i];
        if (!mod.loaded) continue;

        if (mod.onUnload) mod.onUnload();
        mod.loaded = false;
        m_modSystem.UnloadMod(mod.info.name);
        ConsolePrint("Unloaded: " + mod.info.name, 0.8f, 0.5f, 0.2f);
    }
}

void ModdingPlaygroundModule::RescanMods()
{
    m_modSystem.ScanForMods();
    ConsolePrint("Rescanned mods. Found " + std::to_string(m_mods.size()) + " mods.",
                 0.5f, 0.8f, 1.0f);
}

// ---------------------------------------------------------------------------
// ColorMod — changes prop colors
// ---------------------------------------------------------------------------

void ModdingPlaygroundModule::LoadColorMod()
{
    auto* world = m_ctx->GetWorld();

    // Save original colors and apply vibrant ones
    m_savedColors.clear();

    struct ColorAssign { int propIndex; float r, g, b; };
    ColorAssign assigns[] = {
        { 0, 0.2f, 0.4f, 0.15f },   // ground -> darker green
        { 1, 0.8f, 0.2f, 0.2f },     // pillar A -> red
        { 2, 0.2f, 0.2f, 0.8f },     // pillar B -> blue
        { 3, 0.9f, 0.8f, 0.2f },     // wall -> gold
        { 4, 0.6f, 0.1f, 0.8f },     // box A -> purple
        { 5, 0.1f, 0.8f, 0.6f },     // box B -> teal
        { 6, 0.9f, 0.5f, 0.1f },     // platform -> orange
    };

    for (auto& assign : assigns)
    {
        if (assign.propIndex >= static_cast<int>(m_baseProps.size())) continue;
        auto e = m_baseProps[assign.propIndex];
        if (e == entt::null) continue;

        // We don't have direct color on MeshRenderer, so we track the intent
        // In a real engine, this would modify the material instance
        m_savedColors.push_back({ e, 0.5f, 0.5f, 0.5f }); // save default
    }
}

void ModdingPlaygroundModule::UnloadColorMod()
{
    // Restore original colors
    m_savedColors.clear();

    // Also unload WeatherMod if it's loaded (it depends on ColorMod)
    for (auto& mod : m_mods)
    {
        if (mod.info.name == "WeatherMod" && mod.loaded)
        {
            if (mod.onUnload) mod.onUnload();
            mod.loaded = false;
            ConsolePrint("Auto-unloaded WeatherMod (dependency removed).", 0.8f, 0.5f, 0.2f);
        }
    }
}

// ---------------------------------------------------------------------------
// SpawnMod — adds extra entities
// ---------------------------------------------------------------------------

void ModdingPlaygroundModule::LoadSpawnMod()
{
    auto* world = m_ctx->GetWorld();
    auto& mod = m_mods[1]; // SpawnMod is index 1

    struct SpawnDef { const char* name; float x, y, z, sx, sy, sz; };
    SpawnDef spawns[] = {
        { "Mod_Statue_A",    -8.0f, 1.5f,  0.0f, 1.0f, 3.0f, 1.0f },
        { "Mod_Statue_B",     8.0f, 1.5f,  0.0f, 1.0f, 3.0f, 1.0f },
        { "Mod_Barrel_A",    -3.0f, 0.5f, -6.0f, 0.6f, 1.0f, 0.6f },
        { "Mod_Barrel_B",    -1.5f, 0.5f, -6.0f, 0.6f, 1.0f, 0.6f },
        { "Mod_Barrel_C",    -2.3f, 0.5f, -5.0f, 0.6f, 1.0f, 0.6f },
        { "Mod_NPC_Guard",    0.0f, 1.0f,  0.0f, 0.8f, 2.0f, 0.8f },
        { "Mod_NPC_Merchant", 6.0f, 1.0f, -3.0f, 0.8f, 2.0f, 0.8f },
        { "Mod_Crate_A",     -7.0f, 0.5f,  6.0f, 1.0f, 1.0f, 1.0f },
        { "Mod_Crate_B",     -6.0f, 0.5f,  7.0f, 1.0f, 1.0f, 1.0f },
        { "Mod_Lantern",      0.0f, 3.5f, -2.0f, 0.3f, 0.5f, 0.3f },
        { "Mod_Banner_L",    -4.0f, 4.5f,  3.0f, 0.1f, 2.0f, 0.6f },
        { "Mod_Banner_R",     4.0f, 4.5f,  3.0f, 0.1f, 2.0f, 0.6f },
    };

    for (auto& def : spawns)
    {
        auto e = world->CreateEntity();
        world->AddComponent<NameComponent>(e, NameComponent{ def.name });
        world->AddComponent<Transform>(e, Transform{
            { def.x, def.y, def.z },
            { 0.0f, 0.0f, 0.0f },
            { def.sx, def.sy, def.sz }
        });
        world->AddComponent<MeshRenderer>(e, MeshRenderer{
            "Assets/Models/cube.obj", "Assets/Materials/default.mat",
            true, true, true
        });
        mod.entities.push_back(e);
    }
}

void ModdingPlaygroundModule::UnloadSpawnMod()
{
    auto* world = m_ctx->GetWorld();
    auto& mod = m_mods[1];

    for (auto e : mod.entities)
        if (e != entt::null) world->DestroyEntity(e);

    mod.entities.clear();
}

// ---------------------------------------------------------------------------
// WeatherMod — adds weather effect entities
// ---------------------------------------------------------------------------

void ModdingPlaygroundModule::LoadWeatherMod()
{
    auto* world = m_ctx->GetWorld();
    auto& mod = m_mods[2]; // WeatherMod is index 2

    // Create "rain" particle emitter entity (simulated)
    auto rain = world->CreateEntity();
    world->AddComponent<NameComponent>(rain, NameComponent{ "Mod_RainEmitter" });
    world->AddComponent<Transform>(rain, Transform{
        { 0.0f, 15.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f },
        { 30.0f, 1.0f, 30.0f }
    });
    world->AddComponent<MeshRenderer>(rain, MeshRenderer{
        "Assets/Models/cube.obj", "Assets/Materials/water.mat",
        false, false, true
    });
    mod.entities.push_back(rain);

    // Create "fog" volume entity
    auto fog = world->CreateEntity();
    world->AddComponent<NameComponent>(fog, NameComponent{ "Mod_FogVolume" });
    world->AddComponent<Transform>(fog, Transform{
        { 0.0f, 1.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f },
        { 40.0f, 2.0f, 40.0f }
    });
    world->AddComponent<MeshRenderer>(fog, MeshRenderer{
        "Assets/Models/cube.obj", "Assets/Materials/fog.mat",
        false, false, true
    });
    mod.entities.push_back(fog);

    // Create cloud layer entity
    auto clouds = world->CreateEntity();
    world->AddComponent<NameComponent>(clouds, NameComponent{ "Mod_CloudLayer" });
    world->AddComponent<Transform>(clouds, Transform{
        { 0.0f, 18.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f },
        { 50.0f, 0.5f, 50.0f }
    });
    world->AddComponent<MeshRenderer>(clouds, MeshRenderer{
        "Assets/Models/cube.obj", "Assets/Materials/clouds.mat",
        false, false, true
    });
    mod.entities.push_back(clouds);

    // Darken the light to simulate overcast
    auto& lightComp = world->GetComponent<LightComponent>(m_light);
    lightComp.intensity = 1.0f;
    lightComp.color = { 0.6f, 0.65f, 0.7f };

    m_weatherTime = 0.0f;
}

void ModdingPlaygroundModule::UnloadWeatherMod()
{
    auto* world = m_ctx->GetWorld();
    auto& mod = m_mods[2];

    for (auto e : mod.entities)
        if (e != entt::null) world->DestroyEntity(e);

    mod.entities.clear();

    // Restore light
    if (m_light != entt::null)
    {
        auto& lightComp = world->GetComponent<LightComponent>(m_light);
        lightComp.intensity = 2.0f;
        lightComp.color = { 1.0f, 0.95f, 0.85f };
    }
}

// ---------------------------------------------------------------------------
// Console command processing
// ---------------------------------------------------------------------------

void ModdingPlaygroundModule::ProcessConsoleCommand(const std::string& cmd)
{
    ConsolePrint("> " + cmd, 1.0f, 1.0f, 1.0f);

    if (cmd == "help")
    {
        ConsolePrint("Available commands:", 0.7f, 0.7f, 0.7f);
        ConsolePrint("  mod_list           — List all mods", 0.6f, 0.6f, 0.6f);
        ConsolePrint("  mod_enable <name>  — Enable a mod", 0.6f, 0.6f, 0.6f);
        ConsolePrint("  mod_disable <name> — Disable a mod", 0.6f, 0.6f, 0.6f);
        ConsolePrint("  mod_load           — Load all enabled mods", 0.6f, 0.6f, 0.6f);
        ConsolePrint("  mod_info <name>    — Show mod details", 0.6f, 0.6f, 0.6f);
        ConsolePrint("  clear              — Clear console", 0.6f, 0.6f, 0.6f);
    }
    else if (cmd == "mod_list")
    {
        for (auto& mod : m_mods)
        {
            char buf[128];
            std::snprintf(buf, sizeof(buf), "  [%s] %s v%s — %s",
                          mod.loaded ? "LOADED" : (mod.enabled ? "enabled" : "off"),
                          mod.info.name.c_str(),
                          mod.info.version.c_str(),
                          mod.info.description.c_str());
            float r = mod.loaded ? 0.3f : (mod.enabled ? 0.8f : 0.5f);
            float g = mod.loaded ? 0.8f : (mod.enabled ? 0.8f : 0.5f);
            float b = mod.loaded ? 1.0f : (mod.enabled ? 0.2f : 0.5f);
            ConsolePrint(buf, r, g, b);
        }
    }
    else if (cmd == "mod_load")
    {
        LoadEnabledMods();
    }
    else if (cmd == "clear")
    {
        m_consoleLog.clear();
    }
    else if (cmd.substr(0, 11) == "mod_enable ")
    {
        std::string modName = cmd.substr(11);
        bool found = false;
        for (int i = 0; i < static_cast<int>(m_mods.size()); ++i)
        {
            if (m_mods[i].info.name == modName)
            {
                if (!m_mods[i].enabled) ToggleMod(i);
                else ConsolePrint(modName + " is already enabled.", 0.8f, 0.8f, 0.3f);
                found = true;
                break;
            }
        }
        if (!found)
            ConsolePrint("Mod not found: " + modName, 1.0f, 0.3f, 0.3f);
    }
    else if (cmd.substr(0, 12) == "mod_disable ")
    {
        std::string modName = cmd.substr(12);
        bool found = false;
        for (int i = 0; i < static_cast<int>(m_mods.size()); ++i)
        {
            if (m_mods[i].info.name == modName)
            {
                if (m_mods[i].enabled) ToggleMod(i);
                else ConsolePrint(modName + " is already disabled.", 0.8f, 0.8f, 0.3f);
                found = true;
                break;
            }
        }
        if (!found)
            ConsolePrint("Mod not found: " + modName, 1.0f, 0.3f, 0.3f);
    }
    else if (cmd.substr(0, 9) == "mod_info ")
    {
        std::string modName = cmd.substr(9);
        bool found = false;
        for (auto& mod : m_mods)
        {
            if (mod.info.name == modName)
            {
                ConsolePrint("Name: " + mod.info.name, 1.0f, 1.0f, 1.0f);
                ConsolePrint("Version: " + mod.info.version, 0.8f, 0.8f, 0.8f);
                ConsolePrint("Author: " + mod.info.author, 0.8f, 0.8f, 0.8f);
                ConsolePrint("Desc: " + mod.info.description, 0.7f, 0.7f, 0.7f);

                std::string deps = "Dependencies: ";
                if (mod.info.dependencies.empty())
                    deps += "none";
                else
                    for (size_t i = 0; i < mod.info.dependencies.size(); ++i)
                    {
                        if (i > 0) deps += ", ";
                        deps += mod.info.dependencies[i];
                    }
                ConsolePrint(deps, 0.7f, 0.7f, 0.7f);

                char mem[64];
                std::snprintf(mem, sizeof(mem), "Memory: %.1f MB", mod.memoryUsageMB);
                ConsolePrint(mem, 0.7f, 0.7f, 0.7f);
                ConsolePrint(std::string("Status: ") +
                             (mod.loaded ? "LOADED" : (mod.enabled ? "enabled" : "disabled")),
                             0.7f, 0.7f, 0.7f);
                found = true;
                break;
            }
        }
        if (!found)
            ConsolePrint("Mod not found: " + modName, 1.0f, 0.3f, 0.3f);
    }
    else
    {
        ConsolePrint("Unknown command: " + cmd, 1.0f, 0.3f, 0.3f);
    }
}

void ModdingPlaygroundModule::ConsolePrint(const std::string& text, float r, float g, float b)
{
    m_consoleLog.push_back({ text, r, g, b });
    if (static_cast<int>(m_consoleLog.size()) > kMaxConsoleLines)
        m_consoleLog.erase(m_consoleLog.begin());
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

void ModdingPlaygroundModule::OnUpdate(float dt)
{
    auto* input = m_ctx->GetInput();
    auto* world = m_ctx->GetWorld();
    if (!input || !world) return;

    // --- Toggle mod browser (M) ---------------------------------------------
    if (input->WasKeyPressed('M'))
    {
        m_modBrowserOpen = !m_modBrowserOpen;
        if (m_modBrowserOpen) m_consoleOpen = false;
    }

    // --- Toggle console (grave/tilde or C) ----------------------------------
    if (input->WasKeyPressed('C') && !m_modBrowserOpen)
    {
        m_consoleOpen = !m_consoleOpen;
        m_consoleInput.clear();
    }

    // --- Mod browser controls when open -------------------------------------
    if (m_modBrowserOpen)
    {
        if (input->WasKeyPressed('W') || input->WasKeyPressed(VK_UP))
            m_browserSelection = std::max(0, m_browserSelection - 1);
        if (input->WasKeyPressed('S') || input->WasKeyPressed(VK_DOWN))
            m_browserSelection = std::min(static_cast<int>(m_mods.size()) - 1,
                                           m_browserSelection + 1);
    }

    // --- Toggle mods 1/2/3 --------------------------------------------------
    if (input->WasKeyPressed('1'))
        ToggleMod(0);
    if (input->WasKeyPressed('2'))
        ToggleMod(1);
    if (input->WasKeyPressed('3'))
        ToggleMod(2);

    // --- Load enabled mods (L) ----------------------------------------------
    if (input->WasKeyPressed('L'))
        LoadEnabledMods();

    // --- Unload all mods (U) ------------------------------------------------
    if (input->WasKeyPressed('U'))
        UnloadAllMods();

    // --- Rescan (R) ---------------------------------------------------------
    if (input->WasKeyPressed('R'))
        RescanMods();

    // --- Console input handling (simplified: only Enter triggers command) ----
    if (m_consoleOpen && input->WasKeyPressed(VK_RETURN) && !m_consoleInput.empty())
    {
        ProcessConsoleCommand(m_consoleInput);
        m_consoleInput.clear();
    }

    // --- Weather animation --------------------------------------------------
    if (m_mods[2].loaded)
    {
        m_weatherTime += dt;
        // Animate rain entity position slightly
        for (auto e : m_mods[2].entities)
        {
            if (e == entt::null) continue;
            auto& nc = world->GetComponent<NameComponent>(e);
            if (nc.name == "Mod_RainEmitter")
            {
                auto& xf = world->GetComponent<Transform>(e);
                xf.position.y = 15.0f + std::sin(m_weatherTime * 0.5f) * 0.5f;
            }
            if (nc.name == "Mod_CloudLayer")
            {
                auto& xf = world->GetComponent<Transform>(e);
                xf.position.x = std::sin(m_weatherTime * 0.1f) * 5.0f;
            }
        }
    }

    // --- Camera orbit -------------------------------------------------------
    float yawRad   = m_camYaw * (kPi / 180.0f);
    float pitchRad = m_camPitch * (kPi / 180.0f);

    auto& camXf = world->GetComponent<Transform>(m_camera);
    camXf.position.x = std::sin(yawRad) * std::cos(pitchRad) * m_camDist;
    camXf.position.y = -std::sin(pitchRad) * m_camDist;
    camXf.position.z = std::cos(yawRad) * std::cos(pitchRad) * m_camDist;
    camXf.rotation.x = m_camPitch;
    camXf.rotation.y = m_camYaw + 180.0f;

    // Slow auto-rotate
    m_camYaw += 5.0f * dt;
}

// ---------------------------------------------------------------------------
// Render
// ---------------------------------------------------------------------------

void ModdingPlaygroundModule::OnRender()
{
    auto* dd = Spark::DebugDraw::GetInstance();
    if (!dd) return;

    DrawOverlay();

    if (m_modBrowserOpen)
        DrawModBrowser();

    if (m_consoleOpen)
        DrawConsole();

    // Weather debug visualization (rain lines)
    if (m_mods[2].loaded)
    {
        for (int i = 0; i < 30; ++i)
        {
            float rx = -12.0f + static_cast<float>(i % 10) * 2.5f +
                        std::sin(m_weatherTime * 3.0f + static_cast<float>(i)) * 0.5f;
            float rz = -12.0f + static_cast<float>(i / 10) * 8.0f +
                        std::cos(m_weatherTime * 2.0f + static_cast<float>(i)) * 0.5f;
            float ry = 12.0f - std::fmod(m_weatherTime * 8.0f + static_cast<float>(i) * 1.3f, 12.0f);

            dd->Line({ rx, ry, rz }, { rx - 0.1f, ry - 0.8f, rz },
                      { 0.5f, 0.6f, 0.9f, 0.4f });
        }
    }
}

// ---------------------------------------------------------------------------
// Mod browser UI
// ---------------------------------------------------------------------------

void ModdingPlaygroundModule::DrawModBrowser()
{
    auto* dd = Spark::DebugDraw::GetInstance();
    if (!dd) return;

    float cx = m_screenW * 0.5f;
    float cy = m_screenH * 0.5f;
    float boxW = 500.0f;
    float boxH = 350.0f;

    // Background
    dd->ScreenRect({ cx - boxW * 0.5f, cy - boxH * 0.5f },
                    { cx + boxW * 0.5f, cy + boxH * 0.5f },
                    { 0.08f, 0.08f, 0.12f, 0.95f });

    // Border
    dd->ScreenLine({ cx - boxW * 0.5f, cy - boxH * 0.5f },
                    { cx + boxW * 0.5f, cy - boxH * 0.5f },
                    { 0.3f, 0.5f, 0.8f, 1.0f });
    dd->ScreenLine({ cx + boxW * 0.5f, cy - boxH * 0.5f },
                    { cx + boxW * 0.5f, cy + boxH * 0.5f },
                    { 0.3f, 0.5f, 0.8f, 1.0f });
    dd->ScreenLine({ cx + boxW * 0.5f, cy + boxH * 0.5f },
                    { cx - boxW * 0.5f, cy + boxH * 0.5f },
                    { 0.3f, 0.5f, 0.8f, 1.0f });
    dd->ScreenLine({ cx - boxW * 0.5f, cy + boxH * 0.5f },
                    { cx - boxW * 0.5f, cy - boxH * 0.5f },
                    { 0.3f, 0.5f, 0.8f, 1.0f });

    // Title
    float startX = cx - boxW * 0.5f + 15.0f;
    float startY = cy - boxH * 0.5f + 15.0f;
    dd->ScreenText({ startX, startY }, "MOD BROWSER", { 1.0f, 1.0f, 1.0f, 1.0f });
    startY += 28.0f;

    // Separator
    dd->ScreenLine({ cx - boxW * 0.5f + 10.0f, startY },
                    { cx + boxW * 0.5f - 10.0f, startY },
                    { 0.3f, 0.3f, 0.4f, 1.0f });
    startY += 10.0f;

    // Mod list
    char buf[256];
    for (int i = 0; i < static_cast<int>(m_mods.size()); ++i)
    {
        auto& mod = m_mods[i];
        bool selected = (i == m_browserSelection);

        // Selection highlight
        if (selected)
        {
            dd->ScreenRect({ startX - 5.0f, startY - 2.0f },
                            { cx + boxW * 0.5f - 10.0f, startY + 60.0f },
                            { 0.15f, 0.25f, 0.4f, 0.6f });
        }

        // Status indicator
        const char* statusStr = "OFF";
        float sr = 0.5f, sg = 0.5f, sb = 0.5f;
        if (mod.loaded)
        {
            statusStr = "LOADED";
            sr = 0.2f; sg = 0.9f; sb = 0.3f;
        }
        else if (mod.enabled)
        {
            statusStr = "ENABLED";
            sr = 0.9f; sg = 0.8f; sb = 0.2f;
        }

        // Mod name + version + status
        std::snprintf(buf, sizeof(buf), "[%d] %s v%s  [%s]",
                      i + 1, mod.info.name.c_str(), mod.info.version.c_str(), statusStr);
        dd->ScreenText({ startX, startY }, buf, { sr, sg, sb, 1.0f });
        startY += 16.0f;

        // Author
        std::snprintf(buf, sizeof(buf), "    Author: %s", mod.info.author.c_str());
        dd->ScreenText({ startX, startY }, buf, { 0.6f, 0.6f, 0.6f, 0.8f });
        startY += 14.0f;

        // Description
        std::snprintf(buf, sizeof(buf), "    %s", mod.info.description.c_str());
        dd->ScreenText({ startX, startY }, buf, { 0.5f, 0.5f, 0.5f, 0.8f });
        startY += 14.0f;

        // Dependencies
        if (!mod.info.dependencies.empty())
        {
            std::string deps = "    Depends: ";
            for (size_t d = 0; d < mod.info.dependencies.size(); ++d)
            {
                if (d > 0) deps += ", ";
                deps += mod.info.dependencies[d];
            }
            dd->ScreenText({ startX, startY }, deps.c_str(), { 0.7f, 0.4f, 0.3f, 0.8f });
            startY += 14.0f;
        }

        // Memory
        std::snprintf(buf, sizeof(buf), "    Memory: %.1f MB  |  Entities: %zu",
                      mod.memoryUsageMB, mod.entities.size());
        dd->ScreenText({ startX, startY }, buf, { 0.4f, 0.4f, 0.4f, 0.7f });
        startY += 20.0f;
    }
}

// ---------------------------------------------------------------------------
// Console UI
// ---------------------------------------------------------------------------

void ModdingPlaygroundModule::DrawConsole()
{
    auto* dd = Spark::DebugDraw::GetInstance();
    if (!dd) return;

    float consoleH = 300.0f;
    float consoleW = m_screenW - 20.0f;
    float startX = 10.0f;
    float startY = m_screenH - consoleH - 10.0f;

    // Background
    dd->ScreenRect({ startX, startY },
                    { startX + consoleW, startY + consoleH },
                    { 0.0f, 0.0f, 0.0f, 0.85f });

    // Border
    dd->ScreenLine({ startX, startY }, { startX + consoleW, startY },
                    { 0.3f, 0.3f, 0.3f, 1.0f });

    // Title
    dd->ScreenText({ startX + 8.0f, startY + 6.0f }, "CONSOLE (type commands, Enter to submit)",
                    { 0.5f, 0.5f, 0.5f, 1.0f });

    // Log entries
    float logY = startY + 28.0f;
    for (auto& entry : m_consoleLog)
    {
        dd->ScreenText({ startX + 8.0f, logY }, entry.text.c_str(),
                        { entry.r, entry.g, entry.b, 1.0f });
        logY += 14.0f;
    }

    // Input line
    float inputY = startY + consoleH - 22.0f;
    dd->ScreenLine({ startX, inputY - 2.0f }, { startX + consoleW, inputY - 2.0f },
                    { 0.3f, 0.3f, 0.3f, 1.0f });

    std::string inputDisplay = "> " + m_consoleInput + "_";
    dd->ScreenText({ startX + 8.0f, inputY + 2.0f }, inputDisplay.c_str(),
                    { 0.0f, 1.0f, 0.5f, 1.0f });
}

// ---------------------------------------------------------------------------
// Main overlay
// ---------------------------------------------------------------------------

void ModdingPlaygroundModule::DrawOverlay()
{
    auto* dd = Spark::DebugDraw::GetInstance();
    if (!dd) return;

    char buf[256];
    float x = 10.0f;
    float y = 10.0f;

    dd->ScreenText({ x, y }, "MODDING PLAYGROUND", { 1.0f, 1.0f, 1.0f, 1.0f });
    y += 22.0f;

    // Mod status summary
    for (int i = 0; i < static_cast<int>(m_mods.size()); ++i)
    {
        auto& mod = m_mods[i];
        const char* status = mod.loaded ? "LOADED" : (mod.enabled ? "enabled" : "off");
        float sr = mod.loaded ? 0.2f : (mod.enabled ? 0.8f : 0.5f);
        float sg = mod.loaded ? 0.9f : (mod.enabled ? 0.8f : 0.5f);
        float sb = mod.loaded ? 0.3f : (mod.enabled ? 0.2f : 0.5f);

        std::snprintf(buf, sizeof(buf), "[%d] %s: %s",
                      i + 1, mod.info.name.c_str(), status);
        dd->ScreenText({ x, y }, buf, { sr, sg, sb, 1.0f });
        y += 16.0f;
    }

    y += 8.0f;

    // Load order
    dd->ScreenText({ x, y }, "Load Order:", { 0.6f, 0.6f, 0.6f, 0.8f });
    y += 14.0f;
    int loadOrder = 1;
    for (auto& mod : m_mods)
    {
        if (!mod.loaded) continue;
        std::snprintf(buf, sizeof(buf), "  %d. %s", loadOrder++, mod.info.name.c_str());
        dd->ScreenText({ x, y }, buf, { 0.5f, 0.8f, 1.0f, 0.8f });
        y += 14.0f;
    }

    // Total memory
    float totalMem = 0.0f;
    for (auto& mod : m_mods)
        if (mod.loaded) totalMem += mod.memoryUsageMB;
    std::snprintf(buf, sizeof(buf), "Mod Memory: %.1f MB", totalMem);
    dd->ScreenText({ x, y + 8.0f }, buf, { 0.7f, 0.7f, 0.7f, 0.8f });

    // Controls
    float helpY = m_screenH - 50.0f;
    dd->ScreenText({ 10.0f, helpY },
                    "M: Mod browser | 1/2/3: Toggle mods | L: Load | U: Unload | R: Rescan | C: Console",
                    { 0.5f, 0.5f, 0.5f, 0.8f });
}

// ---------------------------------------------------------------------------
// Resize
// ---------------------------------------------------------------------------

void ModdingPlaygroundModule::OnResize(int w, int h)
{
    m_screenW = static_cast<float>(w);
    m_screenH = static_cast<float>(h);
}

// ---------------------------------------------------------------------------
// DLL exports
// ---------------------------------------------------------------------------

SPARK_IMPLEMENT_MODULE(ModdingPlaygroundModule)
