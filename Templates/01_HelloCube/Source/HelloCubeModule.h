#pragma once
/**
 * @file HelloCubeModule.h
 * @brief BASIC — Spinning cube with keyboard controls.
 *
 * Demonstrates:
 *   - Creating an entity via the ECS World
 *   - Attaching NameComponent, Transform, and MeshRenderer
 *   - Reading keyboard input each frame
 *   - Rotating / moving an entity through its Transform
 *   - Adding a point light so the cube is visible
 *
 * Spark Engine systems used:
 *   Core       — IModule, IEngineContext, ModuleInfo
 *   ECS        — World, NameComponent, Transform, MeshRenderer, LightComponent
 *   Input      — InputManager  (IsKeyDown / WasKeyPressed)
 *   Rendering  — GraphicsEngine (implicitly via RenderSystem)
 */

#include <Core/ModuleManager.h>       // Spark::IModule, SPARK_IMPLEMENT_MODULE
#include <Core/EngineContext.h>        // Spark::IEngineContext
#include <Engine/ECS/Components.h>     // World + all component headers

class HelloCubeModule final : public Spark::IModule
{
public:
    // ---- IModule interface --------------------------------------------------

    Spark::ModuleInfo GetModuleInfo() const override
    {
        return { "HelloCube", "0.1.0", SPARK_SDK_VERSION, 1000 };
    }

    void OnLoad(Spark::IEngineContext* ctx) override;
    void OnUnload() override;
    void OnUpdate(float dt) override;
    void OnRender() override;
    void OnResize(int w, int h) override;

private:
    Spark::IEngineContext* m_ctx = nullptr;

    // ECS handles
    entt::entity m_cube  = entt::null;
    entt::entity m_light = entt::null;

    // Rotation state
    float m_yaw   = 0.0f;
    float m_pitch = 0.0f;

    // Movement speed (units / second)
    static constexpr float kMoveSpeed   = 5.0f;
    static constexpr float kRotateSpeed = 90.0f; // degrees / second
};
