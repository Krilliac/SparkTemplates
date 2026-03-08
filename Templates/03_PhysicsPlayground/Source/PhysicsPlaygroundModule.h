#pragma once
/**
 * @file PhysicsPlaygroundModule.h
 * @brief INTERMEDIATE — Physics sandbox with rigid bodies, colliders, and raycasting.
 *
 * Demonstrates:
 *   - Bullet Physics integration via RigidBodyComponent & ColliderComponent
 *   - Static, kinematic, and dynamic body types
 *   - Box, sphere, and capsule collider shapes
 *   - Spawning entities at runtime
 *   - Raycasting through the physics world
 *   - Collision events via EventBus
 *   - Primitive mesh generation (Primitives::CreateCube/Sphere/Plane)
 *   - Multiple entity management with std::vector<entt::entity>
 *
 * Spark Engine systems used:
 *   Physics   — RigidBodyComponent, ColliderComponent, PhysicsUpdateSystem
 *   ECS       — World, Transform, MeshRenderer, NameComponent, ActiveComponent
 *   Events    — EventBus, CollisionEvent
 *   Input     — InputManager
 *   Rendering — LightComponent
 */

#include <Core/ModuleManager.h>
#include <Core/EngineContext.h>
#include <Engine/ECS/Components.h>
#include <Engine/Events/EventSystem.h>
#include <vector>

class PhysicsPlaygroundModule final : public Spark::IModule
{
public:
    Spark::ModuleInfo GetModuleInfo() const override
    {
        return { "PhysicsPlayground", "0.1.0", SPARK_SDK_VERSION, 1000 };
    }

    void OnLoad(Spark::IEngineContext* ctx) override;
    void OnUnload() override;
    void OnUpdate(float dt) override;
    void OnRender() override;
    void OnResize(int w, int h) override;

private:
    // Scene builders
    void BuildArena();
    void SpawnBox(float x, float y, float z);
    void SpawnSphere(float x, float y, float z);
    void SpawnStack(float baseX, float baseZ, int height);
    void LaunchProjectile();
    void ClearDynamic();

    Spark::IEngineContext* m_ctx = nullptr;

    // Static environment
    entt::entity m_floor   = entt::null;
    entt::entity m_wallN   = entt::null;
    entt::entity m_wallS   = entt::null;
    entt::entity m_wallE   = entt::null;
    entt::entity m_wallW   = entt::null;
    entt::entity m_ramp    = entt::null;
    entt::entity m_light   = entt::null;

    // Dynamic objects spawned at runtime
    std::vector<entt::entity> m_dynamicEntities;

    // Spawn tracking
    int m_spawnCount = 0;

    // Collision event subscription
    Spark::SubscriptionID m_collisionSub = 0;
};
