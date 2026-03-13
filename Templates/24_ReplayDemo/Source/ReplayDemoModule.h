#pragma once
/**
 * @file ReplayDemoModule.h
 * @brief ADVANCED — Replay system showcase.
 *
 * Demonstrates:
 *   - Recording gameplay (positions, rotations of all entities)
 *   - Playback with timeline scrubbing
 *   - Variable playback speed (0.25x, 0.5x, 1x, 2x, 4x)
 *   - Pause / resume during playback
 *   - Kill cam (slow-mo replay of last kill from dramatic angle)
 *   - Save / load replay to file
 *   - AI-driven target entities that move around
 *   - Raycast "shooting" of targets
 *   - DebugDraw UI overlay for status and timeline
 *
 * Spark Engine systems used:
 *   Replay     — ReplaySystem
 *   ECS        — World, Transform, MeshRenderer, Camera, NameComponent
 *   Physics    — RigidBodyComponent, ColliderComponent
 *   Input      — InputManager
 *   Events     — EventBus
 *   Debug      — DebugDraw (ScreenText, ScreenRect, ScreenLine)
 */

#include <Core/ModuleManager.h>
#include <Core/EngineContext.h>
#include <Engine/ECS/Components.h>
#include <Engine/Debug/DebugDraw.h>
#include <Engine/Events/EventSystem.h>
#include <Engine/Replay/ReplaySystem.h>
#include <vector>
#include <string>

// ---------------------------------------------------------------------------
// Events
// ---------------------------------------------------------------------------

struct ReplayStartedEvent
{
    bool fromFile;
};

struct ReplayEndedEvent
{
    float duration;
};

// ---------------------------------------------------------------------------
// AI Target state
// ---------------------------------------------------------------------------

struct AITarget
{
    entt::entity entity;
    std::string  name;
    bool         alive;
    float        moveTimer;
    float        moveDirX;
    float        moveDirZ;
    float        moveSpeed;
    float        nextDirChange;
    float        spawnX, spawnY, spawnZ;
};

// ---------------------------------------------------------------------------
// Kill record (for kill cam)
// ---------------------------------------------------------------------------

struct KillRecord
{
    float  time;         // recording time of the kill
    float  victimX, victimY, victimZ;
    float  playerX, playerY, playerZ;
    float  playerYaw, playerPitch;
};

// ---------------------------------------------------------------------------
// Module
// ---------------------------------------------------------------------------

class ReplayDemoModule final : public Spark::IModule
{
public:
    Spark::ModuleInfo GetModuleInfo() const override
    {
        return { "ReplayDemo", "0.2.0", SPARK_SDK_VERSION, 1000 };
    }

    void OnLoad(Spark::IEngineContext* ctx) override;
    void OnUnload() override;
    void OnUpdate(float dt) override;
    void OnRender() override;
    void OnResize(int w, int h) override;

private:
    // Scene
    void BuildScene();
    void ResetTargets();

    // Gameplay
    void HandleLiveInput(float dt);
    void HandlePlaybackInput(float dt);
    void UpdateAI(float dt);
    void ShootRaycast();
    void StartPlayback();
    void StopPlaybackAndRecord();
    void TriggerKillCam();
    void UpdateKillCam(float dt);

    // Drawing
    void DrawCrosshair();
    void DrawStatusOverlay();
    void DrawTimeline();
    void DrawKillCamOverlay();

    Spark::IEngineContext* m_ctx = nullptr;

    // Screen
    float m_screenW = 1920.0f;
    float m_screenH = 1080.0f;

    // Core entities
    entt::entity m_player = entt::null;
    entt::entity m_camera = entt::null;
    entt::entity m_light  = entt::null;
    std::vector<entt::entity> m_sceneEntities;

    // Camera / player
    float m_playerX = 0.0f;
    float m_playerY = 1.7f;
    float m_playerZ = 0.0f;
    float m_camYaw   = 0.0f;
    float m_camPitch = 0.0f;

    // AI targets
    std::vector<AITarget> m_targets;

    // State
    bool m_isRecording  = false;
    bool m_isPlaying    = false;
    bool m_isPaused     = false;
    bool m_inKillCam    = false;

    // Recording time
    float m_recordTime = 0.0f;

    // Playback
    float m_playbackSpeed = 1.0f;
    static constexpr float kSpeedOptions[] = { 0.25f, 0.5f, 1.0f, 2.0f, 4.0f };
    int   m_speedIndex = 2; // default 1x

    // Kill tracking
    std::vector<KillRecord> m_kills;
    int m_killCount = 0;

    // Kill cam state
    float m_killCamTimer    = 0.0f;
    float m_killCamDuration = 3.0f;
    KillRecord m_killCamTarget;

    // Event subscriptions
    uint64_t m_replayStartSubId = 0;
    uint64_t m_replayEndSubId   = 0;
};
