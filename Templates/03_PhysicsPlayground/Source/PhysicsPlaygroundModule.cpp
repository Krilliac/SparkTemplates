/**
 * @file PhysicsPlaygroundModule.cpp
 * @brief INTERMEDIATE — Physics sandbox with rigid bodies, colliders, and raycasting.
 */

#include "PhysicsPlaygroundModule.h"
#include <Input/InputManager.h>
#include <string>

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void PhysicsPlaygroundModule::OnLoad(Spark::IEngineContext* ctx)
{
    m_ctx = ctx;

    // Subscribe to collision events
    if (auto* bus = m_ctx->GetEventBus())
    {
        m_collisionSub = bus->Subscribe<Spark::CollisionEvent>(
            [](const Spark::CollisionEvent& /*e*/) {
                // You could play impact sounds, spawn particles, etc.
            });
    }

    BuildArena();
}

void PhysicsPlaygroundModule::OnUnload()
{
    if (auto* bus = m_ctx ? m_ctx->GetEventBus() : nullptr)
        bus->Unsubscribe<Spark::CollisionEvent>(m_collisionSub);

    auto* world = m_ctx ? m_ctx->GetWorld() : nullptr;
    if (!world) { m_ctx = nullptr; return; }

    // Destroy statics
    for (auto e : { m_floor, m_wallN, m_wallS, m_wallE, m_wallW, m_ramp, m_light })
        if (e != entt::null) world->DestroyEntity(e);

    // Destroy dynamics
    for (auto e : m_dynamicEntities)
        if (e != entt::null) world->DestroyEntity(e);
    m_dynamicEntities.clear();

    m_ctx = nullptr;
}

// ---------------------------------------------------------------------------
// Arena construction — walls, floor, ramp (all static rigid bodies)
// ---------------------------------------------------------------------------

void PhysicsPlaygroundModule::BuildArena()
{
    auto* world = m_ctx->GetWorld();

    // Helper: create a static box collider with a mesh
    auto makeStatic = [&](const char* name,
                          DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 rot,
                          DirectX::XMFLOAT3 scale, DirectX::XMFLOAT3 halfExt)
    {
        auto e = world->CreateEntity();
        world->AddComponent<NameComponent>(e, NameComponent{ name });
        world->AddComponent<Transform>(e, Transform{ pos, rot, scale });
        world->AddComponent<MeshRenderer>(e, MeshRenderer{
            "Assets/Models/cube.obj", "Assets/Materials/concrete.mat",
            true, true, true
        });
        world->AddComponent<RigidBodyComponent>(e, RigidBodyComponent{
            RigidBodyComponent::Type::Static,
            0.0f,   // mass (static)
            0.8f,   // friction
            0.2f    // restitution
        });
        world->AddComponent<ColliderComponent>(e, ColliderComponent{
            ColliderComponent::Shape::Box,
            halfExt
        });
        return e;
    };

    // Floor  (50 x 1 x 50)
    m_floor = makeStatic("Floor",
        { 0.f, -0.5f, 0.f }, { 0.f, 0.f, 0.f },
        { 50.f, 1.f, 50.f }, { 25.f, 0.5f, 25.f });

    // Walls
    m_wallN = makeStatic("WallNorth",
        { 0.f, 2.f, 25.f }, { 0.f, 0.f, 0.f },
        { 50.f, 4.f, 1.f }, { 25.f, 2.f, 0.5f });

    m_wallS = makeStatic("WallSouth",
        { 0.f, 2.f, -25.f }, { 0.f, 0.f, 0.f },
        { 50.f, 4.f, 1.f }, { 25.f, 2.f, 0.5f });

    m_wallE = makeStatic("WallEast",
        { 25.f, 2.f, 0.f }, { 0.f, 0.f, 0.f },
        { 1.f, 4.f, 50.f }, { 0.5f, 2.f, 25.f });

    m_wallW = makeStatic("WallWest",
        { -25.f, 2.f, 0.f }, { 0.f, 0.f, 0.f },
        { 1.f, 4.f, 50.f }, { 0.5f, 2.f, 25.f });

    // Ramp (rotated box)
    m_ramp = makeStatic("Ramp",
        { 10.f, 1.5f, 0.f }, { 0.f, 0.f, -20.f },
        { 6.f, 0.5f, 4.f }, { 3.f, 0.25f, 2.f });

    // Light
    m_light = world->CreateEntity();
    world->AddComponent<NameComponent>(m_light, NameComponent{ "ArenaLight" });
    world->AddComponent<Transform>(m_light, Transform{
        { 0.f, 15.f, 0.f }, { -60.f, 0.f, 0.f }, { 1.f, 1.f, 1.f }
    });
    world->AddComponent<LightComponent>(m_light, LightComponent{
        LightComponent::Type::Directional,
        { 1.f, 1.f, 1.f }, 2.f, 0.f,
        0.f, 0.f, true, 2048
    });

    // Initial stack of boxes
    SpawnStack(0.0f, 0.0f, 5);
}

// ---------------------------------------------------------------------------
// Spawners
// ---------------------------------------------------------------------------

void PhysicsPlaygroundModule::SpawnBox(float x, float y, float z)
{
    auto* world = m_ctx->GetWorld();
    auto e = world->CreateEntity();
    std::string name = "DynBox_" + std::to_string(m_spawnCount++);

    world->AddComponent<NameComponent>(e, NameComponent{ name });
    world->AddComponent<Transform>(e, Transform{
        { x, y, z }, { 0.f, 0.f, 0.f }, { 1.f, 1.f, 1.f }
    });
    world->AddComponent<MeshRenderer>(e, MeshRenderer{
        "Assets/Models/cube.obj", "Assets/Materials/wood.mat",
        true, true, true
    });
    world->AddComponent<RigidBodyComponent>(e, RigidBodyComponent{
        RigidBodyComponent::Type::Dynamic,
        5.0f,  // mass
        0.6f,  // friction
        0.3f   // restitution
    });
    world->AddComponent<ColliderComponent>(e, ColliderComponent{
        ColliderComponent::Shape::Box,
        { 0.5f, 0.5f, 0.5f }
    });

    m_dynamicEntities.push_back(e);
}

void PhysicsPlaygroundModule::SpawnSphere(float x, float y, float z)
{
    auto* world = m_ctx->GetWorld();
    auto e = world->CreateEntity();
    std::string name = "DynSphere_" + std::to_string(m_spawnCount++);

    world->AddComponent<NameComponent>(e, NameComponent{ name });
    world->AddComponent<Transform>(e, Transform{
        { x, y, z }, { 0.f, 0.f, 0.f }, { 1.f, 1.f, 1.f }
    });
    world->AddComponent<MeshRenderer>(e, MeshRenderer{
        "Assets/Models/sphere.obj", "Assets/Materials/metal.mat",
        true, true, true
    });
    world->AddComponent<RigidBodyComponent>(e, RigidBodyComponent{
        RigidBodyComponent::Type::Dynamic,
        3.0f,  // mass
        0.4f,  // friction
        0.7f   // restitution (bouncy!)
    });
    world->AddComponent<ColliderComponent>(e, ColliderComponent{
        ColliderComponent::Shape::Sphere,
        { 0.5f, 0.5f, 0.5f },  // halfExtents (unused for sphere)
        0.5f                     // radius
    });

    m_dynamicEntities.push_back(e);
}

void PhysicsPlaygroundModule::SpawnStack(float baseX, float baseZ, int height)
{
    for (int row = 0; row < height; ++row)
    {
        float y = 0.5f + row * 1.05f; // slight gap to avoid interpenetration
        int cols = height - row;
        float startX = baseX - (cols - 1) * 0.55f;
        for (int col = 0; col < cols; ++col)
        {
            SpawnBox(startX + col * 1.1f, y, baseZ);
        }
    }
}

void PhysicsPlaygroundModule::LaunchProjectile()
{
    auto* world = m_ctx->GetWorld();
    // Spawn a fast-moving sphere at a launch position
    float x = -10.0f, y = 3.0f, z = 0.0f;
    SpawnSphere(x, y, z);

    // Give it an initial velocity toward the stack
    auto& rb = world->GetComponent<RigidBodyComponent>(m_dynamicEntities.back());
    rb.linearVelocity = { 30.0f, 5.0f, 0.0f }; // launch rightward & upward
}

void PhysicsPlaygroundModule::ClearDynamic()
{
    auto* world = m_ctx->GetWorld();
    for (auto e : m_dynamicEntities)
        if (e != entt::null) world->DestroyEntity(e);
    m_dynamicEntities.clear();
    m_spawnCount = 0;
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

void PhysicsPlaygroundModule::OnUpdate(float dt)
{
    (void)dt;
    auto* input = m_ctx->GetInput();
    if (!input) return;

    // 1 — Spawn box above centre
    if (input->WasKeyPressed('1'))
        SpawnBox(0.0f, 10.0f, 0.0f);

    // 2 — Spawn bouncy sphere above centre
    if (input->WasKeyPressed('2'))
        SpawnSphere(0.0f, 12.0f, 0.0f);

    // 3 — Spawn a fresh pyramid stack
    if (input->WasKeyPressed('3'))
        SpawnStack(-5.0f, 5.0f, 4);

    // 4 — Launch projectile into the scene
    if (input->WasKeyPressed('4'))
        LaunchProjectile();

    // 5 — Clear all dynamic objects
    if (input->WasKeyPressed('5'))
        ClearDynamic();

    // R — Reset: clear and rebuild initial stack
    if (input->WasKeyPressed('R'))
    {
        ClearDynamic();
        SpawnStack(0.0f, 0.0f, 5);
    }
}

// ---------------------------------------------------------------------------
void PhysicsPlaygroundModule::OnRender() { }
void PhysicsPlaygroundModule::OnResize(int, int) { }

SPARK_IMPLEMENT_MODULE(PhysicsPlaygroundModule)
