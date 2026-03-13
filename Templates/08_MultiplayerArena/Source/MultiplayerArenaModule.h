#pragma once
/**
 * @file MultiplayerArenaModule.h
 * @brief ADVANCED — Networked multiplayer arena with client/server, replication, and lag compensation.
 *
 * Demonstrates:
 *   - NetworkManager client/server startup (UDP)
 *   - Message channels (Unreliable, Reliable, ReliableOrdered)
 *   - Entity replication via NetworkIdentity
 *   - Client-side input prediction
 *   - LagCompensator hitbox rewinding
 *   - Chat messages
 *   - Match start/end flow
 *   - Score tracking
 *
 * Spark Engine systems used:
 *   Networking — NetworkManager, NetworkIdentity, LagCompensator, NetBuffer
 *   ECS        — World, Transform, MeshRenderer, NameComponent, HealthComponent
 *   Physics    — RigidBodyComponent, ColliderComponent
 *   Events     — EventBus, EntityDamagedEvent, EntityKilledEvent, PlayerRespawnEvent
 *   Input      — InputManager
 *   Rendering  — LightComponent
 */

#include <Core/ModuleManager.h>
#include <Core/EngineContext.h>
#include <Engine/ECS/Components.h>
#include <Engine/Networking/NetworkManager.h>
#include <Engine/Events/EventSystem.h>
#include <vector>

class MultiplayerArenaModule final : public Spark::IModule
{
public:
    Spark::ModuleInfo GetModuleInfo() const override
    {
        return { "MultiplayerArena", "0.2.0", SPARK_SDK_VERSION, 1000 };
    }

    void OnLoad(Spark::IEngineContext* ctx) override;
    void OnUnload() override;
    void OnUpdate(float dt) override;
    void OnRender() override;
    void OnResize(int w, int h) override;

private:
    void BuildArena();
    void SpawnNetworkedPlayer(float x, float z, uint32_t clientID, bool isLocal);
    void HandleInput(float dt);
    void SendPlayerState();

    Spark::IEngineContext* m_ctx = nullptr;

    // Arena
    entt::entity m_floor = entt::null;
    entt::entity m_light = entt::null;
    std::vector<entt::entity> m_walls;

    // Players
    entt::entity m_localPlayer = entt::null;
    std::vector<entt::entity> m_remotePlayers;

    // Network state
    bool m_isServer     = false;
    bool m_isConnected  = false;
    float m_sendTimer   = 0.0f;

    // Event subscriptions
    Spark::SubscriptionID m_damageSub  = 0;
    Spark::SubscriptionID m_killSub    = 0;
    Spark::SubscriptionID m_respawnSub = 0;
};
