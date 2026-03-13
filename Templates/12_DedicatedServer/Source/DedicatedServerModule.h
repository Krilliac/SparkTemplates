#pragma once
/**
 * @file DedicatedServerModule.h
 * @brief ADVANCED — Headless dedicated game server with authoritative simulation.
 *
 * This module is designed to run WITHOUT a window, GPU, or audio device via:
 *     SparkEngine.exe -headless -game DedicatedServer.dll
 *
 * It demonstrates a fully authoritative FPS deathmatch server that:
 *   - Accepts client connections over UDP
 *   - Runs server-side physics for hit validation
 *   - Manages AI bots that fill empty player slots
 *   - Replicates authoritative entity state to all clients
 *   - Records lag compensation snapshots for hit rewinding
 *   - Tracks scores, rounds, and match lifecycle
 *   - Auto-saves match state periodically
 *   - Provides a rich server console for live administration
 *   - Logs all events to stdout (visible in the headless console)
 *
 * This module NEVER touches GraphicsEngine, InputManager, or AudioEngine.
 * It detects headless mode via ctx->IsHeadless() and operates accordingly.
 *
 * Spark Engine systems used:
 *   Networking  — NetworkManager, NetworkIdentity, LagCompensator
 *   ECS         — World, Transform, NameComponent, HealthComponent, TagComponent
 *   Physics     — RigidBodyComponent, ColliderComponent (server-side validation)
 *   AI          — AISystem, BehaviorTree (server-side bots)
 *   Events      — EventBus (damage, kills, connections, round changes)
 *   SaveSystem  — Auto-save match state, leaderboard persistence
 *   Coroutines  — Timed match phases (warmup, playing, cooldown)
 *   DayNightCycle / Weather — Server-authoritative world state
 */

#include <Core/ModuleManager.h>
#include <Core/EngineContext.h>
#include <Engine/ECS/Components.h>
#include <Engine/Networking/NetworkManager.h>
#include <Engine/AI/AISystem.h>
#include <Engine/AI/BehaviorTree.h>
#include <Engine/Events/EventSystem.h>
#include <Engine/SaveSystem/SaveSystem.h>
#include <Engine/Coroutine/CoroutineScheduler.h>
#include <vector>
#include <unordered_map>
#include <string>
#include <cstdio>

class DedicatedServerModule final : public Spark::IModule
{
public:
    Spark::ModuleInfo GetModuleInfo() const override
    {
        return { "DedicatedServer", "0.2.0", SPARK_SDK_VERSION, 1000 };
    }

    void OnLoad(Spark::IEngineContext* ctx) override;
    void OnUnload() override;
    void OnUpdate(float dt) override;

    // OnRender() is intentionally not overridden — default no-op.
    // This module never renders anything.

    void OnResize(int, int) override { /* no window in headless mode */ }

private:
    // --- Match state machine ---
    enum class MatchPhase { WaitingForPlayers, Warmup, Playing, Cooldown, MapRotation };

    // --- Server setup ---
    void BuildArena();
    void RegisterServerConsoleCommands();
    void SetupEventSubscriptions();

    // --- Player management ---
    void OnClientConnected(uint32_t clientID, const std::string& playerName);
    void OnClientDisconnected(uint32_t clientID);
    entt::entity SpawnPlayerEntity(uint32_t clientID, const std::string& name);
    void RespawnPlayer(uint32_t clientID);
    DirectX::XMFLOAT3 GetSpawnPoint(int index);

    // --- Bot management ---
    void FillWithBots();
    void RemoveBot();
    entt::entity SpawnBot(const std::string& name);

    // --- Match flow ---
    void StartWarmup();
    void StartMatch();
    void EndMatch();
    void RotateMap();
    void ResetScores();

    // --- Server tick ---
    void ProcessClientInputs();
    void RunPhysicsValidation();
    void UpdateReplication(float dt);
    void CheckMatchConditions();
    void PerformAutoSave();

    // --- Logging ---
    void Log(const char* fmt, ...);
    void LogEvent(const std::string& event);

    Spark::IEngineContext* m_ctx = nullptr;

    // --- Server config ---
    struct ServerConfig
    {
        uint16_t port           = 27015;
        int      maxPlayers     = 16;
        int      botFillCount   = 8;   // fill to this many total players
        float    tickRate       = 60.f; // Hz (managed by engine loop)
        float    roundTimeLimit = 300.f; // 5 minutes
        int      scoreLimit     = 50;
        float    warmupTime     = 10.f;
        float    cooldownTime   = 8.f;
        float    autoSaveInterval = 60.f;
        float    respawnDelay   = 3.f;
        std::string mapName     = "combat_arena";
    };
    ServerConfig m_config;

    // --- Match state ---
    MatchPhase m_phase        = MatchPhase::WaitingForPlayers;
    float      m_phaseTimer   = 0.0f;
    float      m_matchTime    = 0.0f;
    float      m_saveTimer    = 0.0f;
    float      m_replicateTimer = 0.0f;
    int        m_roundNumber  = 0;

    // --- Connected players ---
    struct PlayerSlot
    {
        uint32_t    clientID    = 0;
        std::string name;
        entt::entity entity     = entt::null;
        int         kills       = 0;
        int         deaths      = 0;
        int         score       = 0;
        float       ping        = 0.0f;
        bool        isBot       = false;
        bool        alive       = true;
        float       respawnTimer = 0.0f;
    };
    std::unordered_map<uint32_t, PlayerSlot> m_players;
    uint32_t m_nextBotID = 10000; // bot IDs start high to avoid collision

    // --- Arena entities ---
    entt::entity m_arenaFloor = entt::null;
    std::vector<entt::entity> m_arenaWalls;
    std::vector<entt::entity> m_arenaCovers;
    std::vector<entt::entity> m_pickups;

    // --- Event subscriptions ---
    Spark::SubscriptionID m_damageSub    = 0;
    Spark::SubscriptionID m_killSub      = 0;
    Spark::SubscriptionID m_connectSub   = 0;
    Spark::SubscriptionID m_disconnectSub = 0;
};
