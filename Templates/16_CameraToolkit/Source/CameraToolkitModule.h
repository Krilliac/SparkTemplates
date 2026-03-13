#pragma once
/**
 * @file CameraToolkitModule.h
 * @brief INTERMEDIATE — Camera system capabilities showcase.
 *
 * Demonstrates:
 *   - First-person camera (mouse look + WASD)
 *   - Third-person chase camera (follow target with spring damping)
 *   - Orbit camera (rotate around point, zoom)
 *   - Free-fly / spectator camera (6DOF movement)
 *   - Camera shake effect (parametric: trauma, frequency, decay)
 *   - Depth of Field (focus distance, aperture)
 *   - Camera interpolation between viewpoints (CatmullRom, Linear, CubicBezier)
 *   - Split-screen setup (2 cameras, 2 viewports)
 *   - Cinematic letterbox mode
 *   - FOV animation (zoom for sniper / sprint)
 *
 * Spark Engine systems used:
 *   ECS        — World, NameComponent, Transform, MeshRenderer, Camera
 *   Input      — InputManager (IsKeyDown, WasKeyPressed, GetMouseDelta)
 *   Rendering  — GraphicsEngine (viewports, DOF, letterbox)
 *   Cinematic  — SequencerManager (camera path interpolation)
 *   Events     — EventBus
 */

#include <Core/ModuleManager.h>
#include <Core/EngineContext.h>
#include <Engine/ECS/Components.h>
#include <Engine/Cinematic/Sequencer.h>
#include <vector>

class CameraToolkitModule final : public Spark::IModule
{
public:
    Spark::ModuleInfo GetModuleInfo() const override
    {
        return { "CameraToolkit", "0.2.0", SPARK_SDK_VERSION, 1000 };
    }

    void OnLoad(Spark::IEngineContext* ctx) override;
    void OnUnload() override;
    void OnUpdate(float dt) override;
    void OnRender() override;
    void OnResize(int w, int h) override;

private:
    void BuildScene();
    void CreateCinematicPath();

    // Camera mode update functions
    void UpdateFirstPerson(float dt);
    void UpdateThirdPerson(float dt);
    void UpdateOrbit(float dt);
    void UpdateFreeFly(float dt);

    // Effects
    void UpdateShake(float dt);
    void ApplyDOF();
    void UpdateFOVAnimation(float dt);

    Spark::IEngineContext* m_ctx = nullptr;

    // Scene entities
    entt::entity m_floor       = entt::null;
    entt::entity m_light       = entt::null;
    entt::entity m_mainCamera  = entt::null;
    entt::entity m_splitCamera = entt::null;  // second camera for split-screen
    entt::entity m_player      = entt::null;  // target for third-person mode
    std::vector<entt::entity> m_props;

    // Camera path
    uint32_t m_pathSequenceID = 0;

    // Camera mode
    enum class CameraMode { FirstPerson, ThirdPerson, Orbit, FreeFly, Count };
    CameraMode m_cameraMode = CameraMode::FirstPerson;

    // First-person state
    float m_fpsYaw   = 0.0f;
    float m_fpsPitch = 0.0f;
    DirectX::XMFLOAT3 m_fpsPos = { 0.f, 2.f, 0.f };

    // Third-person state
    float m_tpsYaw    = 0.0f;
    float m_tpsPitch  = -15.0f;
    float m_tpsDist   = 8.0f;
    float m_tpsSpringK   = 5.0f;   // spring stiffness
    float m_tpsDamping   = 0.8f;   // velocity damping
    DirectX::XMFLOAT3 m_tpsVelocity = { 0.f, 0.f, 0.f };

    // Orbit state
    float m_orbitYaw   = 0.0f;
    float m_orbitPitch = -20.0f;
    float m_orbitDist  = 15.0f;
    DirectX::XMFLOAT3 m_orbitPivot = { 0.f, 2.f, 0.f };

    // Free-fly state
    float m_flyYaw   = 0.0f;
    float m_flyPitch = 0.0f;
    DirectX::XMFLOAT3 m_flyPos = { 0.f, 5.f, -10.f };

    // Camera shake
    float m_shakeTrauma   = 0.0f;   // 0..1
    float m_shakeDecay    = 1.5f;   // trauma decay per second
    float m_shakeFreq     = 25.0f;  // oscillation frequency
    float m_shakeTimer    = 0.0f;

    // Depth of Field
    bool  m_dofEnabled     = false;
    float m_dofFocusDist   = 10.0f;
    float m_dofAperture    = 2.8f;

    // FOV
    float m_baseFOV     = 60.0f;
    float m_currentFOV  = 60.0f;
    float m_targetFOV   = 60.0f;

    // Split-screen & letterbox
    bool m_splitScreen  = false;
    bool m_letterbox    = false;

    // Movement constants
    static constexpr float kMoveSpeed    = 8.0f;
    static constexpr float kFlySpeed     = 12.0f;
    static constexpr float kMouseSens    = 0.2f;
    static constexpr float kFOVSpeed     = 60.0f;  // degrees per second
};
