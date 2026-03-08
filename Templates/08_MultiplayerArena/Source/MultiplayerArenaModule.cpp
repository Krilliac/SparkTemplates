/**
 * @file MultiplayerArenaModule.cpp
 * @brief ADVANCED — Networked multiplayer arena with client/server, replication, and lag compensation.
 */

#include "MultiplayerArenaModule.h"
#include <Input/InputManager.h>
#include <string>

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void MultiplayerArenaModule::OnLoad(Spark::IEngineContext* ctx)
{
    m_ctx = ctx;

    // Subscribe to gameplay events
    if (auto* bus = m_ctx->GetEventBus())
    {
        m_damageSub = bus->Subscribe<Spark::EntityDamagedEvent>(
            [](const Spark::EntityDamagedEvent& /*e*/) {
                // Play hit marker, reduce HUD health bar
            });

        m_killSub = bus->Subscribe<Spark::EntityKilledEvent>(
            [](const Spark::EntityKilledEvent& /*e*/) {
                // Show kill feed, update scoreboard
            });

        m_respawnSub = bus->Subscribe<Spark::PlayerRespawnEvent>(
            [](const Spark::PlayerRespawnEvent& /*e*/) {
                // Reset camera, restore health HUD
            });
    }

    BuildArena();

    // Spawn local player (client 0)
    SpawnNetworkedPlayer(0.0f, 0.0f, 0, true);

    // Simulate remote players for demonstration
    SpawnNetworkedPlayer(-8.0f, 5.0f, 1, false);
    SpawnNetworkedPlayer(8.0f, -5.0f, 2, false);
    SpawnNetworkedPlayer(5.0f, 8.0f, 3, false);
}

void MultiplayerArenaModule::OnUnload()
{
    // Disconnect from network
    auto* netMgr = Spark::NetworkManager::GetInstance();
    if (netMgr && m_isConnected)
        netMgr->Disconnect();

    if (auto* bus = m_ctx ? m_ctx->GetEventBus() : nullptr)
    {
        bus->Unsubscribe<Spark::EntityDamagedEvent>(m_damageSub);
        bus->Unsubscribe<Spark::EntityKilledEvent>(m_killSub);
        bus->Unsubscribe<Spark::PlayerRespawnEvent>(m_respawnSub);
    }

    auto* world = m_ctx ? m_ctx->GetWorld() : nullptr;
    if (!world) { m_ctx = nullptr; return; }

    for (auto e : { m_floor, m_light, m_localPlayer })
        if (e != entt::null) world->DestroyEntity(e);

    for (auto e : m_walls)
        if (e != entt::null) world->DestroyEntity(e);
    m_walls.clear();

    for (auto e : m_remotePlayers)
        if (e != entt::null) world->DestroyEntity(e);
    m_remotePlayers.clear();

    m_ctx = nullptr;
}

// ---------------------------------------------------------------------------
// Arena construction
// ---------------------------------------------------------------------------

void MultiplayerArenaModule::BuildArena()
{
    auto* world = m_ctx->GetWorld();

    // Floor
    m_floor = world->CreateEntity();
    world->AddComponent<NameComponent>(m_floor, NameComponent{ "ArenaFloor" });
    world->AddComponent<Transform>(m_floor, Transform{
        { 0.f, -0.5f, 0.f }, { 0.f, 0.f, 0.f }, { 40.f, 1.f, 40.f }
    });
    world->AddComponent<MeshRenderer>(m_floor, MeshRenderer{
        "Assets/Models/cube.obj", "Assets/Materials/metal.mat",
        true, true, true
    });
    world->AddComponent<RigidBodyComponent>(m_floor, RigidBodyComponent{
        RigidBodyComponent::Type::Static, 0.f, 0.6f, 0.1f
    });
    world->AddComponent<ColliderComponent>(m_floor, ColliderComponent{
        ColliderComponent::Shape::Box, { 20.f, 0.5f, 20.f }
    });

    // Walls
    auto makeWall = [&](const char* name, DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 scale,
                         DirectX::XMFLOAT3 halfExt) {
        auto e = world->CreateEntity();
        world->AddComponent<NameComponent>(e, NameComponent{ name });
        world->AddComponent<Transform>(e, Transform{
            pos, { 0.f, 0.f, 0.f }, scale
        });
        world->AddComponent<MeshRenderer>(e, MeshRenderer{
            "Assets/Models/cube.obj", "Assets/Materials/concrete.mat",
            true, true, true
        });
        world->AddComponent<RigidBodyComponent>(e, RigidBodyComponent{
            RigidBodyComponent::Type::Static, 0.f, 0.8f, 0.2f
        });
        world->AddComponent<ColliderComponent>(e, ColliderComponent{
            ColliderComponent::Shape::Box, halfExt
        });
        m_walls.push_back(e);
    };

    makeWall("WallN", { 0.f, 2.f, 20.f }, { 40.f, 4.f, 1.f }, { 20.f, 2.f, 0.5f });
    makeWall("WallS", { 0.f, 2.f, -20.f }, { 40.f, 4.f, 1.f }, { 20.f, 2.f, 0.5f });
    makeWall("WallE", { 20.f, 2.f, 0.f }, { 1.f, 4.f, 40.f }, { 0.5f, 2.f, 20.f });
    makeWall("WallW", { -20.f, 2.f, 0.f }, { 1.f, 4.f, 40.f }, { 0.5f, 2.f, 20.f });

    // Cover objects in the arena
    makeWall("Cover1", { -6.f, 1.f, 0.f }, { 4.f, 2.f, 1.f }, { 2.f, 1.f, 0.5f });
    makeWall("Cover2", { 6.f, 1.f, 3.f }, { 1.f, 2.f, 4.f }, { 0.5f, 1.f, 2.f });
    makeWall("Cover3", { 0.f, 1.f, -8.f }, { 3.f, 2.f, 3.f }, { 1.5f, 1.f, 1.5f });

    // Light
    m_light = world->CreateEntity();
    world->AddComponent<NameComponent>(m_light, NameComponent{ "ArenaLight" });
    world->AddComponent<Transform>(m_light, Transform{
        { 0.f, 15.f, 0.f }, { -60.f, 0.f, 0.f }, { 1.f, 1.f, 1.f }
    });
    world->AddComponent<LightComponent>(m_light, LightComponent{
        LightComponent::Type::Directional,
        { 1.f, 1.f, 1.f }, 3.0f, 0.f,
        0.f, 0.f, true, 2048
    });
}

// ---------------------------------------------------------------------------
// Networked player spawning
// ---------------------------------------------------------------------------

void MultiplayerArenaModule::SpawnNetworkedPlayer(float x, float z,
                                                   uint32_t clientID, bool isLocal)
{
    auto* world = m_ctx->GetWorld();
    auto e = world->CreateEntity();

    std::string name = isLocal ? "LocalPlayer" : "RemotePlayer_" + std::to_string(clientID);

    world->AddComponent<NameComponent>(e, NameComponent{ name });
    world->AddComponent<Transform>(e, Transform{
        { x, 1.f, z }, { 0.f, 0.f, 0.f }, { 1.f, 2.f, 1.f }
    });
    world->AddComponent<MeshRenderer>(e, MeshRenderer{
        "Assets/Models/character.obj",
        isLocal ? "Assets/Materials/player.mat" : "Assets/Materials/enemy_red.mat",
        true, true, true
    });
    world->AddComponent<HealthComponent>(e, HealthComponent{ 100.f, 100.f, false });

    // Physics collider for the player capsule
    world->AddComponent<RigidBodyComponent>(e, RigidBodyComponent{
        RigidBodyComponent::Type::Kinematic, 0.f, 0.5f, 0.0f
    });
    world->AddComponent<ColliderComponent>(e, ColliderComponent{
        ColliderComponent::Shape::Capsule,
        { 0.4f, 1.f, 0.4f }, // halfExtents
        0.4f,                  // radius
        2.0f                   // height
    });

    // Network identity for replication
    world->AddComponent<NetworkIdentity>(e, NetworkIdentity{
        clientID,     // networkID
        clientID,     // ownerClientID
        isLocal,      // isLocalAuthority
        true,         // replicateTransform
        true          // replicateHealth
    });

    if (isLocal)
        m_localPlayer = e;
    else
        m_remotePlayers.push_back(e);
}

// ---------------------------------------------------------------------------
// Input handling (local player)
// ---------------------------------------------------------------------------

void MultiplayerArenaModule::HandleInput(float dt)
{
    auto* input = m_ctx->GetInput();
    auto* world = m_ctx->GetWorld();
    if (!input || m_localPlayer == entt::null) return;

    auto& t = world->GetComponent<Transform>(m_localPlayer);
    constexpr float moveSpeed = 10.0f;

    // Build input vector
    DirectX::XMFLOAT3 moveDir = { 0.f, 0.f, 0.f };
    if (input->IsKeyDown('W')) moveDir.z += 1.0f;
    if (input->IsKeyDown('S')) moveDir.z -= 1.0f;
    if (input->IsKeyDown('A')) moveDir.x -= 1.0f;
    if (input->IsKeyDown('D')) moveDir.x += 1.0f;

    t.position.x += moveDir.x * moveSpeed * dt;
    t.position.z += moveDir.z * moveSpeed * dt;

    // Send input to server for prediction
    auto* netMgr = Spark::NetworkManager::GetInstance();
    if (netMgr && m_isConnected)
    {
        netMgr->SendClientInput({
            moveDir.x, moveDir.z,
            t.rotation.y,    // look direction
            input->IsKeyDown(VK_LBUTTON) // firing
        });
    }
}

// ---------------------------------------------------------------------------
// Send state updates
// ---------------------------------------------------------------------------

void MultiplayerArenaModule::SendPlayerState()
{
    auto* netMgr = Spark::NetworkManager::GetInstance();
    if (!netMgr || !m_isServer) return;

    // Mark the local player's transform as dirty for replication
    netMgr->MarkPropertyDirty(0, "transform");
    netMgr->MarkPropertyDirty(0, "health");
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

void MultiplayerArenaModule::OnUpdate(float dt)
{
    auto* input = m_ctx->GetInput();
    if (!input) return;

    // H — Host a server
    if (input->WasKeyPressed('H') && !m_isConnected)
    {
        auto* netMgr = Spark::NetworkManager::GetInstance();
        if (netMgr)
        {
            netMgr->StartServer(27015, 8); // port 27015, max 8 clients
            m_isServer = true;
            m_isConnected = true;
        }
    }

    // J — Join a server
    if (input->WasKeyPressed('J') && !m_isConnected)
    {
        auto* netMgr = Spark::NetworkManager::GetInstance();
        if (netMgr)
        {
            netMgr->Connect("127.0.0.1", 27015, "Player");
            m_isConnected = true;
        }
    }

    // T — Send chat message
    if (input->WasKeyPressed('T') && m_isConnected)
    {
        auto* netMgr = Spark::NetworkManager::GetInstance();
        if (netMgr)
            netMgr->SendChatMessage("Hello from the arena!");
    }

    // Handle player movement
    HandleInput(dt);

    // Send state at 20 Hz
    m_sendTimer += dt;
    if (m_sendTimer >= 0.05f)
    {
        m_sendTimer = 0.0f;
        SendPlayerState();
    }

    // Record snapshot for lag compensation
    auto* netMgr = Spark::NetworkManager::GetInstance();
    if (netMgr && m_isServer)
    {
        auto& lagComp = netMgr->GetLagCompensator();
        lagComp.RecordSnapshot();
    }
}

// ---------------------------------------------------------------------------
void MultiplayerArenaModule::OnRender() { }
void MultiplayerArenaModule::OnResize(int, int) { }

SPARK_IMPLEMENT_MODULE(MultiplayerArenaModule)
