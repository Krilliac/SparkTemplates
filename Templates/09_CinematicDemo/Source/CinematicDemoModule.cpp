/**
 * @file CinematicDemoModule.cpp
 * @brief INTERMEDIATE — Cinematic sequencer, camera paths, audio cues, and coroutines.
 */

#include "CinematicDemoModule.h"
#include <Audio/AudioEngine.h>
#include <Audio/MusicManager.h>
#include <Input/InputManager.h>

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void CinematicDemoModule::OnLoad(Spark::IEngineContext* ctx)
{
    m_ctx = ctx;
    BuildScene();
    SetupAudio();
    CreateIntroCinematic();
    CreateFlyoverCinematic();
}

void CinematicDemoModule::OnUnload()
{
    // Stop all sequences and coroutines
    auto* seq = Spark::SequencerManager::GetInstance();
    if (seq) seq->StopAll();

    auto* coro = Spark::CoroutineScheduler::GetInstance();
    if (coro) coro->StopAll();

    auto* world = m_ctx ? m_ctx->GetWorld() : nullptr;
    if (!world) { m_ctx = nullptr; return; }

    for (auto e : { m_floor, m_light, m_camera })
        if (e != entt::null) world->DestroyEntity(e);

    for (auto e : m_props)
        if (e != entt::null) world->DestroyEntity(e);
    m_props.clear();

    m_ctx = nullptr;
}

// ---------------------------------------------------------------------------
// Scene construction
// ---------------------------------------------------------------------------

void CinematicDemoModule::BuildScene()
{
    auto* world = m_ctx->GetWorld();

    // Floor
    m_floor = world->CreateEntity();
    world->AddComponent<NameComponent>(m_floor, NameComponent{ "CinemaFloor" });
    world->AddComponent<Transform>(m_floor, Transform{
        { 0.f, -0.5f, 0.f }, { 0.f, 0.f, 0.f }, { 80.f, 1.f, 80.f }
    });
    world->AddComponent<MeshRenderer>(m_floor, MeshRenderer{
        "Assets/Models/cube.obj", "Assets/Materials/stone.mat",
        true, true, true
    });

    // Main light
    m_light = world->CreateEntity();
    world->AddComponent<NameComponent>(m_light, NameComponent{ "CinemaLight" });
    world->AddComponent<Transform>(m_light, Transform{
        { 0.f, 20.f, 0.f }, { -45.f, 30.f, 0.f }, { 1.f, 1.f, 1.f }
    });
    world->AddComponent<LightComponent>(m_light, LightComponent{
        LightComponent::Type::Directional,
        { 1.f, 0.9f, 0.75f }, 2.0f, 0.f,
        0.f, 0.f, true, 2048
    });

    // Camera entity for cinematic control
    m_camera = world->CreateEntity();
    world->AddComponent<NameComponent>(m_camera, NameComponent{ "CinemaCamera" });
    world->AddComponent<Transform>(m_camera, Transform{
        { 0.f, 5.f, -20.f }, { -10.f, 0.f, 0.f }, { 1.f, 1.f, 1.f }
    });
    world->AddComponent<Camera>(m_camera, Camera{
        60.0f, 0.1f, 1000.0f, true
    });

    // Props — buildings and structures for the camera to fly around
    auto spawnProp = [&](const char* name, const char* mesh,
                          DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 scale) {
        auto e = world->CreateEntity();
        world->AddComponent<NameComponent>(e, NameComponent{ name });
        world->AddComponent<Transform>(e, Transform{
            pos, { 0.f, 0.f, 0.f }, scale
        });
        world->AddComponent<MeshRenderer>(e, MeshRenderer{
            mesh, "Assets/Materials/concrete.mat", true, true, true
        });
        m_props.push_back(e);
        return e;
    };

    spawnProp("Tower",    "Assets/Models/watchtower.obj", { -10.f, 0.f, 10.f }, { 2.f, 2.f, 2.f });
    spawnProp("Building", "Assets/Models/building_small.obj", { 15.f, 0.f, 5.f }, { 1.5f, 1.5f, 1.5f });
    spawnProp("Barrier1", "Assets/Models/barrier.obj", { 5.f, 0.f, -5.f }, { 1.f, 1.f, 1.f });
    spawnProp("Barrier2", "Assets/Models/barrier.obj", { -5.f, 0.f, -8.f }, { 1.f, 1.f, 1.f });
    spawnProp("Turret",   "Assets/Models/turret.obj", { 0.f, 0.f, 15.f }, { 1.f, 1.f, 1.f });

    // Spotlight for dramatic effect
    auto spotlight = world->CreateEntity();
    world->AddComponent<NameComponent>(spotlight, NameComponent{ "Spotlight" });
    world->AddComponent<Transform>(spotlight, Transform{
        { 0.f, 10.f, -5.f }, { -80.f, 0.f, 0.f }, { 1.f, 1.f, 1.f }
    });
    world->AddComponent<LightComponent>(spotlight, LightComponent{
        LightComponent::Type::Spot,
        { 1.f, 0.8f, 0.5f }, 5.0f, 25.0f,
        35.0f, 25.0f, true, 1024
    });
    m_props.push_back(spotlight);
}

// ---------------------------------------------------------------------------
// Audio setup
// ---------------------------------------------------------------------------

void CinematicDemoModule::SetupAudio()
{
    auto* audio = m_ctx->GetAudio();
    if (!audio) return;

    // Load sound effects for cinematic cues
    audio->LoadSound("whoosh", "Assets/Audio/whoosh.wav");
    audio->LoadSound("impact", "Assets/Audio/impact.wav");
    audio->LoadSound("reveal", "Assets/Audio/reveal.wav");

    // Set up music via MusicManager
    auto* music = Spark::MusicManager::GetInstance();
    if (!music) return;

    // Configure audio buses
    music->SetBusVolume(Spark::AudioBus::Music, 0.7f);
    music->SetBusVolume(Spark::AudioBus::SFX, 1.0f);
    music->SetBusVolume(Spark::AudioBus::Ambient, 0.5f);

    // Set reverb zone for cinematic feel
    music->SetReverbZone(Spark::ReverbZone::Hall);
}

// ---------------------------------------------------------------------------
// Cinematic sequences
// ---------------------------------------------------------------------------

void CinematicDemoModule::CreateIntroCinematic()
{
    auto* seq = Spark::SequencerManager::GetInstance();
    if (!seq) return;

    m_introSequenceID = seq->CreateSequence("IntroCinematic");

    // Camera path — sweep from behind the tower, around the scene, ending at player view
    seq->AddCameraPathKey(m_introSequenceID, 0.0f,
        { -15.f, 8.f, 15.f }, { -10.f, 150.f, 0.f }, 70.0f, 0.0f,
        Spark::Interpolation::CatmullRom);

    seq->AddCameraPathKey(m_introSequenceID, 3.0f,
        { 15.f, 12.f, 10.f }, { -20.f, -30.f, 0.f }, 60.0f, 0.0f,
        Spark::Interpolation::CatmullRom);

    seq->AddCameraPathKey(m_introSequenceID, 6.0f,
        { 10.f, 5.f, -15.f }, { -5.f, -60.f, 0.f }, 55.0f, 0.0f,
        Spark::Interpolation::CatmullRom);

    seq->AddCameraPathKey(m_introSequenceID, 9.0f,
        { 0.f, 5.f, -20.f }, { -10.f, 0.f, 0.f }, 60.0f, 0.0f,
        Spark::Interpolation::CubicBezier);

    // Subtitles
    seq->AddSubtitleKey(m_introSequenceID, 0.5f, "Welcome to the arena...", 2.5f);
    seq->AddSubtitleKey(m_introSequenceID, 4.0f, "Where legends are forged.", 2.0f);
    seq->AddSubtitleKey(m_introSequenceID, 7.0f, "Prepare for battle.", 2.0f);

    // Audio cues
    seq->AddAudioCueKey(m_introSequenceID, 0.0f, "whoosh");
    seq->AddAudioCueKey(m_introSequenceID, 3.0f, "whoosh");
    seq->AddAudioCueKey(m_introSequenceID, 8.0f, "reveal");

    // Fade in at start
    seq->AddFadeKey(m_introSequenceID, 0.0f, 1.0f); // start black
    seq->AddFadeKey(m_introSequenceID, 1.0f, 0.0f); // fade to clear

    // Event callback at the end
    seq->AddEventKey(m_introSequenceID, 9.0f, "intro_complete");
}

void CinematicDemoModule::CreateFlyoverCinematic()
{
    auto* seq = Spark::SequencerManager::GetInstance();
    if (!seq) return;

    m_flyoverSequenceID = seq->CreateSequence("FlyoverCinematic");

    // High altitude sweeping camera path
    seq->AddCameraPathKey(m_flyoverSequenceID, 0.0f,
        { -30.f, 25.f, -30.f }, { -30.f, 45.f, 0.f }, 75.0f, 0.0f,
        Spark::Interpolation::CatmullRom);

    seq->AddCameraPathKey(m_flyoverSequenceID, 4.0f,
        { 30.f, 30.f, 0.f }, { -35.f, -20.f, 5.f }, 65.0f, 5.0f,
        Spark::Interpolation::CatmullRom);

    seq->AddCameraPathKey(m_flyoverSequenceID, 8.0f,
        { 0.f, 20.f, 30.f }, { -25.f, 180.f, 0.f }, 60.0f, -5.0f,
        Spark::Interpolation::CatmullRom);

    seq->AddCameraPathKey(m_flyoverSequenceID, 12.0f,
        { -30.f, 25.f, -30.f }, { -30.f, 45.f, 0.f }, 75.0f, 0.0f,
        Spark::Interpolation::CatmullRom);

    // Entity animation — rotate the turret during flyover
    seq->AddEntityTransformKey(m_flyoverSequenceID, 0.0f,
        m_props[4], // turret entity
        { 0.f, 0.f, 15.f }, { 0.f, 0.f, 0.f }, { 1.f, 1.f, 1.f },
        Spark::Interpolation::Linear);

    seq->AddEntityTransformKey(m_flyoverSequenceID, 12.0f,
        m_props[4],
        { 0.f, 0.f, 15.f }, { 0.f, 360.f, 0.f }, { 1.f, 1.f, 1.f },
        Spark::Interpolation::Linear);
}

// ---------------------------------------------------------------------------
// Coroutine demo
// ---------------------------------------------------------------------------

void CinematicDemoModule::StartCoroutineDemo()
{
    auto* coro = Spark::CoroutineScheduler::GetInstance();
    auto* audio = m_ctx->GetAudio();
    if (!coro || !audio) return;

    // Chain of timed actions using the coroutine builder pattern
    coro->StartCoroutine("DemoSequence")
        .Do([audio]() {
            audio->PlaySound("whoosh");
        })
        .WaitForSeconds(1.5f)
        .Do([audio]() {
            audio->PlaySound("impact");
        })
        .WaitForSeconds(2.0f)
        .Do([audio]() {
            audio->PlaySound("reveal");
        })
        .WaitForSeconds(1.0f)
        .Do([]() {
            // Coroutine complete — could trigger next gameplay phase
        });
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

void CinematicDemoModule::OnUpdate(float dt)
{
    auto* input = m_ctx->GetInput();
    if (!input) return;

    // Update sequencer
    auto* seq = Spark::SequencerManager::GetInstance();
    if (seq) seq->Update(dt);

    // Update coroutines
    auto* coro = Spark::CoroutineScheduler::GetInstance();
    if (coro) coro->Update(dt);

    // 1 — Play intro cinematic
    if (input->WasKeyPressed('1'))
    {
        if (seq)
        {
            seq->PlaySequence(m_introSequenceID);
            m_cinematicPlaying = true;
        }
    }

    // 2 — Play flyover cinematic
    if (input->WasKeyPressed('2'))
    {
        if (seq)
        {
            seq->PlaySequence(m_flyoverSequenceID);
            m_cinematicPlaying = true;
        }
    }

    // 3 — Start coroutine demo (timed sound sequence)
    if (input->WasKeyPressed('3'))
        StartCoroutineDemo();

    // Escape — Stop all cinematics
    if (input->WasKeyPressed(VK_ESCAPE))
    {
        if (seq) seq->StopAll();
        m_cinematicPlaying = false;
    }
}

// ---------------------------------------------------------------------------
void CinematicDemoModule::OnRender() { }
void CinematicDemoModule::OnResize(int, int) { }

SPARK_IMPLEMENT_MODULE(CinematicDemoModule)
