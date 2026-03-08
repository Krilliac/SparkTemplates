/**
 * @file ProceduralWorldModule.cpp
 * @brief ADVANCED — Procedural terrain, noise, erosion, WFC, and object scattering.
 */

#include "ProceduralWorldModule.h"
#include <Graphics/MeshLOD.h>
#include <Input/InputManager.h>
#include <string>

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void ProceduralWorldModule::OnLoad(Spark::IEngineContext* ctx)
{
    m_ctx = ctx;

    // Directional sun light
    auto* world = m_ctx->GetWorld();
    m_light = world->CreateEntity();
    world->AddComponent<NameComponent>(m_light, NameComponent{ "Sun" });
    world->AddComponent<Transform>(m_light, Transform{
        { 0.f, 50.f, 0.f }, { -40.f, 30.f, 0.f }, { 1.f, 1.f, 1.f }
    });
    world->AddComponent<LightComponent>(m_light, LightComponent{
        LightComponent::Type::Directional,
        { 1.f, 0.95f, 0.8f }, 2.5f, 0.f,
        0.f, 0.f, true, 2048
    });

    GenerateTerrain();
    ScatterObjects();
    GenerateWFCDungeon();
}

void ProceduralWorldModule::OnUnload()
{
    auto* world = m_ctx ? m_ctx->GetWorld() : nullptr;
    if (!world) { m_ctx = nullptr; return; }

    for (auto e : { m_terrain, m_light })
        if (e != entt::null) world->DestroyEntity(e);

    for (auto e : m_scatteredObjects)
        if (e != entt::null) world->DestroyEntity(e);
    m_scatteredObjects.clear();

    for (auto e : m_wfcTiles)
        if (e != entt::null) world->DestroyEntity(e);
    m_wfcTiles.clear();

    m_ctx = nullptr;
}

// ---------------------------------------------------------------------------
// Terrain generation with noise + erosion
// ---------------------------------------------------------------------------

void ProceduralWorldModule::GenerateTerrain()
{
    auto* world = m_ctx->GetWorld();

    // Configure heightmap from layered noise
    Spark::HeightmapGenerator heightGen;
    Spark::HeightmapSettings settings{};
    settings.width      = 256;
    settings.height     = 256;
    settings.seed       = m_seed;
    settings.octaves    = m_octaves;
    settings.frequency  = m_frequency;
    settings.amplitude  = m_amplitude;

    auto heightmap = heightGen.Generate(settings);

    // Apply erosion passes for realism
    if (m_eroded)
    {
        heightGen.ThermalErosion(heightmap, 30);   // 30 iterations
        heightGen.HydraulicErosion(heightmap, 50); // 50 rain-drop iterations
        heightGen.Normalize(heightmap);
    }

    // Create terrain mesh from the heightmap
    auto terrainMesh = Spark::ProceduralMesh::CreateTerrainFromHeightmap(
        heightmap, 256, 256, 1.0f, m_amplitude);

    // Register as an entity
    m_terrain = world->CreateEntity();
    world->AddComponent<NameComponent>(m_terrain, NameComponent{ "ProceduralTerrain" });
    world->AddComponent<Transform>(m_terrain, Transform{
        { -128.f, 0.f, -128.f }, { 0.f, 0.f, 0.f }, { 1.f, 1.f, 1.f }
    });
    world->AddComponent<MeshRenderer>(m_terrain, MeshRenderer{
        "procedural://terrain", "Assets/Materials/grass.mat",
        true, true, true
    });

    // Set up LOD for the terrain
    if (auto* lodMgr = Spark::LODManager::GetInstance())
    {
        lodMgr->GenerateLODChain(m_terrain, 4); // 4 LOD levels
        lodMgr->SetLODBias(m_terrain, 1.0f);
    }
}

// ---------------------------------------------------------------------------
// Object scattering with rules
// ---------------------------------------------------------------------------

void ProceduralWorldModule::ScatterObjects()
{
    auto* world = m_ctx->GetWorld();

    Spark::ObjectPlacer placer;

    // Rule 1: Trees on gentle slopes at mid-heights
    Spark::PlacementRule treeRule{};
    treeRule.meshPath    = "procedural://tree";
    treeRule.material    = "Assets/Materials/tree_bark.mat";
    treeRule.density     = 0.15f;
    treeRule.minSlope    = 0.0f;
    treeRule.maxSlope    = 30.0f;   // degrees
    treeRule.minHeight   = 5.0f;
    treeRule.maxHeight   = 22.0f;
    treeRule.alignToSurface = true;
    treeRule.randomScale = { 0.8f, 1.3f };

    auto treePositions = placer.Scatter(treeRule, m_seed);
    for (auto& pos : treePositions)
    {
        auto e = world->CreateEntity();
        world->AddComponent<NameComponent>(e, NameComponent{ "Tree" });
        world->AddComponent<Transform>(e, Transform{
            pos.position, pos.rotation, pos.scale
        });
        world->AddComponent<MeshRenderer>(e, MeshRenderer{
            treeRule.meshPath, treeRule.material, true, true, true
        });
        m_scatteredObjects.push_back(e);
    }

    // Rule 2: Rocks on steep slopes
    Spark::PlacementRule rockRule{};
    rockRule.meshPath    = "procedural://rock";
    rockRule.material    = "Assets/Materials/rock.mat";
    rockRule.density     = 0.08f;
    rockRule.minSlope    = 20.0f;
    rockRule.maxSlope    = 70.0f;
    rockRule.minHeight   = 0.0f;
    rockRule.maxHeight   = 25.0f;
    rockRule.alignToSurface = true;
    rockRule.randomScale = { 0.5f, 2.0f };

    auto rockPositions = placer.Scatter(rockRule, m_seed + 1);
    for (auto& pos : rockPositions)
    {
        auto e = world->CreateEntity();
        world->AddComponent<NameComponent>(e, NameComponent{ "Rock" });
        world->AddComponent<Transform>(e, Transform{
            pos.position, pos.rotation, pos.scale
        });
        world->AddComponent<MeshRenderer>(e, MeshRenderer{
            rockRule.meshPath, rockRule.material, true, true, true
        });
        m_scatteredObjects.push_back(e);
    }
}

// ---------------------------------------------------------------------------
// Wave Function Collapse dungeon
// ---------------------------------------------------------------------------

void ProceduralWorldModule::GenerateWFCDungeon()
{
    auto* world = m_ctx->GetWorld();

    Spark::WaveFunctionCollapse wfc;
    wfc.SetGridSize(16, 16);

    // Define tile types with socket compatibility
    wfc.AddTile("floor",   1.0f, { "open", "open", "open", "open" });
    wfc.AddTile("wall_ns", 0.5f, { "wall", "open", "wall", "open" });
    wfc.AddTile("wall_ew", 0.5f, { "open", "wall", "open", "wall" });
    wfc.AddTile("corner",  0.3f, { "wall", "wall", "open", "open" });
    wfc.AddTile("pillar",  0.1f, { "wall", "wall", "wall", "wall" });

    wfc.EnableRotation(true);
    wfc.Collapse(m_seed);

    auto grid = wfc.GetResult();

    // Place tiles in the world offset from the main terrain
    float offsetX = 60.0f; // separate from terrain
    float tileSize = 2.0f;

    for (int y = 0; y < 16; ++y)
    {
        for (int x = 0; x < 16; ++x)
        {
            auto& tile = grid[y * 16 + x];
            auto e = world->CreateEntity();

            std::string tileMesh = "Assets/Models/cube.obj";
            std::string tileMat  = "Assets/Materials/stone.mat";
            float height = 0.1f;

            if (tile.name == "wall_ns" || tile.name == "wall_ew")
            {
                height = 3.0f;
                tileMat = "Assets/Materials/brick.mat";
            }
            else if (tile.name == "corner")
            {
                height = 3.0f;
                tileMat = "Assets/Materials/brick_dark.mat";
            }
            else if (tile.name == "pillar")
            {
                height = 4.0f;
                tileMat = "Assets/Materials/marble.mat";
            }

            world->AddComponent<NameComponent>(e, NameComponent{ "WFC_Tile" });
            world->AddComponent<Transform>(e, Transform{
                { offsetX + x * tileSize, height * 0.5f, y * tileSize },
                { 0.f, tile.rotation, 0.f },
                { tileSize, height, tileSize }
            });
            world->AddComponent<MeshRenderer>(e, MeshRenderer{
                tileMesh, tileMat, true, true, true
            });

            m_wfcTiles.push_back(e);
        }
    }
}

// ---------------------------------------------------------------------------
// Regenerate everything
// ---------------------------------------------------------------------------

void ProceduralWorldModule::RegenerateAll()
{
    auto* world = m_ctx->GetWorld();

    // Clean up existing
    if (m_terrain != entt::null)
    {
        world->DestroyEntity(m_terrain);
        m_terrain = entt::null;
    }

    for (auto e : m_scatteredObjects)
        if (e != entt::null) world->DestroyEntity(e);
    m_scatteredObjects.clear();

    for (auto e : m_wfcTiles)
        if (e != entt::null) world->DestroyEntity(e);
    m_wfcTiles.clear();

    // Regenerate with new seed
    m_seed++;
    GenerateTerrain();
    ScatterObjects();
    GenerateWFCDungeon();
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

void ProceduralWorldModule::OnUpdate(float dt)
{
    (void)dt;
    auto* input = m_ctx->GetInput();
    if (!input) return;

    // G — Regenerate the entire world with a new seed
    if (input->WasKeyPressed('G'))
        RegenerateAll();

    // E — Toggle erosion and regenerate terrain
    if (input->WasKeyPressed('E'))
    {
        m_eroded = !m_eroded;
        auto* world = m_ctx->GetWorld();
        if (m_terrain != entt::null)
        {
            world->DestroyEntity(m_terrain);
            m_terrain = entt::null;
        }
        GenerateTerrain();
    }

    // + / - — Adjust octave count
    if (input->WasKeyPressed(VK_OEM_PLUS) && m_octaves < 10)
    {
        m_octaves++;
        auto* world = m_ctx->GetWorld();
        if (m_terrain != entt::null)
        {
            world->DestroyEntity(m_terrain);
            m_terrain = entt::null;
        }
        GenerateTerrain();
    }
    if (input->WasKeyPressed(VK_OEM_MINUS) && m_octaves > 1)
    {
        m_octaves--;
        auto* world = m_ctx->GetWorld();
        if (m_terrain != entt::null)
        {
            world->DestroyEntity(m_terrain);
            m_terrain = entt::null;
        }
        GenerateTerrain();
    }
}

// ---------------------------------------------------------------------------
void ProceduralWorldModule::OnRender() { }
void ProceduralWorldModule::OnResize(int, int) { }

SPARK_IMPLEMENT_MODULE(ProceduralWorldModule)
