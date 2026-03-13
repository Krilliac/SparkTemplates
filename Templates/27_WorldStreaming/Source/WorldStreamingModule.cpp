/**
 * @file WorldStreamingModule.cpp
 * @brief ADVANCED — Seamless area streaming and world origin rebasing.
 */

#include "WorldStreamingModule.h"
#include <Input/InputManager.h>
#include <cmath>
#include <algorithm>
#include <cstdio>

static constexpr float kPi = 3.14159265358979f;

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void WorldStreamingModule::OnLoad(Spark::IEngineContext* ctx)
{
    m_ctx = ctx;

    // Configure origin system
    m_originSystem.SetThreshold(kOriginThreshold);

    BuildScene();
    RegisterAreas();

    // Load the starting area
    LoadArea(0);
}

void WorldStreamingModule::OnUnload()
{
    auto* world = m_ctx ? m_ctx->GetWorld() : nullptr;
    if (!world) { m_ctx = nullptr; return; }

    // Unload all areas
    for (int i = 0; i < static_cast<int>(m_areas.size()); ++i)
        UnloadArea(i);

    for (auto e : m_coreEntities)
        if (e != entt::null) world->DestroyEntity(e);

    m_coreEntities.clear();
    m_areas.clear();
    m_ctx = nullptr;
}

// ---------------------------------------------------------------------------
// Scene construction (core entities only)
// ---------------------------------------------------------------------------

void WorldStreamingModule::BuildScene()
{
    auto* world = m_ctx->GetWorld();

    // --- Camera -------------------------------------------------------------
    m_camera = world->CreateEntity();
    world->AddComponent<NameComponent>(m_camera, NameComponent{ "MainCamera" });
    world->AddComponent<Transform>(m_camera, Transform{
        { 0.0f, 30.0f, -30.0f },
        { -40.0f, 0.0f, 0.0f },
        { 1.0f, 1.0f, 1.0f }
    });
    world->AddComponent<Camera>(m_camera, Camera{
        60.0f, 0.1f, 1000.0f, true
    });
    m_coreEntities.push_back(m_camera);

    // --- Light --------------------------------------------------------------
    m_light = world->CreateEntity();
    world->AddComponent<NameComponent>(m_light, NameComponent{ "SunLight" });
    world->AddComponent<Transform>(m_light, Transform{
        { 0.0f, 50.0f, 0.0f },
        { -45.0f, 30.0f, 0.0f },
        { 1.0f, 1.0f, 1.0f }
    });
    world->AddComponent<LightComponent>(m_light, LightComponent{
        LightComponent::Type::Directional,
        { 1.0f, 0.95f, 0.85f }, 2.5f, 0.0f,
        0.0f, 0.0f, true, 2048
    });
    m_coreEntities.push_back(m_light);

    // --- Player -------------------------------------------------------------
    m_player = world->CreateEntity();
    world->AddComponent<NameComponent>(m_player, NameComponent{ "Player" });
    world->AddComponent<Transform>(m_player, Transform{
        { 50.0f, 1.0f, 50.0f },
        { 0.0f, 0.0f, 0.0f },
        { 1.0f, 2.0f, 1.0f }
    });
    world->AddComponent<MeshRenderer>(m_player, MeshRenderer{
        "Assets/Models/cube.obj", "Assets/Materials/player.mat",
        true, true, true
    });
    m_coreEntities.push_back(m_player);
}

// ---------------------------------------------------------------------------
// Register areas in 2x2 grid
// ---------------------------------------------------------------------------

void WorldStreamingModule::RegisterAreas()
{
    // Layout:
    //  [Forest (0,0)] [Desert (100,0)]
    //  [Snow   (0,100)] [City  (100,100)]

    AreaDef forest;
    forest.name = "ForestArea";
    forest.originX = 0.0f;
    forest.originZ = 0.0f;
    forest.sizeX = kAreaSize;
    forest.sizeZ = kAreaSize;
    forest.colorR = 0.15f;
    forest.colorG = 0.5f;
    forest.colorB = 0.1f;
    forest.loaded = false;
    m_areas.push_back(forest);

    AreaDef desert;
    desert.name = "DesertArea";
    desert.originX = kAreaSize;
    desert.originZ = 0.0f;
    desert.sizeX = kAreaSize;
    desert.sizeZ = kAreaSize;
    desert.colorR = 0.8f;
    desert.colorG = 0.7f;
    desert.colorB = 0.3f;
    desert.loaded = false;
    m_areas.push_back(desert);

    AreaDef snow;
    snow.name = "SnowArea";
    snow.originX = 0.0f;
    snow.originZ = kAreaSize;
    snow.sizeX = kAreaSize;
    snow.sizeZ = kAreaSize;
    snow.colorR = 0.85f;
    snow.colorG = 0.9f;
    snow.colorB = 0.95f;
    snow.loaded = false;
    m_areas.push_back(snow);

    AreaDef city;
    city.name = "CityArea";
    city.originX = kAreaSize;
    city.originZ = kAreaSize;
    city.sizeX = kAreaSize;
    city.sizeZ = kAreaSize;
    city.colorR = 0.4f;
    city.colorG = 0.4f;
    city.colorB = 0.45f;
    city.loaded = false;
    m_areas.push_back(city);

    // Register with SeamlessAreaManager
    for (int i = 0; i < static_cast<int>(m_areas.size()); ++i)
    {
        auto& area = m_areas[i];
        m_areaManager.RegisterArea(
            area.name,
            { area.originX, 0.0f, area.originZ },
            { area.originX + area.sizeX, 50.0f, area.originZ + area.sizeZ },
            [this, i]() { LoadArea(i); },
            [this, i]() { UnloadArea(i); }
        );
    }
}

// ---------------------------------------------------------------------------
// Area loading/unloading
// ---------------------------------------------------------------------------

void WorldStreamingModule::LoadArea(int index)
{
    if (index < 0 || index >= static_cast<int>(m_areas.size())) return;
    auto& area = m_areas[index];
    if (area.loaded) return;

    area.loaded = true;

    // Create ground plane for the area
    auto ground = CreateProp("Ground", area.originX + area.sizeX * 0.5f, -0.1f,
                              area.originZ + area.sizeZ * 0.5f,
                              area.sizeX, 0.2f, area.sizeZ,
                              area.colorR, area.colorG, area.colorB);
    area.entities.push_back(ground);

    // Populate with area-specific content
    switch (index)
    {
    case 0: PopulateForest(index); break;
    case 1: PopulateDesert(index); break;
    case 2: PopulateSnow(index); break;
    case 3: PopulateCity(index); break;
    }
}

void WorldStreamingModule::UnloadArea(int index)
{
    if (index < 0 || index >= static_cast<int>(m_areas.size())) return;
    auto& area = m_areas[index];
    if (!area.loaded) return;

    auto* world = m_ctx->GetWorld();
    if (!world) return;

    for (auto e : area.entities)
        if (e != entt::null) world->DestroyEntity(e);

    area.entities.clear();
    area.loaded = false;
}

// ---------------------------------------------------------------------------
// Area content creation
// ---------------------------------------------------------------------------

void WorldStreamingModule::PopulateForest(int index)
{
    auto& area = m_areas[index];
    float ox = area.originX;
    float oz = area.originZ;

    // Trees (tall green pillars)
    for (int i = 0; i < 20; ++i)
    {
        float tx = ox + 10.0f + static_cast<float>(i % 5) * 18.0f + static_cast<float>(i / 5) * 3.0f;
        float tz = oz + 10.0f + static_cast<float>(i / 5) * 20.0f + static_cast<float>(i % 5) * 2.0f;
        char name[32];
        std::snprintf(name, sizeof(name), "Tree_%d", i);

        // Trunk
        auto trunk = CreateProp(name, tx, 3.0f, tz, 0.8f, 6.0f, 0.8f,
                                 0.4f, 0.25f, 0.1f);
        area.entities.push_back(trunk);

        // Canopy
        char cname[32];
        std::snprintf(cname, sizeof(cname), "Canopy_%d", i);
        auto canopy = CreateProp(cname, tx, 7.0f, tz, 3.0f, 3.0f, 3.0f,
                                  0.1f, 0.55f, 0.1f);
        area.entities.push_back(canopy);
    }

    // Rocks
    for (int i = 0; i < 8; ++i)
    {
        float rx = ox + 5.0f + static_cast<float>(i) * 11.5f;
        float rz = oz + 15.0f + static_cast<float>(i % 3) * 25.0f;
        char name[32];
        std::snprintf(name, sizeof(name), "Rock_%d", i);
        auto rock = CreateProp(name, rx, 0.5f, rz, 2.0f, 1.0f, 1.5f,
                                0.35f, 0.35f, 0.3f);
        area.entities.push_back(rock);
    }
}

void WorldStreamingModule::PopulateDesert(int index)
{
    auto& area = m_areas[index];
    float ox = area.originX;
    float oz = area.originZ;

    // Sand dunes (flattened cubes)
    for (int i = 0; i < 10; ++i)
    {
        float dx = ox + 8.0f + static_cast<float>(i % 4) * 22.0f;
        float dz = oz + 12.0f + static_cast<float>(i / 4) * 30.0f;
        float h = 1.5f + static_cast<float>(i % 3) * 1.0f;
        char name[32];
        std::snprintf(name, sizeof(name), "Dune_%d", i);
        auto dune = CreateProp(name, dx, h * 0.5f, dz, 8.0f, h, 6.0f,
                                0.85f, 0.75f, 0.4f);
        area.entities.push_back(dune);
    }

    // Cacti (tall thin pillars)
    for (int i = 0; i < 6; ++i)
    {
        float cx = ox + 20.0f + static_cast<float>(i) * 12.0f;
        float cz = oz + 25.0f + static_cast<float>(i % 2) * 40.0f;
        char name[32];
        std::snprintf(name, sizeof(name), "Cactus_%d", i);
        auto cactus = CreateProp(name, cx, 2.0f, cz, 0.5f, 4.0f, 0.5f,
                                  0.2f, 0.5f, 0.15f);
        area.entities.push_back(cactus);
    }

    // Obelisk
    auto obelisk = CreateProp("Obelisk", ox + 50.0f, 5.0f, oz + 50.0f,
                               2.0f, 10.0f, 2.0f, 0.6f, 0.55f, 0.35f);
    area.entities.push_back(obelisk);
}

void WorldStreamingModule::PopulateSnow(int index)
{
    auto& area = m_areas[index];
    float ox = area.originX;
    float oz = area.originZ;

    // Snow-covered pine trees (white canopies)
    for (int i = 0; i < 15; ++i)
    {
        float tx = ox + 8.0f + static_cast<float>(i % 5) * 17.0f + static_cast<float>(i / 5) * 4.0f;
        float tz = oz + 8.0f + static_cast<float>(i / 5) * 28.0f;
        char name[32];
        std::snprintf(name, sizeof(name), "SnowTree_%d", i);
        auto trunk = CreateProp(name, tx, 2.5f, tz, 0.6f, 5.0f, 0.6f,
                                 0.35f, 0.2f, 0.1f);
        area.entities.push_back(trunk);

        char cname[32];
        std::snprintf(cname, sizeof(cname), "SnowCanopy_%d", i);
        auto canopy = CreateProp(cname, tx, 6.0f, tz, 2.5f, 2.5f, 2.5f,
                                  0.9f, 0.95f, 1.0f);
        area.entities.push_back(canopy);
    }

    // Ice blocks
    for (int i = 0; i < 5; ++i)
    {
        float ix = ox + 15.0f + static_cast<float>(i) * 15.0f;
        float iz = oz + 40.0f + static_cast<float>(i % 2) * 20.0f;
        char name[32];
        std::snprintf(name, sizeof(name), "IceBlock_%d", i);
        auto ice = CreateProp(name, ix, 1.0f, iz, 3.0f, 2.0f, 3.0f,
                               0.6f, 0.8f, 1.0f);
        area.entities.push_back(ice);
    }

    // Snowman
    auto snowman = CreateProp("Snowman", ox + 50.0f, 1.5f, oz + 50.0f,
                               2.0f, 3.0f, 2.0f, 0.95f, 0.95f, 0.98f);
    area.entities.push_back(snowman);
}

void WorldStreamingModule::PopulateCity(int index)
{
    auto& area = m_areas[index];
    float ox = area.originX;
    float oz = area.originZ;

    // Buildings (tall boxes)
    for (int i = 0; i < 12; ++i)
    {
        float bx = ox + 10.0f + static_cast<float>(i % 4) * 22.0f;
        float bz = oz + 10.0f + static_cast<float>(i / 4) * 25.0f;
        float bh = 8.0f + static_cast<float>(i % 5) * 4.0f;
        float bw = 4.0f + static_cast<float>(i % 3) * 2.0f;
        char name[32];
        std::snprintf(name, sizeof(name), "Building_%d", i);

        float shade = 0.3f + static_cast<float>(i % 4) * 0.1f;
        auto building = CreateProp(name, bx, bh * 0.5f, bz, bw, bh, bw,
                                    shade, shade, shade + 0.05f);
        area.entities.push_back(building);
    }

    // Street lamps
    for (int i = 0; i < 8; ++i)
    {
        float lx = ox + 5.0f + static_cast<float>(i) * 12.0f;
        float lz = oz + 5.0f;
        char name[32];
        std::snprintf(name, sizeof(name), "Lamp_%d", i);
        auto lamp = CreateProp(name, lx, 2.5f, lz, 0.3f, 5.0f, 0.3f,
                                0.5f, 0.5f, 0.45f);
        area.entities.push_back(lamp);
    }

    // Central plaza fountain
    auto fountain = CreateProp("Fountain", ox + 50.0f, 1.0f, oz + 50.0f,
                                4.0f, 2.0f, 4.0f, 0.3f, 0.5f, 0.7f);
    area.entities.push_back(fountain);
}

// ---------------------------------------------------------------------------
// Create a prop entity
// ---------------------------------------------------------------------------

entt::entity WorldStreamingModule::CreateProp(const char* name, float x, float y, float z,
                                               float sx, float sy, float sz,
                                               float r, float g, float b)
{
    auto* world = m_ctx->GetWorld();

    auto entity = world->CreateEntity();
    world->AddComponent<NameComponent>(entity, NameComponent{ name });
    world->AddComponent<Transform>(entity, Transform{
        { x, y, z }, { 0.0f, 0.0f, 0.0f }, { sx, sy, sz }
    });
    world->AddComponent<MeshRenderer>(entity, MeshRenderer{
        "Assets/Models/cube.obj", "Assets/Materials/default.mat",
        true, true, true
    });

    return entity;
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

void WorldStreamingModule::OnUpdate(float dt)
{
    auto* input = m_ctx->GetInput();
    auto* world = m_ctx->GetWorld();
    if (!input || !world) return;

    // --- Toggle controls ----------------------------------------------------
    if (input->WasKeyPressed(VK_F1))
        m_showBoundaries = !m_showBoundaries;

    if (input->WasKeyPressed(VK_F2))
        m_showOriginInfo = !m_showOriginInfo;

    // --- Teleport between areas (Tab) ---------------------------------------
    if (input->WasKeyPressed(VK_TAB))
    {
        m_currentAreaIndex = (m_currentAreaIndex + 1) % static_cast<int>(m_areas.size());
        TeleportToArea(m_currentAreaIndex);
    }

    // --- Player movement (WASD) ---------------------------------------------
    auto& playerXf = world->GetComponent<Transform>(m_player);

    float moveX = 0.0f, moveZ = 0.0f;
    if (input->IsKeyDown('W')) moveZ += 1.0f;
    if (input->IsKeyDown('S')) moveZ -= 1.0f;
    if (input->IsKeyDown('A')) moveX -= 1.0f;
    if (input->IsKeyDown('D')) moveX += 1.0f;

    float len = std::sqrt(moveX * moveX + moveZ * moveZ);
    if (len > 0.01f)
    {
        moveX /= len;
        moveZ /= len;
    }

    playerXf.position.x += moveX * m_playerSpeed * dt;
    playerXf.position.z += moveZ * m_playerSpeed * dt;

    // --- Update area streaming based on player position ---------------------
    UpdateAreaStreaming();

    // --- Update origin rebasing ---------------------------------------------
    UpdateOriginRebasing();

    // --- Transition effect --------------------------------------------------
    if (m_isTransitioning)
    {
        m_transitionTimer -= dt;
        m_transitionProgress = std::max(0.0f, m_transitionTimer / 1.0f);
        if (m_transitionTimer <= 0.0f)
            m_isTransitioning = false;
    }

    // --- Camera follow (3rd person orbit) -----------------------------------
    float yawRad   = m_camYaw * (kPi / 180.0f);
    float pitchRad = m_camPitch * (kPi / 180.0f);

    auto& camXf = world->GetComponent<Transform>(m_camera);
    float t = 1.0f - std::exp(-4.0f * dt);
    float targetX = playerXf.position.x + std::sin(yawRad) * std::cos(pitchRad) * m_camDist;
    float targetY = playerXf.position.y - std::sin(pitchRad) * m_camDist;
    float targetZ = playerXf.position.z + std::cos(yawRad) * std::cos(pitchRad) * m_camDist;

    camXf.position.x += (targetX - camXf.position.x) * t;
    camXf.position.y += (targetY - camXf.position.y) * t;
    camXf.position.z += (targetZ - camXf.position.z) * t;
    camXf.rotation.x = m_camPitch;
    camXf.rotation.y = m_camYaw + 180.0f;

    // --- Determine current area from player position ------------------------
    for (int i = 0; i < static_cast<int>(m_areas.size()); ++i)
    {
        auto& area = m_areas[i];
        if (playerXf.position.x >= area.originX &&
            playerXf.position.x < area.originX + area.sizeX &&
            playerXf.position.z >= area.originZ &&
            playerXf.position.z < area.originZ + area.sizeZ)
        {
            m_currentAreaIndex = i;
            break;
        }
    }
}

// ---------------------------------------------------------------------------
// Area streaming — load/unload based on distance to player
// ---------------------------------------------------------------------------

void WorldStreamingModule::UpdateAreaStreaming()
{
    auto* world = m_ctx->GetWorld();
    auto& playerXf = world->GetComponent<Transform>(m_player);

    for (int i = 0; i < static_cast<int>(m_areas.size()); ++i)
    {
        auto& area = m_areas[i];

        // Distance from player to area centre
        float areaCX = area.originX + area.sizeX * 0.5f;
        float areaCZ = area.originZ + area.sizeZ * 0.5f;
        float dx = playerXf.position.x - areaCX;
        float dz = playerXf.position.z - areaCZ;
        float dist = std::sqrt(dx * dx + dz * dz);

        if (!area.loaded && dist < kLoadDistance)
        {
            LoadArea(i);
        }
        else if (area.loaded && dist > kUnloadDistance)
        {
            UnloadArea(i);
        }
    }
}

// ---------------------------------------------------------------------------
// Origin rebasing
// ---------------------------------------------------------------------------

void WorldStreamingModule::UpdateOriginRebasing()
{
    auto* world = m_ctx->GetWorld();
    auto& playerXf = world->GetComponent<Transform>(m_player);

    float distFromOrigin = std::sqrt(
        playerXf.position.x * playerXf.position.x +
        playerXf.position.z * playerXf.position.z);

    if (distFromOrigin > kOriginThreshold)
    {
        // Store the shift amount
        float shiftX = playerXf.position.x;
        float shiftZ = playerXf.position.z;

        m_originOffset.x += shiftX;
        m_originOffset.z += shiftZ;

        // Shift all entities back toward origin
        // In a real engine, WorldOriginSystem::RebaseOrigin() does this.
        // Here we simulate it by adjusting transforms.

        // Shift player to origin area
        playerXf.position.x -= shiftX;
        playerXf.position.z -= shiftZ;

        // Shift camera
        auto& camXf = world->GetComponent<Transform>(m_camera);
        camXf.position.x -= shiftX;
        camXf.position.z -= shiftZ;

        // Shift all area origins and their entities
        for (auto& area : m_areas)
        {
            area.originX -= shiftX;
            area.originZ -= shiftZ;

            for (auto e : area.entities)
            {
                if (e == entt::null) continue;
                auto& xf = world->GetComponent<Transform>(e);
                xf.position.x -= shiftX;
                xf.position.z -= shiftZ;
            }
        }

        m_originSystem.RebaseOrigin({ shiftX, 0.0f, shiftZ });
    }
}

// ---------------------------------------------------------------------------
// Teleport
// ---------------------------------------------------------------------------

void WorldStreamingModule::TeleportToArea(int index)
{
    if (index < 0 || index >= static_cast<int>(m_areas.size())) return;

    auto* world = m_ctx->GetWorld();
    auto& area = m_areas[index];

    auto& playerXf = world->GetComponent<Transform>(m_player);
    playerXf.position.x = area.originX + area.sizeX * 0.5f;
    playerXf.position.z = area.originZ + area.sizeZ * 0.5f;
    playerXf.position.y = 1.0f;

    // Trigger transition effect
    m_isTransitioning = true;
    m_transitionTimer = 1.0f;
    m_transitionProgress = 1.0f;

    // Force streaming update
    UpdateAreaStreaming();
}

// ---------------------------------------------------------------------------
// Render
// ---------------------------------------------------------------------------

void WorldStreamingModule::OnRender()
{
    auto* dd = Spark::DebugDraw::GetInstance();
    if (!dd) return;

    if (m_showBoundaries)
        DrawAreaBoundaries();

    if (m_showOriginInfo)
        DrawOriginMarker();

    DrawOverlay();

    if (m_isTransitioning)
        DrawLoadingBar();
}

// ---------------------------------------------------------------------------
// Draw area boundaries as wireframe boxes
// ---------------------------------------------------------------------------

void WorldStreamingModule::DrawAreaBoundaries()
{
    auto* dd = Spark::DebugDraw::GetInstance();
    if (!dd) return;

    for (auto& area : m_areas)
    {
        float x0 = area.originX;
        float z0 = area.originZ;
        float x1 = area.originX + area.sizeX;
        float z1 = area.originZ + area.sizeZ;
        float y0 = 0.0f;
        float y1 = 20.0f;

        DirectX::XMFLOAT4 color = { area.colorR, area.colorG, area.colorB,
                                      area.loaded ? 0.8f : 0.3f };

        // Bottom face
        dd->Line({ x0, y0, z0 }, { x1, y0, z0 }, color);
        dd->Line({ x1, y0, z0 }, { x1, y0, z1 }, color);
        dd->Line({ x1, y0, z1 }, { x0, y0, z1 }, color);
        dd->Line({ x0, y0, z1 }, { x0, y0, z0 }, color);

        // Top face
        dd->Line({ x0, y1, z0 }, { x1, y1, z0 }, color);
        dd->Line({ x1, y1, z0 }, { x1, y1, z1 }, color);
        dd->Line({ x1, y1, z1 }, { x0, y1, z1 }, color);
        dd->Line({ x0, y1, z1 }, { x0, y1, z0 }, color);

        // Vertical edges
        dd->Line({ x0, y0, z0 }, { x0, y1, z0 }, color);
        dd->Line({ x1, y0, z0 }, { x1, y1, z0 }, color);
        dd->Line({ x1, y0, z1 }, { x1, y1, z1 }, color);
        dd->Line({ x0, y0, z1 }, { x0, y1, z1 }, color);
    }
}

// ---------------------------------------------------------------------------
// Draw origin marker
// ---------------------------------------------------------------------------

void WorldStreamingModule::DrawOriginMarker()
{
    auto* dd = Spark::DebugDraw::GetInstance();
    if (!dd) return;

    DirectX::XMFLOAT4 red   = { 1.0f, 0.0f, 0.0f, 1.0f };
    DirectX::XMFLOAT4 green = { 0.0f, 1.0f, 0.0f, 1.0f };
    DirectX::XMFLOAT4 blue  = { 0.0f, 0.0f, 1.0f, 1.0f };

    // Axis lines at world origin
    dd->Line({ -5.0f, 0.1f, 0.0f }, { 5.0f, 0.1f, 0.0f }, red);
    dd->Line({ 0.0f, 0.1f, 0.0f }, { 0.0f, 5.0f, 0.0f }, green);
    dd->Line({ 0.0f, 0.1f, -5.0f }, { 0.0f, 0.1f, 5.0f }, blue);
}

// ---------------------------------------------------------------------------
// HUD overlay
// ---------------------------------------------------------------------------

void WorldStreamingModule::DrawOverlay()
{
    auto* dd = Spark::DebugDraw::GetInstance();
    auto* world = m_ctx->GetWorld();
    if (!dd || !world) return;

    char buf[256];
    float x = 10.0f;
    float y = 10.0f;

    dd->ScreenText({ x, y }, "WORLD STREAMING DEMO", { 1.0f, 1.0f, 1.0f, 1.0f });
    y += 22.0f;

    // Current area
    if (m_currentAreaIndex >= 0 && m_currentAreaIndex < static_cast<int>(m_areas.size()))
    {
        auto& area = m_areas[m_currentAreaIndex];
        std::snprintf(buf, sizeof(buf), "Current Area: %s", area.name.c_str());
        dd->ScreenText({ x, y }, buf, { area.colorR, area.colorG, area.colorB, 1.0f });
    }
    y += 18.0f;

    // Loaded areas count
    int loadedCount = 0;
    for (auto& area : m_areas)
        if (area.loaded) loadedCount++;

    std::snprintf(buf, sizeof(buf), "Loaded Areas: %d / %d", loadedCount,
                  static_cast<int>(m_areas.size()));
    dd->ScreenText({ x, y }, buf, { 0.8f, 0.8f, 0.8f, 1.0f });
    y += 18.0f;

    // Player position
    auto& playerXf = world->GetComponent<Transform>(m_player);
    std::snprintf(buf, sizeof(buf), "Player Pos: (%.1f, %.1f, %.1f)",
                  playerXf.position.x, playerXf.position.y, playerXf.position.z);
    dd->ScreenText({ x, y }, buf, { 0.7f, 0.7f, 0.7f, 1.0f });
    y += 18.0f;

    // Origin offset
    if (m_showOriginInfo)
    {
        std::snprintf(buf, sizeof(buf), "Origin Offset: (%.1f, %.1f, %.1f)",
                      m_originOffset.x, m_originOffset.y, m_originOffset.z);
        dd->ScreenText({ x, y }, buf, { 0.6f, 0.8f, 1.0f, 1.0f });
        y += 18.0f;

        float distFromOrigin = std::sqrt(
            playerXf.position.x * playerXf.position.x +
            playerXf.position.z * playerXf.position.z);
        std::snprintf(buf, sizeof(buf), "Dist From Origin: %.1f (threshold: %.0f)",
                      distFromOrigin, kOriginThreshold);
        dd->ScreenText({ x, y }, buf, { 0.6f, 0.8f, 1.0f, 1.0f });
        y += 18.0f;
    }

    // Area list
    y += 8.0f;
    dd->ScreenText({ x, y }, "Areas:", { 0.6f, 0.6f, 0.6f, 1.0f });
    y += 16.0f;

    for (int i = 0; i < static_cast<int>(m_areas.size()); ++i)
    {
        auto& area = m_areas[i];
        std::snprintf(buf, sizeof(buf), " [%d] %s — %s (%zu entities)",
                      i + 1, area.name.c_str(),
                      area.loaded ? "LOADED" : "unloaded",
                      area.entities.size());

        float alpha = area.loaded ? 1.0f : 0.5f;
        bool isCurrent = (i == m_currentAreaIndex);
        dd->ScreenText({ x, y }, buf,
                        { isCurrent ? 1.0f : area.colorR,
                          isCurrent ? 1.0f : area.colorG,
                          isCurrent ? 1.0f : area.colorB,
                          alpha });
        y += 14.0f;
    }

    // Controls
    float helpY = m_screenH - 50.0f;
    dd->ScreenText({ 10.0f, helpY },
                    "WASD: Move | Tab: Teleport | F1: Boundaries | F2: Origin info",
                    { 0.5f, 0.5f, 0.5f, 0.8f });
}

// ---------------------------------------------------------------------------
// Loading progress bar
// ---------------------------------------------------------------------------

void WorldStreamingModule::DrawLoadingBar()
{
    auto* dd = Spark::DebugDraw::GetInstance();
    if (!dd) return;

    float barW = 300.0f;
    float barH = 8.0f;
    float cx = m_screenW * 0.5f;
    float cy = m_screenH * 0.5f;

    // Fade overlay
    dd->ScreenRect({ 0.0f, 0.0f }, { m_screenW, m_screenH },
                    { 0.0f, 0.0f, 0.0f, m_transitionProgress * 0.6f });

    // Progress bar background
    dd->ScreenRect({ cx - barW * 0.5f, cy - barH * 0.5f },
                    { cx + barW * 0.5f, cy + barH * 0.5f },
                    { 0.2f, 0.2f, 0.2f, m_transitionProgress });

    // Progress fill
    float fill = 1.0f - m_transitionProgress; // progress fills as transition completes
    dd->ScreenRect({ cx - barW * 0.5f, cy - barH * 0.5f },
                    { cx - barW * 0.5f + barW * fill, cy + barH * 0.5f },
                    { 0.3f, 0.8f, 1.0f, m_transitionProgress });

    dd->ScreenText({ cx - 30.0f, cy - 20.0f }, "Loading...",
                    { 1.0f, 1.0f, 1.0f, m_transitionProgress });
}

// ---------------------------------------------------------------------------
// Resize
// ---------------------------------------------------------------------------

void WorldStreamingModule::OnResize(int w, int h)
{
    m_screenW = static_cast<float>(w);
    m_screenH = static_cast<float>(h);
}

// ---------------------------------------------------------------------------
// DLL exports
// ---------------------------------------------------------------------------

SPARK_IMPLEMENT_MODULE(WorldStreamingModule)
