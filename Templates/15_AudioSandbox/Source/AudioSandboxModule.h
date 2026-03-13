#pragma once
/**
 * @file AudioSandboxModule.h
 * @brief INTERMEDIATE — Complete audio system demonstration.
 *
 * Demonstrates:
 *   - AudioEngine: LoadSound, PlaySound, Play3DSound with position
 *   - MusicManager: playlists, crossfade, bus volumes (Music, SFX, Ambient, Voice)
 *   - 3D positional audio with distance attenuation
 *   - Reverb zones (None, SmallRoom, Hall, Cave, Arena, Outdoor)
 *   - Sound pooling and channel management
 *   - Audio listener following the camera
 *   - Doppler effect with moving sound sources
 *   - Pitch / volume manipulation
 *   - Sound occlusion (behind walls)
 *   - Multiple audio emitter entities
 *   - EventBus integration for sound triggers
 *
 * Spark Engine systems used:
 *   Audio    — AudioEngine, MusicManager, AudioBus, ReverbZone
 *   ECS      — World, NameComponent, Transform, MeshRenderer, AudioEmitterComponent
 *   Events   — EventBus
 *   Input    — InputManager
 *   Rendering — LightComponent, Camera
 */

#include <Core/ModuleManager.h>
#include <Core/EngineContext.h>
#include <Engine/ECS/Components.h>
#include <Engine/Events/EventSystem.h>
#include <vector>

class AudioSandboxModule final : public Spark::IModule
{
public:
    Spark::ModuleInfo GetModuleInfo() const override
    {
        return { "AudioSandbox", "0.2.0", SPARK_SDK_VERSION, 1000 };
    }

    void OnLoad(Spark::IEngineContext* ctx) override;
    void OnUnload() override;
    void OnUpdate(float dt) override;
    void OnRender() override;
    void OnResize(int w, int h) override;

private:
    void BuildScene();
    void LoadAudioAssets();
    void SetupMusicPlaylist();
    void SetupEventSounds();
    void SpawnMovingSoundSource();
    void CycleReverbZone();
    void UpdateMovingSources(float dt);
    void UpdateListenerPosition();

    Spark::IEngineContext* m_ctx = nullptr;

    // Scene entities
    entt::entity m_floor      = entt::null;
    entt::entity m_light      = entt::null;
    entt::entity m_camera     = entt::null;
    entt::entity m_listener   = entt::null;  // visual marker for listener

    // Audio emitter entities (static)
    entt::entity m_ambientEmitter  = entt::null;
    entt::entity m_alarmEmitter    = entt::null;

    // Walls for occlusion demo
    std::vector<entt::entity> m_walls;

    // Moving sound sources (Doppler demo)
    struct MovingSource {
        entt::entity entity;
        DirectX::XMFLOAT3 velocity;
        float lifetime;
    };
    std::vector<MovingSource> m_movingSources;

    // Event subscriptions
    Spark::SubscriptionID m_collisionSoundSub = 0;

    // State
    int   m_reverbIndex  = 0;
    float m_masterVolume = 1.0f;
    bool  m_musicPlaying = false;

    // Listener position (WASD controlled)
    float m_listenerX = 0.0f;
    float m_listenerZ = 0.0f;

    static constexpr float kListenerSpeed = 8.0f;
};
