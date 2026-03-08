/**
 * @file AIArenaModule.cpp
 * @brief INTERMEDIATE — AI showcase with behavior trees, NavMesh, perception, and steering.
 */

#include "AIArenaModule.h"
#include <Input/InputManager.h>
#include <string>

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void AIArenaModule::OnLoad(Spark::IEngineContext* ctx)
{
    m_ctx = ctx;

    // Subscribe to AI-relevant events
    if (auto* bus = m_ctx->GetEventBus())
    {
        m_damageSub = bus->Subscribe<Spark::EntityDamagedEvent>(
            [](const Spark::EntityDamagedEvent& /*e*/) {
                // Could trigger alert state on nearby AI agents
            });

        m_killSub = bus->Subscribe<Spark::EntityKilledEvent>(
            [](const Spark::EntityKilledEvent& /*e*/) {
                // Could cause nearby agents to flee
            });
    }

    BuildArena();
    BuildNavMesh();
}

void AIArenaModule::OnUnload()
{
    if (auto* bus = m_ctx ? m_ctx->GetEventBus() : nullptr)
    {
        bus->Unsubscribe<Spark::EntityDamagedEvent>(m_damageSub);
        bus->Unsubscribe<Spark::EntityKilledEvent>(m_killSub);
    }

    auto* world = m_ctx ? m_ctx->GetWorld() : nullptr;
    if (!world) { m_ctx = nullptr; return; }

    for (auto e : { m_floor, m_light, m_player })
        if (e != entt::null) world->DestroyEntity(e);

    for (auto e : m_agents)
        if (e != entt::null) world->DestroyEntity(e);
    m_agents.clear();

    for (auto e : m_obstacles)
        if (e != entt::null) world->DestroyEntity(e);
    m_obstacles.clear();

    m_ctx = nullptr;
}

// ---------------------------------------------------------------------------
// Arena construction
// ---------------------------------------------------------------------------

void AIArenaModule::BuildArena()
{
    auto* world = m_ctx->GetWorld();

    // Floor
    m_floor = world->CreateEntity();
    world->AddComponent<NameComponent>(m_floor, NameComponent{ "Floor" });
    world->AddComponent<Transform>(m_floor, Transform{
        { 0.f, -0.5f, 0.f }, { 0.f, 0.f, 0.f }, { 60.f, 1.f, 60.f }
    });
    world->AddComponent<MeshRenderer>(m_floor, MeshRenderer{
        "Assets/Models/cube.obj", "Assets/Materials/concrete.mat",
        true, true, true
    });
    world->AddComponent<RigidBodyComponent>(m_floor, RigidBodyComponent{
        RigidBodyComponent::Type::Static, 0.f, 0.8f, 0.2f
    });
    world->AddComponent<ColliderComponent>(m_floor, ColliderComponent{
        ColliderComponent::Shape::Box, { 30.f, 0.5f, 30.f }
    });

    // Directional light
    m_light = world->CreateEntity();
    world->AddComponent<NameComponent>(m_light, NameComponent{ "SunLight" });
    world->AddComponent<Transform>(m_light, Transform{
        { 0.f, 20.f, 0.f }, { -50.f, 30.f, 0.f }, { 1.f, 1.f, 1.f }
    });
    world->AddComponent<LightComponent>(m_light, LightComponent{
        LightComponent::Type::Directional,
        { 1.f, 0.95f, 0.85f }, 2.5f, 0.f,
        0.f, 0.f, true, 2048
    });

    // Player entity — the target for AI agents
    m_player = world->CreateEntity();
    world->AddComponent<NameComponent>(m_player, NameComponent{ "Player" });
    world->AddComponent<Transform>(m_player, Transform{
        { 0.f, 1.f, 0.f }, { 0.f, 0.f, 0.f }, { 1.f, 2.f, 1.f }
    });
    world->AddComponent<MeshRenderer>(m_player, MeshRenderer{
        "Assets/Models/character.obj", "Assets/Materials/player.mat",
        true, true, true
    });
    world->AddComponent<HealthComponent>(m_player, HealthComponent{ 100.f, 100.f, false });
    world->AddComponent<TagComponent>(m_player, TagComponent{});
    world->GetComponent<TagComponent>(m_player).AddTag("player");

    // Obstacles — scatter some crates for NavMesh avoidance
    auto spawnObstacle = [&](float x, float z, float size) {
        auto e = world->CreateEntity();
        world->AddComponent<NameComponent>(e, NameComponent{ "Obstacle" });
        world->AddComponent<Transform>(e, Transform{
            { x, size * 0.5f, z }, { 0.f, 0.f, 0.f }, { size, size, size }
        });
        world->AddComponent<MeshRenderer>(e, MeshRenderer{
            "Assets/Models/crate.obj", "Assets/Materials/wood.mat",
            true, true, true
        });
        world->AddComponent<RigidBodyComponent>(e, RigidBodyComponent{
            RigidBodyComponent::Type::Static, 0.f, 0.8f, 0.1f
        });
        world->AddComponent<ColliderComponent>(e, ColliderComponent{
            ColliderComponent::Shape::Box, { size * 0.5f, size * 0.5f, size * 0.5f }
        });
        m_obstacles.push_back(e);
    };

    spawnObstacle(-8.f, 5.f, 3.f);
    spawnObstacle(6.f, -7.f, 2.f);
    spawnObstacle(12.f, 10.f, 4.f);
    spawnObstacle(-5.f, -12.f, 2.5f);

    // Spawn AI agents with different behaviors
    SpawnPatroller(-15.f, 0.f, "Patroller_A");
    SpawnPatroller(15.f, 5.f, "Patroller_B");
    SpawnGuard(0.f, 15.f, "Guard_A");
    SpawnGuard(-10.f, -15.f, "Guard_B");
    SpawnFleer(10.f, -10.f, "Coward");
}

// ---------------------------------------------------------------------------
// NavMesh
// ---------------------------------------------------------------------------

void AIArenaModule::BuildNavMesh()
{
    auto* navManager = Spark::NavMeshManager::GetInstance();
    if (!navManager) return;

    // Build a NavMesh from the arena floor bounds
    // The NavMeshBuilder generates a walkable mesh from the collision geometry
    Spark::NavMeshBuilder builder;
    builder.SetBounds({ -30.f, 0.f, -30.f }, { 30.f, 0.f, 30.f });
    builder.SetCellSize(0.3f);
    builder.SetAgentRadius(0.5f);
    builder.SetAgentHeight(2.0f);
    builder.Build();
}

// ---------------------------------------------------------------------------
// AI agent spawners
// ---------------------------------------------------------------------------

void AIArenaModule::SpawnPatroller(float x, float z, const std::string& name)
{
    auto* world = m_ctx->GetWorld();
    auto e = world->CreateEntity();

    world->AddComponent<NameComponent>(e, NameComponent{ name });
    world->AddComponent<Transform>(e, Transform{
        { x, 1.f, z }, { 0.f, 0.f, 0.f }, { 1.f, 2.f, 1.f }
    });
    world->AddComponent<MeshRenderer>(e, MeshRenderer{
        "Assets/Models/character.obj", "Assets/Materials/enemy_blue.mat",
        true, true, true
    });
    world->AddComponent<HealthComponent>(e, HealthComponent{ 50.f, 50.f, false });

    // AI component — starts in Idle, transitions to Patrolling
    world->AddComponent<AIComponent>(e, AIComponent{
        AIComponent::State::Idle,
        "patrol_behavior",          // behaviorTreeName
        m_player,                   // targetEntity
        { 0.f, 0.f, 0.f },        // lastKnownTargetPos
        {},                         // currentPath
        { 15.f, 8.f, 4.f, 0.5f, 0.3f } // Config: detection=15, attack=8, speed=4, accuracy=0.5, reaction=0.3
    });

    // Register a pre-built patrol behavior
    if (auto* aiSys = m_ctx->GetAISystem())
    {
        auto tree = Spark::BehaviorTree::CreatePatrolBehavior();

        // Configure patrol waypoints on the blackboard
        auto& bb = tree->GetBlackboard();
        bb.SetFloat("patrolRadius", 12.0f);
        bb.SetFloat("waitTime", 2.0f);

        aiSys->RegisterBehavior(name + "_bt", std::move(tree));
    }

    m_agents.push_back(e);
}

void AIArenaModule::SpawnGuard(float x, float z, const std::string& name)
{
    auto* world = m_ctx->GetWorld();
    auto e = world->CreateEntity();

    world->AddComponent<NameComponent>(e, NameComponent{ name });
    world->AddComponent<Transform>(e, Transform{
        { x, 1.f, z }, { 0.f, 0.f, 0.f }, { 1.2f, 2.2f, 1.2f }
    });
    world->AddComponent<MeshRenderer>(e, MeshRenderer{
        "Assets/Models/character.obj", "Assets/Materials/enemy_red.mat",
        true, true, true
    });
    world->AddComponent<HealthComponent>(e, HealthComponent{ 100.f, 100.f, false });

    world->AddComponent<AIComponent>(e, AIComponent{
        AIComponent::State::Idle,
        "guard_behavior",
        m_player,
        { 0.f, 0.f, 0.f },
        {},
        { 20.f, 5.f, 3.f, 0.8f, 0.2f } // Config: detection=20, attack=5, speed=3, accuracy=0.8, reaction=0.2
    });

    // Build a custom behavior tree for the guard
    if (auto* aiSys = m_ctx->GetAISystem())
    {
        auto tree = Spark::BehaviorTree::CreateGuardBehavior();

        auto& bb = tree->GetBlackboard();
        bb.SetFloat3("guardPosition", { x, 1.f, z });
        bb.SetFloat("guardRadius", 8.0f);
        bb.SetBool("aggressive", true);

        aiSys->RegisterBehavior(name + "_bt", std::move(tree));
    }

    m_agents.push_back(e);
}

void AIArenaModule::SpawnFleer(float x, float z, const std::string& name)
{
    auto* world = m_ctx->GetWorld();
    auto e = world->CreateEntity();

    world->AddComponent<NameComponent>(e, NameComponent{ name });
    world->AddComponent<Transform>(e, Transform{
        { x, 1.f, z }, { 0.f, 0.f, 0.f }, { 0.8f, 1.6f, 0.8f }
    });
    world->AddComponent<MeshRenderer>(e, MeshRenderer{
        "Assets/Models/character.obj", "Assets/Materials/enemy_green.mat",
        true, true, true
    });
    world->AddComponent<HealthComponent>(e, HealthComponent{ 30.f, 30.f, false });

    world->AddComponent<AIComponent>(e, AIComponent{
        AIComponent::State::Idle,
        "flee_behavior",
        m_player,
        { 0.f, 0.f, 0.f },
        {},
        { 25.f, 0.f, 6.f, 0.1f, 0.1f } // Config: detection=25, attack=0, speed=6, accuracy=0.1, reaction=0.1
    });

    // Flee behavior — runs away when player gets close
    if (auto* aiSys = m_ctx->GetAISystem())
    {
        auto tree = Spark::BehaviorTree::CreateFleeBehavior();

        auto& bb = tree->GetBlackboard();
        bb.SetFloat("fleeDistance", 20.0f);
        bb.SetFloat("safeDistance", 30.0f);

        aiSys->RegisterBehavior(name + "_bt", std::move(tree));
    }

    m_agents.push_back(e);
}

// ---------------------------------------------------------------------------
// Reset
// ---------------------------------------------------------------------------

void AIArenaModule::ResetArena()
{
    auto* world = m_ctx->GetWorld();

    for (auto e : m_agents)
        if (e != entt::null) world->DestroyEntity(e);
    m_agents.clear();
    m_agentCount = 0;

    // Re-spawn agents
    SpawnPatroller(-15.f, 0.f, "Patroller_A");
    SpawnPatroller(15.f, 5.f, "Patroller_B");
    SpawnGuard(0.f, 15.f, "Guard_A");
    SpawnGuard(-10.f, -15.f, "Guard_B");
    SpawnFleer(10.f, -10.f, "Coward");
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

void AIArenaModule::OnUpdate(float dt)
{
    (void)dt;
    auto* input = m_ctx->GetInput();
    if (!input) return;

    auto* world = m_ctx->GetWorld();

    // WASD — move the player entity so AI agents can track it
    constexpr float moveSpeed = 8.0f;
    auto& playerT = world->GetComponent<Transform>(m_player);

    if (input->IsKeyDown('W'))
        playerT.position.z += moveSpeed * dt;
    if (input->IsKeyDown('S'))
        playerT.position.z -= moveSpeed * dt;
    if (input->IsKeyDown('A'))
        playerT.position.x -= moveSpeed * dt;
    if (input->IsKeyDown('D'))
        playerT.position.x += moveSpeed * dt;

    // 1 — Spawn an extra patroller near the player
    if (input->WasKeyPressed('1'))
    {
        std::string name = "ExtraPatrol_" + std::to_string(m_agentCount++);
        SpawnPatroller(playerT.position.x + 5.f, playerT.position.z, name);
    }

    // 2 — Spawn an extra guard at player location
    if (input->WasKeyPressed('2'))
    {
        std::string name = "ExtraGuard_" + std::to_string(m_agentCount++);
        SpawnGuard(playerT.position.x, playerT.position.z - 5.f, name);
    }

    // R — Reset all AI agents
    if (input->WasKeyPressed('R'))
        ResetArena();
}

// ---------------------------------------------------------------------------
void AIArenaModule::OnRender() { }
void AIArenaModule::OnResize(int, int) { }

SPARK_IMPLEMENT_MODULE(AIArenaModule)
