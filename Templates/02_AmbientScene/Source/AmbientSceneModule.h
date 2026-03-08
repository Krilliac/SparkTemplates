#pragma once
/**
 * @file AmbientSceneModule.h
 * @brief BASIC — Atmospheric scene with day/night cycle, weather, and lighting.
 *
 * Demonstrates:
 *   - DayNightCycle system — sun arc, ambient colour interpolation, period callbacks
 *   - Multiple light types  — Directional (sun), Point (lanterns), Spot (spotlight)
 *   - WeatherComponent      — wind, intensity, weather type
 *   - EventBus              — subscribing to TimeOfDayChangedEvent & WeatherChangedEvent
 *   - Scene composition     — ground plane + decorative objects
 *
 * Spark Engine systems used:
 *   Core         — IModule, IEngineContext
 *   ECS          — World, Transform, MeshRenderer, LightComponent, WeatherComponent
 *   World        — DayNightCycle
 *   Events       — EventBus, TimeOfDayChangedEvent, WeatherChangedEvent
 *   Input        — InputManager
 */

#include <Core/ModuleManager.h>
#include <Core/EngineContext.h>
#include <Engine/ECS/Components.h>
#include <Engine/World/DayNightCycle.h>
#include <Engine/Events/EventSystem.h>

class AmbientSceneModule final : public Spark::IModule
{
public:
    Spark::ModuleInfo GetModuleInfo() const override
    {
        return { "AmbientScene", "0.1.0", SPARK_SDK_VERSION, 1000 };
    }

    void OnLoad(Spark::IEngineContext* ctx) override;
    void OnUnload() override;
    void OnUpdate(float dt) override;
    void OnRender() override;
    void OnResize(int w, int h) override;

private:
    void BuildScene();
    void SetupLighting();
    void CycleWeather();

    Spark::IEngineContext* m_ctx = nullptr;

    // Day/night
    Spark::DayNightCycle m_dayNight;
    float m_timeScale = 60.0f; // 1 real second = 1 in-game minute

    // Entities
    entt::entity m_ground     = entt::null;
    entt::entity m_sun        = entt::null;
    entt::entity m_lanternA   = entt::null;
    entt::entity m_lanternB   = entt::null;
    entt::entity m_spotlight  = entt::null;
    entt::entity m_weatherEnt = entt::null;
    entt::entity m_pillar     = entt::null;

    // Weather cycling
    int  m_weatherIndex = 0;
    bool m_autoWeather  = true;
    float m_weatherTimer = 0.0f;
    static constexpr float kWeatherInterval = 30.0f; // seconds between changes

    // Event subscriptions
    Spark::SubscriptionID m_timeSub    = 0;
    Spark::SubscriptionID m_weatherSub = 0;
};
