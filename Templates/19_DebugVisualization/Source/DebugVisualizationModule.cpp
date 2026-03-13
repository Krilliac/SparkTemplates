/**
 * @file DebugVisualizationModule.cpp
 * @brief ADVANCED — Debug and development tool showcase.
 */

#include "DebugVisualizationModule.h"
#include <Input/InputManager.h>
#include <cmath>
#include <algorithm>

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void DebugVisualizationModule::OnLoad(Spark::IEngineContext* ctx)
{
    m_ctx = ctx;
    BuildScene();
    SetupConsoleCommands();
}

void DebugVisualizationModule::OnUnload()
{
    auto* world = m_ctx ? m_ctx->GetWorld() : nullptr;
    if (!world) { m_ctx = nullptr; return; }

    for (auto e : { m_camera, m_light })
        if (e != entt::null) world->DestroyEntity(e);

    for (auto e : m_sceneEntities)
        if (e != entt::null) world->DestroyEntity(e);
    for (auto e : m_aiAgents)
        if (e != entt::null) world->DestroyEntity(e);

    m_sceneEntities.clear();
    m_aiAgents.clear();
    m_ctx = nullptr;
}

// ---------------------------------------------------------------------------
// Scene construction
// ---------------------------------------------------------------------------

void DebugVisualizationModule::BuildScene()
{
    auto* world = m_ctx->GetWorld();

    // --- Camera -------------------------------------------------------------
    m_camera = world->CreateEntity();
    world->AddComponent<NameComponent>(m_camera, NameComponent{ "DebugCamera" });
    world->AddComponent<Transform>(m_camera, Transform{
        { 0.0f, 10.0f, -25.0f },
        { -30.0f, 0.0f, 0.0f },
        { 1.0f, 1.0f, 1.0f }
    });
    world->AddComponent<Camera>(m_camera, Camera{
        60.0f, 0.1f, 200.0f, true
    });

    // --- Directional light --------------------------------------------------
    m_light = world->CreateEntity();
    world->AddComponent<NameComponent>(m_light, NameComponent{ "SunLight" });
    world->AddComponent<Transform>(m_light, Transform{
        { 0.0f, 20.0f, 0.0f },
        { -50.0f, 30.0f, 0.0f },
        { 1.0f, 1.0f, 1.0f }
    });
    world->AddComponent<LightComponent>(m_light, LightComponent{
        LightComponent::Type::Directional,
        { 1.0f, 0.95f, 0.85f }, 2.0f, 0.0f,
        0.0f, 0.0f, true, 2048
    });

    // --- Ground plane -------------------------------------------------------
    auto ground = world->CreateEntity();
    world->AddComponent<NameComponent>(ground, NameComponent{ "Ground" });
    world->AddComponent<Transform>(ground, Transform{
        { 0.0f, -0.1f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 60.0f, 0.2f, 60.0f }
    });
    world->AddComponent<MeshRenderer>(ground, MeshRenderer{
        "Assets/Models/cube.obj", "Assets/Materials/grid_floor.mat",
        false, true, true
    });
    world->AddComponent<RigidBodyComponent>(ground, RigidBodyComponent{
        RigidBodyComponent::Type::Static, 0.0f, 0.8f, 0.2f
    });
    world->AddComponent<ColliderComponent>(ground, ColliderComponent{
        ColliderComponent::Shape::Box, { 30.0f, 0.1f, 30.0f }
    });
    m_sceneEntities.push_back(ground);

    // --- Various geometry for debug inspection ------------------------------
    // Cubes of different sizes
    struct ObjDef { const char* name; float x, y, z; float sx, sy, sz; const char* mat; };
    ObjDef objs[] = {
        { "RedCube",     -6.0f, 1.0f,  4.0f,  2.0f, 2.0f, 2.0f, "Assets/Materials/red.mat" },
        { "GreenCube",    0.0f, 1.5f,  4.0f,  3.0f, 3.0f, 3.0f, "Assets/Materials/green.mat" },
        { "BlueCube",     6.0f, 0.75f, 4.0f,  1.5f, 1.5f, 1.5f, "Assets/Materials/blue.mat" },
        { "TallPillar",  -4.0f, 3.0f, -3.0f,  0.8f, 6.0f, 0.8f, "Assets/Materials/concrete.mat" },
        { "WidePlank",    4.0f, 0.5f, -3.0f,  5.0f, 0.3f, 1.0f, "Assets/Materials/wood.mat" },
        { "Sphere_A",    -8.0f, 1.0f,  0.0f,  2.0f, 2.0f, 2.0f, "Assets/Materials/chrome.mat" },
        { "Sphere_B",     8.0f, 1.5f,  0.0f,  3.0f, 3.0f, 3.0f, "Assets/Materials/gold.mat" },
    };

    for (auto& def : objs)
    {
        auto e = world->CreateEntity();
        world->AddComponent<NameComponent>(e, NameComponent{ def.name });
        world->AddComponent<Transform>(e, Transform{
            { def.x, def.y, def.z },
            { 0.0f, 0.0f, 0.0f },
            { def.sx, def.sy, def.sz }
        });

        bool isSphere = (std::string(def.name).find("Sphere") != std::string::npos);
        world->AddComponent<MeshRenderer>(e, MeshRenderer{
            isSphere ? "Assets/Models/sphere.obj" : "Assets/Models/cube.obj",
            def.mat, true, true, true
        });

        // Physics colliders for raycast testing
        world->AddComponent<RigidBodyComponent>(e, RigidBodyComponent{
            RigidBodyComponent::Type::Static, 0.0f, 0.6f, 0.2f
        });
        if (isSphere)
        {
            world->AddComponent<ColliderComponent>(e, ColliderComponent{
                ColliderComponent::Shape::Sphere, { def.sx * 0.5f, 0.0f, 0.0f }
            });
        }
        else
        {
            world->AddComponent<ColliderComponent>(e, ColliderComponent{
                ColliderComponent::Shape::Box,
                { def.sx * 0.5f, def.sy * 0.5f, def.sz * 0.5f }
            });
        }

        m_sceneEntities.push_back(e);
    }

    // --- AI agents for perception visualisation ----------------------------
    struct AgentDef { const char* name; float x, z; float detectionRange; float fovDeg; };
    AgentDef agents[] = {
        { "Guard_A",  -10.0f, 8.0f,  15.0f, 60.0f },
        { "Guard_B",   10.0f, 8.0f,  12.0f, 90.0f },
        { "Sentry",     0.0f, 12.0f, 20.0f, 45.0f },
    };

    for (auto& def : agents)
    {
        auto e = world->CreateEntity();
        world->AddComponent<NameComponent>(e, NameComponent{ def.name });
        world->AddComponent<Transform>(e, Transform{
            { def.x, 1.0f, def.z },
            { 0.0f, 0.0f, 0.0f },
            { 1.0f, 2.0f, 1.0f }
        });
        world->AddComponent<MeshRenderer>(e, MeshRenderer{
            "Assets/Models/cube.obj", "Assets/Materials/enemy_red.mat",
            true, true, true
        });
        world->AddComponent<AIComponent>(e, AIComponent{
            AIComponent::State::Idle,
            "guard_behavior",
            entt::null,          // no target
            { 0.0f, 0.0f, 0.0f },
            {},
            { def.detectionRange, 5.0f, 3.0f, 0.5f, 0.3f }
        });
        world->AddComponent<TagComponent>(e);
        world->GetComponent<TagComponent>(e).AddTag("ai_agent");

        m_aiAgents.push_back(e);
    }
}

// ---------------------------------------------------------------------------
// Console commands
// ---------------------------------------------------------------------------

void DebugVisualizationModule::SetupConsoleCommands()
{
    m_console.Initialize(m_ctx);

    m_console.RegisterCommand("help", "Show available commands",
        [this](const std::vector<std::string>& /*args*/) {
            m_console.Print("Available commands:");
            m_console.Print("  help           - Show this message");
            m_console.Print("  wireframe      - Toggle wireframe mode");
            m_console.Print("  physics        - Toggle physics debug");
            m_console.Print("  navmesh        - Toggle NavMesh debug");
            m_console.Print("  grid           - Toggle world grid");
            m_console.Print("  stats          - Toggle performance stats");
            m_console.Print("  bbox           - Toggle bounding boxes");
            m_console.Print("  frustum        - Toggle frustum visualisation");
            m_console.Print("  spawn <name>   - Spawn a debug cube");
            m_console.Print("  clear          - Clear console output");
            m_console.Print("  entity_count   - Print current entity count");
        });

    m_console.RegisterCommand("wireframe", "Toggle wireframe rendering",
        [this](const std::vector<std::string>& /*args*/) {
            m_wireframe = !m_wireframe;
            if (auto* gfx = m_ctx->GetGraphics())
                gfx->SetWireframe(m_wireframe);
            m_console.Print(m_wireframe ? "Wireframe: ON" : "Wireframe: OFF");
        });

    m_console.RegisterCommand("physics", "Toggle physics debug",
        [this](const std::vector<std::string>& /*args*/) {
            m_physicsDebug = !m_physicsDebug;
            if (auto* gfx = m_ctx->GetGraphics())
                gfx->TogglePhysicsDebug(m_physicsDebug);
            m_console.Print(m_physicsDebug ? "Physics debug: ON" : "Physics debug: OFF");
        });

    m_console.RegisterCommand("navmesh", "Toggle NavMesh debug",
        [this](const std::vector<std::string>& /*args*/) {
            m_navMeshDebug = !m_navMeshDebug;
            if (auto* gfx = m_ctx->GetGraphics())
                gfx->ToggleNavMeshDebug(m_navMeshDebug);
            m_console.Print(m_navMeshDebug ? "NavMesh debug: ON" : "NavMesh debug: OFF");
        });

    m_console.RegisterCommand("grid", "Toggle world grid",
        [this](const std::vector<std::string>& /*args*/) {
            m_showGrid = !m_showGrid;
            m_console.Print(m_showGrid ? "Grid: ON" : "Grid: OFF");
        });

    m_console.RegisterCommand("stats", "Toggle performance stats",
        [this](const std::vector<std::string>& /*args*/) {
            m_showPerfStats = !m_showPerfStats;
            m_console.Print(m_showPerfStats ? "Stats: ON" : "Stats: OFF");
        });

    m_console.RegisterCommand("bbox", "Toggle bounding boxes",
        [this](const std::vector<std::string>& /*args*/) {
            m_showBBoxes = !m_showBBoxes;
            m_console.Print(m_showBBoxes ? "Bounding boxes: ON" : "Bounding boxes: OFF");
        });

    m_console.RegisterCommand("frustum", "Toggle frustum visualisation",
        [this](const std::vector<std::string>& /*args*/) {
            m_showFrustum = !m_showFrustum;
            m_console.Print(m_showFrustum ? "Frustum: ON" : "Frustum: OFF");
        });

    m_console.RegisterCommand("spawn", "Spawn a debug cube at origin",
        [this](const std::vector<std::string>& args) {
            auto* world = m_ctx->GetWorld();
            std::string name = args.size() > 1 ? args[1] : "DebugCube";
            auto e = world->CreateEntity();
            world->AddComponent<NameComponent>(e, NameComponent{ name });
            world->AddComponent<Transform>(e, Transform{
                { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }
            });
            world->AddComponent<MeshRenderer>(e, MeshRenderer{
                "Assets/Models/cube.obj", "Assets/Materials/default.mat",
                true, true, true
            });
            m_sceneEntities.push_back(e);
            m_console.Print("Spawned entity: " + name);
        });

    m_console.RegisterCommand("entity_count", "Print entity count",
        [this](const std::vector<std::string>& /*args*/) {
            m_console.Print("Entities: " + std::to_string(m_entityCount));
        });

    m_console.RegisterCommand("clear", "Clear console output",
        [this](const std::vector<std::string>& /*args*/) {
            m_console.Clear();
        });
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

void DebugVisualizationModule::OnUpdate(float dt)
{
    auto* input = m_ctx->GetInput();
    auto* world = m_ctx->GetWorld();
    if (!input || !world) return;

    // --- FPS calculation ----------------------------------------------------
    m_fpsTimer += dt;
    m_frameCount++;
    if (m_fpsTimer >= 0.5f)
    {
        m_fps = static_cast<float>(m_frameCount) / m_fpsTimer;
        m_frameTime = m_fpsTimer / static_cast<float>(m_frameCount) * 1000.0f;
        m_fpsTimer = 0.0f;
        m_frameCount = 0;
    }

    // Cache render stats
    if (auto* gfx = m_ctx->GetGraphics())
    {
        auto stats = gfx->GetRenderStats();
        m_drawCalls  = stats.drawCalls;
        m_triangles  = stats.triangles;
        m_entityCount = stats.entityCount;
    }

    // --- Toggle keys --------------------------------------------------------
    if (input->WasKeyPressed(VK_F1))
        m_showDebugOverlay = !m_showDebugOverlay;

    if (input->WasKeyPressed(VK_F2))
    {
        m_wireframe = !m_wireframe;
        if (auto* gfx = m_ctx->GetGraphics())
            gfx->SetWireframe(m_wireframe);
    }

    if (input->WasKeyPressed(VK_F3))
    {
        m_physicsDebug = !m_physicsDebug;
        if (auto* gfx = m_ctx->GetGraphics())
            gfx->TogglePhysicsDebug(m_physicsDebug);
    }

    if (input->WasKeyPressed(VK_F4))
    {
        m_navMeshDebug = !m_navMeshDebug;
        if (auto* gfx = m_ctx->GetGraphics())
            gfx->ToggleNavMeshDebug(m_navMeshDebug);
    }

    if (input->WasKeyPressed(VK_F5))
        m_showPerfStats = !m_showPerfStats;

    if (input->WasKeyPressed('G'))
        m_showGrid = !m_showGrid;

    if (input->WasKeyPressed('C'))
    {
        m_consoleOpen = !m_consoleOpen;
        m_console.SetVisible(m_consoleOpen);
    }

    // --- Left-click raycast / select entity ---------------------------------
    if (input->WasMouseButtonPressed(0))
        PerformRaycast();

    // Decay ray visualisation timer
    if (m_rayVisTimer > 0.0f)
        m_rayVisTimer -= dt;

    // --- Console input (when open) ------------------------------------------
    if (m_consoleOpen)
    {
        m_console.Update(dt);
        return; // swallow input while console is open
    }

    // --- Camera orbit -------------------------------------------------------
    if (input->IsMouseButtonDown(1))
    {
        auto delta = input->GetMouseDelta();
        m_camYaw   += delta.x * 0.3f;
        m_camPitch -= delta.y * 0.3f;
        m_camPitch  = std::clamp(m_camPitch, -85.0f, 10.0f);
    }

    float scroll = input->GetMouseScrollDelta();
    m_camDist -= scroll * 1.5f;
    m_camDist  = std::clamp(m_camDist, 5.0f, 80.0f);

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
// Raycast
// ---------------------------------------------------------------------------

void DebugVisualizationModule::PerformRaycast()
{
    auto* world = m_ctx->GetWorld();
    auto* input = m_ctx->GetInput();
    if (!world || !input) return;

    // Get camera position and forward direction
    auto& camXf = world->GetComponent<Transform>(m_camera);

    // Screen-centre ray (simplified — in a real setup you'd unproject mouse pos)
    float radY = (m_camYaw + 180.0f) * (3.14159f / 180.0f);
    float radP = m_camPitch * (3.14159f / 180.0f);

    float dirX = std::cos(radP) * std::sin(radY);
    float dirY = std::sin(radP);
    float dirZ = std::cos(radP) * std::cos(radY);

    // Normalise
    float len = std::sqrt(dirX * dirX + dirY * dirY + dirZ * dirZ);
    if (len > 0.001f) { dirX /= len; dirY /= len; dirZ /= len; }

    Spark::RaycastHit hit;
    if (world->Raycast(
            { camXf.position.x, camXf.position.y, camXf.position.z },
            { dirX, dirY, dirZ },
            100.0f,
            hit))
    {
        m_rayHit      = true;
        m_rayHitX     = hit.point.x;
        m_rayHitY     = hit.point.y;
        m_rayHitZ     = hit.point.z;
        m_rayNormalX  = hit.normal.x;
        m_rayNormalY  = hit.normal.y;
        m_rayNormalZ  = hit.normal.z;
        m_rayVisTimer = 3.0f;  // show hit for 3 seconds

        // Select the hit entity for the inspector
        m_selectedEntity = hit.entity;
        m_inspectorActive = (m_selectedEntity != entt::null);
    }
    else
    {
        m_rayHit = false;
        m_selectedEntity = entt::null;
        m_inspectorActive = false;
    }
}

// ---------------------------------------------------------------------------
// Render (debug draw calls happen here each frame)
// ---------------------------------------------------------------------------

void DebugVisualizationModule::OnRender()
{
    auto* dd = Spark::DebugDraw::GetInstance();
    if (!dd) return;

    if (m_showGrid)
        DrawWorldGrid();

    if (m_showDebugOverlay)
        DrawDebugOverlay();

    if (m_showBBoxes)
        DrawBoundingBoxes();

    if (m_showFrustum)
        DrawFrustum();

    // AI perception cones (always drawn when debug overlay is on)
    if (m_showDebugOverlay)
        DrawAIPerception();

    // Raycast hit visualisation
    if (m_rayHit && m_rayVisTimer > 0.0f)
    {
        // Hit point sphere
        dd->Sphere({ m_rayHitX, m_rayHitY, m_rayHitZ }, 0.15f,
                    { 1.0f, 0.0f, 0.0f, 1.0f });

        // Normal arrow
        dd->Arrow(
            { m_rayHitX, m_rayHitY, m_rayHitZ },
            { m_rayHitX + m_rayNormalX * 1.5f,
              m_rayHitY + m_rayNormalY * 1.5f,
              m_rayHitZ + m_rayNormalZ * 1.5f },
            { 0.0f, 1.0f, 0.0f, 1.0f });

        dd->Text({ m_rayHitX, m_rayHitY + 0.5f, m_rayHitZ },
                 "HIT", { 1.0f, 1.0f, 0.0f, 1.0f });
    }

    // Performance stats overlay
    if (m_showPerfStats)
        DrawPerformanceStats();

    // Entity inspector
    if (m_inspectorActive)
        DrawEntityInspector();

    // Memory stats
    DrawMemoryStats();

    // Console rendering
    if (m_consoleOpen)
        m_console.Render();
}

// ---------------------------------------------------------------------------
// Debug drawing: world grid
// ---------------------------------------------------------------------------

void DebugVisualizationModule::DrawWorldGrid()
{
    auto* dd = Spark::DebugDraw::GetInstance();
    if (!dd) return;

    const float ext = kGridExtent;
    const float step = kGridStep;

    // Draw grid lines on the XZ plane at Y=0
    for (float x = -ext; x <= ext; x += step)
    {
        float alpha = (std::fmod(std::abs(x), 10.0f) < 0.01f) ? 0.6f : 0.25f;
        dd->Line(
            { x, 0.01f, -ext },
            { x, 0.01f,  ext },
            { 0.5f, 0.5f, 0.5f, alpha });
    }
    for (float z = -ext; z <= ext; z += step)
    {
        float alpha = (std::fmod(std::abs(z), 10.0f) < 0.01f) ? 0.6f : 0.25f;
        dd->Line(
            { -ext, 0.01f, z },
            {  ext, 0.01f, z },
            { 0.5f, 0.5f, 0.5f, alpha });
    }

    // Origin axes
    dd->Arrow({ 0.0f, 0.02f, 0.0f }, { 3.0f, 0.02f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f }); // X red
    dd->Arrow({ 0.0f, 0.02f, 0.0f }, { 0.0f, 3.0f,  0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f }); // Y green
    dd->Arrow({ 0.0f, 0.02f, 0.0f }, { 0.0f, 0.02f, 3.0f }, { 0.0f, 0.0f, 1.0f, 1.0f }); // Z blue

    dd->Text({ 3.2f, 0.02f, 0.0f }, "X", { 1.0f, 0.0f, 0.0f, 1.0f });
    dd->Text({ 0.0f, 3.2f,  0.0f }, "Y", { 0.0f, 1.0f, 0.0f, 1.0f });
    dd->Text({ 0.0f, 0.02f, 3.2f }, "Z", { 0.0f, 0.0f, 1.0f, 1.0f });
}

// ---------------------------------------------------------------------------
// Debug overlay: per-entity origin markers
// ---------------------------------------------------------------------------

void DebugVisualizationModule::DrawDebugOverlay()
{
    auto* dd = Spark::DebugDraw::GetInstance();
    auto* world = m_ctx->GetWorld();
    if (!dd || !world) return;

    // Draw small axes at each entity origin
    for (auto e : m_sceneEntities)
    {
        if (e == entt::null) continue;
        if (!world->HasComponent<Transform>(e)) continue;
        auto& xf = world->GetComponent<Transform>(e);
        float s = 0.5f;

        dd->Line(xf.position, { xf.position.x + s, xf.position.y, xf.position.z },
                 { 1.0f, 0.3f, 0.3f, 0.7f });
        dd->Line(xf.position, { xf.position.x, xf.position.y + s, xf.position.z },
                 { 0.3f, 1.0f, 0.3f, 0.7f });
        dd->Line(xf.position, { xf.position.x, xf.position.y, xf.position.z + s },
                 { 0.3f, 0.3f, 1.0f, 0.7f });
    }
}

// ---------------------------------------------------------------------------
// Bounding boxes for all MeshRenderer entities
// ---------------------------------------------------------------------------

void DebugVisualizationModule::DrawBoundingBoxes()
{
    auto* dd = Spark::DebugDraw::GetInstance();
    auto* world = m_ctx->GetWorld();
    if (!dd || !world) return;

    for (auto e : m_sceneEntities)
    {
        if (e == entt::null) continue;
        if (!world->HasComponent<Transform>(e)) continue;
        if (!world->HasComponent<MeshRenderer>(e)) continue;

        auto& xf = world->GetComponent<Transform>(e);

        // AABB from Transform (axis-aligned approximation)
        float hw = xf.scale.x * 0.5f;
        float hh = xf.scale.y * 0.5f;
        float hd = xf.scale.z * 0.5f;

        dd->Box(
            { xf.position.x - hw, xf.position.y - hh, xf.position.z - hd },
            { xf.position.x + hw, xf.position.y + hh, xf.position.z + hd },
            { 1.0f, 1.0f, 0.0f, 0.5f });
    }
}

// ---------------------------------------------------------------------------
// Camera frustum visualisation
// ---------------------------------------------------------------------------

void DebugVisualizationModule::DrawFrustum()
{
    auto* dd = Spark::DebugDraw::GetInstance();
    auto* world = m_ctx->GetWorld();
    if (!dd || !world) return;

    if (m_camera == entt::null) return;
    auto& cam  = world->GetComponent<Camera>(m_camera);
    auto& xf   = world->GetComponent<Transform>(m_camera);

    float nearDist = cam.nearPlane;
    float farDist  = std::min(cam.farPlane, 50.0f); // clamp for visibility
    float fovRad   = cam.fov * (3.14159f / 180.0f);
    float aspect   = 16.0f / 9.0f; // assume standard

    float nearH = std::tan(fovRad * 0.5f) * nearDist;
    float nearW = nearH * aspect;
    float farH  = std::tan(fovRad * 0.5f) * farDist;
    float farW  = farH * aspect;

    // Simplified: draw frustum in camera local space then offset by position
    // For brevity, draw axis-aligned frustum at camera position facing forward
    float radY = (m_camYaw + 180.0f) * (3.14159f / 180.0f);
    float fwdX = std::sin(radY);
    float fwdZ = std::cos(radY);

    // Near plane corners (simplified 2D projection)
    auto nearCenter = [&](float d) -> Spark::DebugDraw::Vec3 {
        return { xf.position.x + fwdX * d,
                 xf.position.y,
                 xf.position.z + fwdZ * d };
    };

    auto nc = nearCenter(nearDist);
    auto fc = nearCenter(farDist);

    // Draw frustum as lines from near to far corners
    dd->Line(nc, fc, { 0.0f, 1.0f, 1.0f, 0.6f });

    // Near plane box
    dd->Box(
        { nc.x - nearW, nc.y - nearH, nc.z - nearW },
        { nc.x + nearW, nc.y + nearH, nc.z + nearW },
        { 0.0f, 1.0f, 1.0f, 0.4f });

    // Far plane box
    dd->Box(
        { fc.x - farW, fc.y - farH, fc.z - farW },
        { fc.x + farW, fc.y + farH, fc.z + farW },
        { 0.0f, 1.0f, 1.0f, 0.3f });
}

// ---------------------------------------------------------------------------
// AI perception cones
// ---------------------------------------------------------------------------

void DebugVisualizationModule::DrawAIPerception()
{
    auto* dd = Spark::DebugDraw::GetInstance();
    auto* world = m_ctx->GetWorld();
    if (!dd || !world) return;

    for (auto e : m_aiAgents)
    {
        if (e == entt::null) continue;
        if (!world->HasComponent<AIComponent>(e)) continue;
        if (!world->HasComponent<Transform>(e)) continue;

        auto& xf  = world->GetComponent<Transform>(e);
        auto& ai  = world->GetComponent<AIComponent>(e);

        float sightRange = ai.config.detectionRange;
        float hearRange  = sightRange * 0.5f;

        // Sight cone (facing entity forward, approximated as wireframe cone)
        float fwdX = std::sin(xf.rotation.y * 3.14159f / 180.0f);
        float fwdZ = std::cos(xf.rotation.y * 3.14159f / 180.0f);

        // Draw sight cone with multiple lines
        int segments = 8;
        float halfAngle = 30.0f * (3.14159f / 180.0f);
        for (int i = 0; i < segments; ++i)
        {
            float angle = -halfAngle + (2.0f * halfAngle * static_cast<float>(i) / (segments - 1));
            float dx = std::cos(angle) * fwdX - std::sin(angle) * fwdZ;
            float dz = std::sin(angle) * fwdX + std::cos(angle) * fwdZ;

            dd->Line(
                { xf.position.x, xf.position.y + 1.0f, xf.position.z },
                { xf.position.x + dx * sightRange,
                  xf.position.y + 1.0f,
                  xf.position.z + dz * sightRange },
                { 1.0f, 1.0f, 0.0f, 0.4f });
        }

        // Hearing radius (circle on ground)
        dd->Circle(
            { xf.position.x, 0.05f, xf.position.z },
            hearRange,
            { 0.0f, 0.5f, 1.0f, 0.3f });

        // Label
        dd->Text(
            { xf.position.x, xf.position.y + 2.5f, xf.position.z },
            world->GetComponent<NameComponent>(e).name,
            { 1.0f, 1.0f, 1.0f, 0.8f });
    }
}

// ---------------------------------------------------------------------------
// Performance stats overlay (screen-space text)
// ---------------------------------------------------------------------------

void DebugVisualizationModule::DrawPerformanceStats()
{
    auto* dd = Spark::DebugDraw::GetInstance();
    if (!dd) return;

    // Screen-space text at fixed position
    char buf[256];
    std::snprintf(buf, sizeof(buf), "FPS: %.1f", m_fps);
    dd->ScreenText({ 10.0f, 10.0f }, buf, { 0.0f, 1.0f, 0.0f, 1.0f });

    std::snprintf(buf, sizeof(buf), "Frame: %.2f ms", m_frameTime);
    dd->ScreenText({ 10.0f, 28.0f }, buf, { 0.0f, 1.0f, 0.0f, 1.0f });

    std::snprintf(buf, sizeof(buf), "Draw calls: %d", m_drawCalls);
    dd->ScreenText({ 10.0f, 46.0f }, buf, { 0.8f, 0.8f, 0.8f, 1.0f });

    std::snprintf(buf, sizeof(buf), "Triangles: %d", m_triangles);
    dd->ScreenText({ 10.0f, 64.0f }, buf, { 0.8f, 0.8f, 0.8f, 1.0f });

    std::snprintf(buf, sizeof(buf), "Entities: %d", m_entityCount);
    dd->ScreenText({ 10.0f, 82.0f }, buf, { 0.8f, 0.8f, 0.8f, 1.0f });

    // Mode indicators
    float y = 110.0f;
    if (m_wireframe)
    {
        dd->ScreenText({ 10.0f, y }, "[WIREFRAME]", { 1.0f, 0.5f, 0.0f, 1.0f });
        y += 18.0f;
    }
    if (m_physicsDebug)
    {
        dd->ScreenText({ 10.0f, y }, "[PHYSICS DEBUG]", { 0.0f, 1.0f, 1.0f, 1.0f });
        y += 18.0f;
    }
    if (m_navMeshDebug)
    {
        dd->ScreenText({ 10.0f, y }, "[NAVMESH DEBUG]", { 0.5f, 1.0f, 0.5f, 1.0f });
        y += 18.0f;
    }
}

// ---------------------------------------------------------------------------
// Entity inspector (for selected entity)
// ---------------------------------------------------------------------------

void DebugVisualizationModule::DrawEntityInspector()
{
    auto* dd = Spark::DebugDraw::GetInstance();
    auto* world = m_ctx->GetWorld();
    if (!dd || !world) return;
    if (m_selectedEntity == entt::null) return;

    float x = 400.0f;
    float y = 10.0f;

    dd->ScreenText({ x, y }, "--- Entity Inspector ---", { 1.0f, 1.0f, 1.0f, 1.0f });
    y += 20.0f;

    // Name
    if (world->HasComponent<NameComponent>(m_selectedEntity))
    {
        auto& nc = world->GetComponent<NameComponent>(m_selectedEntity);
        dd->ScreenText({ x, y }, ("Name: " + std::string(nc.name)).c_str(),
                        { 1.0f, 1.0f, 0.5f, 1.0f });
        y += 18.0f;
    }

    // Transform
    if (world->HasComponent<Transform>(m_selectedEntity))
    {
        auto& xf = world->GetComponent<Transform>(m_selectedEntity);
        char buf[128];

        std::snprintf(buf, sizeof(buf), "Pos: (%.2f, %.2f, %.2f)",
                      xf.position.x, xf.position.y, xf.position.z);
        dd->ScreenText({ x, y }, buf, { 0.8f, 0.8f, 1.0f, 1.0f });
        y += 18.0f;

        std::snprintf(buf, sizeof(buf), "Rot: (%.1f, %.1f, %.1f)",
                      xf.rotation.x, xf.rotation.y, xf.rotation.z);
        dd->ScreenText({ x, y }, buf, { 0.8f, 0.8f, 1.0f, 1.0f });
        y += 18.0f;

        std::snprintf(buf, sizeof(buf), "Scale: (%.2f, %.2f, %.2f)",
                      xf.scale.x, xf.scale.y, xf.scale.z);
        dd->ScreenText({ x, y }, buf, { 0.8f, 0.8f, 1.0f, 1.0f });
        y += 18.0f;

        // Highlight selected entity with a wireframe box
        float hw = xf.scale.x * 0.5f;
        float hh = xf.scale.y * 0.5f;
        float hd = xf.scale.z * 0.5f;
        dd->Box(
            { xf.position.x - hw, xf.position.y - hh, xf.position.z - hd },
            { xf.position.x + hw, xf.position.y + hh, xf.position.z + hd },
            { 0.0f, 1.0f, 0.0f, 1.0f });
    }

    // Components list
    dd->ScreenText({ x, y }, "Components:", { 0.7f, 0.7f, 0.7f, 1.0f });
    y += 18.0f;

    if (world->HasComponent<MeshRenderer>(m_selectedEntity))
    {
        dd->ScreenText({ x + 10.0f, y }, "MeshRenderer", { 0.6f, 0.9f, 0.6f, 1.0f });
        y += 16.0f;
    }
    if (world->HasComponent<RigidBodyComponent>(m_selectedEntity))
    {
        dd->ScreenText({ x + 10.0f, y }, "RigidBodyComponent", { 0.6f, 0.9f, 0.6f, 1.0f });
        y += 16.0f;
    }
    if (world->HasComponent<ColliderComponent>(m_selectedEntity))
    {
        dd->ScreenText({ x + 10.0f, y }, "ColliderComponent", { 0.6f, 0.9f, 0.6f, 1.0f });
        y += 16.0f;
    }
    if (world->HasComponent<LightComponent>(m_selectedEntity))
    {
        dd->ScreenText({ x + 10.0f, y }, "LightComponent", { 0.6f, 0.9f, 0.6f, 1.0f });
        y += 16.0f;
    }
    if (world->HasComponent<AIComponent>(m_selectedEntity))
    {
        dd->ScreenText({ x + 10.0f, y }, "AIComponent", { 0.6f, 0.9f, 0.6f, 1.0f });
        y += 16.0f;
    }
    if (world->HasComponent<TagComponent>(m_selectedEntity))
    {
        dd->ScreenText({ x + 10.0f, y }, "TagComponent", { 0.6f, 0.9f, 0.6f, 1.0f });
        y += 16.0f;
    }
}

// ---------------------------------------------------------------------------
// Memory stats (minimal)
// ---------------------------------------------------------------------------

void DebugVisualizationModule::DrawMemoryStats()
{
    if (!m_showPerfStats) return;

    auto* dd = Spark::DebugDraw::GetInstance();
    auto* gfx = m_ctx->GetGraphics();
    if (!dd || !gfx) return;

    auto memInfo = gfx->GetMemoryUsage();

    char buf[128];
    std::snprintf(buf, sizeof(buf), "VRAM: %.1f / %.1f MB",
                  memInfo.usedMB, memInfo.totalMB);
    dd->ScreenText({ 10.0f, 100.0f }, buf, { 0.9f, 0.6f, 0.3f, 1.0f });
}

// ---------------------------------------------------------------------------

void DebugVisualizationModule::OnResize(int /*w*/, int /*h*/) { }

// ---------------------------------------------------------------------------
// DLL exports
// ---------------------------------------------------------------------------

SPARK_IMPLEMENT_MODULE(DebugVisualizationModule)
