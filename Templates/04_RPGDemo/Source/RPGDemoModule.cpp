/**
 * @file RPGDemoModule.cpp
 * @brief INTERMEDIATE — RPG mechanics: inventory, quests, health, save/load, events.
 */

#include "RPGDemoModule.h"
#include <Input/InputManager.h>
#include <string>

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void RPGDemoModule::OnLoad(Spark::IEngineContext* ctx)
{
    m_ctx = ctx;

    // Define a simple quest line
    m_questLine = {
        "Gather Supplies",
        "Explore the Ruins",
        "Defeat the Guardian",
        "Return to Village"
    };
    m_currentQuestIdx = 0;

    // Initialise save system with the ECS world
    m_saveSystem.Initialize(m_ctx->GetWorld());

    SetupEvents();
    BuildWorld();

    // Start the first quest
    StartQuest(m_questLine[0]);
}

void RPGDemoModule::OnUnload()
{
    if (auto* bus = m_ctx ? m_ctx->GetEventBus() : nullptr)
    {
        bus->Unsubscribe<Spark::EntityDamagedEvent>(m_damageSub);
        bus->Unsubscribe<Spark::EntityKilledEvent>(m_killSub);
        bus->Unsubscribe<Spark::ItemPickedUpEvent>(m_pickupSub);
        bus->Unsubscribe<Spark::QuestCompletedEvent>(m_questSub);
    }

    auto* world = m_ctx ? m_ctx->GetWorld() : nullptr;
    if (world)
    {
        if (m_player != entt::null) world->DestroyEntity(m_player);
        if (m_enemy  != entt::null) world->DestroyEntity(m_enemy);
        for (auto e : m_collectables)
            if (e != entt::null) world->DestroyEntity(e);
    }
    m_collectables.clear();
    m_ctx = nullptr;
}

// ---------------------------------------------------------------------------
// Event wiring
// ---------------------------------------------------------------------------

void RPGDemoModule::SetupEvents()
{
    auto* bus = m_ctx->GetEventBus();
    if (!bus) return;

    m_damageSub = bus->Subscribe<Spark::EntityDamagedEvent>(
        [](const Spark::EntityDamagedEvent& /*e*/) {
            // Flash screen red, play hurt sound, etc.
        });

    m_killSub = bus->Subscribe<Spark::EntityKilledEvent>(
        [this](const Spark::EntityKilledEvent& /*e*/) {
            // Could trigger game-over screen, respawn logic, etc.
        });

    m_pickupSub = bus->Subscribe<Spark::ItemPickedUpEvent>(
        [this](const Spark::ItemPickedUpEvent& /*e*/) {
            // Award currency, update HUD, etc.
            auto* world = m_ctx->GetWorld();
            if (world && m_player != entt::null)
            {
                auto& inv = world->GetComponent<InventoryTag>(m_player);
                inv.currency += 10; // 10 gold per pickup
            }
        });

    m_questSub = bus->Subscribe<Spark::QuestCompletedEvent>(
        [](const Spark::QuestCompletedEvent& /*e*/) {
            // Show completion banner, play fanfare, etc.
        });
}

// ---------------------------------------------------------------------------
// World construction
// ---------------------------------------------------------------------------

void RPGDemoModule::BuildWorld()
{
    auto* world = m_ctx->GetWorld();

    // --- Player entity with full RPG components ---
    m_player = world->CreateEntity();
    world->AddComponent<NameComponent>(m_player, NameComponent{ "Player" });
    world->AddComponent<Transform>(m_player, Transform{
        { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 2.0f, 1.0f }
    });
    world->AddComponent<MeshRenderer>(m_player, MeshRenderer{
        "Assets/Models/cube.obj", "Assets/Materials/player.mat",
        true, true, true
    });
    world->AddComponent<HealthComponent>(m_player, HealthComponent{
        100.0f, 100.0f, false, false
    });
    world->AddComponent<InventoryTag>(m_player, InventoryTag{
        20,      // maxSlots
        100.0f,  // maxWeight
        50,      // starting currency (gold)
        true     // hasInventory
    });
    world->AddComponent<QuestTrackerTag>(m_player, QuestTrackerTag{
        0, 0, false
    });
    world->AddComponent<TagComponent>(m_player);
    world->GetComponent<TagComponent>(m_player).AddTag("player");
    world->GetComponent<TagComponent>(m_player).AddTag("friendly");
    world->AddComponent<ActiveComponent>(m_player, ActiveComponent{ true });

    // --- Enemy entity ---
    m_enemy = world->CreateEntity();
    world->AddComponent<NameComponent>(m_enemy, NameComponent{ "Guardian" });
    world->AddComponent<Transform>(m_enemy, Transform{
        { 10.0f, 1.0f, 10.0f }, { 0.0f, 0.0f, 0.0f }, { 1.5f, 2.5f, 1.5f }
    });
    world->AddComponent<MeshRenderer>(m_enemy, MeshRenderer{
        "Assets/Models/cube.obj", "Assets/Materials/enemy.mat",
        true, true, true
    });
    world->AddComponent<HealthComponent>(m_enemy, HealthComponent{
        200.0f, 200.0f, false, false
    });
    world->AddComponent<TagComponent>(m_enemy);
    world->GetComponent<TagComponent>(m_enemy).AddTag("enemy");
    world->GetComponent<TagComponent>(m_enemy).AddTag("boss");
    world->AddComponent<ActiveComponent>(m_enemy, ActiveComponent{ true });

    // --- Scatter collectables around the scene ---
    SpawnCollectable("HealthPotion",  -5.0f,  3.0f, "healing");
    SpawnCollectable("GoldCoin_1",     3.0f, -2.0f, "currency");
    SpawnCollectable("GoldCoin_2",    -7.0f, -5.0f, "currency");
    SpawnCollectable("QuestScroll",    8.0f,  1.0f, "quest_item");
    SpawnCollectable("MagicGem",      -3.0f,  8.0f, "rare");

    // --- Ground ---
    auto ground = world->CreateEntity();
    world->AddComponent<NameComponent>(ground, NameComponent{ "Ground" });
    world->AddComponent<Transform>(ground, Transform{
        { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 40.0f, 0.2f, 40.0f }
    });
    world->AddComponent<MeshRenderer>(ground, MeshRenderer{
        "Assets/Models/plane.obj", "Assets/Materials/grass.mat",
        false, true, true
    });

    // --- Light ---
    auto light = world->CreateEntity();
    world->AddComponent<NameComponent>(light, NameComponent{ "SunLight" });
    world->AddComponent<Transform>(light, Transform{
        { 0.0f, 20.0f, 0.0f }, { -50.0f, 30.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }
    });
    world->AddComponent<LightComponent>(light, LightComponent{
        LightComponent::Type::Directional,
        { 1.0f, 0.95f, 0.85f }, 1.5f, 0.0f,
        0.0f, 0.0f, true, 2048
    });
}

void RPGDemoModule::SpawnCollectable(const char* name, float x, float z,
                                      const char* tag)
{
    auto* world = m_ctx->GetWorld();
    auto e = world->CreateEntity();

    world->AddComponent<NameComponent>(e, NameComponent{ name });
    world->AddComponent<Transform>(e, Transform{
        { x, 0.5f, z }, { 0.0f, 0.0f, 0.0f }, { 0.4f, 0.4f, 0.4f }
    });
    world->AddComponent<MeshRenderer>(e, MeshRenderer{
        "Assets/Models/sphere.obj", "Assets/Materials/gold.mat",
        false, false, true
    });
    world->AddComponent<TagComponent>(e);
    world->GetComponent<TagComponent>(e).AddTag("collectable");
    world->GetComponent<TagComponent>(e).AddTag(tag);
    world->AddComponent<ActiveComponent>(e, ActiveComponent{ true });

    m_collectables.push_back(e);
}

// ---------------------------------------------------------------------------
// Gameplay actions
// ---------------------------------------------------------------------------

void RPGDemoModule::TryPickup()
{
    auto* world = m_ctx->GetWorld();
    if (m_player == entt::null) return;

    auto& playerXf = world->GetComponent<Transform>(m_player);
    const float pickupRange = 3.0f;

    for (auto it = m_collectables.begin(); it != m_collectables.end(); )
    {
        auto e = *it;
        if (e == entt::null || !world->HasComponent<ActiveComponent>(e))
        {
            it = m_collectables.erase(it);
            continue;
        }

        auto& itemXf = world->GetComponent<Transform>(e);
        float dx = itemXf.position.x - playerXf.position.x;
        float dz = itemXf.position.z - playerXf.position.z;
        float distSq = dx * dx + dz * dz;

        if (distSq <= pickupRange * pickupRange)
        {
            // Check if it's a healing item
            auto& tags = world->GetComponent<TagComponent>(e);
            if (tags.HasTag("healing"))
                TryHealPlayer(25.0f);

            // Publish pickup event
            if (auto* bus = m_ctx->GetEventBus())
                bus->Publish(Spark::ItemPickedUpEvent{ m_player, e });

            // Remove the item from the world
            world->DestroyEntity(e);
            it = m_collectables.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void RPGDemoModule::TryDamagePlayer(float amount)
{
    auto* world = m_ctx->GetWorld();
    if (m_player == entt::null) return;

    auto& hp = world->GetComponent<HealthComponent>(m_player);
    hp.TakeDamage(amount);

    if (auto* bus = m_ctx->GetEventBus())
        bus->Publish(Spark::EntityDamagedEvent{ m_player, entt::null, amount });

    if (hp.isDead)
    {
        if (auto* bus = m_ctx->GetEventBus())
            bus->Publish(Spark::EntityKilledEvent{ m_player, entt::null });
    }
}

void RPGDemoModule::TryHealPlayer(float amount)
{
    auto* world = m_ctx->GetWorld();
    if (m_player == entt::null) return;

    auto& hp = world->GetComponent<HealthComponent>(m_player);
    hp.Heal(amount);
}

void RPGDemoModule::StartQuest(const std::string& questName)
{
    (void)questName; // quest name used for display/logging
    auto* world = m_ctx->GetWorld();
    if (m_player == entt::null) return;

    auto& qt = world->GetComponent<QuestTrackerTag>(m_player);
    qt.activeQuestCount++;
}

void RPGDemoModule::CompleteCurrentQuest()
{
    auto* world = m_ctx->GetWorld();
    if (m_player == entt::null) return;
    if (m_currentQuestIdx >= static_cast<int>(m_questLine.size())) return;

    auto& qt = world->GetComponent<QuestTrackerTag>(m_player);
    qt.activeQuestCount--;
    qt.completedQuestCount++;

    // Publish event
    if (auto* bus = m_ctx->GetEventBus())
        bus->Publish(Spark::QuestCompletedEvent{ m_questLine[m_currentQuestIdx] });

    // Advance quest line
    m_currentQuestIdx++;
    if (m_currentQuestIdx < static_cast<int>(m_questLine.size()))
        StartQuest(m_questLine[m_currentQuestIdx]);
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

void RPGDemoModule::OnUpdate(float dt)
{
    auto* input = m_ctx->GetInput();
    auto* world = m_ctx->GetWorld();
    if (!input || !world) return;

    // --- Player movement (WASD) ---
    if (m_player != entt::null)
    {
        auto& xf = world->GetComponent<Transform>(m_player);
        const float speed = 6.0f;
        if (input->IsKeyDown('W')) xf.position.z += speed * dt;
        if (input->IsKeyDown('S')) xf.position.z -= speed * dt;
        if (input->IsKeyDown('A')) xf.position.x -= speed * dt;
        if (input->IsKeyDown('D')) xf.position.x += speed * dt;
    }

    // --- Interact / Pickup (E) ---
    if (input->WasKeyPressed('E'))
        TryPickup();

    // --- Damage self for testing (X) ---
    if (input->WasKeyPressed('X'))
        TryDamagePlayer(15.0f);

    // --- Heal self (H) ---
    if (input->WasKeyPressed('H'))
        TryHealPlayer(20.0f);

    // --- Complete current quest (Q) ---
    if (input->WasKeyPressed('Q'))
        CompleteCurrentQuest();

    // --- Save / Load ---
    if (input->WasKeyPressed(VK_F5))
        m_saveSystem.QuickSave();

    if (input->WasKeyPressed(VK_F9))
        m_saveSystem.QuickLoad();

    if (input->WasKeyPressed(VK_F6))
        m_saveSystem.Save("manual_save_1");

    if (input->WasKeyPressed(VK_F7))
        m_saveSystem.Load("manual_save_1");

    // --- Auto-save ---
    m_autoSaveTimer += dt;
    if (m_autoSaveTimer >= kAutoSaveInterval)
    {
        m_autoSaveTimer = 0.0f;
        m_saveSystem.AutoSave();
    }
}

// ---------------------------------------------------------------------------
void RPGDemoModule::OnRender() { }
void RPGDemoModule::OnResize(int, int) { }

SPARK_IMPLEMENT_MODULE(RPGDemoModule)
