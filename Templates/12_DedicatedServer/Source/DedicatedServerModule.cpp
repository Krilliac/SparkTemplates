/**
 * @file DedicatedServerModule.cpp
 * @brief ADVANCED — Headless dedicated game server with authoritative simulation.
 *
 * Launch with:  SparkEngine.exe -headless -game DedicatedServer.dll
 *
 * This module demonstrates a complete authoritative multiplayer server that
 * runs without any graphics, input, or audio. Every system it uses
 * (ECS, physics, AI, networking, events, save, coroutines) works in headless mode.
 */

#include "DedicatedServerModule.h"
#include <cstdarg>
#include <ctime>
#include <algorithm>

// ---------------------------------------------------------------------------
// Logging helper — prints to stdout (visible in headless console)
// ---------------------------------------------------------------------------

void DedicatedServerModule::Log(const char* fmt, ...)
{
    // Timestamp
    time_t now = time(nullptr);
    struct tm t;
#ifdef _WIN32
    localtime_s(&t, &now);
#else
    localtime_r(&now, &t);
#endif
    printf("[%02d:%02d:%02d] ", t.tm_hour, t.tm_min, t.tm_sec);

    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
    fflush(stdout);
}

void DedicatedServerModule::LogEvent(const std::string& event)
{
    Log("[EVENT] %s", event.c_str());
}

// ===========================================================================
// Lifecycle
// ===========================================================================

void DedicatedServerModule::OnLoad(Spark::IEngineContext* ctx)
{
    m_ctx = ctx;

    // --- Verify headless mode ---
    // This module works in both modes, but is designed for headless.
    if (ctx->IsHeadless())
    {
        Log("Running in HEADLESS dedicated server mode");
        Log("  No window, no GPU, no audio");
    }
    else
    {
        Log("WARNING: Running in windowed mode — consider using -headless for production servers");
    }

    Log("Server config: port=%d maxPlayers=%d botFill=%d scoreLimit=%d roundTime=%.0fs",
        m_config.port, m_config.maxPlayers, m_config.botFillCount,
        m_config.scoreLimit, m_config.roundTimeLimit);

    // --- Start the network server ---
    auto* netMgr = Spark::NetworkManager::GetInstance();
    if (netMgr)
    {
        netMgr->StartServer(m_config.port, m_config.maxPlayers);
        Log("Network server started on port %d (max %d clients)", m_config.port, m_config.maxPlayers);

        // Register connection/disconnection callbacks
        netMgr->SetOnClientConnected([this](uint32_t id, const std::string& name) {
            OnClientConnected(id, name);
        });
        netMgr->SetOnClientDisconnected([this](uint32_t id) {
            OnClientDisconnected(id);
        });
    }
    else
    {
        Log("ERROR: NetworkManager not available — server cannot accept connections");
    }

    // --- Build the server-side arena (physics colliders only, no rendering) ---
    BuildArena();

    // --- Subscribe to gameplay events ---
    SetupEventSubscriptions();

    // --- Register server admin console commands ---
    RegisterServerConsoleCommands();

    // --- Initialize save system for leaderboards ---
    auto* saveSys = Spark::SaveSystem::GetInstance();
    if (saveSys)
    {
        Log("SaveSystem ready — auto-save every %.0fs", m_config.autoSaveInterval);
    }

    // --- Fill empty slots with AI bots ---
    FillWithBots();

    // --- Start in waiting phase ---
    m_phase = MatchPhase::WaitingForPlayers;
    Log("Server ready. Waiting for players...");
}

void DedicatedServerModule::OnUnload()
{
    Log("Server shutting down...");

    // Disconnect all clients
    auto* netMgr = Spark::NetworkManager::GetInstance();
    if (netMgr)
    {
        netMgr->BroadcastMessage("Server shutting down", Spark::ChannelType::Reliable);
        netMgr->ShutdownServer();
    }

    // Unsubscribe events
    if (auto* bus = m_ctx ? m_ctx->GetEventBus() : nullptr)
    {
        bus->Unsubscribe<Spark::EntityDamagedEvent>(m_damageSub);
        bus->Unsubscribe<Spark::EntityKilledEvent>(m_killSub);
        bus->Unsubscribe<Spark::ClientConnectedEvent>(m_connectSub);
        bus->Unsubscribe<Spark::ClientDisconnectedEvent>(m_disconnectSub);
    }

    // Destroy all entities
    auto* world = m_ctx ? m_ctx->GetWorld() : nullptr;
    if (world)
    {
        for (auto& [id, slot] : m_players)
            if (slot.entity != entt::null) world->DestroyEntity(slot.entity);

        if (m_arenaFloor != entt::null) world->DestroyEntity(m_arenaFloor);
        for (auto e : m_arenaWalls) if (e != entt::null) world->DestroyEntity(e);
        for (auto e : m_arenaCovers) if (e != entt::null) world->DestroyEntity(e);
        for (auto e : m_pickups) if (e != entt::null) world->DestroyEntity(e);
    }

    m_players.clear();
    m_arenaWalls.clear();
    m_arenaCovers.clear();
    m_pickups.clear();

    // Final save
    PerformAutoSave();

    Log("Server shut down cleanly. %d rounds played.", m_roundNumber);
    m_ctx = nullptr;
}

// ===========================================================================
// Arena construction — physics only, no MeshRenderer
// ===========================================================================

void DedicatedServerModule::BuildArena()
{
    auto* world = m_ctx->GetWorld();

    Log("Building server-side arena: %s", m_config.mapName.c_str());

    // Floor — static collider, no visual mesh needed on the server
    m_arenaFloor = world->CreateEntity();
    world->AddComponent<NameComponent>(m_arenaFloor, NameComponent{ "ArenaFloor" });
    world->AddComponent<Transform>(m_arenaFloor, Transform{
        { 0.f, -0.5f, 0.f }, { 0.f, 0.f, 0.f }, { 60.f, 1.f, 60.f }
    });
    world->AddComponent<RigidBodyComponent>(m_arenaFloor, RigidBodyComponent{
        RigidBodyComponent::Type::Static, 0.f, 0.8f, 0.2f
    });
    world->AddComponent<ColliderComponent>(m_arenaFloor, ColliderComponent{
        ColliderComponent::Shape::Box, { 30.f, 0.5f, 30.f }
    });

    // Walls — physics colliders for boundary enforcement
    auto makeWall = [&](const char* name, DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 halfExt) {
        auto e = world->CreateEntity();
        world->AddComponent<NameComponent>(e, NameComponent{ name });
        world->AddComponent<Transform>(e, Transform{
            pos, { 0.f, 0.f, 0.f },
            { halfExt.x * 2.f, halfExt.y * 2.f, halfExt.z * 2.f }
        });
        world->AddComponent<RigidBodyComponent>(e, RigidBodyComponent{
            RigidBodyComponent::Type::Static, 0.f, 0.8f, 0.2f
        });
        world->AddComponent<ColliderComponent>(e, ColliderComponent{
            ColliderComponent::Shape::Box, halfExt
        });
        m_arenaWalls.push_back(e);
    };

    makeWall("WallN", { 0.f, 2.5f, 30.f },  { 30.f, 2.5f, 0.5f });
    makeWall("WallS", { 0.f, 2.5f, -30.f }, { 30.f, 2.5f, 0.5f });
    makeWall("WallE", { 30.f, 2.5f, 0.f },  { 0.5f, 2.5f, 30.f });
    makeWall("WallW", { -30.f, 2.5f, 0.f }, { 0.5f, 2.5f, 30.f });

    // Cover objects — physics-only obstacles
    auto makeCover = [&](const char* name, DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 halfExt) {
        auto e = world->CreateEntity();
        world->AddComponent<NameComponent>(e, NameComponent{ name });
        world->AddComponent<Transform>(e, Transform{
            pos, { 0.f, 0.f, 0.f },
            { halfExt.x * 2.f, halfExt.y * 2.f, halfExt.z * 2.f }
        });
        world->AddComponent<RigidBodyComponent>(e, RigidBodyComponent{
            RigidBodyComponent::Type::Static, 0.f, 0.6f, 0.1f
        });
        world->AddComponent<ColliderComponent>(e, ColliderComponent{
            ColliderComponent::Shape::Box, halfExt
        });
        m_arenaCovers.push_back(e);
    };

    makeCover("Cover_A", { -10.f, 1.f, 5.f },   { 2.f, 1.f, 0.5f });
    makeCover("Cover_B", { 8.f, 1.f, -8.f },    { 0.5f, 1.f, 3.f });
    makeCover("Cover_C", { 0.f, 1.f, 15.f },    { 3.f, 1.f, 1.f });
    makeCover("Cover_D", { -15.f, 1.f, -10.f }, { 1.f, 1.f, 2.f });
    makeCover("Cover_E", { 15.f, 1.f, 12.f },   { 2.f, 1.f, 1.5f });
    makeCover("Cover_F", { 5.f, 1.5f, 0.f },    { 1.5f, 1.5f, 1.5f });

    Log("Arena built: 1 floor + 4 walls + 6 cover objects (physics-only, no meshes)");
}

// ===========================================================================
// Spawn points
// ===========================================================================

DirectX::XMFLOAT3 DedicatedServerModule::GetSpawnPoint(int index)
{
    // 8 spawn points arranged in a circle
    static const DirectX::XMFLOAT3 spawns[] = {
        { -20.f, 1.f, 0.f },   { 20.f, 1.f, 0.f },
        { 0.f, 1.f, -20.f },   { 0.f, 1.f, 20.f },
        { -15.f, 1.f, -15.f }, { 15.f, 1.f, 15.f },
        { -15.f, 1.f, 15.f },  { 15.f, 1.f, -15.f }
    };
    return spawns[index % 8];
}

// ===========================================================================
// Player management
// ===========================================================================

void DedicatedServerModule::OnClientConnected(uint32_t clientID, const std::string& playerName)
{
    Log("Player connected: %s (clientID=%u)", playerName.c_str(), clientID);

    // Remove a bot to make room if at capacity
    int humanCount = 0;
    for (auto& [id, slot] : m_players)
        if (!slot.isBot) humanCount++;

    if (humanCount >= m_config.maxPlayers)
    {
        Log("Server full — rejecting %s", playerName.c_str());
        auto* netMgr = Spark::NetworkManager::GetInstance();
        if (netMgr)
            netMgr->SendMessageToClient(clientID, "Server is full",
                                         Spark::ChannelType::Reliable);
        return;
    }

    // Remove one bot to make room for the human player
    RemoveBot();

    // Spawn the player entity
    auto entity = SpawnPlayerEntity(clientID, playerName);

    PlayerSlot slot{};
    slot.clientID = clientID;
    slot.name     = playerName;
    slot.entity   = entity;
    slot.isBot    = false;
    slot.alive    = true;
    m_players[clientID] = slot;

    // Register for network replication
    auto* netMgr = Spark::NetworkManager::GetInstance();
    if (netMgr)
    {
        netMgr->RegisterReplicatedEntity(entity, clientID);
        netMgr->BroadcastMessage(playerName + " joined the game",
                                  Spark::ChannelType::Reliable);
    }

    // Start warmup if we have enough players
    if (m_phase == MatchPhase::WaitingForPlayers)
    {
        int total = 0;
        for (auto& [id, s] : m_players)
            if (!s.isBot) total++;
        if (total >= 2)
            StartWarmup();
    }

    if (auto* bus = m_ctx->GetEventBus())
        bus->Publish(Spark::ClientConnectedEvent{ clientID, playerName });
}

void DedicatedServerModule::OnClientDisconnected(uint32_t clientID)
{
    auto it = m_players.find(clientID);
    if (it == m_players.end()) return;

    Log("Player disconnected: %s (clientID=%u, score=%d)",
        it->second.name.c_str(), clientID, it->second.score);

    // Destroy their entity
    auto* world = m_ctx->GetWorld();
    if (world && it->second.entity != entt::null)
        world->DestroyEntity(it->second.entity);

    // Unregister from replication
    auto* netMgr = Spark::NetworkManager::GetInstance();
    if (netMgr)
    {
        netMgr->UnregisterReplicatedEntity(clientID);
        netMgr->BroadcastMessage(it->second.name + " left the game",
                                  Spark::ChannelType::Reliable);
    }

    m_players.erase(it);

    // Backfill with a bot
    FillWithBots();

    if (auto* bus = m_ctx->GetEventBus())
        bus->Publish(Spark::ClientDisconnectedEvent{ clientID });
}

entt::entity DedicatedServerModule::SpawnPlayerEntity(uint32_t clientID,
                                                       const std::string& name)
{
    auto* world = m_ctx->GetWorld();
    auto e = world->CreateEntity();

    auto spawnPos = GetSpawnPoint(static_cast<int>(clientID));

    world->AddComponent<NameComponent>(e, NameComponent{ name });
    world->AddComponent<Transform>(e, Transform{
        spawnPos, { 0.f, 0.f, 0.f }, { 1.f, 2.f, 1.f }
    });
    // No MeshRenderer — server doesn't render
    world->AddComponent<HealthComponent>(e, HealthComponent{ 100.f, 100.f, false });
    world->AddComponent<RigidBodyComponent>(e, RigidBodyComponent{
        RigidBodyComponent::Type::Kinematic, 0.f, 0.5f, 0.0f
    });
    world->AddComponent<ColliderComponent>(e, ColliderComponent{
        ColliderComponent::Shape::Capsule, { 0.4f, 1.f, 0.4f }, 0.4f, 2.0f
    });
    world->AddComponent<NetworkIdentity>(e, NetworkIdentity{
        clientID, clientID, false, true, true
    });
    world->AddComponent<TagComponent>(e, TagComponent{});
    world->GetComponent<TagComponent>(e).AddTag("player");

    return e;
}

void DedicatedServerModule::RespawnPlayer(uint32_t clientID)
{
    auto it = m_players.find(clientID);
    if (it == m_players.end()) return;

    auto* world = m_ctx->GetWorld();
    if (!world || it->second.entity == entt::null) return;

    auto spawnPos = GetSpawnPoint(static_cast<int>(clientID + m_roundNumber));

    auto& t = world->GetComponent<Transform>(it->second.entity);
    t.position = spawnPos;

    auto& hp = world->GetComponent<HealthComponent>(it->second.entity);
    hp.current = hp.max;
    hp.isDead = false;

    it->second.alive = true;
    it->second.respawnTimer = 0.0f;

    Log("Respawned %s at (%.0f, %.0f, %.0f)",
        it->second.name.c_str(), spawnPos.x, spawnPos.y, spawnPos.z);
}

// ===========================================================================
// Bot management
// ===========================================================================

void DedicatedServerModule::FillWithBots()
{
    int totalPlayers = static_cast<int>(m_players.size());
    int botsNeeded = m_config.botFillCount - totalPlayers;

    for (int i = 0; i < botsNeeded; ++i)
    {
        std::string botName = "Bot_" + std::to_string(m_nextBotID - 10000);
        SpawnBot(botName);
    }

    if (botsNeeded > 0)
        Log("Spawned %d bot(s) to fill server (%d/%d slots)",
            botsNeeded, (int)m_players.size(), m_config.botFillCount);
}

void DedicatedServerModule::RemoveBot()
{
    for (auto it = m_players.begin(); it != m_players.end(); ++it)
    {
        if (it->second.isBot)
        {
            auto* world = m_ctx->GetWorld();
            if (world && it->second.entity != entt::null)
                world->DestroyEntity(it->second.entity);

            Log("Removed bot: %s (making room for human player)", it->second.name.c_str());
            m_players.erase(it);
            return;
        }
    }
}

entt::entity DedicatedServerModule::SpawnBot(const std::string& name)
{
    auto* world = m_ctx->GetWorld();
    uint32_t botID = m_nextBotID++;

    auto e = world->CreateEntity();
    auto spawnPos = GetSpawnPoint(static_cast<int>(botID));

    world->AddComponent<NameComponent>(e, NameComponent{ name });
    world->AddComponent<Transform>(e, Transform{
        spawnPos, { 0.f, 0.f, 0.f }, { 1.f, 2.f, 1.f }
    });
    world->AddComponent<HealthComponent>(e, HealthComponent{ 100.f, 100.f, false });
    world->AddComponent<RigidBodyComponent>(e, RigidBodyComponent{
        RigidBodyComponent::Type::Kinematic, 0.f, 0.5f, 0.0f
    });
    world->AddComponent<ColliderComponent>(e, ColliderComponent{
        ColliderComponent::Shape::Capsule, { 0.4f, 1.f, 0.4f }, 0.4f, 2.0f
    });

    // AI component for bot behavior
    world->AddComponent<AIComponent>(e, AIComponent{
        AIComponent::State::Idle, "bot_combat", entt::null,
        { 0.f, 0.f, 0.f }, {},
        { 20.f, 8.f, 5.f, 0.6f, 0.3f }
    });

    // Register behavior tree
    if (auto* aiSys = m_ctx->GetAISystem())
    {
        auto tree = Spark::BehaviorTree::CreateCombatBehavior();
        auto& bb = tree->GetBlackboard();
        bb.SetFloat("aggressiveness", 0.7f);
        bb.SetFloat("accuracy", 0.5f + (float)(rand() % 30) * 0.01f);
        aiSys->RegisterBehavior(name + "_bt", std::move(tree));
    }

    PlayerSlot slot{};
    slot.clientID = botID;
    slot.name     = name;
    slot.entity   = e;
    slot.isBot    = true;
    slot.alive    = true;
    m_players[botID] = slot;

    return e;
}

// ===========================================================================
// Match flow
// ===========================================================================

void DedicatedServerModule::StartWarmup()
{
    m_phase = MatchPhase::Warmup;
    m_phaseTimer = 0.0f;
    Log("=== WARMUP PHASE === (%0.fs)", m_config.warmupTime);

    auto* netMgr = Spark::NetworkManager::GetInstance();
    if (netMgr)
        netMgr->BroadcastMessage("Warmup started! Match begins in "
            + std::to_string((int)m_config.warmupTime) + " seconds",
            Spark::ChannelType::Reliable);
}

void DedicatedServerModule::StartMatch()
{
    m_phase = MatchPhase::Playing;
    m_phaseTimer = 0.0f;
    m_matchTime = 0.0f;
    m_roundNumber++;
    ResetScores();

    Log("=== ROUND %d STARTED === (scoreLimit=%d, timeLimit=%.0fs)",
        m_roundNumber, m_config.scoreLimit, m_config.roundTimeLimit);

    auto* netMgr = Spark::NetworkManager::GetInstance();
    if (netMgr)
        netMgr->BroadcastMessage("Round " + std::to_string(m_roundNumber)
            + " — FIGHT!", Spark::ChannelType::Reliable);

    // Respawn everyone
    for (auto& [id, slot] : m_players)
        RespawnPlayer(id);
}

void DedicatedServerModule::EndMatch()
{
    m_phase = MatchPhase::Cooldown;
    m_phaseTimer = 0.0f;

    // Find winner
    std::string winner = "Nobody";
    int highScore = 0;
    for (auto& [id, slot] : m_players)
    {
        if (slot.score > highScore)
        {
            highScore = slot.score;
            winner = slot.name;
        }
    }

    Log("=== ROUND %d ENDED === Winner: %s (score=%d)", m_roundNumber, winner.c_str(), highScore);

    // Print scoreboard
    Log("--- SCOREBOARD ---");
    std::vector<std::pair<uint32_t, PlayerSlot*>> sorted;
    for (auto& [id, slot] : m_players)
        sorted.push_back({ id, &slot });
    std::sort(sorted.begin(), sorted.end(),
        [](auto& a, auto& b) { return a.second->score > b.second->score; });

    int rank = 1;
    for (auto& [id, slot] : sorted)
    {
        Log("  #%d  %-20s  K:%d  D:%d  Score:%d  %s",
            rank++, slot->name.c_str(), slot->kills, slot->deaths,
            slot->score, slot->isBot ? "(BOT)" : "");
    }
    Log("------------------");

    auto* netMgr = Spark::NetworkManager::GetInstance();
    if (netMgr)
        netMgr->BroadcastMessage("Round over! Winner: " + winner
            + " (score: " + std::to_string(highScore) + ")",
            Spark::ChannelType::Reliable);

    // Save match results
    PerformAutoSave();
}

void DedicatedServerModule::RotateMap()
{
    Log("Map rotation (placeholder — restarting on same map)");
    m_phase = MatchPhase::WaitingForPlayers;

    // In a real server, load next map from rotation list
    // For now, restart warmup if we have players
    int humans = 0;
    for (auto& [id, slot] : m_players)
        if (!slot.isBot) humans++;
    if (humans >= 1)
        StartWarmup();
}

void DedicatedServerModule::ResetScores()
{
    for (auto& [id, slot] : m_players)
    {
        slot.kills = 0;
        slot.deaths = 0;
        slot.score = 0;
    }
}

// ===========================================================================
// Event subscriptions
// ===========================================================================

void DedicatedServerModule::SetupEventSubscriptions()
{
    auto* bus = m_ctx->GetEventBus();
    if (!bus) return;

    m_damageSub = bus->Subscribe<Spark::EntityDamagedEvent>(
        [this](const Spark::EntityDamagedEvent& e) {
            auto* world = m_ctx->GetWorld();
            if (!world) return;
            std::string name = "Unknown";
            if (world->HasComponent<NameComponent>(e.entity))
                name = world->GetComponent<NameComponent>(e.entity).name;
            LogEvent(name + " took " + std::to_string((int)e.damage) + " damage");
        });

    m_killSub = bus->Subscribe<Spark::EntityKilledEvent>(
        [this](const Spark::EntityKilledEvent& e) {
            // Find the killed player and update stats
            for (auto& [id, slot] : m_players)
            {
                if (slot.entity == e.entity)
                {
                    slot.deaths++;
                    slot.alive = false;
                    slot.respawnTimer = m_config.respawnDelay;

                    Log("KILL: %s was eliminated (deaths=%d)", slot.name.c_str(), slot.deaths);

                    // Award kill to the attacker
                    if (e.killer != entt::null)
                    {
                        for (auto& [kid, kslot] : m_players)
                        {
                            if (kslot.entity == e.killer)
                            {
                                kslot.kills++;
                                kslot.score += 10;
                                Log("  Killer: %s (kills=%d, score=%d)",
                                    kslot.name.c_str(), kslot.kills, kslot.score);
                                break;
                            }
                        }
                    }
                    break;
                }
            }
        });
}

// ===========================================================================
// Server console commands
// ===========================================================================

void DedicatedServerModule::RegisterServerConsoleCommands()
{
    auto& console = Spark::SimpleConsole::GetInstance();

    console.RegisterCommand("sv_status",
        [this](const std::vector<std::string>&) -> std::string {
            std::string result = "=== Server Status ===\n";
            result += "Map: " + m_config.mapName + "\n";
            result += "Phase: ";
            switch (m_phase) {
                case MatchPhase::WaitingForPlayers: result += "Waiting for Players\n"; break;
                case MatchPhase::Warmup: result += "Warmup\n"; break;
                case MatchPhase::Playing: result += "Playing (round " + std::to_string(m_roundNumber) + ")\n"; break;
                case MatchPhase::Cooldown: result += "Cooldown\n"; break;
                case MatchPhase::MapRotation: result += "Map Rotation\n"; break;
            }
            int humans = 0, bots = 0;
            for (auto& [id, slot] : m_players)
                slot.isBot ? bots++ : humans++;
            result += "Players: " + std::to_string(humans) + " humans, " + std::to_string(bots) + " bots\n";
            if (m_phase == MatchPhase::Playing)
                result += "Match time: " + std::to_string((int)m_matchTime) + "s / "
                        + std::to_string((int)m_config.roundTimeLimit) + "s\n";
            return result;
        }, "Show server status", "Server");

    console.RegisterCommand("sv_players",
        [this](const std::vector<std::string>&) -> std::string {
            std::string result = "=== Players ===\n";
            for (auto& [id, slot] : m_players)
            {
                result += "  " + slot.name + " | K:" + std::to_string(slot.kills)
                        + " D:" + std::to_string(slot.deaths)
                        + " S:" + std::to_string(slot.score)
                        + (slot.isBot ? " (BOT)" : "")
                        + (slot.alive ? "" : " [DEAD]") + "\n";
            }
            return result;
        }, "List all players", "Server");

    console.RegisterCommand("sv_kick",
        [this](const std::vector<std::string>& args) -> std::string {
            if (args.empty()) return "Usage: sv_kick <player_name>";
            for (auto& [id, slot] : m_players)
            {
                if (slot.name == args[0] && !slot.isBot)
                {
                    auto* netMgr = Spark::NetworkManager::GetInstance();
                    if (netMgr)
                    {
                        netMgr->SendMessageToClient(id, "You have been kicked",
                            Spark::ChannelType::Reliable);
                        netMgr->DisconnectClient(id);
                    }
                    return "Kicked " + args[0];
                }
            }
            return "Player not found: " + args[0];
        }, "Kick a player", "Server");

    console.RegisterCommand("sv_bot_add",
        [this](const std::vector<std::string>&) -> std::string {
            std::string name = "Bot_" + std::to_string(m_nextBotID - 10000);
            SpawnBot(name);
            return "Added bot: " + name;
        }, "Add a bot", "Server");

    console.RegisterCommand("sv_bot_remove",
        [this](const std::vector<std::string>&) -> std::string {
            RemoveBot();
            return "Removed a bot";
        }, "Remove a bot", "Server");

    console.RegisterCommand("sv_restart_round",
        [this](const std::vector<std::string>&) -> std::string {
            StartMatch();
            return "Round restarted";
        }, "Restart the current round", "Server");

    console.RegisterCommand("sv_end_round",
        [this](const std::vector<std::string>&) -> std::string {
            EndMatch();
            return "Round ended";
        }, "End the current round", "Server");

    console.RegisterCommand("sv_save",
        [this](const std::vector<std::string>&) -> std::string {
            PerformAutoSave();
            return "Server state saved";
        }, "Force save server state", "Server");

    console.RegisterCommand("sv_set",
        [this](const std::vector<std::string>& args) -> std::string {
            if (args.size() < 2) return "Usage: sv_set <var> <value>";
            if (args[0] == "scorelimit") {
                m_config.scoreLimit = std::stoi(args[1]);
                return "Score limit set to " + args[1];
            }
            if (args[0] == "timelimit") {
                m_config.roundTimeLimit = std::stof(args[1]);
                return "Time limit set to " + args[1] + "s";
            }
            if (args[0] == "botfill") {
                m_config.botFillCount = std::stoi(args[1]);
                FillWithBots();
                return "Bot fill count set to " + args[1];
            }
            if (args[0] == "respawndelay") {
                m_config.respawnDelay = std::stof(args[1]);
                return "Respawn delay set to " + args[1] + "s";
            }
            return "Unknown variable: " + args[0];
        }, "Set server variable (scorelimit, timelimit, botfill, respawndelay)", "Server");
}

// ===========================================================================
// Server tick — runs every frame (60 Hz in headless mode)
// ===========================================================================

void DedicatedServerModule::OnUpdate(float dt)
{
    // --- Process client inputs (server-authoritative movement) ---
    ProcessClientInputs();

    // --- Run server-side physics for hit validation ---
    RunPhysicsValidation();

    // --- Handle respawn timers ---
    for (auto& [id, slot] : m_players)
    {
        if (!slot.alive)
        {
            slot.respawnTimer -= dt;
            if (slot.respawnTimer <= 0.0f)
                RespawnPlayer(id);
        }
    }

    // --- Match phase state machine ---
    m_phaseTimer += dt;

    switch (m_phase)
    {
    case MatchPhase::WaitingForPlayers:
        // Waiting — nothing to do, StartWarmup() is triggered on connect
        break;

    case MatchPhase::Warmup:
        if (m_phaseTimer >= m_config.warmupTime)
            StartMatch();
        break;

    case MatchPhase::Playing:
        m_matchTime += dt;
        CheckMatchConditions();
        break;

    case MatchPhase::Cooldown:
        if (m_phaseTimer >= m_config.cooldownTime)
            RotateMap();
        break;

    case MatchPhase::MapRotation:
        RotateMap();
        break;
    }

    // --- Replicate state to clients at 20 Hz ---
    UpdateReplication(dt);

    // --- Record lag compensation snapshot ---
    auto* netMgr = Spark::NetworkManager::GetInstance();
    if (netMgr)
        netMgr->GetLagCompensator().RecordSnapshot();

    // --- Auto-save periodically ---
    m_saveTimer += dt;
    if (m_saveTimer >= m_config.autoSaveInterval)
    {
        m_saveTimer = 0.0f;
        PerformAutoSave();
    }
}

// ===========================================================================
// Server tick helpers
// ===========================================================================

void DedicatedServerModule::ProcessClientInputs()
{
    auto* netMgr = Spark::NetworkManager::GetInstance();
    auto* world = m_ctx->GetWorld();
    if (!netMgr || !world) return;

    // Process each connected client's pending inputs
    for (auto& [id, slot] : m_players)
    {
        if (slot.isBot || !slot.alive) continue;

        auto inputs = netMgr->GetPendingInputs(id);
        if (inputs.empty()) continue;

        if (slot.entity == entt::null || !world->HasComponent<Transform>(slot.entity))
            continue;

        auto& t = world->GetComponent<Transform>(slot.entity);
        constexpr float moveSpeed = 10.0f;

        for (auto& input : inputs)
        {
            t.position.x += input.moveX * moveSpeed * (1.0f / 60.0f);
            t.position.z += input.moveZ * moveSpeed * (1.0f / 60.0f);
            t.rotation.y = input.lookYaw;
        }

        // Mark transform as dirty for replication
        netMgr->MarkPropertyDirty(id, "transform");
    }
}

void DedicatedServerModule::RunPhysicsValidation()
{
    // The physics system runs automatically via the engine's ECS systems.
    // Here we do server-authoritative hit validation using lag compensation.
    //
    // When a client reports a hit:
    // 1. Rewind the world to the client's timestamp using LagCompensator
    // 2. Perform a raycast at the rewound positions
    // 3. If the ray hits the target's rewound hitbox, validate the damage
    //
    // This is handled by the NetworkManager's hit validation pipeline.
    // The server always has the final say on whether damage is applied.
}

void DedicatedServerModule::UpdateReplication(float dt)
{
    m_replicateTimer += dt;
    if (m_replicateTimer < 0.05f) // 20 Hz replication
        return;
    m_replicateTimer = 0.0f;

    auto* netMgr = Spark::NetworkManager::GetInstance();
    if (!netMgr) return;

    // Mark all player transforms and health as dirty for replication
    for (auto& [id, slot] : m_players)
    {
        if (slot.entity == entt::null) continue;
        netMgr->MarkPropertyDirty(id, "transform");
        netMgr->MarkPropertyDirty(id, "health");
    }
}

void DedicatedServerModule::CheckMatchConditions()
{
    // Check score limit
    for (auto& [id, slot] : m_players)
    {
        if (slot.score >= m_config.scoreLimit)
        {
            Log("Score limit reached by %s (%d/%d)",
                slot.name.c_str(), slot.score, m_config.scoreLimit);
            EndMatch();
            return;
        }
    }

    // Check time limit
    if (m_matchTime >= m_config.roundTimeLimit)
    {
        Log("Time limit reached (%.0fs)", m_matchTime);
        EndMatch();
    }
}

void DedicatedServerModule::PerformAutoSave()
{
    auto* saveSys = Spark::SaveSystem::GetInstance();
    auto* world = m_ctx->GetWorld();
    if (!saveSys || !world) return;

    // Build save metadata
    Spark::SaveMetadata meta{};
    meta.mapName = m_config.mapName;
    meta.roundNumber = m_roundNumber;
    meta.playerCount = static_cast<int>(m_players.size());

    saveSys->Save("server_state", *world, meta);
}

// ===========================================================================
// Module export
// ===========================================================================

SPARK_IMPLEMENT_MODULE(DedicatedServerModule)
