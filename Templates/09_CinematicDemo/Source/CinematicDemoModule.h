#pragma once
/**
 * @file CinematicDemoModule.h
 * @brief INTERMEDIATE — Cinematic sequencer, camera paths, audio cues, and coroutines.
 *
 * Demonstrates:
 *   - SequencerManager for cutscene playback
 *   - Track types: CameraPath, EntityTransform, AudioCue, Subtitle, Fade, Event
 *   - CatmullRom and CubicBezier interpolation
 *   - Camera FOV/roll animation
 *   - CoroutineScheduler for scripted sequences
 *   - AudioEngine / MusicManager for background music + 3D sound
 *   - Event callbacks from sequencer
 *
 * Spark Engine systems used:
 *   Cinematic  — SequencerManager, Sequence tracks
 *   Coroutine  — CoroutineScheduler (WaitForSeconds, WaitUntil, WaitForFrames)
 *   Audio      — AudioEngine, MusicManager (buses, playlists, reverb zones)
 *   ECS        — World, Transform, MeshRenderer, NameComponent
 *   Events     — EventBus
 *   Input      — InputManager
 *   Rendering  — LightComponent, Camera component
 */

#include <Core/ModuleManager.h>
#include <Core/EngineContext.h>
#include <Engine/ECS/Components.h>
#include <Engine/Cinematic/Sequencer.h>
#include <Engine/Coroutine/CoroutineScheduler.h>
#include <vector>

class CinematicDemoModule final : public Spark::IModule
{
public:
    Spark::ModuleInfo GetModuleInfo() const override
    {
        return { "CinematicDemo", "0.1.0", SPARK_SDK_VERSION, 1000 };
    }

    void OnLoad(Spark::IEngineContext* ctx) override;
    void OnUnload() override;
    void OnUpdate(float dt) override;
    void OnRender() override;
    void OnResize(int w, int h) override;

private:
    void BuildScene();
    void CreateIntroCinematic();
    void CreateFlyoverCinematic();
    void SetupAudio();
    void StartCoroutineDemo();

    Spark::IEngineContext* m_ctx = nullptr;

    // Scene entities
    entt::entity m_floor  = entt::null;
    entt::entity m_light  = entt::null;
    entt::entity m_camera = entt::null;
    std::vector<entt::entity> m_props;

    // Sequence IDs
    uint32_t m_introSequenceID   = 0;
    uint32_t m_flyoverSequenceID = 0;

    bool m_cinematicPlaying = false;
};
