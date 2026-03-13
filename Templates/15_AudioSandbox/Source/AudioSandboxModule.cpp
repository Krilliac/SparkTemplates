/**
 * @file AudioSandboxModule.cpp
 * @brief INTERMEDIATE — Complete audio system demonstration.
 */

#include "AudioSandboxModule.h"
#include <Audio/AudioEngine.h>
#include <Audio/MusicManager.h>
#include <Input/InputManager.h>
#include <cmath>

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void AudioSandboxModule::OnLoad(Spark::IEngineContext* ctx)
{
    m_ctx = ctx;
    BuildScene();
    LoadAudioAssets();
    SetupMusicPlaylist();
    SetupEventSounds();
}

void AudioSandboxModule::OnUnload()
{
    // Unsubscribe events
    if (m_collisionSoundSub)
    {
        auto* bus = m_ctx ? m_ctx->GetEventBus() : nullptr;
        if (bus) bus->Unsubscribe(m_collisionSoundSub);
        m_collisionSoundSub = 0;
    }

    // Stop music
    auto* music = Spark::MusicManager::GetInstance();
    if (music) music->StopAll();

    auto* world = m_ctx ? m_ctx->GetWorld() : nullptr;
    if (!world) { m_ctx = nullptr; return; }

    for (auto e : { m_floor, m_light, m_camera, m_listener,
                    m_ambientEmitter, m_alarmEmitter })
        if (e != entt::null) world->DestroyEntity(e);

    for (auto e : m_walls)
        if (e != entt::null) world->DestroyEntity(e);
    m_walls.clear();

    for (auto& ms : m_movingSources)
        if (ms.entity != entt::null) world->DestroyEntity(ms.entity);
    m_movingSources.clear();

    m_ctx = nullptr;
}

// ---------------------------------------------------------------------------
// Scene construction
// ---------------------------------------------------------------------------

void AudioSandboxModule::BuildScene()
{
    auto* world = m_ctx->GetWorld();

    // Floor
    m_floor = world->CreateEntity();
    world->AddComponent<NameComponent>(m_floor, NameComponent{ "Floor" });
    world->AddComponent<Transform>(m_floor, Transform{
        { 0.f, -0.5f, 0.f }, { 0.f, 0.f, 0.f }, { 60.f, 1.f, 60.f }
    });
    world->AddComponent<MeshRenderer>(m_floor, MeshRenderer{
        "Assets/Models/cube.obj", "Assets/Materials/stone.mat",
        false, true, true
    });

    // Directional light
    m_light = world->CreateEntity();
    world->AddComponent<NameComponent>(m_light, NameComponent{ "SunLight" });
    world->AddComponent<Transform>(m_light, Transform{
        { 0.f, 20.f, 0.f }, { -45.f, 30.f, 0.f }, { 1.f, 1.f, 1.f }
    });
    world->AddComponent<LightComponent>(m_light, LightComponent{
        LightComponent::Type::Directional,
        { 1.f, 0.95f, 0.85f }, 1.8f, 0.f,
        0.f, 0.f, true, 2048
    });

    // Camera (top-down-ish view)
    m_camera = world->CreateEntity();
    world->AddComponent<NameComponent>(m_camera, NameComponent{ "AudioCamera" });
    world->AddComponent<Transform>(m_camera, Transform{
        { 0.f, 25.f, -15.f }, { -55.f, 0.f, 0.f }, { 1.f, 1.f, 1.f }
    });
    world->AddComponent<Camera>(m_camera, Camera{
        60.0f, 0.1f, 500.0f, true
    });

    // Listener marker (visual sphere showing where the audio listener is)
    m_listener = world->CreateEntity();
    world->AddComponent<NameComponent>(m_listener, NameComponent{ "ListenerMarker" });
    world->AddComponent<Transform>(m_listener, Transform{
        { 0.f, 0.5f, 0.f }, { 0.f, 0.f, 0.f }, { 0.8f, 0.8f, 0.8f }
    });
    world->AddComponent<MeshRenderer>(m_listener, MeshRenderer{
        "Assets/Models/sphere.obj", "Assets/Materials/emissive_blue.mat",
        false, false, true
    });

    // Walls for occlusion demo
    auto spawnWall = [&](const char* name, DirectX::XMFLOAT3 pos,
                         DirectX::XMFLOAT3 rot, DirectX::XMFLOAT3 scale) {
        auto e = world->CreateEntity();
        world->AddComponent<NameComponent>(e, NameComponent{ name });
        world->AddComponent<Transform>(e, Transform{ pos, rot, scale });
        world->AddComponent<MeshRenderer>(e, MeshRenderer{
            "Assets/Models/cube.obj", "Assets/Materials/concrete.mat",
            true, true, true
        });
        m_walls.push_back(e);
        return e;
    };

    spawnWall("Wall_Left",  { -8.f, 2.f, 0.f },  { 0.f, 0.f, 0.f }, { 0.5f, 4.f, 12.f });
    spawnWall("Wall_Right", { 8.f, 2.f, 0.f },   { 0.f, 0.f, 0.f }, { 0.5f, 4.f, 12.f });
    spawnWall("Wall_Back",  { 0.f, 2.f, 10.f },  { 0.f, 0.f, 0.f }, { 16.f, 4.f, 0.5f });

    // Static audio emitter: ambient wind (behind a wall)
    m_ambientEmitter = world->CreateEntity();
    world->AddComponent<NameComponent>(m_ambientEmitter, NameComponent{ "AmbientWind" });
    world->AddComponent<Transform>(m_ambientEmitter, Transform{
        { 0.f, 1.f, 15.f }, { 0.f, 0.f, 0.f }, { 0.5f, 0.5f, 0.5f }
    });
    world->AddComponent<MeshRenderer>(m_ambientEmitter, MeshRenderer{
        "Assets/Models/sphere.obj", "Assets/Materials/emissive_green.mat",
        false, false, true
    });
    world->AddComponent<AudioEmitterComponent>(m_ambientEmitter, AudioEmitterComponent{
        "wind_loop",           // soundName
        true,                  // loop
        true,                  // is3D
        1.0f,                  // volume
        1.0f,                  // pitch
        1.0f,                  // minDistance
        30.0f,                 // maxDistance
        true                   // autoPlay
    });

    // Static audio emitter: alarm siren (on the right side)
    m_alarmEmitter = world->CreateEntity();
    world->AddComponent<NameComponent>(m_alarmEmitter, NameComponent{ "AlarmSiren" });
    world->AddComponent<Transform>(m_alarmEmitter, Transform{
        { 12.f, 2.f, 5.f }, { 0.f, 0.f, 0.f }, { 0.5f, 0.5f, 0.5f }
    });
    world->AddComponent<MeshRenderer>(m_alarmEmitter, MeshRenderer{
        "Assets/Models/sphere.obj", "Assets/Materials/emissive_red.mat",
        false, false, true
    });
    world->AddComponent<AudioEmitterComponent>(m_alarmEmitter, AudioEmitterComponent{
        "alarm_loop", true, true, 0.6f, 1.0f, 2.0f, 25.0f, true
    });
}

// ---------------------------------------------------------------------------
// Audio asset loading
// ---------------------------------------------------------------------------

void AudioSandboxModule::LoadAudioAssets()
{
    auto* audio = m_ctx->GetAudio();
    if (!audio) return;

    // One-shot sounds
    audio->LoadSound("gunshot",   "Assets/Audio/gunshot.wav");
    audio->LoadSound("explosion", "Assets/Audio/explosion.wav");
    audio->LoadSound("bell",      "Assets/Audio/bell.wav");
    audio->LoadSound("alarm",     "Assets/Audio/alarm.wav");
    audio->LoadSound("footstep",  "Assets/Audio/footstep.wav");

    // Looping ambient sounds
    audio->LoadSound("wind_loop",  "Assets/Audio/wind_loop.wav");
    audio->LoadSound("alarm_loop", "Assets/Audio/alarm_loop.wav");

    // Doppler demo sound
    audio->LoadSound("engine_loop", "Assets/Audio/engine_loop.wav");

    // Collision impact
    audio->LoadSound("impact", "Assets/Audio/impact.wav");
}

// ---------------------------------------------------------------------------
// Music playlist
// ---------------------------------------------------------------------------

void AudioSandboxModule::SetupMusicPlaylist()
{
    auto* music = Spark::MusicManager::GetInstance();
    if (!music) return;

    // Configure audio buses
    music->SetBusVolume(Spark::AudioBus::Music,   0.5f);
    music->SetBusVolume(Spark::AudioBus::SFX,     1.0f);
    music->SetBusVolume(Spark::AudioBus::Ambient,  0.6f);
    music->SetBusVolume(Spark::AudioBus::Voice,    1.0f);

    // Create a playlist
    music->CreatePlaylist("BackgroundMusic");
    music->AddToPlaylist("BackgroundMusic", "Assets/Music/track01.ogg");
    music->AddToPlaylist("BackgroundMusic", "Assets/Music/track02.ogg");
    music->AddToPlaylist("BackgroundMusic", "Assets/Music/track03.ogg");

    // Set crossfade duration between tracks
    music->SetCrossfadeDuration(2.0f);

    // Set initial reverb zone
    music->SetReverbZone(Spark::ReverbZone::None);
}

// ---------------------------------------------------------------------------
// EventBus sound triggers
// ---------------------------------------------------------------------------

void AudioSandboxModule::SetupEventSounds()
{
    auto* bus = m_ctx->GetEventBus();
    if (!bus) return;

    // Play impact sound whenever a collision event fires
    m_collisionSoundSub = bus->Subscribe<Spark::CollisionEvent>(
        [this](const Spark::CollisionEvent& evt)
        {
            auto* audio = m_ctx->GetAudio();
            if (!audio) return;

            // Play 3D impact at collision point
            audio->Play3DSound("impact", evt.contactPoint, 0.8f);
        });
}

// ---------------------------------------------------------------------------
// Moving sound source (Doppler)
// ---------------------------------------------------------------------------

void AudioSandboxModule::SpawnMovingSoundSource()
{
    auto* world = m_ctx->GetWorld();
    auto* audio = m_ctx->GetAudio();
    if (!world || !audio) return;

    // Spawn at left side, moving right (or vice versa)
    bool fromLeft = (m_movingSources.size() % 2 == 0);
    float startX = fromLeft ? -25.f : 25.f;
    float velX   = fromLeft ? 15.f : -15.f;

    auto e = world->CreateEntity();
    world->AddComponent<NameComponent>(e, NameComponent{ "DopplerSource" });
    world->AddComponent<Transform>(e, Transform{
        { startX, 1.f, m_listenerZ }, { 0.f, 0.f, 0.f }, { 0.6f, 0.6f, 0.6f }
    });
    world->AddComponent<MeshRenderer>(e, MeshRenderer{
        "Assets/Models/sphere.obj", "Assets/Materials/emissive_orange.mat",
        false, false, true
    });

    // 3D audio emitter with Doppler enabled
    world->AddComponent<AudioEmitterComponent>(e, AudioEmitterComponent{
        "engine_loop",  // soundName
        true,           // loop
        true,           // is3D
        1.0f,           // volume
        1.0f,           // pitch
        1.0f,           // minDistance
        40.0f,          // maxDistance
        true            // autoPlay
    });

    // Set Doppler factor on the emitter
    auto& emitter = world->GetComponent<AudioEmitterComponent>(e);
    emitter.dopplerFactor = 2.0f;  // exaggerated for demo
    emitter.velocity = { velX, 0.f, 0.f };

    m_movingSources.push_back({ e, { velX, 0.f, 0.f }, 0.0f });
}

// ---------------------------------------------------------------------------
// Reverb zone cycling
// ---------------------------------------------------------------------------

void AudioSandboxModule::CycleReverbZone()
{
    auto* music = Spark::MusicManager::GetInstance();
    if (!music) return;

    m_reverbIndex = (m_reverbIndex + 1) % 6;

    Spark::ReverbZone zones[] = {
        Spark::ReverbZone::None,
        Spark::ReverbZone::SmallRoom,
        Spark::ReverbZone::Hall,
        Spark::ReverbZone::Cave,
        Spark::ReverbZone::Arena,
        Spark::ReverbZone::Outdoor
    };

    music->SetReverbZone(zones[m_reverbIndex]);
}

// ---------------------------------------------------------------------------
// Update moving sources
// ---------------------------------------------------------------------------

void AudioSandboxModule::UpdateMovingSources(float dt)
{
    auto* world = m_ctx->GetWorld();
    if (!world) return;

    for (auto it = m_movingSources.begin(); it != m_movingSources.end(); )
    {
        it->lifetime += dt;

        if (it->lifetime > 5.0f || it->entity == entt::null)
        {
            // Remove expired sources
            if (it->entity != entt::null)
                world->DestroyEntity(it->entity);
            it = m_movingSources.erase(it);
            continue;
        }

        // Move the entity
        auto& xf = world->GetComponent<Transform>(it->entity);
        xf.position.x += it->velocity.x * dt;
        xf.position.y += it->velocity.y * dt;
        xf.position.z += it->velocity.z * dt;

        // Update emitter velocity for Doppler calculation
        if (world->HasComponent<AudioEmitterComponent>(it->entity))
        {
            auto& emitter = world->GetComponent<AudioEmitterComponent>(it->entity);
            emitter.velocity = it->velocity;
        }

        ++it;
    }
}

// ---------------------------------------------------------------------------
// Listener position update
// ---------------------------------------------------------------------------

void AudioSandboxModule::UpdateListenerPosition()
{
    auto* audio = m_ctx->GetAudio();
    auto* world = m_ctx->GetWorld();
    if (!audio || !world) return;

    // Update the audio engine listener position
    DirectX::XMFLOAT3 listenerPos = { m_listenerX, 1.0f, m_listenerZ };
    DirectX::XMFLOAT3 listenerFwd = { 0.f, 0.f, 1.f };
    DirectX::XMFLOAT3 listenerUp  = { 0.f, 1.f, 0.f };
    audio->SetListenerPosition(listenerPos, listenerFwd, listenerUp);

    // Update visual marker
    if (m_listener != entt::null)
    {
        auto& xf = world->GetComponent<Transform>(m_listener);
        xf.position = { m_listenerX, 0.5f, m_listenerZ };
    }

    // Update sound occlusion for emitters behind walls
    // The engine raycasts from listener to each emitter and applies attenuation
    if (world->HasComponent<AudioEmitterComponent>(m_ambientEmitter))
    {
        auto& emitterXf = world->GetComponent<Transform>(m_ambientEmitter);
        float occlusionFactor = audio->CalculateOcclusion(listenerPos, emitterXf.position);
        auto& emitter = world->GetComponent<AudioEmitterComponent>(m_ambientEmitter);
        emitter.occlusionFactor = occlusionFactor;
    }

    if (world->HasComponent<AudioEmitterComponent>(m_alarmEmitter))
    {
        auto& emitterXf = world->GetComponent<Transform>(m_alarmEmitter);
        float occlusionFactor = audio->CalculateOcclusion(listenerPos, emitterXf.position);
        auto& emitter = world->GetComponent<AudioEmitterComponent>(m_alarmEmitter);
        emitter.occlusionFactor = occlusionFactor;
    }
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

void AudioSandboxModule::OnUpdate(float dt)
{
    auto* input = m_ctx->GetInput();
    auto* audio = m_ctx->GetAudio();
    if (!input) return;

    // --- WASD move listener ------------------------------------------------
    if (input->IsKeyDown('W')) m_listenerZ += kListenerSpeed * dt;
    if (input->IsKeyDown('S')) m_listenerZ -= kListenerSpeed * dt;
    if (input->IsKeyDown('A')) m_listenerX -= kListenerSpeed * dt;
    if (input->IsKeyDown('D')) m_listenerX += kListenerSpeed * dt;

    // --- 1-5: Play different one-shot sounds at listener position ----------
    if (audio)
    {
        DirectX::XMFLOAT3 pos = { m_listenerX, 1.f, m_listenerZ };

        if (input->WasKeyPressed('1'))
            audio->Play3DSound("gunshot", pos, 1.0f);

        if (input->WasKeyPressed('2'))
            audio->Play3DSound("explosion", pos, 1.0f);

        if (input->WasKeyPressed('3'))
            audio->Play3DSound("bell", pos, 1.0f);

        if (input->WasKeyPressed('4'))
            audio->Play3DSound("alarm", pos, 0.8f);

        if (input->WasKeyPressed('5'))
            audio->Play3DSound("footstep", pos, 1.0f);
    }

    // --- M: Toggle music ---------------------------------------------------
    if (input->WasKeyPressed('M'))
    {
        auto* music = Spark::MusicManager::GetInstance();
        if (music)
        {
            m_musicPlaying = !m_musicPlaying;
            if (m_musicPlaying)
                music->PlayPlaylist("BackgroundMusic", true); // shuffle
            else
                music->StopAll();
        }
    }

    // --- R: Cycle reverb zones ---------------------------------------------
    if (input->WasKeyPressed('R'))
        CycleReverbZone();

    // --- +/-: Master volume ------------------------------------------------
    if (input->WasKeyPressed(VK_OEM_PLUS) || input->WasKeyPressed(VK_ADD))
    {
        m_masterVolume = std::min(1.0f, m_masterVolume + 0.1f);
        if (audio) audio->SetMasterVolume(m_masterVolume);
    }
    if (input->WasKeyPressed(VK_OEM_MINUS) || input->WasKeyPressed(VK_SUBTRACT))
    {
        m_masterVolume = std::max(0.0f, m_masterVolume - 0.1f);
        if (audio) audio->SetMasterVolume(m_masterVolume);
    }

    // --- Space: Spawn moving sound source (Doppler) ------------------------
    if (input->WasKeyPressed(VK_SPACE))
        SpawnMovingSoundSource();

    // Update systems
    UpdateMovingSources(dt);
    UpdateListenerPosition();
}

// ---------------------------------------------------------------------------
void AudioSandboxModule::OnRender() { /* RenderSystem auto-draws */ }
void AudioSandboxModule::OnResize(int, int) { }

// ---------------------------------------------------------------------------
// DLL exports
// ---------------------------------------------------------------------------

SPARK_IMPLEMENT_MODULE(AudioSandboxModule)
