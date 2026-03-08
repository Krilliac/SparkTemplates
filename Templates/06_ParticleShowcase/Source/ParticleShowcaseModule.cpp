/**
 * @file ParticleShowcaseModule.cpp
 * @brief INTERMEDIATE — Visual effects with particles, decals, fog, and post-processing.
 */

#include "ParticleShowcaseModule.h"
#include <Graphics/GraphicsEngine.h>
#include <Graphics/PostProcessingSystem.h>
#include <Graphics/DecalSystem.h>
#include <Graphics/FogSystem.h>
#include <Input/InputManager.h>

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void ParticleShowcaseModule::OnLoad(Spark::IEngineContext* ctx)
{
    m_ctx = ctx;
    BuildScene();
    SetupPostProcessing();
}

void ParticleShowcaseModule::OnUnload()
{
    auto* world = m_ctx ? m_ctx->GetWorld() : nullptr;
    if (!world) { m_ctx = nullptr; return; }

    for (auto e : { m_floor, m_light })
        if (e != entt::null) world->DestroyEntity(e);

    for (auto e : m_emitters)
        if (e != entt::null) world->DestroyEntity(e);
    m_emitters.clear();

    m_ctx = nullptr;
}

// ---------------------------------------------------------------------------
// Scene construction
// ---------------------------------------------------------------------------

void ParticleShowcaseModule::BuildScene()
{
    auto* world = m_ctx->GetWorld();

    // Dark floor to contrast with bright particles
    m_floor = world->CreateEntity();
    world->AddComponent<NameComponent>(m_floor, NameComponent{ "Floor" });
    world->AddComponent<Transform>(m_floor, Transform{
        { 0.f, -0.5f, 0.f }, { 0.f, 0.f, 0.f }, { 40.f, 1.f, 40.f }
    });
    world->AddComponent<MeshRenderer>(m_floor, MeshRenderer{
        "Assets/Models/cube.obj", "Assets/Materials/dark_concrete.mat",
        true, true, true
    });

    // Soft directional light
    m_light = world->CreateEntity();
    world->AddComponent<NameComponent>(m_light, NameComponent{ "AmbientSun" });
    world->AddComponent<Transform>(m_light, Transform{
        { 0.f, 15.f, 0.f }, { -45.f, 20.f, 0.f }, { 1.f, 1.f, 1.f }
    });
    world->AddComponent<LightComponent>(m_light, LightComponent{
        LightComponent::Type::Directional,
        { 0.8f, 0.85f, 1.f }, 1.5f, 0.f,
        0.f, 0.f, true, 1024
    });

    // --- Particle emitter stations ---

    // Station 1: Fire — Point emitter, additive blending, upward gravity
    {
        auto e = world->CreateEntity();
        world->AddComponent<NameComponent>(e, NameComponent{ "Fire_Emitter" });
        world->AddComponent<Transform>(e, Transform{
            { -10.f, 0.5f, 0.f }, { 0.f, 0.f, 0.f }, { 1.f, 1.f, 1.f }
        });
        world->AddComponent<ParticleEmitterComponent>(e, ParticleEmitterComponent{
            "fire",                     // effectName
            true,                       // autoPlay
            50.0f,                      // emissionRate
            1.5f,                       // lifetime
            { 1.0f, 0.5f, 0.1f, 1.f }, // startColor (orange)
            0.3f,                       // startSize
            2.0f                        // startSpeed
        });
        m_emitters.push_back(e);
    }

    // Station 2: Smoke — Cone emitter, alpha blend, slow rise
    {
        auto e = world->CreateEntity();
        world->AddComponent<NameComponent>(e, NameComponent{ "Smoke_Emitter" });
        world->AddComponent<Transform>(e, Transform{
            { -5.f, 0.5f, 0.f }, { 0.f, 0.f, 0.f }, { 1.f, 1.f, 1.f }
        });
        world->AddComponent<ParticleEmitterComponent>(e, ParticleEmitterComponent{
            "smoke", true, 20.0f, 3.0f,
            { 0.4f, 0.4f, 0.4f, 0.6f }, // gray, semi-transparent
            0.5f, 1.0f
        });
        m_emitters.push_back(e);
    }

    // Station 3: Sparks — Sphere emitter, additive, high speed, gravity-affected
    {
        auto e = world->CreateEntity();
        world->AddComponent<NameComponent>(e, NameComponent{ "Sparks_Emitter" });
        world->AddComponent<Transform>(e, Transform{
            { 0.f, 2.f, 0.f }, { 0.f, 0.f, 0.f }, { 1.f, 1.f, 1.f }
        });
        world->AddComponent<ParticleEmitterComponent>(e, ParticleEmitterComponent{
            "sparks", true, 80.0f, 0.8f,
            { 1.0f, 0.9f, 0.3f, 1.f }, // bright yellow
            0.05f, 8.0f
        });
        m_emitters.push_back(e);
    }

    // Station 4: Magic orb — Circle emitter, premultiplied alpha
    {
        auto e = world->CreateEntity();
        world->AddComponent<NameComponent>(e, NameComponent{ "Magic_Emitter" });
        world->AddComponent<Transform>(e, Transform{
            { 5.f, 2.f, 0.f }, { 0.f, 0.f, 0.f }, { 1.f, 1.f, 1.f }
        });
        world->AddComponent<ParticleEmitterComponent>(e, ParticleEmitterComponent{
            "magic", true, 40.0f, 2.0f,
            { 0.3f, 0.5f, 1.0f, 0.8f }, // blue glow
            0.15f, 3.0f
        });
        m_emitters.push_back(e);
    }

    // Station 5: Rain — Box emitter, high emission, fast downward
    {
        auto e = world->CreateEntity();
        world->AddComponent<NameComponent>(e, NameComponent{ "Rain_Emitter" });
        world->AddComponent<Transform>(e, Transform{
            { 10.f, 12.f, 0.f }, { 0.f, 0.f, 0.f }, { 10.f, 1.f, 10.f }
        });
        world->AddComponent<ParticleEmitterComponent>(e, ParticleEmitterComponent{
            "rain", true, 200.0f, 1.2f,
            { 0.6f, 0.7f, 0.9f, 0.4f }, // light blue, semi-transparent
            0.02f, 12.0f
        });
        m_emitters.push_back(e);
    }

    // Set up initial fog
    if (auto* fog = m_ctx->GetGraphics()->GetFogSystem())
    {
        fog->SetMode(Spark::FogSystem::Mode::Height);
        fog->SetColor({ 0.5f, 0.55f, 0.65f });
        fog->SetDensity(0.02f);
        fog->SetHeightFalloff(0.1f);
    }

    // Place some decals on the floor
    if (auto* decals = m_ctx->GetGraphics()->GetDecalSystem())
    {
        decals->SpawnDecal(Spark::DecalSystem::DecalType::ScorchMark,
            { -10.f, 0.01f, 0.f }, { 0.f, 0.f, 0.f }, { 2.f, 2.f, 2.f });
        decals->SpawnDecal(Spark::DecalSystem::DecalType::ScorchMark,
            { 0.f, 0.01f, 2.f }, { 0.f, 45.f, 0.f }, { 1.5f, 1.5f, 1.5f });
        decals->SpawnDecal(Spark::DecalSystem::DecalType::Crack,
            { 5.f, 0.01f, -1.f }, { 0.f, 0.f, 0.f }, { 3.f, 3.f, 3.f });
    }
}

// ---------------------------------------------------------------------------
// Post-processing setup
// ---------------------------------------------------------------------------

void ParticleShowcaseModule::SetupPostProcessing()
{
    auto* gfx = m_ctx->GetGraphics();
    if (!gfx) return;

    auto* post = gfx->GetPostProcessingSystem();
    if (!post) return;

    // Enable bloom for glowing particles
    post->SetBloomEnabled(true);
    post->SetBloomThreshold(0.8f);
    post->SetBloomIntensity(1.2f);

    // Tone mapping for HDR
    post->SetToneMappingEnabled(true);

    // SSAO for depth
    gfx->SetSSAOSettings({ true, 0.5f, 1.0f, 0.03f });

    // FXAA
    post->SetFXAAEnabled(true);
}

// ---------------------------------------------------------------------------
// Fog cycling
// ---------------------------------------------------------------------------

void ParticleShowcaseModule::CycleWeatherFog()
{
    auto* fog = m_ctx->GetGraphics()->GetFogSystem();
    if (!fog) return;

    m_fogMode = (m_fogMode + 1) % 5;

    switch (m_fogMode)
    {
    case 0: // Height fog
        fog->SetMode(Spark::FogSystem::Mode::Height);
        fog->SetColor({ 0.5f, 0.55f, 0.65f });
        fog->SetDensity(0.02f);
        break;
    case 1: // Dense linear fog
        fog->SetMode(Spark::FogSystem::Mode::Linear);
        fog->SetColor({ 0.7f, 0.7f, 0.7f });
        fog->SetStart(5.0f);
        fog->SetEnd(30.0f);
        break;
    case 2: // Exponential fog
        fog->SetMode(Spark::FogSystem::Mode::Exponential);
        fog->SetColor({ 0.4f, 0.5f, 0.6f });
        fog->SetDensity(0.05f);
        break;
    case 3: // Volumetric fog
        fog->SetMode(Spark::FogSystem::Mode::Volumetric);
        fog->SetColor({ 0.3f, 0.35f, 0.4f });
        fog->SetDensity(0.03f);
        break;
    case 4: // No fog
        fog->SetMode(Spark::FogSystem::Mode::Linear);
        fog->SetStart(1000.0f);
        fog->SetEnd(1001.0f);
        break;
    }
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

void ParticleShowcaseModule::OnUpdate(float dt)
{
    auto* input = m_ctx->GetInput();
    if (!input) return;

    m_effectTimer += dt;

    // 1 — Trigger explosion preset at origin
    if (input->WasKeyPressed('1'))
    {
        if (auto* particles = m_ctx->GetGraphics()->GetParticleSystem())
            particles->SpawnExplosion({ 0.f, 1.f, 5.f });
    }

    // 2 — Trigger muzzle flash
    if (input->WasKeyPressed('2'))
    {
        if (auto* particles = m_ctx->GetGraphics()->GetParticleSystem())
            particles->SpawnMuzzleFlash({ 0.f, 1.5f, 5.f });
    }

    // 3 — Spawn smoke puff
    if (input->WasKeyPressed('3'))
    {
        if (auto* particles = m_ctx->GetGraphics()->GetParticleSystem())
            particles->SpawnSmoke({ 0.f, 0.5f, 5.f });
    }

    // 4 — Spawn sparks
    if (input->WasKeyPressed('4'))
    {
        if (auto* particles = m_ctx->GetGraphics()->GetParticleSystem())
            particles->SpawnSparks({ 0.f, 1.f, 5.f });
    }

    // F — Cycle fog modes
    if (input->WasKeyPressed('F'))
        CycleWeatherFog();

    // B — Toggle bloom
    if (input->WasKeyPressed('B'))
    {
        if (auto* post = m_ctx->GetGraphics()->GetPostProcessingSystem())
        {
            bool enabled = !post->IsBloomEnabled();
            post->SetBloomEnabled(enabled);
        }
    }

    // Spawn bullet-hole decals periodically for visual variety
    if (m_effectTimer > 3.0f)
    {
        m_effectTimer = 0.0f;
        if (auto* decals = m_ctx->GetGraphics()->GetDecalSystem())
        {
            float rx = (static_cast<float>(rand() % 200) - 100.f) * 0.1f;
            float rz = (static_cast<float>(rand() % 200) - 100.f) * 0.1f;
            decals->SpawnDecal(Spark::DecalSystem::DecalType::BulletHole,
                { rx, 0.01f, rz }, { 0.f, 0.f, 0.f }, { 0.2f, 0.2f, 0.2f });
        }
    }
}

// ---------------------------------------------------------------------------
void ParticleShowcaseModule::OnRender() { }
void ParticleShowcaseModule::OnResize(int, int) { }

SPARK_IMPLEMENT_MODULE(ParticleShowcaseModule)
