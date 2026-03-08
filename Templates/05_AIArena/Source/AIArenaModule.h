#pragma once
/**
 * @file AIArenaModule.h
 * @brief INTERMEDIATE — AI showcase with behavior trees, NavMesh, perception, and steering.
 *
 * Demonstrates:
 *   - BehaviorTree construction (Sequence, Selector, Action, Condition nodes)
 *   - Blackboard data sharing between AI nodes
 *   - NavMesh building and pathfinding (A*)
 *   - PerceptionSystem (sight cone, hearing radius)
 *   - SteeringBehaviors (Seek, Flee, Wander, Arrive, Separation)
 *   - AIComponent state machine (Idle → Patrolling → Alert → Combat → Fleeing)
 *   - Pre-built behavior presets (Patrol, Combat, Guard, Flee)
 *
 * Spark Engine systems used:
 *   AI        — AISystem, BehaviorTree, NavMesh, PerceptionSystem, SteeringBehaviors
 *   ECS       — World, Transform, MeshRenderer, NameComponent, HealthComponent
 *   Physics   — RigidBodyComponent, ColliderComponent
 *   Events    — EventBus, EntityDamagedEvent, EntityKilledEvent
 *   Input     — InputManager
 *   Rendering — LightComponent
 */

#include <Core/ModuleManager.h>
#include <Core/EngineContext.h>
#include <Engine/ECS/Components.h>
#include <Engine/AI/AISystem.h>
#include <Engine/AI/BehaviorTree.h>
#include <Engine/AI/NavMesh.h>
#include <Engine/AI/PerceptionSystem.h>
#include <Engine/AI/SteeringBehaviors.h>
#include <Engine/Events/EventSystem.h>
#include <vector>

class AIArenaModule final : public Spark::IModule
{
public:
    Spark::ModuleInfo GetModuleInfo() const override
    {
        return { "AIArena", "0.1.0", SPARK_SDK_VERSION, 1000 };
    }

    void OnLoad(Spark::IEngineContext* ctx) override;
    void OnUnload() override;
    void OnUpdate(float dt) override;
    void OnRender() override;
    void OnResize(int w, int h) override;

private:
    void BuildArena();
    void SpawnPatroller(float x, float z, const std::string& name);
    void SpawnGuard(float x, float z, const std::string& name);
    void SpawnFleer(float x, float z, const std::string& name);
    void BuildNavMesh();
    void ResetArena();

    Spark::IEngineContext* m_ctx = nullptr;

    // Environment
    entt::entity m_floor    = entt::null;
    entt::entity m_light    = entt::null;
    entt::entity m_player   = entt::null;

    // AI agents
    std::vector<entt::entity> m_agents;

    // Obstacles for NavMesh
    std::vector<entt::entity> m_obstacles;

    // Event subscriptions
    Spark::SubscriptionID m_damageSub = 0;
    Spark::SubscriptionID m_killSub   = 0;

    int m_agentCount = 0;
};
