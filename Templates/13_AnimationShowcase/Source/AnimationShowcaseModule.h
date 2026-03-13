#pragma once
/**
 * @file AnimationShowcaseModule.h
 * @brief INTERMEDIATE — Skeletal animation, state machines, blending, layers, and IK.
 *
 * Demonstrates:
 *   - AnimationComponent with clips (idle, walk, run, attack, jump)
 *   - AnimationStateMachine with states and conditional transitions
 *   - Animation blending / crossfade between states
 *   - AnimationLayer system (base body + upper body override)
 *   - IKComponent with IKTarget for feet and hands
 *   - Animation events (footstep sounds at specific keyframes)
 *   - Runtime animation speed / weight control
 *   - Multiple animated entities with different state machines
 *
 * Spark Engine systems used:
 *   Animation  — AnimationComponent, AnimationStateMachine, AnimationLayer, IKComponent
 *   ECS        — World, NameComponent, Transform, MeshRenderer
 *   Input      — InputManager (IsKeyDown / WasKeyPressed / IsMouseButtonDown)
 *   Audio      — AudioEngine (footstep / impact sounds)
 *   Events     — EventBus (AnimationEventFired)
 *   Rendering  — LightComponent, Camera
 */

#include <Core/ModuleManager.h>
#include <Core/EngineContext.h>
#include <Engine/ECS/Components.h>
#include <Engine/Animation/AnimationSystem.h>
#include <Engine/Events/EventSystem.h>
#include <vector>

class AnimationShowcaseModule final : public Spark::IModule
{
public:
    Spark::ModuleInfo GetModuleInfo() const override
    {
        return { "AnimationShowcase", "0.2.0", SPARK_SDK_VERSION, 1000 };
    }

    void OnLoad(Spark::IEngineContext* ctx) override;
    void OnUnload() override;
    void OnUpdate(float dt) override;
    void OnRender() override;
    void OnResize(int w, int h) override;

private:
    void BuildScene();
    void SetupPlayerAnimation();
    void SetupAnimationEvents();
    void SpawnPatrolNPC(float x, float z);
    void SpawnDanceNPC(float x, float z);
    void UpdatePlayerMovement(float dt);
    void UpdateCamera(float dt);

    Spark::IEngineContext* m_ctx = nullptr;

    // Scene entities
    entt::entity m_floor   = entt::null;
    entt::entity m_light   = entt::null;
    entt::entity m_camera  = entt::null;
    entt::entity m_player  = entt::null;

    // NPC entities
    std::vector<entt::entity> m_npcs;

    // Event subscription
    Spark::SubscriptionID m_animEventSub = 0;

    // Player state
    float m_playerSpeed    = 0.0f;
    float m_playerYaw      = 0.0f;
    bool  m_ikEnabled      = true;
    bool  m_animPaused     = false;
    int   m_npcCounter     = 0;

    // Camera orbit
    float m_camYaw   = 0.0f;
    float m_camPitch = -20.0f;
    float m_camDist  = 12.0f;

    // Constants
    static constexpr float kWalkSpeed    = 3.0f;
    static constexpr float kRunSpeed     = 7.0f;
    static constexpr float kRotateSpeed  = 180.0f;
    static constexpr float kBlendTime    = 0.25f;
};
