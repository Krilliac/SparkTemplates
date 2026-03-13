/**
 * @file ShadowLightingModule.cpp
 * @brief ADVANCED — Lighting and shadow showcase with CSM, IBL, SSR, SSAO.
 */

#include "ShadowLightingModule.h"
#include <Input/InputManager.h>
#include <cmath>
#include <algorithm>

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void ShadowLightingModule::OnLoad(Spark::IEngineContext* ctx)
{
    m_ctx = ctx;

    BuildScene();
    CreateLights();
    PlaceLightProbes();

    // Configure initial rendering features
    auto* gfx = m_ctx->GetGraphics();
    if (gfx)
    {
        gfx->SetCSMSplits(m_csmSplits[0], m_csmSplits[1],
                           m_csmSplits[2], m_csmSplits[3]);
        gfx->SetEnvironmentMap("Assets/Textures/env_skybox.hdr");
        gfx->ToggleIBL(m_iblEnabled);
        gfx->ToggleSSR(m_ssrEnabled);
        gfx->SetSSAO(m_ssaoEnabled, m_ssaoRadius, m_ssaoIntensity);
    }

    ApplyShadowQuality();
}

void ShadowLightingModule::OnUnload()
{
    auto* world = m_ctx ? m_ctx->GetWorld() : nullptr;
    if (!world) { m_ctx = nullptr; return; }

    for (auto e : { m_camera, m_sunLight })
        if (e != entt::null) world->DestroyEntity(e);

    for (auto e : m_pointLights)
        if (e != entt::null) world->DestroyEntity(e);
    for (auto e : m_spotLights)
        if (e != entt::null) world->DestroyEntity(e);
    for (auto e : m_areaLights)
        if (e != entt::null) world->DestroyEntity(e);
    for (auto e : m_sceneEntities)
        if (e != entt::null) world->DestroyEntity(e);

    m_pointLights.clear();
    m_spotLights.clear();
    m_areaLights.clear();
    m_sceneEntities.clear();
    m_ctx = nullptr;
}

// ---------------------------------------------------------------------------
// Scene construction
// ---------------------------------------------------------------------------

void ShadowLightingModule::BuildScene()
{
    auto* world = m_ctx->GetWorld();

    // --- Camera -------------------------------------------------------------
    m_camera = world->CreateEntity();
    world->AddComponent<NameComponent>(m_camera, NameComponent{ "MainCamera" });
    world->AddComponent<Transform>(m_camera, Transform{
        { 0.0f, 8.0f, -20.0f },
        { -25.0f, 0.0f, 0.0f },
        { 1.0f, 1.0f, 1.0f }
    });
    world->AddComponent<Camera>(m_camera, Camera{
        60.0f, 0.1f, 500.0f, true
    });

    BuildExterior();
    BuildInterior();
}

void ShadowLightingModule::BuildExterior()
{
    auto* world = m_ctx->GetWorld();

    // Ground plane
    auto ground = world->CreateEntity();
    world->AddComponent<NameComponent>(ground, NameComponent{ "Ground" });
    world->AddComponent<Transform>(ground, Transform{
        { 0.0f, -0.1f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 60.0f, 0.2f, 60.0f }
    });
    world->AddComponent<MeshRenderer>(ground, MeshRenderer{
        "Assets/Models/cube.obj", "Assets/Materials/stone_floor.mat",
        false, true, true
    });
    m_sceneEntities.push_back(ground);

    // Shadow-casting pillars (demonstrate CSM across different cascades)
    const float pillarPositions[][2] = {
        { -8.0f,  4.0f }, { -4.0f,  4.0f }, {  0.0f,  4.0f },
        {  4.0f,  4.0f }, {  8.0f,  4.0f },
        { -8.0f, -4.0f }, { -4.0f, -4.0f }, {  0.0f, -4.0f },
        {  4.0f, -4.0f }, {  8.0f, -4.0f }
    };

    for (int i = 0; i < 10; ++i)
    {
        auto pillar = world->CreateEntity();
        std::string name = "Pillar_" + std::to_string(i);
        world->AddComponent<NameComponent>(pillar, NameComponent{ name });
        world->AddComponent<Transform>(pillar, Transform{
            { pillarPositions[i][0], 2.5f, pillarPositions[i][1] },
            { 0.0f, 0.0f, 0.0f },
            { 0.6f, 5.0f, 0.6f }
        });
        world->AddComponent<MeshRenderer>(pillar, MeshRenderer{
            "Assets/Models/cube.obj", "Assets/Materials/marble.mat",
            true, true, true
        });
        m_sceneEntities.push_back(pillar);
    }

    // Reflective spheres (demonstrate SSR and IBL)
    const float sphereX[] = { -6.0f, -2.0f, 2.0f, 6.0f };
    const char* sphereMats[] = {
        "Assets/Materials/chrome.mat",
        "Assets/Materials/gold_reflective.mat",
        "Assets/Materials/glass.mat",
        "Assets/Materials/rough_metal.mat"
    };

    for (int i = 0; i < 4; ++i)
    {
        auto sphere = world->CreateEntity();
        std::string name = "ReflectSphere_" + std::to_string(i);
        world->AddComponent<NameComponent>(sphere, NameComponent{ name });
        world->AddComponent<Transform>(sphere, Transform{
            { sphereX[i], 1.0f, 0.0f },
            { 0.0f, 0.0f, 0.0f },
            { 1.5f, 1.5f, 1.5f }
        });
        world->AddComponent<MeshRenderer>(sphere, MeshRenderer{
            "Assets/Models/sphere.obj", sphereMats[i],
            true, true, true
        });
        m_sceneEntities.push_back(sphere);
    }

    // Far-distance objects (test distant CSM cascades)
    for (int i = 0; i < 5; ++i)
    {
        auto farObj = world->CreateEntity();
        std::string name = "FarTree_" + std::to_string(i);
        float angle = static_cast<float>(i) * 72.0f * (3.14159f / 180.0f);
        float dist = 25.0f;
        world->AddComponent<NameComponent>(farObj, NameComponent{ name });
        world->AddComponent<Transform>(farObj, Transform{
            { std::cos(angle) * dist, 3.0f, std::sin(angle) * dist },
            { 0.0f, 0.0f, 0.0f },
            { 1.0f, 6.0f, 1.0f }
        });
        world->AddComponent<MeshRenderer>(farObj, MeshRenderer{
            "Assets/Models/cube.obj", "Assets/Materials/bark.mat",
            true, true, true
        });
        m_sceneEntities.push_back(farObj);
    }
}

void ShadowLightingModule::BuildInterior()
{
    auto* world = m_ctx->GetWorld();

    // Interior room (walls and ceiling to demonstrate indoor lighting)
    auto floor = world->CreateEntity();
    world->AddComponent<NameComponent>(floor, NameComponent{ "InteriorFloor" });
    world->AddComponent<Transform>(floor, Transform{
        { 20.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 10.0f, 0.2f, 10.0f }
    });
    world->AddComponent<MeshRenderer>(floor, MeshRenderer{
        "Assets/Models/cube.obj", "Assets/Materials/wood_floor.mat",
        false, true, true
    });
    m_sceneEntities.push_back(floor);

    // Back wall
    auto backWall = world->CreateEntity();
    world->AddComponent<NameComponent>(backWall, NameComponent{ "BackWall" });
    world->AddComponent<Transform>(backWall, Transform{
        { 20.0f, 3.0f, 5.0f }, { 0.0f, 0.0f, 0.0f }, { 10.0f, 6.0f, 0.3f }
    });
    world->AddComponent<MeshRenderer>(backWall, MeshRenderer{
        "Assets/Models/cube.obj", "Assets/Materials/plaster.mat",
        true, true, true
    });
    m_sceneEntities.push_back(backWall);

    // Side walls
    auto leftWall = world->CreateEntity();
    world->AddComponent<NameComponent>(leftWall, NameComponent{ "LeftWall" });
    world->AddComponent<Transform>(leftWall, Transform{
        { 15.0f, 3.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.3f, 6.0f, 10.0f }
    });
    world->AddComponent<MeshRenderer>(leftWall, MeshRenderer{
        "Assets/Models/cube.obj", "Assets/Materials/plaster.mat",
        true, true, true
    });
    m_sceneEntities.push_back(leftWall);

    auto rightWall = world->CreateEntity();
    world->AddComponent<NameComponent>(rightWall, NameComponent{ "RightWall" });
    world->AddComponent<Transform>(rightWall, Transform{
        { 25.0f, 3.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.3f, 6.0f, 10.0f }
    });
    world->AddComponent<MeshRenderer>(rightWall, MeshRenderer{
        "Assets/Models/cube.obj", "Assets/Materials/plaster.mat",
        true, true, true
    });
    m_sceneEntities.push_back(rightWall);

    // Ceiling
    auto ceiling = world->CreateEntity();
    world->AddComponent<NameComponent>(ceiling, NameComponent{ "Ceiling" });
    world->AddComponent<Transform>(ceiling, Transform{
        { 20.0f, 6.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 10.0f, 0.2f, 10.0f }
    });
    world->AddComponent<MeshRenderer>(ceiling, MeshRenderer{
        "Assets/Models/cube.obj", "Assets/Materials/plaster.mat",
        false, true, true
    });
    m_sceneEntities.push_back(ceiling);

    // Interior furniture for shadow interplay
    auto table = world->CreateEntity();
    world->AddComponent<NameComponent>(table, NameComponent{ "Table" });
    world->AddComponent<Transform>(table, Transform{
        { 20.0f, 0.8f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 3.0f, 0.15f, 2.0f }
    });
    world->AddComponent<MeshRenderer>(table, MeshRenderer{
        "Assets/Models/cube.obj", "Assets/Materials/wood_dark.mat",
        true, true, true
    });
    m_sceneEntities.push_back(table);

    // Objects on the table
    auto vase = world->CreateEntity();
    world->AddComponent<NameComponent>(vase, NameComponent{ "Vase" });
    world->AddComponent<Transform>(vase, Transform{
        { 20.0f, 1.4f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.3f, 0.8f, 0.3f }
    });
    world->AddComponent<MeshRenderer>(vase, MeshRenderer{
        "Assets/Models/sphere.obj", "Assets/Materials/ceramic.mat",
        true, true, true
    });
    m_sceneEntities.push_back(vase);
}

// ---------------------------------------------------------------------------
// Lights
// ---------------------------------------------------------------------------

void ShadowLightingModule::CreateLights()
{
    auto* world = m_ctx->GetWorld();

    // --- Directional (sun) light with CSM -----------------------------------
    m_sunLight = world->CreateEntity();
    world->AddComponent<NameComponent>(m_sunLight, NameComponent{ "SunLight" });
    world->AddComponent<Transform>(m_sunLight, Transform{
        { 0.0f, 30.0f, 0.0f },
        { m_todAngle, 30.0f, 0.0f },
        { 1.0f, 1.0f, 1.0f }
    });
    // Warm daylight ~5500K
    float sunR, sunG, sunB;
    KelvinToRGB(5500.0f, sunR, sunG, sunB);
    world->AddComponent<LightComponent>(m_sunLight, LightComponent{
        LightComponent::Type::Directional,
        { sunR, sunG, sunB }, 2.0f, 0.0f,
        0.0f, 0.0f, true, 2048
    });

    // --- Point lights (exterior, colored) -----------------------------------
    struct PointLightDef { float x, y, z; float kelvin; float intensity; float range; };
    PointLightDef pointDefs[] = {
        { -6.0f, 3.0f, -3.0f, 3200.0f, 3.0f, 12.0f },   // warm tungsten
        {  6.0f, 3.0f, -3.0f, 6500.0f, 2.5f, 12.0f },   // cool daylight
        {  0.0f, 4.0f,  6.0f, 4000.0f, 2.0f, 15.0f },   // neutral
        { 20.0f, 4.5f,  0.0f, 2700.0f, 4.0f, 10.0f },   // interior warm bulb
    };

    for (int i = 0; i < 4; ++i)
    {
        auto& def = pointDefs[i];
        auto e = world->CreateEntity();
        std::string name = "PointLight_" + std::to_string(i);
        world->AddComponent<NameComponent>(e, NameComponent{ name });
        world->AddComponent<Transform>(e, Transform{
            { def.x, def.y, def.z },
            { 0.0f, 0.0f, 0.0f },
            { 1.0f, 1.0f, 1.0f }
        });

        float r, g, b;
        KelvinToRGB(def.kelvin, r, g, b);
        world->AddComponent<LightComponent>(e, LightComponent{
            LightComponent::Type::Point,
            { r, g, b }, def.intensity, def.range,
            0.0f, 0.0f, true, 1024
        });

        // Small visible sphere to show light position
        world->AddComponent<MeshRenderer>(e, MeshRenderer{
            "Assets/Models/sphere.obj", "Assets/Materials/emissive_white.mat",
            false, false, true
        });

        m_pointLights.push_back(e);
    }

    // --- Spot lights (with cookie textures) ---------------------------------
    struct SpotLightDef { float x, y, z; float rx, ry, rz; float angle; float innerAngle; };
    SpotLightDef spotDefs[] = {
        { -3.0f, 8.0f,  2.0f, -60.0f,  0.0f, 0.0f, 35.0f, 25.0f },
        {  3.0f, 8.0f,  2.0f, -60.0f,  0.0f, 0.0f, 45.0f, 30.0f },
        { 20.0f, 5.5f, -2.0f, -80.0f, 15.0f, 0.0f, 40.0f, 28.0f },
    };

    for (int i = 0; i < 3; ++i)
    {
        auto& def = spotDefs[i];
        auto e = world->CreateEntity();
        std::string name = "SpotLight_" + std::to_string(i);
        world->AddComponent<NameComponent>(e, NameComponent{ name });
        world->AddComponent<Transform>(e, Transform{
            { def.x, def.y, def.z },
            { def.rx, def.ry, def.rz },
            { 1.0f, 1.0f, 1.0f }
        });

        float r, g, b;
        KelvinToRGB(4200.0f, r, g, b);
        world->AddComponent<LightComponent>(e, LightComponent{
            LightComponent::Type::Spot,
            { r, g, b }, 5.0f, 20.0f,
            def.angle, def.innerAngle, true, 1024
        });

        m_spotLights.push_back(e);
    }

    // --- Area lights (soft shadows) -----------------------------------------
    struct AreaLightDef { float x, y, z; float rx, ry, rz; float w, h; };
    AreaLightDef areaDefs[] = {
        { -5.0f, 6.0f, 0.0f, -90.0f, 0.0f, 0.0f, 3.0f, 1.5f },   // rectangle
        {  5.0f, 6.0f, 0.0f, -90.0f, 0.0f, 0.0f, 2.0f, 2.0f },   // disc-ish
        { 20.0f, 5.8f, 0.0f, -90.0f, 0.0f, 0.0f, 4.0f, 2.0f },   // interior panel
    };

    for (int i = 0; i < 3; ++i)
    {
        auto& def = areaDefs[i];
        auto e = world->CreateEntity();
        std::string name = "AreaLight_" + std::to_string(i);
        world->AddComponent<NameComponent>(e, NameComponent{ name });
        world->AddComponent<Transform>(e, Transform{
            { def.x, def.y, def.z },
            { def.rx, def.ry, def.rz },
            { def.w, 0.05f, def.h }
        });

        float r, g, b;
        KelvinToRGB(5000.0f, r, g, b);
        world->AddComponent<LightComponent>(e, LightComponent{
            LightComponent::Type::Area,
            { r, g, b }, 3.0f, 15.0f,
            0.0f, 0.0f, true, 512
        });

        // Visible emissive panel
        world->AddComponent<MeshRenderer>(e, MeshRenderer{
            "Assets/Models/cube.obj", "Assets/Materials/emissive_white.mat",
            false, false, true
        });

        m_areaLights.push_back(e);
    }
}

// ---------------------------------------------------------------------------
// Light probes (for indirect lighting)
// ---------------------------------------------------------------------------

void ShadowLightingModule::PlaceLightProbes()
{
    auto* gfx = m_ctx->GetGraphics();
    if (!gfx) return;

    // Exterior probes — grid
    for (float x = -10.0f; x <= 10.0f; x += 5.0f)
    {
        for (float z = -10.0f; z <= 10.0f; z += 5.0f)
        {
            gfx->PlaceLightProbe({ x, 2.0f, z });
        }
    }

    // Interior probes
    gfx->PlaceLightProbe({ 18.0f, 3.0f,  0.0f });
    gfx->PlaceLightProbe({ 20.0f, 3.0f,  0.0f });
    gfx->PlaceLightProbe({ 22.0f, 3.0f,  0.0f });
    gfx->PlaceLightProbe({ 20.0f, 3.0f,  3.0f });
    gfx->PlaceLightProbe({ 20.0f, 3.0f, -3.0f });
}

// ---------------------------------------------------------------------------
// Kelvin to RGB conversion (approximate, Tanner Helland's method)
// ---------------------------------------------------------------------------

void ShadowLightingModule::KelvinToRGB(float kelvin, float& r, float& g, float& b)
{
    float temp = kelvin / 100.0f;

    // Red
    if (temp <= 66.0f)
        r = 1.0f;
    else
    {
        float x = temp - 60.0f;
        r = 329.698727446f * std::pow(x, -0.1332047592f) / 255.0f;
        r = std::clamp(r, 0.0f, 1.0f);
    }

    // Green
    if (temp <= 66.0f)
    {
        float x = temp;
        g = (99.4708025861f * std::log(x) - 161.1195681661f) / 255.0f;
        g = std::clamp(g, 0.0f, 1.0f);
    }
    else
    {
        float x = temp - 60.0f;
        g = 288.1221695283f * std::pow(x, -0.0755148492f) / 255.0f;
        g = std::clamp(g, 0.0f, 1.0f);
    }

    // Blue
    if (temp >= 66.0f)
        b = 1.0f;
    else if (temp <= 19.0f)
        b = 0.0f;
    else
    {
        float x = temp - 10.0f;
        b = (138.5177312231f * std::log(x) - 305.0447927307f) / 255.0f;
        b = std::clamp(b, 0.0f, 1.0f);
    }
}

// ---------------------------------------------------------------------------
// Shadow quality presets
// ---------------------------------------------------------------------------

void ShadowLightingModule::ApplyShadowQuality()
{
    auto* gfx = m_ctx->GetGraphics();
    if (!gfx) return;

    int resolution = 2048;
    int pcfSize    = 5;
    float bias     = 0.005f;

    switch (m_shadowQuality)
    {
        case ShadowQuality::Low:    resolution =  512; pcfSize = 1; bias = 0.008f; break;
        case ShadowQuality::Medium: resolution = 1024; pcfSize = 3; bias = 0.006f; break;
        case ShadowQuality::High:   resolution = 2048; pcfSize = 5; bias = 0.005f; break;
        case ShadowQuality::Ultra:  resolution = 4096; pcfSize = 7; bias = 0.003f; break;
    }

    gfx->SetShadowMapResolution(resolution);
    gfx->SetShadowPCFFilterSize(pcfSize);
    gfx->SetShadowBias(bias);
}

void ShadowLightingModule::CycleShadowQuality()
{
    int q = static_cast<int>(m_shadowQuality);
    q = (q + 1) % 4;
    m_shadowQuality = static_cast<ShadowQuality>(q);
    ApplyShadowQuality();
}

// ---------------------------------------------------------------------------
// Time of day (animates sun direction)
// ---------------------------------------------------------------------------

void ShadowLightingModule::UpdateTimeOfDay(float dt)
{
    if (!m_timeOfDay) return;

    m_todAngle += m_todSpeed * dt;
    if (m_todAngle > 360.0f) m_todAngle -= 360.0f;

    auto* world = m_ctx->GetWorld();
    if (m_sunLight != entt::null)
    {
        auto& xf = world->GetComponent<Transform>(m_sunLight);
        xf.rotation.x = m_todAngle;

        // Adjust sun color based on angle (warm at horizon, white at zenith)
        float normalized = std::fmod(std::abs(m_todAngle), 180.0f) / 180.0f;
        float kelvin = 2000.0f + normalized * 4500.0f; // 2000K-6500K

        auto& lc = world->GetComponent<LightComponent>(m_sunLight);
        KelvinToRGB(kelvin, lc.color.x, lc.color.y, lc.color.z);

        // Dim the light near the horizon
        float elevation = std::sin(m_todAngle * 3.14159f / 180.0f);
        lc.intensity = std::max(0.1f, elevation * 2.0f) * m_intensityMul;
    }
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

void ShadowLightingModule::OnUpdate(float dt)
{
    auto* input = m_ctx->GetInput();
    auto* world = m_ctx->GetWorld();
    if (!input || !world) return;

    // --- Toggle light types -------------------------------------------------
    if (input->WasKeyPressed('1') && m_sunLight != entt::null)
    {
        m_dirLightOn = !m_dirLightOn;
        auto& lc = world->GetComponent<LightComponent>(m_sunLight);
        lc.intensity = m_dirLightOn ? 2.0f * m_intensityMul : 0.0f;
    }

    if (input->WasKeyPressed('2'))
    {
        m_pointLightsOn = !m_pointLightsOn;
        for (auto e : m_pointLights)
        {
            if (e == entt::null) continue;
            auto& lc = world->GetComponent<LightComponent>(e);
            lc.intensity = m_pointLightsOn ? 3.0f * m_intensityMul : 0.0f;
        }
    }

    if (input->WasKeyPressed('3'))
    {
        m_spotLightsOn = !m_spotLightsOn;
        for (auto e : m_spotLights)
        {
            if (e == entt::null) continue;
            auto& lc = world->GetComponent<LightComponent>(e);
            lc.intensity = m_spotLightsOn ? 5.0f * m_intensityMul : 0.0f;
        }
    }

    if (input->WasKeyPressed('4'))
    {
        m_areaLightsOn = !m_areaLightsOn;
        for (auto e : m_areaLights)
        {
            if (e == entt::null) continue;
            auto& lc = world->GetComponent<LightComponent>(e);
            lc.intensity = m_areaLightsOn ? 3.0f * m_intensityMul : 0.0f;
        }
    }

    // --- Shadow quality cycle (S) -------------------------------------------
    if (input->WasKeyPressed('S'))
        CycleShadowQuality();

    // --- CSM debug toggle (C) -----------------------------------------------
    if (input->WasKeyPressed('C'))
    {
        m_csmDebug = !m_csmDebug;
        if (auto* gfx = m_ctx->GetGraphics())
            gfx->ToggleCSMDebug(m_csmDebug);
    }

    // --- IBL toggle (I) -----------------------------------------------------
    if (input->WasKeyPressed('I'))
    {
        m_iblEnabled = !m_iblEnabled;
        if (auto* gfx = m_ctx->GetGraphics())
            gfx->ToggleIBL(m_iblEnabled);
    }

    // --- SSR toggle (R) -----------------------------------------------------
    if (input->WasKeyPressed('R'))
    {
        m_ssrEnabled = !m_ssrEnabled;
        if (auto* gfx = m_ctx->GetGraphics())
            gfx->ToggleSSR(m_ssrEnabled);
    }

    // --- SSAO toggle (A) ----------------------------------------------------
    if (input->WasKeyPressed('A'))
    {
        m_ssaoEnabled = !m_ssaoEnabled;
        if (auto* gfx = m_ctx->GetGraphics())
            gfx->SetSSAO(m_ssaoEnabled, m_ssaoRadius, m_ssaoIntensity);
    }

    // --- Intensity +/- ------------------------------------------------------
    if (input->IsKeyDown(VK_OEM_PLUS) || input->IsKeyDown(VK_ADD))
    {
        m_intensityMul += 0.5f * dt;
        m_intensityMul = std::min(m_intensityMul, 5.0f);
    }
    if (input->IsKeyDown(VK_OEM_MINUS) || input->IsKeyDown(VK_SUBTRACT))
    {
        m_intensityMul -= 0.5f * dt;
        m_intensityMul = std::max(m_intensityMul, 0.1f);
    }

    // --- Time-of-day toggle (T) ---------------------------------------------
    if (input->WasKeyPressed('T'))
        m_timeOfDay = !m_timeOfDay;

    UpdateTimeOfDay(dt);

    // --- Camera orbit (mouse) -----------------------------------------------
    if (input->IsMouseButtonDown(1))
    {
        auto delta = input->GetMouseDelta();
        m_camYaw   += delta.x * 0.3f;
        m_camPitch -= delta.y * 0.3f;
        m_camPitch  = std::clamp(m_camPitch, -85.0f, 10.0f);
    }

    float scroll = input->GetMouseScrollDelta();
    m_camDist -= scroll * 1.5f;
    m_camDist  = std::clamp(m_camDist, 5.0f, 60.0f);

    float radY = m_camYaw   * (3.14159f / 180.0f);
    float radP = m_camPitch * (3.14159f / 180.0f);

    auto& camXf = world->GetComponent<Transform>(m_camera);
    camXf.position = {
        std::cos(radP) * std::sin(radY) * m_camDist,
        -std::sin(radP) * m_camDist + 5.0f,
        std::cos(radP) * std::cos(radY) * m_camDist
    };
    camXf.rotation = { m_camPitch, m_camYaw + 180.0f, 0.0f };
}

// ---------------------------------------------------------------------------
// Render / Resize
// ---------------------------------------------------------------------------

void ShadowLightingModule::OnRender() { /* RenderSystem auto-draws MeshRenderers */ }

void ShadowLightingModule::OnResize(int /*w*/, int /*h*/) { }

// ---------------------------------------------------------------------------
// DLL exports
// ---------------------------------------------------------------------------

SPARK_IMPLEMENT_MODULE(ShadowLightingModule)
