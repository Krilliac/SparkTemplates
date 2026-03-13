#pragma once
/**
 * @file ScriptingPlaygroundModule.h
 * @brief INTERMEDIATE — AngelScript integration with hot-reload and lifecycle callbacks.
 *
 * Demonstrates:
 *   - AngelScriptEngine compilation from file and from string
 *   - Attaching/detaching scripts to ECS entities via Script component
 *   - Lifecycle callbacks: Start(), Update(float), OnCollision(entity)
 *   - Script-accessible engine API (ASPrint, ASCreateEntity, ASGetTransform, ASGetKey)
 *   - Hot-reload workflow (recompile on key press)
 *   - Multiple script modules
 *
 * Spark Engine systems used:
 *   Scripting  — AngelScriptEngine, Script component
 *   ECS        — World, Transform, MeshRenderer, NameComponent
 *   Physics    — RigidBodyComponent, ColliderComponent
 *   Input      — InputManager
 *   Rendering  — LightComponent
 */

#include <Core/ModuleManager.h>
#include <Core/EngineContext.h>
#include <Engine/ECS/Components.h>
#include <Engine/Scripting/AngelScriptEngine.h>
#include <vector>

class ScriptingPlaygroundModule final : public Spark::IModule
{
public:
    Spark::ModuleInfo GetModuleInfo() const override
    {
        return { "ScriptingPlayground", "0.2.0", SPARK_SDK_VERSION, 1000 };
    }

    void OnLoad(Spark::IEngineContext* ctx) override;
    void OnUnload() override;
    void OnUpdate(float dt) override;
    void OnRender() override;
    void OnResize(int w, int h) override;

private:
    void BuildScene();
    void AttachScripts();
    void HotReloadScripts();
    void SpawnScriptedEntity(const std::string& name, const std::string& scriptClass,
                              float x, float z);

    Spark::IEngineContext* m_ctx = nullptr;

    entt::entity m_floor = entt::null;
    entt::entity m_light = entt::null;

    std::vector<entt::entity> m_scriptedEntities;
};
