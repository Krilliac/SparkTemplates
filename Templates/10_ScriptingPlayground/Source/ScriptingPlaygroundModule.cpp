/**
 * @file ScriptingPlaygroundModule.cpp
 * @brief INTERMEDIATE — AngelScript integration with hot-reload and lifecycle callbacks.
 */

#include "ScriptingPlaygroundModule.h"
#include <Input/InputManager.h>
#include <string>

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void ScriptingPlaygroundModule::OnLoad(Spark::IEngineContext* ctx)
{
    m_ctx = ctx;

    // Compile script files
    auto* scriptEngine = Spark::AngelScriptEngine::GetInstance();
    if (scriptEngine)
    {
        // Compile the player controller script from the Assets folder
        scriptEngine->CompileScriptFile("Assets/Scripts/PlayerController.as");

        // Compile a spinning object script from a string (for demonstration)
        scriptEngine->CompileScriptFromString(R"(
            class Spinner
            {
                float rotSpeed = 90.0f;
                float bobSpeed = 2.0f;
                float bobHeight = 0.5f;
                float elapsed = 0.0f;

                void Start()
                {
                    ASPrint("Spinner script started!");
                }

                void Update(float dt)
                {
                    elapsed += dt;

                    // Rotate the entity
                    Transform@ t = ASGetTransform();
                    if (t !is null)
                    {
                        t.rotation.y += rotSpeed * dt;

                        // Bob up and down
                        t.position.y = 1.5f + sin(elapsed * bobSpeed) * bobHeight;
                    }
                }

                void OnCollision(uint other)
                {
                    ASPrint("Spinner collided with entity!");
                }
            }
        )", "SpinnerModule");

        // Compile an orbit script
        scriptEngine->CompileScriptFromString(R"(
            class Orbiter
            {
                float orbitRadius = 5.0f;
                float orbitSpeed = 1.5f;
                float elapsed = 0.0f;

                void Start()
                {
                    ASPrint("Orbiter script started!");
                }

                void Update(float dt)
                {
                    elapsed += dt;

                    Transform@ t = ASGetTransform();
                    if (t !is null)
                    {
                        t.position.x = cos(elapsed * orbitSpeed) * orbitRadius;
                        t.position.z = sin(elapsed * orbitSpeed) * orbitRadius;
                        t.position.y = 1.0f;

                        // Face the direction of movement
                        t.rotation.y = -elapsed * orbitSpeed * 57.2958f;
                    }
                }
            }
        )", "OrbiterModule");
    }

    BuildScene();
    AttachScripts();
}

void ScriptingPlaygroundModule::OnUnload()
{
    auto* scriptEngine = Spark::AngelScriptEngine::GetInstance();
    auto* world = m_ctx ? m_ctx->GetWorld() : nullptr;

    if (scriptEngine && world)
    {
        for (auto e : m_scriptedEntities)
            if (e != entt::null) scriptEngine->DetachScript(e);
    }

    if (!world) { m_ctx = nullptr; return; }

    for (auto e : { m_floor, m_light })
        if (e != entt::null) world->DestroyEntity(e);

    for (auto e : m_scriptedEntities)
        if (e != entt::null) world->DestroyEntity(e);
    m_scriptedEntities.clear();

    m_ctx = nullptr;
}

// ---------------------------------------------------------------------------
// Scene construction
// ---------------------------------------------------------------------------

void ScriptingPlaygroundModule::BuildScene()
{
    auto* world = m_ctx->GetWorld();

    // Floor
    m_floor = world->CreateEntity();
    world->AddComponent<NameComponent>(m_floor, NameComponent{ "Floor" });
    world->AddComponent<Transform>(m_floor, Transform{
        { 0.f, -0.5f, 0.f }, { 0.f, 0.f, 0.f }, { 30.f, 1.f, 30.f }
    });
    world->AddComponent<MeshRenderer>(m_floor, MeshRenderer{
        "Assets/Models/cube.obj", "Assets/Materials/concrete.mat",
        true, true, true
    });

    // Light
    m_light = world->CreateEntity();
    world->AddComponent<NameComponent>(m_light, NameComponent{ "ScriptLight" });
    world->AddComponent<Transform>(m_light, Transform{
        { 0.f, 15.f, 0.f }, { -50.f, 20.f, 0.f }, { 1.f, 1.f, 1.f }
    });
    world->AddComponent<LightComponent>(m_light, LightComponent{
        LightComponent::Type::Directional,
        { 1.f, 1.f, 1.f }, 2.0f, 0.f,
        0.f, 0.f, true, 1024
    });
}

// ---------------------------------------------------------------------------
// Script attachment
// ---------------------------------------------------------------------------

void ScriptingPlaygroundModule::AttachScripts()
{
    // Spawn entities with different scripts
    SpawnScriptedEntity("SpinningCube", "Spinner", -5.0f, 0.0f);
    SpawnScriptedEntity("OrbitingSphere", "Orbiter", 5.0f, 0.0f);

    // A second spinner with different parameters
    SpawnScriptedEntity("SpinningCube2", "Spinner", 0.0f, 8.0f);
}

void ScriptingPlaygroundModule::SpawnScriptedEntity(const std::string& name,
                                                     const std::string& scriptClass,
                                                     float x, float z)
{
    auto* world = m_ctx->GetWorld();
    auto e = world->CreateEntity();

    world->AddComponent<NameComponent>(e, NameComponent{ name });
    world->AddComponent<Transform>(e, Transform{
        { x, 1.f, z }, { 0.f, 0.f, 0.f }, { 1.f, 1.f, 1.f }
    });

    // Use different meshes based on script type
    std::string mesh = (scriptClass == "Orbiter")
        ? "Assets/Models/sphere.obj"
        : "Assets/Models/cube.obj";

    world->AddComponent<MeshRenderer>(e, MeshRenderer{
        mesh, "Assets/Materials/metal.mat", true, true, true
    });

    // Attach the Script component
    std::string moduleName = (scriptClass == "Orbiter") ? "OrbiterModule" : "SpinnerModule";
    world->AddComponent<Script>(e, Script{
        "", // scriptPath (empty when compiled from string)
        scriptClass,
        moduleName,
        true // enabled
    });

    // Bind the script to the entity via AngelScriptEngine
    auto* scriptEngine = Spark::AngelScriptEngine::GetInstance();
    if (scriptEngine)
    {
        scriptEngine->AttachScript(e, scriptClass, moduleName);
        scriptEngine->CallStart(e);
    }

    m_scriptedEntities.push_back(e);
}

// ---------------------------------------------------------------------------
// Hot-reload
// ---------------------------------------------------------------------------

void ScriptingPlaygroundModule::HotReloadScripts()
{
    auto* scriptEngine = Spark::AngelScriptEngine::GetInstance();
    if (!scriptEngine) return;

    // Detach all scripts
    for (auto e : m_scriptedEntities)
        if (e != entt::null) scriptEngine->DetachScript(e);

    // Recompile from files (picks up any external edits)
    scriptEngine->CompileScriptFile("Assets/Scripts/PlayerController.as");

    // Re-attach
    auto* world = m_ctx->GetWorld();
    for (auto e : m_scriptedEntities)
    {
        if (e == entt::null || !world->HasComponent<Script>(e)) continue;
        auto& sc = world->GetComponent<Script>(e);
        scriptEngine->AttachScript(e, sc.className, sc.moduleName);
        scriptEngine->CallStart(e);
    }
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

void ScriptingPlaygroundModule::OnUpdate(float dt)
{
    auto* input = m_ctx->GetInput();
    if (!input) return;

    // Update all scripts
    auto* scriptEngine = Spark::AngelScriptEngine::GetInstance();
    if (scriptEngine)
    {
        for (auto e : m_scriptedEntities)
            if (e != entt::null) scriptEngine->CallUpdate(e, dt);
    }

    // F5 — Hot-reload all scripts
    if (input->WasKeyPressed(VK_F5))
        HotReloadScripts();

    // 1 — Spawn another spinning cube
    if (input->WasKeyPressed('1'))
    {
        float rx = static_cast<float>(rand() % 20 - 10);
        float rz = static_cast<float>(rand() % 20 - 10);
        SpawnScriptedEntity("DynSpinner", "Spinner", rx, rz);
    }

    // 2 — Spawn another orbiting sphere
    if (input->WasKeyPressed('2'))
    {
        SpawnScriptedEntity("DynOrbiter", "Orbiter", 0.0f, 0.0f);
    }
}

// ---------------------------------------------------------------------------
void ScriptingPlaygroundModule::OnRender() { }
void ScriptingPlaygroundModule::OnResize(int, int) { }

SPARK_IMPLEMENT_MODULE(ScriptingPlaygroundModule)
