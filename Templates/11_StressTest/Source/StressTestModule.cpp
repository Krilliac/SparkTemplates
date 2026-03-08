/**
 * @file StressTestModule.cpp
 * @brief ADVANCED — Full engine stress test exercising every major system simultaneously.
 *
 * This is the ultimate SparkEngine showcase. It initializes ALL engine systems
 * and runs them concurrently to validate stability, performance, and correctness
 * under maximum load.
 */

#include "StressTestModule.h"
#include <Graphics/GraphicsEngine.h>
#include <Graphics/PostProcessingSystem.h>
#include <Graphics/DecalSystem.h>
#include <Graphics/FogSystem.h>
#include <Graphics/MeshLOD.h>
#include <Audio/AudioEngine.h>
#include <Audio/MusicManager.h>
#include <Input/InputManager.h>
#include <string>
#include <cmath>

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void StressTestModule::OnLoad(Spark::IEngineContext* ctx)
{
    m_ctx = ctx;

    // Subscribe to ALL event types to stress the EventBus
    if (auto* bus = m_ctx->GetEventBus())
    {
        m_collisionSub = bus->Subscribe<Spark::CollisionEvent>(
            [](const Spark::CollisionEvent&) { /* stress: high frequency */ });
        m_damageSub = bus->Subscribe<Spark::EntityDamagedEvent>(
            [](const Spark::EntityDamagedEvent&) { });
        m_killSub = bus->Subscribe<Spark::EntityKilledEvent>(
            [](const Spark::EntityKilledEvent&) { });
        m_pickupSub = bus->Subscribe<Spark::ItemPickedUpEvent>(
            [](const Spark::ItemPickedUpEvent&) { });
        m_questSub = bus->Subscribe<Spark::QuestCompletedEvent>(
            [](const Spark::QuestCompletedEvent&) { });
        m_weatherSub = bus->Subscribe<Spark::WeatherChangedEvent>(
            [](const Spark::WeatherChangedEvent&) { });
        m_timeSub = bus->Subscribe<Spark::TimeOfDayChangedEvent>(
            [](const Spark::TimeOfDayChangedEvent&) { });
    }

    // Initialize ALL systems in sequence
    Phase1_Environment();
    Phase2_Physics();
    Phase3_AI();
    Phase4_Particles();
    Phase5_Audio();
    Phase6_Networking();
    Phase7_Animation();
    Phase8_Procedural();
    Phase9_Scripting();
    Phase10_Cinematic();
    Phase11_Gameplay();
}

void StressTestModule::OnUnload()
{
    // Shut down singletons
    if (auto* seq = Spark::SequencerManager::GetInstance()) seq->StopAll();
    if (auto* coro = Spark::CoroutineScheduler::GetInstance()) coro->StopAll();
    if (auto* net = Spark::NetworkManager::GetInstance()) net->Disconnect();

    // Unsubscribe events
    if (auto* bus = m_ctx ? m_ctx->GetEventBus() : nullptr)
    {
        bus->Unsubscribe<Spark::CollisionEvent>(m_collisionSub);
        bus->Unsubscribe<Spark::EntityDamagedEvent>(m_damageSub);
        bus->Unsubscribe<Spark::EntityKilledEvent>(m_killSub);
        bus->Unsubscribe<Spark::ItemPickedUpEvent>(m_pickupSub);
        bus->Unsubscribe<Spark::QuestCompletedEvent>(m_questSub);
        bus->Unsubscribe<Spark::WeatherChangedEvent>(m_weatherSub);
        bus->Unsubscribe<Spark::TimeOfDayChangedEvent>(m_timeSub);
    }

    // Detach scripts
    if (auto* se = Spark::AngelScriptEngine::GetInstance())
        for (auto e : m_scriptedEntities)
            if (e != entt::null) se->DetachScript(e);

    // Destroy ALL entities
    auto* world = m_ctx ? m_ctx->GetWorld() : nullptr;
    if (!world) { m_ctx = nullptr; return; }

    for (auto e : { m_terrain, m_player, m_sunLight, m_camera })
        if (e != entt::null) world->DestroyEntity(e);

    auto destroyVec = [&](std::vector<entt::entity>& vec) {
        for (auto e : vec)
            if (e != entt::null) world->DestroyEntity(e);
        vec.clear();
    };

    destroyVec(m_staticEntities);
    destroyVec(m_dynamicBodies);
    destroyVec(m_aiAgents);
    destroyVec(m_particleEmitters);
    destroyVec(m_animatedEntities);
    destroyVec(m_networkEntities);
    destroyVec(m_scriptedEntities);
    destroyVec(m_gameplayEntities);
    destroyVec(m_lights);
    destroyVec(m_wfcTiles);

    m_ctx = nullptr;
}

// ===========================================================================
// PHASE 1: Environment — Terrain, Lighting, Day/Night, Weather, Fog
// ===========================================================================

void StressTestModule::Phase1_Environment()
{
    auto* world = m_ctx->GetWorld();

    // --- Large terrain floor ---
    m_terrain = world->CreateEntity();
    world->AddComponent<NameComponent>(m_terrain, NameComponent{ "StressTerrain" });
    world->AddComponent<Transform>(m_terrain, Transform{
        { 0.f, -0.5f, 0.f }, { 0.f, 0.f, 0.f }, { 200.f, 1.f, 200.f }
    });
    world->AddComponent<MeshRenderer>(m_terrain, MeshRenderer{
        "Assets/Models/cube.obj", "Assets/Materials/grass.mat", true, true, true
    });
    world->AddComponent<RigidBodyComponent>(m_terrain, RigidBodyComponent{
        RigidBodyComponent::Type::Static, 0.f, 0.8f, 0.2f
    });
    world->AddComponent<ColliderComponent>(m_terrain, ColliderComponent{
        ColliderComponent::Shape::Box, { 100.f, 0.5f, 100.f }
    });

    // --- Sun (directional, cascaded shadows) ---
    m_sunLight = world->CreateEntity();
    world->AddComponent<NameComponent>(m_sunLight, NameComponent{ "Sun" });
    world->AddComponent<Transform>(m_sunLight, Transform{
        { 0.f, 50.f, 0.f }, { -45.f, 30.f, 0.f }, { 1.f, 1.f, 1.f }
    });
    world->AddComponent<LightComponent>(m_sunLight, LightComponent{
        LightComponent::Type::Directional,
        { 1.f, 0.95f, 0.85f }, 3.0f, 0.f,
        0.f, 0.f, true, 4096  // High-res shadow map
    });

    // --- 8 point lights scattered around ---
    for (int i = 0; i < 8; ++i)
    {
        float angle = (float)i / 8.0f * 6.2832f;
        float x = cosf(angle) * 30.0f;
        float z = sinf(angle) * 30.0f;

        auto light = world->CreateEntity();
        world->AddComponent<NameComponent>(light, NameComponent{
            "PointLight_" + std::to_string(i) });
        world->AddComponent<Transform>(light, Transform{
            { x, 5.f, z }, { 0.f, 0.f, 0.f }, { 1.f, 1.f, 1.f }
        });

        // Vary light colors
        float r = 0.5f + 0.5f * cosf(angle);
        float g = 0.5f + 0.5f * cosf(angle + 2.1f);
        float b = 0.5f + 0.5f * cosf(angle + 4.2f);

        world->AddComponent<LightComponent>(light, LightComponent{
            LightComponent::Type::Point,
            { r, g, b }, 2.0f, 20.0f,
            0.f, 0.f, true, 512
        });
        m_lights.push_back(light);
    }

    // --- 4 spotlights ---
    for (int i = 0; i < 4; ++i)
    {
        float angle = (float)i / 4.0f * 6.2832f + 0.4f;
        auto spot = world->CreateEntity();
        world->AddComponent<NameComponent>(spot, NameComponent{
            "Spotlight_" + std::to_string(i) });
        world->AddComponent<Transform>(spot, Transform{
            { cosf(angle) * 15.f, 12.f, sinf(angle) * 15.f },
            { -70.f, angle * 57.3f, 0.f },
            { 1.f, 1.f, 1.f }
        });
        world->AddComponent<LightComponent>(spot, LightComponent{
            LightComponent::Type::Spot,
            { 1.f, 0.9f, 0.7f }, 4.0f, 30.0f,
            40.0f, 30.0f, true, 1024
        });
        m_lights.push_back(spot);
    }

    // --- Camera ---
    m_camera = world->CreateEntity();
    world->AddComponent<NameComponent>(m_camera, NameComponent{ "MainCamera" });
    world->AddComponent<Transform>(m_camera, Transform{
        { 0.f, 10.f, -40.f }, { -15.f, 0.f, 0.f }, { 1.f, 1.f, 1.f }
    });
    world->AddComponent<Camera>(m_camera, Camera{ 70.0f, 0.1f, 500.0f, true });

    // --- Day/Night cycle (accelerated) ---
    auto* dayNight = Spark::DayNightCycle::GetInstance();
    if (dayNight)
    {
        dayNight->SetTimeScale(60.0f); // 1 real second = 1 game minute
        dayNight->SetTime(8.0f);       // Start at 8 AM
    }

    // --- Weather ---
    auto weatherEntity = world->CreateEntity();
    world->AddComponent<NameComponent>(weatherEntity, NameComponent{ "WeatherController" });
    world->AddComponent<WeatherComponent>(weatherEntity, WeatherComponent{
        WeatherComponent::Type::Clear,
        0.0f, 1.f, 0.f, 0.5f, 2.0f, 3.0f
    });
    m_staticEntities.push_back(weatherEntity);

    // --- Fog (start with volumetric) ---
    if (auto* fog = m_ctx->GetGraphics()->GetFogSystem())
    {
        fog->SetMode(Spark::FogSystem::Mode::Volumetric);
        fog->SetColor({ 0.4f, 0.45f, 0.55f });
        fog->SetDensity(0.015f);
    }

    // --- Full post-processing pipeline ---
    if (auto* gfx = m_ctx->GetGraphics())
    {
        gfx->SetRenderPath(Spark::RenderPath::Deferred);
        gfx->SetQualityPreset(Spark::QualityPreset::Ultra);
        gfx->SetHDREnabled(true);
        gfx->SetMSAALevel(Spark::MSAALevel::X4);
        gfx->SetSSAOSettings({ true, 0.5f, 1.0f, 0.03f });
        gfx->SetSSRSettings({ true, 64, 0.1f });
        gfx->SetTAASettings({ true, 0.1f });
        gfx->SetVolumetricSettings({ true, 64 });

        if (auto* post = gfx->GetPostProcessingSystem())
        {
            post->SetBloomEnabled(true);
            post->SetBloomThreshold(0.7f);
            post->SetBloomIntensity(1.5f);
            post->SetToneMappingEnabled(true);
            post->SetFXAAEnabled(true);
            post->SetColorGradingEnabled(true);
        }
    }
}

// ===========================================================================
// PHASE 2: Physics — 100+ active rigid bodies
// ===========================================================================

void StressTestModule::Phase2_Physics()
{
    auto* world = m_ctx->GetWorld();

    // Walls around the physics arena
    auto makeWall = [&](const char* name, DirectX::XMFLOAT3 pos,
                         DirectX::XMFLOAT3 scale, DirectX::XMFLOAT3 halfExt) {
        auto e = world->CreateEntity();
        world->AddComponent<NameComponent>(e, NameComponent{ name });
        world->AddComponent<Transform>(e, Transform{ pos, { 0.f, 0.f, 0.f }, scale });
        world->AddComponent<MeshRenderer>(e, MeshRenderer{
            "Assets/Models/cube.obj", "Assets/Materials/concrete.mat", true, true, true
        });
        world->AddComponent<RigidBodyComponent>(e, RigidBodyComponent{
            RigidBodyComponent::Type::Static, 0.f, 0.8f, 0.2f
        });
        world->AddComponent<ColliderComponent>(e, ColliderComponent{
            ColliderComponent::Shape::Box, halfExt
        });
        m_staticEntities.push_back(e);
    };

    makeWall("WallN", { 0.f, 3.f, 50.f },  { 100.f, 6.f, 2.f }, { 50.f, 3.f, 1.f });
    makeWall("WallS", { 0.f, 3.f, -50.f }, { 100.f, 6.f, 2.f }, { 50.f, 3.f, 1.f });
    makeWall("WallE", { 50.f, 3.f, 0.f },  { 2.f, 6.f, 100.f }, { 1.f, 3.f, 50.f });
    makeWall("WallW", { -50.f, 3.f, 0.f }, { 2.f, 6.f, 100.f }, { 1.f, 3.f, 50.f });

    // Ramps at corners
    for (int i = 0; i < 4; ++i)
    {
        float angle = (float)i * 1.5708f; // 90 degrees
        float x = cosf(angle) * 35.f;
        float z = sinf(angle) * 35.f;
        auto ramp = world->CreateEntity();
        world->AddComponent<NameComponent>(ramp, NameComponent{
            "Ramp_" + std::to_string(i) });
        world->AddComponent<Transform>(ramp, Transform{
            { x, 1.5f, z }, { 0.f, angle * 57.3f, -20.f }, { 8.f, 0.5f, 4.f }
        });
        world->AddComponent<MeshRenderer>(ramp, MeshRenderer{
            "Assets/Models/cube.obj", "Assets/Materials/metal.mat", true, true, true
        });
        world->AddComponent<RigidBodyComponent>(ramp, RigidBodyComponent{
            RigidBodyComponent::Type::Static, 0.f, 0.6f, 0.3f
        });
        world->AddComponent<ColliderComponent>(ramp, ColliderComponent{
            ColliderComponent::Shape::Box, { 4.f, 0.25f, 2.f }
        });
        m_staticEntities.push_back(ramp);
    }

    // Spawn initial physics objects: 60 boxes + 40 spheres
    SpawnPhysicsBatch(100);
}

void StressTestModule::SpawnPhysicsBatch(int count)
{
    auto* world = m_ctx->GetWorld();

    for (int i = 0; i < count; ++i)
    {
        auto e = world->CreateEntity();
        bool isSphere = (i % 5 == 0);
        std::string name = (isSphere ? "DynSphere_" : "DynBox_")
                           + std::to_string(m_entityCounter++);

        float x = (float)(rand() % 80 - 40);
        float y = 5.f + (float)(rand() % 20);
        float z = (float)(rand() % 80 - 40);

        world->AddComponent<NameComponent>(e, NameComponent{ name });
        world->AddComponent<Transform>(e, Transform{
            { x, y, z }, { 0.f, 0.f, 0.f },
            isSphere ? DirectX::XMFLOAT3{ 0.8f, 0.8f, 0.8f }
                     : DirectX::XMFLOAT3{ 1.f, 1.f, 1.f }
        });
        world->AddComponent<MeshRenderer>(e, MeshRenderer{
            isSphere ? "Assets/Models/sphere.obj" : "Assets/Models/crate.obj",
            isSphere ? "Assets/Materials/metal.mat" : "Assets/Materials/wood.mat",
            true, true, true
        });
        world->AddComponent<RigidBodyComponent>(e, RigidBodyComponent{
            RigidBodyComponent::Type::Dynamic,
            isSphere ? 3.0f : 5.0f,
            isSphere ? 0.4f : 0.6f,
            isSphere ? 0.7f : 0.3f
        });

        if (isSphere)
        {
            world->AddComponent<ColliderComponent>(e, ColliderComponent{
                ColliderComponent::Shape::Sphere, { 0.4f, 0.4f, 0.4f }, 0.4f
            });
        }
        else
        {
            world->AddComponent<ColliderComponent>(e, ColliderComponent{
                ColliderComponent::Shape::Box, { 0.5f, 0.5f, 0.5f }
            });
        }

        m_dynamicBodies.push_back(e);
    }
}

// ===========================================================================
// PHASE 3: AI — 20+ agents with NavMesh, behavior trees, perception, steering
// ===========================================================================

void StressTestModule::Phase3_AI()
{
    auto* world = m_ctx->GetWorld();

    // Player entity (AI target)
    m_player = world->CreateEntity();
    world->AddComponent<NameComponent>(m_player, NameComponent{ "Player" });
    world->AddComponent<Transform>(m_player, Transform{
        { 0.f, 1.f, 0.f }, { 0.f, 0.f, 0.f }, { 1.f, 2.f, 1.f }
    });
    world->AddComponent<MeshRenderer>(m_player, MeshRenderer{
        "Assets/Models/character.obj", "Assets/Materials/player.mat", true, true, true
    });
    world->AddComponent<HealthComponent>(m_player, HealthComponent{ 200.f, 200.f, false });
    world->AddComponent<TagComponent>(m_player, TagComponent{});
    world->GetComponent<TagComponent>(m_player).AddTag("player");

    // Build NavMesh
    auto* navMgr = Spark::NavMeshManager::GetInstance();
    if (navMgr)
    {
        Spark::NavMeshBuilder builder;
        builder.SetBounds({ -50.f, 0.f, -50.f }, { 50.f, 0.f, 50.f });
        builder.SetCellSize(0.5f);
        builder.SetAgentRadius(0.5f);
        builder.SetAgentHeight(2.0f);
        builder.Build();
    }

    auto* aiSys = m_ctx->GetAISystem();

    // Spawn 10 patrollers
    for (int i = 0; i < 10; ++i)
    {
        float angle = (float)i / 10.0f * 6.2832f;
        float x = cosf(angle) * 25.0f;
        float z = sinf(angle) * 25.0f;

        auto e = world->CreateEntity();
        std::string name = "Patroller_" + std::to_string(i);

        world->AddComponent<NameComponent>(e, NameComponent{ name });
        world->AddComponent<Transform>(e, Transform{
            { x, 1.f, z }, { 0.f, 0.f, 0.f }, { 1.f, 2.f, 1.f }
        });
        world->AddComponent<MeshRenderer>(e, MeshRenderer{
            "Assets/Models/character.obj", "Assets/Materials/enemy_blue.mat",
            true, true, true
        });
        world->AddComponent<HealthComponent>(e, HealthComponent{ 50.f, 50.f, false });
        world->AddComponent<AIComponent>(e, AIComponent{
            AIComponent::State::Idle, "patrol", m_player,
            { 0.f, 0.f, 0.f }, {},
            { 18.f, 8.f, 4.5f, 0.5f, 0.3f }
        });

        if (aiSys)
        {
            auto tree = Spark::BehaviorTree::CreatePatrolBehavior();
            auto& bb = tree->GetBlackboard();
            bb.SetFloat("patrolRadius", 15.0f + (float)(i % 5) * 3.0f);
            bb.SetFloat("waitTime", 1.0f + (float)(i % 3));
            aiSys->RegisterBehavior(name + "_bt", std::move(tree));
        }

        m_aiAgents.push_back(e);
    }

    // Spawn 6 guards
    for (int i = 0; i < 6; ++i)
    {
        float angle = (float)i / 6.0f * 6.2832f + 0.5f;
        float x = cosf(angle) * 15.0f;
        float z = sinf(angle) * 15.0f;

        auto e = world->CreateEntity();
        std::string name = "Guard_" + std::to_string(i);

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
            AIComponent::State::Idle, "guard", m_player,
            { 0.f, 0.f, 0.f }, {},
            { 22.f, 6.f, 3.5f, 0.8f, 0.15f }
        });

        if (aiSys)
        {
            auto tree = Spark::BehaviorTree::CreateGuardBehavior();
            auto& bb = tree->GetBlackboard();
            bb.SetFloat3("guardPosition", { x, 1.f, z });
            bb.SetFloat("guardRadius", 10.0f);
            bb.SetBool("aggressive", true);
            aiSys->RegisterBehavior(name + "_bt", std::move(tree));
        }

        m_aiAgents.push_back(e);
    }

    // Spawn 4 fleeing agents
    for (int i = 0; i < 4; ++i)
    {
        float x = (float)(rand() % 40 - 20);
        float z = (float)(rand() % 40 - 20);

        auto e = world->CreateEntity();
        std::string name = "Fleer_" + std::to_string(i);

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
            AIComponent::State::Idle, "flee", m_player,
            { 0.f, 0.f, 0.f }, {},
            { 30.f, 0.f, 7.f, 0.1f, 0.1f }
        });

        if (aiSys)
        {
            auto tree = Spark::BehaviorTree::CreateFleeBehavior();
            auto& bb = tree->GetBlackboard();
            bb.SetFloat("fleeDistance", 25.0f);
            bb.SetFloat("safeDistance", 40.0f);
            aiSys->RegisterBehavior(name + "_bt", std::move(tree));
        }

        m_aiAgents.push_back(e);
    }
}

// ===========================================================================
// PHASE 4: Particles — 10+ emitters, decals, post-processing
// ===========================================================================

void StressTestModule::Phase4_Particles()
{
    auto* world = m_ctx->GetWorld();

    struct EmitterDef {
        const char* name;
        DirectX::XMFLOAT3 pos;
        const char* effect;
        float rate;
        float lifetime;
        DirectX::XMFLOAT4 color;
        float size;
        float speed;
    };

    EmitterDef emitters[] = {
        { "Fire1",   { -20.f, 0.5f, -20.f }, "fire",    60.f, 1.5f, { 1.f, 0.5f, 0.1f, 1.f }, 0.3f, 2.5f },
        { "Fire2",   { 20.f, 0.5f, 20.f },   "fire",    50.f, 1.5f, { 1.f, 0.4f, 0.0f, 1.f }, 0.4f, 2.0f },
        { "Smoke1",  { -20.f, 0.5f, 20.f },  "smoke",   25.f, 3.0f, { 0.3f, 0.3f, 0.3f, 0.5f }, 0.5f, 1.0f },
        { "Sparks1", { 0.f, 3.f, -15.f },    "sparks",  100.f, 0.6f, { 1.f, 0.9f, 0.3f, 1.f }, 0.04f, 10.f },
        { "Sparks2", { 10.f, 3.f, 10.f },    "sparks",  80.f, 0.8f, { 1.f, 0.7f, 0.2f, 1.f }, 0.05f, 8.f },
        { "Magic1",  { -10.f, 2.f, 0.f },    "magic",   45.f, 2.0f, { 0.3f, 0.5f, 1.f, 0.8f }, 0.15f, 3.f },
        { "Magic2",  { 10.f, 2.f, 0.f },     "magic",   45.f, 2.0f, { 1.f, 0.3f, 0.8f, 0.8f }, 0.15f, 3.f },
        { "Rain",    { 0.f, 20.f, 0.f },     "rain",    300.f, 1.0f, { 0.6f, 0.7f, 0.9f, 0.3f }, 0.02f, 15.f },
        { "Snow",    { 30.f, 20.f, 0.f },    "snow",    150.f, 4.0f, { 1.f, 1.f, 1.f, 0.6f }, 0.06f, 1.f },
        { "Dust",    { -30.f, 0.5f, 0.f },   "dust",    40.f, 5.0f, { 0.6f, 0.5f, 0.4f, 0.3f }, 0.2f, 0.5f },
    };

    for (auto& def : emitters)
    {
        auto e = world->CreateEntity();
        world->AddComponent<NameComponent>(e, NameComponent{ def.name });
        world->AddComponent<Transform>(e, Transform{
            def.pos, { 0.f, 0.f, 0.f }, { 1.f, 1.f, 1.f }
        });
        world->AddComponent<ParticleEmitterComponent>(e, ParticleEmitterComponent{
            def.effect, true, def.rate, def.lifetime,
            def.color, def.size, def.speed
        });
        m_particleEmitters.push_back(e);
    }

    // Scatter decals across the floor
    if (auto* decals = m_ctx->GetGraphics()->GetDecalSystem())
    {
        for (int i = 0; i < 30; ++i)
        {
            float x = (float)(rand() % 80 - 40);
            float z = (float)(rand() % 80 - 40);

            Spark::DecalSystem::DecalType type;
            switch (i % 4)
            {
            case 0: type = Spark::DecalSystem::DecalType::BulletHole; break;
            case 1: type = Spark::DecalSystem::DecalType::ScorchMark; break;
            case 2: type = Spark::DecalSystem::DecalType::Crack; break;
            default: type = Spark::DecalSystem::DecalType::Footprint; break;
            }

            float scale = 0.3f + (float)(rand() % 20) * 0.1f;
            decals->SpawnDecal(type,
                { x, 0.01f, z },
                { 0.f, (float)(rand() % 360), 0.f },
                { scale, scale, scale });
        }
    }
}

// ===========================================================================
// PHASE 5: Audio — 3D positional, music, reverb
// ===========================================================================

void StressTestModule::Phase5_Audio()
{
    auto* audio = m_ctx->GetAudio();
    if (!audio) return;

    audio->LoadSound("explosion", "Assets/Audio/explosion.wav");
    audio->LoadSound("gunshot", "Assets/Audio/gunshot.wav");
    audio->LoadSound("footstep", "Assets/Audio/footstep.wav");
    audio->LoadSound("ambient_wind", "Assets/Audio/wind.wav");
    audio->LoadSound("impact", "Assets/Audio/impact.wav");
    audio->LoadSound("alert", "Assets/Audio/alert.wav");

    auto* music = Spark::MusicManager::GetInstance();
    if (!music) return;

    music->SetBusVolume(Spark::AudioBus::Master, 0.8f);
    music->SetBusVolume(Spark::AudioBus::Music, 0.5f);
    music->SetBusVolume(Spark::AudioBus::SFX, 1.0f);
    music->SetBusVolume(Spark::AudioBus::Ambient, 0.6f);
    music->SetBusVolume(Spark::AudioBus::Voice, 0.9f);

    music->SetReverbZone(Spark::ReverbZone::LargeRoom);
    music->SetCombatIntensity(Spark::CombatIntensity::LowCombat);

    // Start ambient wind as looping 3D sound
    audio->PlaySound3D("ambient_wind", { 0.f, 5.f, 0.f }, 1.0f, 1.0f, 5.0f, 100.0f, true);
}

// ===========================================================================
// PHASE 6: Networking — Server + replicated entities
// ===========================================================================

void StressTestModule::Phase6_Networking()
{
    auto* world = m_ctx->GetWorld();
    auto* netMgr = Spark::NetworkManager::GetInstance();
    if (!netMgr) return;

    // Start a local server for stress testing replication
    netMgr->StartServer(27015, 16);

    // Register the player for replication
    netMgr->RegisterReplicatedEntity(m_player, 0);

    // Create "simulated" networked entities
    for (int i = 0; i < 4; ++i)
    {
        float angle = (float)i / 4.0f * 6.2832f;
        auto e = world->CreateEntity();

        world->AddComponent<NameComponent>(e, NameComponent{
            "NetPlayer_" + std::to_string(i) });
        world->AddComponent<Transform>(e, Transform{
            { cosf(angle) * 20.f, 1.f, sinf(angle) * 20.f },
            { 0.f, 0.f, 0.f }, { 1.f, 2.f, 1.f }
        });
        world->AddComponent<MeshRenderer>(e, MeshRenderer{
            "Assets/Models/character.obj", "Assets/Materials/enemy_red.mat",
            true, true, true
        });
        world->AddComponent<HealthComponent>(e, HealthComponent{ 100.f, 100.f, false });
        world->AddComponent<NetworkIdentity>(e, NetworkIdentity{
            (uint32_t)(i + 1), (uint32_t)(i + 1), false, true, true
        });

        netMgr->RegisterReplicatedEntity(e, i + 1);
        m_networkEntities.push_back(e);
    }
}

// ===========================================================================
// PHASE 7: Animation — Skeletal, IK, state machines, blending
// ===========================================================================

void StressTestModule::Phase7_Animation()
{
    auto* world = m_ctx->GetWorld();

    for (int i = 0; i < 6; ++i)
    {
        float x = -25.f + (float)i * 10.f;
        auto e = world->CreateEntity();

        world->AddComponent<NameComponent>(e, NameComponent{
            "AnimEntity_" + std::to_string(i) });
        world->AddComponent<Transform>(e, Transform{
            { x, 0.f, 30.f }, { 0.f, 0.f, 0.f }, { 1.f, 1.f, 1.f }
        });
        world->AddComponent<MeshRenderer>(e, MeshRenderer{
            "Assets/Models/character.obj", "Assets/Materials/player.mat",
            true, true, true
        });

        // Animation controller with different animations per entity
        std::string anim;
        switch (i % 3)
        {
        case 0: anim = "idle"; break;
        case 1: anim = "walk"; break;
        case 2: anim = "run"; break;
        }

        world->AddComponent<AnimationController>(e, AnimationController{
            anim,        // currentAnimation
            "idle",      // defaultAnimation
            1.0f,        // playbackSpeed
            true,        // loop
            { "idle", "walk", "run", "attack", "death" }
        });

        m_animatedEntities.push_back(e);
    }
}

// ===========================================================================
// PHASE 8: Procedural — Noise terrain section + WFC dungeon
// ===========================================================================

void StressTestModule::Phase8_Procedural()
{
    auto* world = m_ctx->GetWorld();

    // Generate a small heightmap-based terrain section
    Spark::HeightmapGenerator heightGen;
    Spark::HeightmapSettings settings{};
    settings.width     = 64;
    settings.height    = 64;
    settings.seed      = 12345;
    settings.octaves   = 5;
    settings.frequency = 0.03f;
    settings.amplitude = 15.0f;

    auto heightmap = heightGen.Generate(settings);
    heightGen.ThermalErosion(heightmap, 20);
    heightGen.HydraulicErosion(heightmap, 30);
    heightGen.Normalize(heightmap);

    auto terrainMesh = Spark::ProceduralMesh::CreateTerrainFromHeightmap(
        heightmap, 64, 64, 1.0f, 15.0f);

    auto procTerrain = world->CreateEntity();
    world->AddComponent<NameComponent>(procTerrain, NameComponent{ "ProcTerrain" });
    world->AddComponent<Transform>(procTerrain, Transform{
        { 60.f, 0.f, -32.f }, { 0.f, 0.f, 0.f }, { 1.f, 1.f, 1.f }
    });
    world->AddComponent<MeshRenderer>(procTerrain, MeshRenderer{
        "procedural://stress_terrain", "Assets/Materials/rock.mat", true, true, true
    });
    m_staticEntities.push_back(procTerrain);

    // LOD for procedural terrain
    if (auto* lodMgr = Spark::LODManager::GetInstance())
        lodMgr->GenerateLODChain(procTerrain, 3);

    // WFC dungeon
    Spark::WaveFunctionCollapse wfc;
    wfc.SetGridSize(12, 12);
    wfc.AddTile("floor",   1.0f, { "open", "open", "open", "open" });
    wfc.AddTile("wall_ns", 0.4f, { "wall", "open", "wall", "open" });
    wfc.AddTile("wall_ew", 0.4f, { "open", "wall", "open", "wall" });
    wfc.AddTile("corner",  0.2f, { "wall", "wall", "open", "open" });
    wfc.AddTile("pillar",  0.1f, { "wall", "wall", "wall", "wall" });
    wfc.EnableRotation(true);
    wfc.Collapse(42);

    auto grid = wfc.GetResult();
    float offsetX = -60.f;
    float tileSize = 2.0f;

    for (int y = 0; y < 12; ++y)
    {
        for (int x = 0; x < 12; ++x)
        {
            auto& tile = grid[y * 12 + x];
            auto e = world->CreateEntity();
            float height = (tile.name == "floor") ? 0.1f :
                           (tile.name == "pillar") ? 4.0f : 3.0f;

            world->AddComponent<NameComponent>(e, NameComponent{ "WFC_Tile" });
            world->AddComponent<Transform>(e, Transform{
                { offsetX + x * tileSize, height * 0.5f, -60.f + y * tileSize },
                { 0.f, tile.rotation, 0.f },
                { tileSize, height, tileSize }
            });
            world->AddComponent<MeshRenderer>(e, MeshRenderer{
                "Assets/Models/cube.obj", "Assets/Materials/stone.mat",
                true, true, true
            });
            m_wfcTiles.push_back(e);
        }
    }

    // Scatter procedural rocks using ObjectPlacer
    Spark::ObjectPlacer placer;
    Spark::PlacementRule rockRule{};
    rockRule.meshPath = "procedural://rock";
    rockRule.material = "Assets/Materials/rock.mat";
    rockRule.density  = 0.1f;
    rockRule.minSlope = 0.f;
    rockRule.maxSlope = 45.f;
    rockRule.minHeight = 0.f;
    rockRule.maxHeight = 15.f;
    rockRule.alignToSurface = true;
    rockRule.randomScale = { 0.5f, 1.5f };

    auto rockPositions = placer.Scatter(rockRule, 99);
    for (auto& pos : rockPositions)
    {
        auto e = world->CreateEntity();
        world->AddComponent<NameComponent>(e, NameComponent{ "ProcRock" });
        world->AddComponent<Transform>(e, Transform{
            pos.position, pos.rotation, pos.scale
        });
        world->AddComponent<MeshRenderer>(e, MeshRenderer{
            rockRule.meshPath, rockRule.material, true, true, true
        });
        m_staticEntities.push_back(e);
    }
}

// ===========================================================================
// PHASE 9: Scripting — AngelScript modules
// ===========================================================================

void StressTestModule::Phase9_Scripting()
{
    auto* world = m_ctx->GetWorld();
    auto* se = Spark::AngelScriptEngine::GetInstance();
    if (!se) return;

    // Compile stress-test script
    se->CompileScriptFromString(R"(
        class StressSpinner
        {
            float speed = 180.0f;
            void Start() { ASPrint("StressSpinner active!"); }
            void Update(float dt)
            {
                Transform@ t = ASGetTransform();
                if (t !is null)
                {
                    t.rotation.y += speed * dt;
                    t.rotation.x += speed * 0.5f * dt;
                }
            }
        }
    )", "StressScripts");

    // Attach to multiple entities
    for (int i = 0; i < 5; ++i)
    {
        auto e = world->CreateEntity();
        world->AddComponent<NameComponent>(e, NameComponent{
            "ScriptCube_" + std::to_string(i) });
        world->AddComponent<Transform>(e, Transform{
            { -30.f + (float)i * 5.f, 2.f, -30.f },
            { 0.f, 0.f, 0.f }, { 1.f, 1.f, 1.f }
        });
        world->AddComponent<MeshRenderer>(e, MeshRenderer{
            "Assets/Models/cube.obj", "Assets/Materials/metal.mat", true, true, true
        });
        world->AddComponent<Script>(e, Script{
            "", "StressSpinner", "StressScripts", true
        });
        se->AttachScript(e, "StressSpinner", "StressScripts");
        se->CallStart(e);
        m_scriptedEntities.push_back(e);
    }
}

// ===========================================================================
// PHASE 10: Cinematic — Sequencer + coroutines
// ===========================================================================

void StressTestModule::Phase10_Cinematic()
{
    auto* seq = Spark::SequencerManager::GetInstance();
    if (!seq) return;

    m_stressCinematicID = seq->CreateSequence("StressFlyover");

    // 20-second multi-keyframe flyover
    seq->AddCameraPathKey(m_stressCinematicID, 0.0f,
        { -50.f, 30.f, -50.f }, { -25.f, 45.f, 0.f }, 80.f, 0.f,
        Spark::Interpolation::CatmullRom);
    seq->AddCameraPathKey(m_stressCinematicID, 5.0f,
        { 50.f, 25.f, -30.f }, { -20.f, -30.f, 5.f }, 70.f, 5.f,
        Spark::Interpolation::CatmullRom);
    seq->AddCameraPathKey(m_stressCinematicID, 10.0f,
        { 40.f, 15.f, 40.f }, { -10.f, -135.f, 0.f }, 65.f, -3.f,
        Spark::Interpolation::CatmullRom);
    seq->AddCameraPathKey(m_stressCinematicID, 15.0f,
        { -40.f, 20.f, 30.f }, { -15.f, 135.f, 0.f }, 70.f, 0.f,
        Spark::Interpolation::CatmullRom);
    seq->AddCameraPathKey(m_stressCinematicID, 20.0f,
        { 0.f, 10.f, -40.f }, { -15.f, 0.f, 0.f }, 70.f, 0.f,
        Spark::Interpolation::CubicBezier);

    seq->AddSubtitleKey(m_stressCinematicID, 1.0f, "Every system. Running at once.", 3.0f);
    seq->AddSubtitleKey(m_stressCinematicID, 6.0f, "Physics. AI. Particles. Networking.", 3.0f);
    seq->AddSubtitleKey(m_stressCinematicID, 12.0f, "Procedural. Animation. Scripting.", 3.0f);
    seq->AddSubtitleKey(m_stressCinematicID, 17.0f, "SparkEngine — No limits.", 3.0f);

    seq->AddAudioCueKey(m_stressCinematicID, 0.0f, "ambient_wind");
    seq->AddAudioCueKey(m_stressCinematicID, 10.0f, "explosion");

    seq->AddFadeKey(m_stressCinematicID, 0.0f, 1.0f);
    seq->AddFadeKey(m_stressCinematicID, 1.5f, 0.0f);
    seq->AddFadeKey(m_stressCinematicID, 19.0f, 0.0f);
    seq->AddFadeKey(m_stressCinematicID, 20.0f, 1.0f);

    // Set up periodic coroutines for stress
    auto* coro = Spark::CoroutineScheduler::GetInstance();
    if (!coro) return;

    auto* audio = m_ctx->GetAudio();
    coro->StartCoroutine("PeriodicExplosions")
        .Do([audio]() { if (audio) audio->PlaySound("explosion"); })
        .WaitForSeconds(5.0f)
        .Do([audio]() { if (audio) audio->PlaySound("gunshot"); })
        .WaitForSeconds(3.0f)
        .Do([audio]() { if (audio) audio->PlaySound("alert"); })
        .WaitForSeconds(4.0f);
}

// ===========================================================================
// PHASE 11: Gameplay — Health, inventory, quests, save system
// ===========================================================================

void StressTestModule::Phase11_Gameplay()
{
    auto* world = m_ctx->GetWorld();

    // Add inventory and quest tracking to the player
    world->AddComponent<InventoryTag>(m_player, InventoryTag{
        30,     // maxSlots
        200.f,  // maxWeight
        100.f   // currency
    });
    world->AddComponent<QuestTrackerTag>(m_player, QuestTrackerTag{
        0, 0, false
    });

    // Spawn collectables
    for (int i = 0; i < 20; ++i)
    {
        float x = (float)(rand() % 80 - 40);
        float z = (float)(rand() % 80 - 40);

        auto e = world->CreateEntity();
        world->AddComponent<NameComponent>(e, NameComponent{
            "Collectable_" + std::to_string(i) });
        world->AddComponent<Transform>(e, Transform{
            { x, 0.5f, z }, { 0.f, 0.f, 0.f }, { 0.5f, 0.5f, 0.5f }
        });
        world->AddComponent<MeshRenderer>(e, MeshRenderer{
            "Assets/Models/sphere.obj", "Assets/Materials/gold.mat",
            true, true, true
        });
        world->AddComponent<TagComponent>(e, TagComponent{});

        auto& tag = world->GetComponent<TagComponent>(e);
        switch (i % 4)
        {
        case 0: tag.AddTag("healing"); tag.AddTag("collectable"); break;
        case 1: tag.AddTag("currency"); tag.AddTag("collectable"); break;
        case 2: tag.AddTag("quest_item"); tag.AddTag("collectable"); break;
        case 3: tag.AddTag("rare"); tag.AddTag("collectable"); break;
        }

        m_gameplayEntities.push_back(e);
    }

    // Set up auto-save
    auto* saveSys = Spark::SaveSystem::GetInstance();
    if (saveSys)
    {
        // Perform initial save
        saveSys->Save("stress_test_initial", *world, {});
    }
}

// ===========================================================================
// Runtime stress helpers
// ===========================================================================

void StressTestModule::DamageRandomEntity()
{
    auto* world = m_ctx->GetWorld();
    if (m_aiAgents.empty()) return;

    int idx = rand() % (int)m_aiAgents.size();
    auto e = m_aiAgents[idx];
    if (e == entt::null || !world->HasComponent<HealthComponent>(e)) return;

    auto& hp = world->GetComponent<HealthComponent>(e);
    hp.TakeDamage(10.0f);

    // Publish damage event
    if (auto* bus = m_ctx->GetEventBus())
        bus->Publish(Spark::EntityDamagedEvent{ e, 10.0f });
}

void StressTestModule::CycleRenderPath()
{
    auto* gfx = m_ctx->GetGraphics();
    if (!gfx) return;

    m_renderPathIndex = (m_renderPathIndex + 1) % 3;
    switch (m_renderPathIndex)
    {
    case 0: gfx->SetRenderPath(Spark::RenderPath::Deferred); break;
    case 1: gfx->SetRenderPath(Spark::RenderPath::ForwardPlus); break;
    case 2: gfx->SetRenderPath(Spark::RenderPath::Clustered); break;
    }
}

void StressTestModule::TriggerAutoSave()
{
    auto* saveSys = Spark::SaveSystem::GetInstance();
    auto* world = m_ctx->GetWorld();
    if (saveSys && world)
        saveSys->AutoSave(*world);
}

void StressTestModule::SpawnExplosionCluster(float x, float z)
{
    auto* particles = m_ctx->GetGraphics()->GetParticleSystem();
    auto* decals = m_ctx->GetGraphics()->GetDecalSystem();
    auto* audio = m_ctx->GetAudio();

    if (particles)
    {
        particles->SpawnExplosion({ x, 1.f, z });
        particles->SpawnSparks({ x, 1.5f, z });
        particles->SpawnSmoke({ x, 0.5f, z });
    }

    if (decals)
    {
        decals->SpawnDecal(Spark::DecalSystem::DecalType::ScorchMark,
            { x, 0.01f, z }, { 0.f, (float)(rand() % 360), 0.f },
            { 3.f, 3.f, 3.f });
    }

    if (audio)
        audio->PlaySound3D("explosion", { x, 1.f, z }, 1.0f, 1.0f, 5.0f, 80.0f, false);
}

// ===========================================================================
// Update — Everything runs every frame
// ===========================================================================

void StressTestModule::OnUpdate(float dt)
{
    auto* input = m_ctx->GetInput();
    auto* world = m_ctx->GetWorld();
    if (!input || !world) return;

    // --- Update singletons ---
    if (auto* seq = Spark::SequencerManager::GetInstance()) seq->Update(dt);
    if (auto* coro = Spark::CoroutineScheduler::GetInstance()) coro->Update(dt);

    // --- Update scripts ---
    if (auto* se = Spark::AngelScriptEngine::GetInstance())
        for (auto e : m_scriptedEntities)
            if (e != entt::null) se->CallUpdate(e, dt);

    // --- Network lag compensation snapshots ---
    if (auto* net = Spark::NetworkManager::GetInstance())
        net->GetLagCompensator().RecordSnapshot();

    // --- Player movement (stress the physics + AI tracking) ---
    if (m_player != entt::null)
    {
        auto& pt = world->GetComponent<Transform>(m_player);
        constexpr float speed = 12.0f;
        if (input->IsKeyDown('W')) pt.position.z += speed * dt;
        if (input->IsKeyDown('S')) pt.position.z -= speed * dt;
        if (input->IsKeyDown('A')) pt.position.x -= speed * dt;
        if (input->IsKeyDown('D')) pt.position.x += speed * dt;
    }

    // --- Periodic physics spawns (every 2 seconds, add 10 more bodies) ---
    m_spawnTimer += dt;
    if (m_spawnTimer > 2.0f)
    {
        m_spawnTimer = 0.0f;
        if (m_dynamicBodies.size() < 300) // cap at 300 dynamic bodies
            SpawnPhysicsBatch(10);
    }

    // --- Periodic AI damage (every 0.5 seconds) ---
    m_damageTimer += dt;
    if (m_damageTimer > 0.5f)
    {
        m_damageTimer = 0.0f;
        DamageRandomEntity();
    }

    // --- Periodic explosions (every 4 seconds) ---
    m_explosionTimer += dt;
    if (m_explosionTimer > 4.0f)
    {
        m_explosionTimer = 0.0f;
        float ex = (float)(rand() % 60 - 30);
        float ez = (float)(rand() % 60 - 30);
        SpawnExplosionCluster(ex, ez);
    }

    // --- Auto-save every 30 seconds ---
    m_autoSaveTimer += dt;
    if (m_autoSaveTimer > 30.0f)
    {
        m_autoSaveTimer = 0.0f;
        TriggerAutoSave();
    }

    // --- Weather cycling every 15 seconds ---
    m_weatherTimer += dt;
    if (m_weatherTimer > 15.0f)
    {
        m_weatherTimer = 0.0f;
        m_weatherIndex = (m_weatherIndex + 1) % 4;

        if (!m_staticEntities.empty())
        {
            auto weatherEnt = m_staticEntities[0];
            if (world->HasComponent<WeatherComponent>(weatherEnt))
            {
                auto& wc = world->GetComponent<WeatherComponent>(weatherEnt);
                switch (m_weatherIndex)
                {
                case 0: wc.weatherType = WeatherComponent::Type::Clear; wc.intensity = 0.0f; break;
                case 1: wc.weatherType = WeatherComponent::Type::Cloudy; wc.intensity = 0.5f; break;
                case 2: wc.weatherType = WeatherComponent::Type::Rain; wc.intensity = 0.8f; break;
                case 3: wc.weatherType = WeatherComponent::Type::Snow; wc.intensity = 0.6f; break;
                }
            }
        }
    }

    // --- Fog cycling every 20 seconds ---
    m_fogCycleTimer += dt;
    if (m_fogCycleTimer > 20.0f)
    {
        m_fogCycleTimer = 0.0f;
        m_fogIndex = (m_fogIndex + 1) % 4;

        if (auto* fog = m_ctx->GetGraphics()->GetFogSystem())
        {
            switch (m_fogIndex)
            {
            case 0: fog->SetMode(Spark::FogSystem::Mode::Volumetric); fog->SetDensity(0.015f); break;
            case 1: fog->SetMode(Spark::FogSystem::Mode::Height); fog->SetDensity(0.03f); break;
            case 2: fog->SetMode(Spark::FogSystem::Mode::Exponential); fog->SetDensity(0.04f); break;
            case 3: fog->SetMode(Spark::FogSystem::Mode::Linear); fog->SetStart(10.f); fog->SetEnd(80.f); break;
            }
        }
    }

    // --- Interactive controls ---

    // 1 — Spawn 50 more physics bodies
    if (input->WasKeyPressed('1'))
        SpawnPhysicsBatch(50);

    // 2 — Trigger explosion cluster at random position
    if (input->WasKeyPressed('2'))
    {
        float ex = (float)(rand() % 60 - 30);
        float ez = (float)(rand() % 60 - 30);
        SpawnExplosionCluster(ex, ez);
    }

    // 3 — Cycle render path (Deferred → ForwardPlus → Clustered)
    if (input->WasKeyPressed('3'))
        CycleRenderPath();

    // 4 — Play cinematic flyover
    if (input->WasKeyPressed('4'))
    {
        if (auto* seq = Spark::SequencerManager::GetInstance())
            seq->PlaySequence(m_stressCinematicID);
    }

    // 5 — Force save
    if (input->WasKeyPressed('5'))
        TriggerAutoSave();

    // F5 — Quick save
    if (input->WasKeyPressed(VK_F5))
    {
        auto* saveSys = Spark::SaveSystem::GetInstance();
        if (saveSys) saveSys->QuickSave(*world);
    }

    // F9 — Quick load
    if (input->WasKeyPressed(VK_F9))
    {
        auto* saveSys = Spark::SaveSystem::GetInstance();
        if (saveSys) saveSys->QuickLoad(*world);
    }
}

// ---------------------------------------------------------------------------
void StressTestModule::OnRender() { }
void StressTestModule::OnResize(int, int) { }

SPARK_IMPLEMENT_MODULE(StressTestModule)
