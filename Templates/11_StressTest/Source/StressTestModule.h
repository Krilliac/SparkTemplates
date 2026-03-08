#pragma once
/**
 * @file StressTestModule.h
 * @brief ADVANCED — Full engine stress test exercising every major system simultaneously.
 *
 * This template is designed to push SparkEngine to its absolute limits by running
 * all 17+ engine systems concurrently in a single scene:
 *
 *   - ECS with 500+ entities (spawned/destroyed dynamically)
 *   - Physics with 100+ active rigid bodies (boxes, spheres, capsules)
 *   - AI with 20+ behavior-tree-driven agents using NavMesh + perception + steering
 *   - Animation with skeletal playback, IK solvers, state machines, blending
 *   - Particle system with 10+ concurrent emitters (fire, smoke, sparks, rain, magic)
 *   - Decal system with surface-mapped bullet holes, scorch marks, cracks
 *   - Fog system cycling through all modes (Linear, Exp, Height, Volumetric)
 *   - Post-processing full pipeline (Bloom, SSAO, SSR, TAA, ColorGrading, FXAA)
 *   - Procedural generation (noise terrain + erosion + object scatter + WFC)
 *   - Networking simulation (server + local replication + lag compensation)
 *   - Save system (auto-save, quick-save, serialization of full world state)
 *   - Cinematic sequencer (camera paths, subtitles, audio cues, fades, events)
 *   - Coroutine scheduler (timed event chains)
 *   - Day/night cycle with weather transitions
 *   - Audio engine with 3D positional sound, music manager, reverb zones
 *   - AngelScript with multiple compiled modules + hot-reload
 *   - Event bus with high-frequency publish/subscribe across all systems
 *   - Material system with PBR, transparency, emissive, subsurface scattering
 *   - Lighting with directional, point, spot, area lights + cascaded shadow maps
 *   - LOD system with auto-generated LOD chains
 *   - Frustum culling under heavy entity load
 *   - Inventory + quest + health systems under rapid state changes
 *   - Render path switching (Forward → Deferred → ForwardPlus)
 *
 * Spark Engine systems used: ALL OF THEM.
 */

#include <Core/ModuleManager.h>
#include <Core/EngineContext.h>
#include <Engine/ECS/Components.h>
#include <Engine/AI/AISystem.h>
#include <Engine/AI/BehaviorTree.h>
#include <Engine/AI/NavMesh.h>
#include <Engine/AI/SteeringBehaviors.h>
#include <Engine/Networking/NetworkManager.h>
#include <Engine/Cinematic/Sequencer.h>
#include <Engine/Coroutine/CoroutineScheduler.h>
#include <Engine/SaveSystem/SaveSystem.h>
#include <Engine/Scripting/AngelScriptEngine.h>
#include <Engine/Procedural/ProceduralGeneration.h>
#include <Engine/Events/EventSystem.h>
#include <Engine/World/DayNightCycle.h>
#include <vector>

class StressTestModule final : public Spark::IModule
{
public:
    Spark::ModuleInfo GetModuleInfo() const override
    {
        return { "StressTest", "0.1.0", SPARK_SDK_VERSION, 1000 };
    }

    void OnLoad(Spark::IEngineContext* ctx) override;
    void OnUnload() override;
    void OnUpdate(float dt) override;
    void OnRender() override;
    void OnResize(int w, int h) override;

private:
    // --- Setup phases ---
    void Phase1_Environment();      // Terrain, lighting, day/night, weather, fog
    void Phase2_Physics();          // 100+ rigid bodies, collision arena
    void Phase3_AI();               // 20+ AI agents, NavMesh, behavior trees
    void Phase4_Particles();        // 10+ emitters, decals, post-processing
    void Phase5_Audio();            // 3D audio, music, reverb zones
    void Phase6_Networking();       // Server + replicated entities
    void Phase7_Animation();        // Animated entities with IK, state machines
    void Phase8_Procedural();       // Noise terrain, WFC dungeon, object scatter
    void Phase9_Scripting();        // AngelScript modules
    void Phase10_Cinematic();       // Sequencer + coroutines
    void Phase11_Gameplay();        // Health, inventory, quests, save system

    // --- Runtime stress ---
    void SpawnPhysicsBatch(int count);
    void DamageRandomEntity();
    void CycleRenderPath();
    void TriggerAutoSave();
    void SpawnExplosionCluster(float x, float z);

    Spark::IEngineContext* m_ctx = nullptr;

    // --- Entity pools ---
    entt::entity m_terrain     = entt::null;
    entt::entity m_player      = entt::null;
    entt::entity m_sunLight    = entt::null;
    entt::entity m_camera      = entt::null;

    std::vector<entt::entity> m_staticEntities;
    std::vector<entt::entity> m_dynamicBodies;
    std::vector<entt::entity> m_aiAgents;
    std::vector<entt::entity> m_particleEmitters;
    std::vector<entt::entity> m_animatedEntities;
    std::vector<entt::entity> m_networkEntities;
    std::vector<entt::entity> m_scriptedEntities;
    std::vector<entt::entity> m_gameplayEntities;
    std::vector<entt::entity> m_lights;
    std::vector<entt::entity> m_wfcTiles;

    // --- Timers and counters ---
    float  m_spawnTimer        = 0.0f;
    float  m_damageTimer       = 0.0f;
    float  m_explosionTimer    = 0.0f;
    float  m_autoSaveTimer     = 0.0f;
    float  m_weatherTimer      = 0.0f;
    float  m_fogCycleTimer     = 0.0f;
    int    m_entityCounter     = 0;
    int    m_renderPathIndex   = 0;
    int    m_weatherIndex      = 0;
    int    m_fogIndex          = 0;

    // --- Sequencer IDs ---
    uint32_t m_stressCinematicID = 0;

    // --- Event subscriptions ---
    Spark::SubscriptionID m_collisionSub  = 0;
    Spark::SubscriptionID m_damageSub     = 0;
    Spark::SubscriptionID m_killSub       = 0;
    Spark::SubscriptionID m_pickupSub     = 0;
    Spark::SubscriptionID m_questSub      = 0;
    Spark::SubscriptionID m_weatherSub    = 0;
    Spark::SubscriptionID m_timeSub       = 0;
};
