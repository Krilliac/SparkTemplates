/**
 * @file AmbientSceneModule.cpp
 * @brief BASIC — Atmospheric scene with day/night cycle, weather, and lighting.
 */

#include "AmbientSceneModule.h"
#include <Input/InputManager.h>
#include <cmath>

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void AmbientSceneModule::OnLoad(Spark::IEngineContext* ctx)
{
    m_ctx = ctx;

    // Initialise the day/night cycle starting at 6 AM (sunrise)
    m_dayNight.SetTime(6.0f);
    m_dayNight.SetTimeScale(m_timeScale);

    // Hook up to the engine event bus if available
    if (auto* bus = m_ctx->GetEventBus())
    {
        m_dayNight.SetEventBus(bus);

        m_timeSub = bus->Subscribe<Spark::TimeOfDayChangedEvent>(
            [](const Spark::TimeOfDayChangedEvent& /*e*/) {
                // Could adjust ambient music, toggle streetlights, etc.
            });

        m_weatherSub = bus->Subscribe<Spark::WeatherChangedEvent>(
            [](const Spark::WeatherChangedEvent& /*e*/) {
                // Could trigger particle rain/snow, muffle audio, etc.
            });
    }

    BuildScene();
    SetupLighting();
}

void AmbientSceneModule::OnUnload()
{
    if (auto* bus = m_ctx ? m_ctx->GetEventBus() : nullptr)
    {
        bus->Unsubscribe<Spark::TimeOfDayChangedEvent>(m_timeSub);
        bus->Unsubscribe<Spark::WeatherChangedEvent>(m_weatherSub);
    }

    auto* world = m_ctx ? m_ctx->GetWorld() : nullptr;
    if (world)
    {
        for (auto e : { m_ground, m_sun, m_lanternA, m_lanternB,
                        m_spotlight, m_weatherEnt, m_pillar })
        {
            if (e != entt::null) world->DestroyEntity(e);
        }
    }
    m_ctx = nullptr;
}

// ---------------------------------------------------------------------------
// Scene construction
// ---------------------------------------------------------------------------

void AmbientSceneModule::BuildScene()
{
    auto* world = m_ctx->GetWorld();

    // Ground plane
    m_ground = world->CreateEntity();
    world->AddComponent<NameComponent>(m_ground, NameComponent{ "Ground" });
    world->AddComponent<Transform>(m_ground, Transform{
        { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 50.0f, 1.0f, 50.0f }
    });
    world->AddComponent<MeshRenderer>(m_ground, MeshRenderer{
        "Assets/Models/plane.obj", "Assets/Materials/grass.mat",
        false, true, true
    });

    // Decorative pillar
    m_pillar = world->CreateEntity();
    world->AddComponent<NameComponent>(m_pillar, NameComponent{ "Pillar" });
    world->AddComponent<Transform>(m_pillar, Transform{
        { 0.0f, 2.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.5f, 4.0f, 0.5f }
    });
    world->AddComponent<MeshRenderer>(m_pillar, MeshRenderer{
        "Assets/Models/cube.obj", "Assets/Materials/stone.mat",
        true, true, true
    });

    // Weather entity (data-only, no mesh)
    m_weatherEnt = world->CreateEntity();
    world->AddComponent<NameComponent>(m_weatherEnt, NameComponent{ "Weather" });
    world->AddComponent<WeatherComponent>(m_weatherEnt, WeatherComponent{
        0,           // weatherType: 0 = clear
        0.0f,        // intensity
        1.0f, 0.0f, 0.0f,  // windX, windY, windZ
        2.0f,        // windSpeed
        3.0f,        // transitionTime
        true         // enabled
    });
}

void AmbientSceneModule::SetupLighting()
{
    auto* world = m_ctx->GetWorld();

    // Directional "sun" light — the day/night cycle drives its colour/intensity
    m_sun = world->CreateEntity();
    world->AddComponent<NameComponent>(m_sun, NameComponent{ "Sun" });
    world->AddComponent<Transform>(m_sun, Transform{
        { 0.0f, 50.0f, 0.0f }, { -45.0f, 30.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }
    });
    world->AddComponent<LightComponent>(m_sun, LightComponent{
        LightComponent::Type::Directional,
        { 1.0f, 0.95f, 0.8f }, 1.5f, 0.0f,
        0.0f, 0.0f, true, 2048
    });

    // Two point "lantern" lights on either side of the pillar
    auto makeLantern = [&](const char* name, float x, float z) -> entt::entity {
        auto e = world->CreateEntity();
        world->AddComponent<NameComponent>(e, NameComponent{ name });
        world->AddComponent<Transform>(e, Transform{
            { x, 3.0f, z }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }
        });
        world->AddComponent<LightComponent>(e, LightComponent{
            LightComponent::Type::Point,
            { 1.0f, 0.6f, 0.2f }, // warm orange
            1.0f, 8.0f,
            0.0f, 0.0f, false, 512
        });
        return e;
    };
    m_lanternA = makeLantern("LanternLeft",  -3.0f, 0.0f);
    m_lanternB = makeLantern("LanternRight",  3.0f, 0.0f);

    // Spot light aiming down at the pillar base
    m_spotlight = world->CreateEntity();
    world->AddComponent<NameComponent>(m_spotlight, NameComponent{ "Spotlight" });
    world->AddComponent<Transform>(m_spotlight, Transform{
        { 0.0f, 8.0f, -4.0f }, { 60.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }
    });
    world->AddComponent<LightComponent>(m_spotlight, LightComponent{
        LightComponent::Type::Spot,
        { 0.5f, 0.7f, 1.0f }, // cool blue
        2.0f, 15.0f,
        35.0f, 20.0f, true, 1024
    });
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

void AmbientSceneModule::OnUpdate(float dt)
{
    auto* input = m_ctx->GetInput();
    auto* world = m_ctx->GetWorld();

    // Advance the day/night cycle
    m_dayNight.Update(dt);

    // Sync the sun direction & colour from the cycle to the directional light
    {
        auto sunDir   = m_dayNight.GetSunDirection();
        auto sunColor = m_dayNight.GetSunColor();
        float sunInt  = m_dayNight.GetSunIntensity();

        auto& sunXf    = world->GetComponent<Transform>(m_sun);
        sunXf.rotation = {
            -std::asin(sunDir.y) * (180.0f / 3.14159f),
            std::atan2(sunDir.x, sunDir.z) * (180.0f / 3.14159f),
            0.0f
        };

        auto& sunLight = world->GetComponent<LightComponent>(m_sun);
        sunLight.color     = { sunColor.r, sunColor.g, sunColor.b };
        sunLight.intensity = sunInt * 1.5f;
    }

    // Toggle time-scale with T
    if (input && input->WasKeyPressed('T'))
    {
        m_timeScale = (m_timeScale > 30.0f) ? 1.0f : 60.0f;
        m_dayNight.SetTimeScale(m_timeScale);
    }

    // Manual weather cycling with C
    if (input && input->WasKeyPressed('C'))
        CycleWeather();

    // Auto weather cycling
    if (m_autoWeather)
    {
        m_weatherTimer += dt;
        if (m_weatherTimer >= kWeatherInterval)
        {
            m_weatherTimer = 0.0f;
            CycleWeather();
        }
    }
}

void AmbientSceneModule::CycleWeather()
{
    auto* world = m_ctx->GetWorld();
    if (!world || m_weatherEnt == entt::null) return;

    m_weatherIndex = (m_weatherIndex + 1) % 4;

    auto& wc = world->GetComponent<WeatherComponent>(m_weatherEnt);
    // 0 = clear, 1 = cloudy, 2 = rain, 3 = snow
    wc.weatherType = m_weatherIndex;
    wc.intensity   = (m_weatherIndex == 0) ? 0.0f : 0.5f + 0.2f * m_weatherIndex;
    wc.windSpeed   = 1.0f + 2.0f * m_weatherIndex;

    // Publish event if bus exists
    if (auto* bus = m_ctx->GetEventBus())
    {
        bus->Publish(Spark::WeatherChangedEvent{
            m_weatherIndex, wc.intensity
        });
    }
}

// ---------------------------------------------------------------------------
void AmbientSceneModule::OnRender() { }
void AmbientSceneModule::OnResize(int, int) { }

// ---------------------------------------------------------------------------
SPARK_IMPLEMENT_MODULE(AmbientSceneModule)
