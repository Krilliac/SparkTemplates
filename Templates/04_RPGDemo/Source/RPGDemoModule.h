#pragma once
/**
 * @file RPGDemoModule.h
 * @brief INTERMEDIATE — RPG mechanics: inventory, quests, health, save/load, events.
 *
 * Demonstrates:
 *   - HealthComponent     — TakeDamage(), Heal(), death detection
 *   - InventoryTag        — slot/weight tracking, currency
 *   - QuestTrackerTag     — quest counting and completion
 *   - TagComponent        — entity categorisation with string tags
 *   - ActiveComponent     — enabling / disabling entities
 *   - SaveSystem          — Save, Load, QuickSave, QuickLoad, AutoSave
 *   - EventBus            — EntityDamagedEvent, EntityKilledEvent,
 *                           ItemPickedUpEvent, QuestCompletedEvent
 *   - Custom game state   — via SaveSystem custom data (key-value pairs)
 *   - LifecycleSystem     — automatic death callback processing
 *
 * Spark Engine systems used:
 *   ECS        — World, all gameplay components
 *   Events     — EventBus + 4 gameplay event types
 *   SaveSystem — SaveSystem (save/load/quicksave/autosave)
 *   Input      — InputManager
 */

#include <Core/ModuleManager.h>
#include <Core/EngineContext.h>
#include <Engine/ECS/Components.h>
#include <Engine/Events/EventSystem.h>
#include <Engine/SaveSystem/SaveSystem.h>
#include <vector>

class RPGDemoModule final : public Spark::IModule
{
public:
    Spark::ModuleInfo GetModuleInfo() const override
    {
        return { "RPGDemo", "0.1.0", SPARK_SDK_VERSION, 1000 };
    }

    void OnLoad(Spark::IEngineContext* ctx) override;
    void OnUnload() override;
    void OnUpdate(float dt) override;
    void OnRender() override;
    void OnResize(int w, int h) override;

private:
    void BuildWorld();
    void SetupEvents();
    void SpawnCollectable(const char* name, float x, float z, const char* tag);
    void TryPickup();
    void TryDamagePlayer(float amount);
    void TryHealPlayer(float amount);
    void StartQuest(const std::string& questName);
    void CompleteCurrentQuest();

    Spark::IEngineContext* m_ctx = nullptr;
    Spark::SaveSystem     m_saveSystem;

    // Player entity
    entt::entity m_player = entt::null;

    // Collectables / items in the world
    std::vector<entt::entity> m_collectables;

    // NPCs / enemies
    entt::entity m_enemy = entt::null;

    // Quest state (simple sequential quest line)
    std::vector<std::string> m_questLine;
    int m_currentQuestIdx = 0;

    // Auto-save timer
    float m_autoSaveTimer = 0.0f;
    static constexpr float kAutoSaveInterval = 120.0f; // every 2 minutes

    // Event subscriptions
    Spark::SubscriptionID m_damageSub  = 0;
    Spark::SubscriptionID m_killSub    = 0;
    Spark::SubscriptionID m_pickupSub  = 0;
    Spark::SubscriptionID m_questSub   = 0;
};
