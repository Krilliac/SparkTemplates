/**
 * @file MaterialLabModule.cpp
 * @brief INTERMEDIATE — Material and rendering system showcase.
 */

#include "MaterialLabModule.h"
#include <Input/InputManager.h>
#include <cmath>

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void MaterialLabModule::OnLoad(Spark::IEngineContext* ctx)
{
    m_ctx = ctx;
    BuildScene();
    CreateSphereGrid();
    CreateShowcasePedestals();
    SetupPostProcessing();
}

void MaterialLabModule::OnUnload()
{
    auto* world = m_ctx ? m_ctx->GetWorld() : nullptr;
    if (!world) { m_ctx = nullptr; return; }

    for (auto e : { m_floor, m_sunLight, m_fillLight, m_camera,
                    m_emissiveSphere, m_transparentOrb, m_sssSphere,
                    m_mirrorSphere, m_uvAnimCube })
        if (e != entt::null) world->DestroyEntity(e);

    for (auto e : m_gridSpheres)
        if (e != entt::null) world->DestroyEntity(e);
    m_gridSpheres.clear();

    m_ctx = nullptr;
}

// ---------------------------------------------------------------------------
// Scene construction
// ---------------------------------------------------------------------------

void MaterialLabModule::BuildScene()
{
    auto* world = m_ctx->GetWorld();

    // Floor
    m_floor = world->CreateEntity();
    world->AddComponent<NameComponent>(m_floor, NameComponent{ "Floor" });
    world->AddComponent<Transform>(m_floor, Transform{
        { 0.f, -0.5f, 0.f }, { 0.f, 0.f, 0.f }, { 50.f, 1.f, 50.f }
    });
    world->AddComponent<MeshRenderer>(m_floor, MeshRenderer{
        "Assets/Models/cube.obj", "Assets/Materials/dark_concrete.mat",
        false, true, true
    });

    // Directional sun light
    m_sunLight = world->CreateEntity();
    world->AddComponent<NameComponent>(m_sunLight, NameComponent{ "SunLight" });
    world->AddComponent<Transform>(m_sunLight, Transform{
        { 0.f, 20.f, 0.f }, { -50.f, 30.f, 0.f }, { 1.f, 1.f, 1.f }
    });
    world->AddComponent<LightComponent>(m_sunLight, LightComponent{
        LightComponent::Type::Directional,
        { 1.f, 0.95f, 0.9f }, 2.5f, 0.f,
        0.f, 0.f, true, 2048
    });

    // Soft fill light from the opposite side
    m_fillLight = world->CreateEntity();
    world->AddComponent<NameComponent>(m_fillLight, NameComponent{ "FillLight" });
    world->AddComponent<Transform>(m_fillLight, Transform{
        { -10.f, 8.f, -10.f }, { 0.f, 0.f, 0.f }, { 1.f, 1.f, 1.f }
    });
    world->AddComponent<LightComponent>(m_fillLight, LightComponent{
        LightComponent::Type::Point,
        { 0.4f, 0.5f, 0.7f }, 1.0f, 30.0f,
        0.f, 0.f, false, 512
    });

    // Camera
    m_camera = world->CreateEntity();
    world->AddComponent<NameComponent>(m_camera, NameComponent{ "LabCamera" });
    world->AddComponent<Transform>(m_camera, Transform{
        { 0.f, 8.f, -20.f }, { -15.f, 0.f, 0.f }, { 1.f, 1.f, 1.f }
    });
    world->AddComponent<Camera>(m_camera, Camera{
        60.0f, 0.1f, 500.0f, true
    });
}

// ---------------------------------------------------------------------------
// Sphere grid — roughness (rows) vs metallic (columns)
// ---------------------------------------------------------------------------

void MaterialLabModule::CreateSphereGrid()
{
    auto* world = m_ctx->GetWorld();

    float spacing = 2.5f;
    float startX = -(kGridCols - 1) * spacing * 0.5f;
    float startZ = -(kGridRows - 1) * spacing * 0.5f;

    for (int row = 0; row < kGridRows; ++row)
    {
        for (int col = 0; col < kGridCols; ++col)
        {
            float roughness = static_cast<float>(row) / static_cast<float>(kGridRows - 1);
            float metallic  = static_cast<float>(col) / static_cast<float>(kGridCols - 1);

            std::string name = "Sphere_R" + std::to_string(row) + "_M" + std::to_string(col);

            auto e = world->CreateEntity();
            world->AddComponent<NameComponent>(e, NameComponent{ name.c_str() });
            world->AddComponent<Transform>(e, Transform{
                { startX + col * spacing, 1.0f, startZ + row * spacing },
                { 0.f, 0.f, 0.f },
                { 1.f, 1.f, 1.f }
            });
            world->AddComponent<MeshRenderer>(e, MeshRenderer{
                "Assets/Models/sphere.obj", "Assets/Materials/pbr_base.mat",
                true, true, true
            });

            // PBR material component with explicit parameters
            world->AddComponent<MaterialComponent>(e, MaterialComponent{});
            auto& mat = world->GetComponent<MaterialComponent>(e);
            mat.SetAlbedo({ 0.8f, 0.2f, 0.2f, 1.0f });    // red base
            mat.SetRoughness(roughness);
            mat.SetMetallic(metallic);
            mat.SetNormalMap("Assets/Textures/flat_normal.png");
            mat.SetAOMap("Assets/Textures/white.png");
            mat.SetTransparencyMode(MaterialComponent::TransparencyMode::Opaque);
            mat.SetRenderQueue(MaterialComponent::RenderQueue::Opaque);

            m_gridSpheres.push_back(e);
        }
    }
}

// ---------------------------------------------------------------------------
// Showcase pedestals — special material types
// ---------------------------------------------------------------------------

void MaterialLabModule::CreateShowcasePedestals()
{
    auto* world = m_ctx->GetWorld();
    float pedestalZ = (kGridRows - 1) * 2.5f * 0.5f + 5.0f;

    // --- Emissive sphere (pulsing glow) ------------------------------------
    m_emissiveSphere = world->CreateEntity();
    world->AddComponent<NameComponent>(m_emissiveSphere, NameComponent{ "EmissiveSphere" });
    world->AddComponent<Transform>(m_emissiveSphere, Transform{
        { -8.f, 1.5f, pedestalZ }, { 0.f, 0.f, 0.f }, { 1.5f, 1.5f, 1.5f }
    });
    world->AddComponent<MeshRenderer>(m_emissiveSphere, MeshRenderer{
        "Assets/Models/sphere.obj", "Assets/Materials/pbr_base.mat",
        true, true, true
    });
    world->AddComponent<MaterialComponent>(m_emissiveSphere, MaterialComponent{});
    {
        auto& mat = world->GetComponent<MaterialComponent>(m_emissiveSphere);
        mat.SetAlbedo({ 0.05f, 0.05f, 0.05f, 1.0f });
        mat.SetRoughness(0.3f);
        mat.SetMetallic(0.0f);
        mat.SetEmissiveColor({ 2.0f, 0.5f, 0.1f });
        mat.SetEmissiveIntensity(3.0f);
    }

    // --- Transparent glass orb ---------------------------------------------
    m_transparentOrb = world->CreateEntity();
    world->AddComponent<NameComponent>(m_transparentOrb, NameComponent{ "GlassOrb" });
    world->AddComponent<Transform>(m_transparentOrb, Transform{
        { -3.f, 1.5f, pedestalZ }, { 0.f, 0.f, 0.f }, { 1.5f, 1.5f, 1.5f }
    });
    world->AddComponent<MeshRenderer>(m_transparentOrb, MeshRenderer{
        "Assets/Models/sphere.obj", "Assets/Materials/pbr_base.mat",
        false, true, true
    });
    world->AddComponent<MaterialComponent>(m_transparentOrb, MaterialComponent{});
    {
        auto& mat = world->GetComponent<MaterialComponent>(m_transparentOrb);
        mat.SetAlbedo({ 0.9f, 0.95f, 1.0f, 0.3f });
        mat.SetRoughness(0.05f);
        mat.SetMetallic(0.0f);
        mat.SetTransparencyMode(MaterialComponent::TransparencyMode::AlphaBlend);
        mat.SetRenderQueue(MaterialComponent::RenderQueue::Transparent);
        mat.SetRefractionIndex(1.5f);
    }

    // --- SSS sphere (skin-like material) -----------------------------------
    m_sssSphere = world->CreateEntity();
    world->AddComponent<NameComponent>(m_sssSphere, NameComponent{ "SSSSphere" });
    world->AddComponent<Transform>(m_sssSphere, Transform{
        { 3.f, 1.5f, pedestalZ }, { 0.f, 0.f, 0.f }, { 1.5f, 1.5f, 1.5f }
    });
    world->AddComponent<MeshRenderer>(m_sssSphere, MeshRenderer{
        "Assets/Models/sphere.obj", "Assets/Materials/pbr_base.mat",
        true, true, true
    });
    world->AddComponent<MaterialComponent>(m_sssSphere, MaterialComponent{});
    {
        auto& mat = world->GetComponent<MaterialComponent>(m_sssSphere);
        mat.SetAlbedo({ 0.9f, 0.7f, 0.6f, 1.0f });   // skin tone
        mat.SetRoughness(0.5f);
        mat.SetMetallic(0.0f);
        mat.SetSSSEnabled(true);
        mat.SetSSSColor({ 1.0f, 0.3f, 0.2f });        // subsurface red
        mat.SetSSSRadius(0.5f);
        mat.SetSSSIntensity(1.0f);
    }

    // --- Mirror sphere (perfect reflector) ---------------------------------
    m_mirrorSphere = world->CreateEntity();
    world->AddComponent<NameComponent>(m_mirrorSphere, NameComponent{ "MirrorSphere" });
    world->AddComponent<Transform>(m_mirrorSphere, Transform{
        { 8.f, 1.5f, pedestalZ }, { 0.f, 0.f, 0.f }, { 1.5f, 1.5f, 1.5f }
    });
    world->AddComponent<MeshRenderer>(m_mirrorSphere, MeshRenderer{
        "Assets/Models/sphere.obj", "Assets/Materials/pbr_base.mat",
        true, true, true
    });
    world->AddComponent<MaterialComponent>(m_mirrorSphere, MaterialComponent{});
    {
        auto& mat = world->GetComponent<MaterialComponent>(m_mirrorSphere);
        mat.SetAlbedo({ 0.95f, 0.95f, 0.95f, 1.0f });
        mat.SetRoughness(0.0f);
        mat.SetMetallic(1.0f);
        mat.SetReflectionProbe("Assets/Cubemaps/studio_env.hdr");
    }

    // --- UV-animated cube (scrolling texture) ------------------------------
    m_uvAnimCube = world->CreateEntity();
    world->AddComponent<NameComponent>(m_uvAnimCube, NameComponent{ "UVAnimCube" });
    world->AddComponent<Transform>(m_uvAnimCube, Transform{
        { 0.f, 1.5f, pedestalZ + 5.f }, { 0.f, 0.f, 0.f }, { 2.f, 2.f, 2.f }
    });
    world->AddComponent<MeshRenderer>(m_uvAnimCube, MeshRenderer{
        "Assets/Models/cube.obj", "Assets/Materials/pbr_base.mat",
        true, true, true
    });
    world->AddComponent<MaterialComponent>(m_uvAnimCube, MaterialComponent{});
    {
        auto& mat = world->GetComponent<MaterialComponent>(m_uvAnimCube);
        mat.SetAlbedoMap("Assets/Textures/grid_pattern.png");
        mat.SetRoughness(0.5f);
        mat.SetMetallic(0.0f);
        mat.SetUVTiling({ 3.0f, 3.0f });
        mat.SetUVOffset({ 0.0f, 0.0f });
    }
}

// ---------------------------------------------------------------------------
// Post-processing
// ---------------------------------------------------------------------------

void MaterialLabModule::SetupPostProcessing()
{
    auto* gfx = m_ctx->GetGraphics();
    if (!gfx) return;

    auto* post = gfx->GetPostProcessingSystem();
    if (!post) return;

    // Bloom to interact with emissive materials
    post->SetBloomEnabled(true);
    post->SetBloomThreshold(1.0f);
    post->SetBloomIntensity(1.5f);

    // Tone mapping
    post->SetToneMappingEnabled(true);

    // SSAO for subtle depth cues
    gfx->SetSSAOSettings({ true, 0.5f, 1.0f, 0.03f });

    // FXAA
    post->SetFXAAEnabled(true);
}

// ---------------------------------------------------------------------------
// Material presets
// ---------------------------------------------------------------------------

void MaterialLabModule::ApplyMaterialPreset(int preset)
{
    auto* world = m_ctx->GetWorld();
    if (!world) return;

    m_currentPreset = preset;

    struct PresetParams {
        DirectX::XMFLOAT4 albedo;
        float baseRoughness;
        float baseMetallic;
    };

    PresetParams presets[] = {
        { { 0.8f, 0.2f, 0.2f, 1.0f }, 0.0f, 1.0f },  // 1: Metal (red)
        { { 0.1f, 0.6f, 0.9f, 1.0f }, 0.0f, 0.0f },  // 2: Plastic (blue)
        { { 0.9f, 0.95f, 1.0f, 0.3f }, 0.0f, 0.0f },  // 3: Glass
        { { 0.9f, 0.7f, 0.6f, 1.0f }, 0.0f, 0.0f },  // 4: Skin (SSS)
    };

    auto& p = presets[preset % 4];

    for (int row = 0; row < kGridRows; ++row)
    {
        for (int col = 0; col < kGridCols; ++col)
        {
            int idx = row * kGridCols + col;
            auto e = m_gridSpheres[idx];

            if (!world->HasComponent<MaterialComponent>(e)) continue;
            auto& mat = world->GetComponent<MaterialComponent>(e);

            float roughness = p.baseRoughness + static_cast<float>(row) / static_cast<float>(kGridRows - 1);
            roughness = std::min(1.0f, roughness + m_roughnessOffset);
            float metallic  = p.baseMetallic  + static_cast<float>(col) / static_cast<float>(kGridCols - 1);
            metallic = std::min(1.0f, metallic);

            mat.SetAlbedo(p.albedo);
            mat.SetRoughness(roughness);
            mat.SetMetallic(metallic);

            // Glass preset uses alpha blend
            if (preset == 2)
            {
                mat.SetTransparencyMode(MaterialComponent::TransparencyMode::AlphaBlend);
                mat.SetRenderQueue(MaterialComponent::RenderQueue::Transparent);
            }
            else
            {
                mat.SetTransparencyMode(MaterialComponent::TransparencyMode::Opaque);
                mat.SetRenderQueue(MaterialComponent::RenderQueue::Opaque);
            }

            // Skin preset enables SSS
            if (preset == 3)
            {
                mat.SetSSSEnabled(true);
                mat.SetSSSColor({ 1.0f, 0.3f, 0.2f });
                mat.SetSSSRadius(0.4f);
                mat.SetSSSIntensity(0.8f);
            }
            else
            {
                mat.SetSSSEnabled(false);
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Render path cycling
// ---------------------------------------------------------------------------

void MaterialLabModule::CycleRenderPath()
{
    auto* gfx = m_ctx->GetGraphics();
    if (!gfx) return;

    m_renderPathIndex = (m_renderPathIndex + 1) % 3;

    switch (m_renderPathIndex)
    {
    case 0:
        gfx->SetRenderPath(Spark::RenderPath::Forward);
        break;
    case 1:
        gfx->SetRenderPath(Spark::RenderPath::Deferred);
        break;
    case 2:
        gfx->SetRenderPath(Spark::RenderPath::ForwardPlus);
        break;
    }
}

// ---------------------------------------------------------------------------
// Animated materials
// ---------------------------------------------------------------------------

void MaterialLabModule::UpdateAnimatedMaterials(float dt)
{
    auto* world = m_ctx->GetWorld();
    if (!world) return;

    m_animTimer += dt;

    // Pulsing emissive sphere
    if (m_emissiveOn && world->HasComponent<MaterialComponent>(m_emissiveSphere))
    {
        auto& mat = world->GetComponent<MaterialComponent>(m_emissiveSphere);
        float pulse = (std::sin(m_animTimer * 2.0f) + 1.0f) * 0.5f;  // 0..1

        // Cycle hue over time
        float hue = std::fmod(m_animTimer * 0.3f, 1.0f);
        float r = std::max(0.0f, std::sin(hue * 6.2832f) * 2.0f);
        float g = std::max(0.0f, std::sin((hue + 0.333f) * 6.2832f) * 2.0f);
        float b = std::max(0.0f, std::sin((hue + 0.666f) * 6.2832f) * 2.0f);
        mat.SetEmissiveColor({ r, g, b });
        mat.SetEmissiveIntensity(1.0f + pulse * 4.0f);
    }

    // UV scroll on the animated cube
    if (world->HasComponent<MaterialComponent>(m_uvAnimCube))
    {
        auto& mat = world->GetComponent<MaterialComponent>(m_uvAnimCube);
        mat.SetUVOffset({ m_animTimer * 0.2f, m_animTimer * 0.1f });
    }

    // Slowly rotate showcase objects
    for (auto e : { m_emissiveSphere, m_transparentOrb, m_sssSphere, m_mirrorSphere })
    {
        if (e == entt::null || !world->HasComponent<Transform>(e)) continue;
        auto& xf = world->GetComponent<Transform>(e);
        xf.rotation.y += 30.0f * dt;
    }

    // Rotate UV cube
    if (m_uvAnimCube != entt::null && world->HasComponent<Transform>(m_uvAnimCube))
    {
        auto& xf = world->GetComponent<Transform>(m_uvAnimCube);
        xf.rotation.y += 20.0f * dt;
        xf.rotation.x += 10.0f * dt;
    }
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

void MaterialLabModule::OnUpdate(float dt)
{
    auto* input = m_ctx->GetInput();
    auto* world = m_ctx->GetWorld();
    if (!input || !world) return;

    // 1-4 — Material presets
    if (input->WasKeyPressed('1')) ApplyMaterialPreset(0);
    if (input->WasKeyPressed('2')) ApplyMaterialPreset(1);
    if (input->WasKeyPressed('3')) ApplyMaterialPreset(2);
    if (input->WasKeyPressed('4')) ApplyMaterialPreset(3);

    // M — Toggle metallic on grid spheres
    if (input->WasKeyPressed('M'))
    {
        for (auto e : m_gridSpheres)
        {
            if (!world->HasComponent<MaterialComponent>(e)) continue;
            auto& mat = world->GetComponent<MaterialComponent>(e);
            float m = mat.GetMetallic();
            mat.SetMetallic(m > 0.5f ? 0.0f : 1.0f);
        }
    }

    // T — Toggle transparency mode on grid spheres
    if (input->WasKeyPressed('T'))
    {
        for (auto e : m_gridSpheres)
        {
            if (!world->HasComponent<MaterialComponent>(e)) continue;
            auto& mat = world->GetComponent<MaterialComponent>(e);

            auto mode = mat.GetTransparencyMode();
            if (mode == MaterialComponent::TransparencyMode::Opaque)
            {
                mat.SetTransparencyMode(MaterialComponent::TransparencyMode::AlphaTest);
                mat.SetRenderQueue(MaterialComponent::RenderQueue::Opaque);
            }
            else if (mode == MaterialComponent::TransparencyMode::AlphaTest)
            {
                mat.SetTransparencyMode(MaterialComponent::TransparencyMode::AlphaBlend);
                mat.SetRenderQueue(MaterialComponent::RenderQueue::Transparent);
            }
            else
            {
                mat.SetTransparencyMode(MaterialComponent::TransparencyMode::Opaque);
                mat.SetRenderQueue(MaterialComponent::RenderQueue::Opaque);
            }
        }
    }

    // E — Toggle emissive on showcase sphere
    if (input->WasKeyPressed('E'))
    {
        m_emissiveOn = !m_emissiveOn;
        if (world->HasComponent<MaterialComponent>(m_emissiveSphere))
        {
            auto& mat = world->GetComponent<MaterialComponent>(m_emissiveSphere);
            mat.SetEmissiveIntensity(m_emissiveOn ? 3.0f : 0.0f);
        }
    }

    // R — Cycle render paths
    if (input->WasKeyPressed('R'))
        CycleRenderPath();

    // +/- — Adjust roughness offset on grid
    if (input->WasKeyPressed(VK_OEM_PLUS) || input->WasKeyPressed(VK_ADD))
    {
        m_roughnessOffset = std::min(1.0f, m_roughnessOffset + 0.1f);
        ApplyMaterialPreset(m_currentPreset);
    }
    if (input->WasKeyPressed(VK_OEM_MINUS) || input->WasKeyPressed(VK_SUBTRACT))
    {
        m_roughnessOffset = std::max(-1.0f, m_roughnessOffset - 0.1f);
        ApplyMaterialPreset(m_currentPreset);
    }

    // Camera orbit
    if (input->IsMouseButtonDown(1))
    {
        auto md = input->GetMouseDelta();
        m_camYaw   += md.x * 0.3f;
        m_camPitch -= md.y * 0.3f;
        m_camPitch  = std::max(-80.0f, std::min(10.0f, m_camPitch));
    }

    float scroll = input->GetMouseScrollDelta();
    m_camDist -= scroll * 1.0f;
    m_camDist  = std::max(5.0f, std::min(40.0f, m_camDist));

    float radYaw   = m_camYaw   * (3.14159f / 180.0f);
    float radPitch = m_camPitch * (3.14159f / 180.0f);

    auto& camXf = world->GetComponent<Transform>(m_camera);
    camXf.position = {
        std::cos(radPitch) * std::sin(radYaw) * m_camDist,
        5.0f - std::sin(radPitch) * m_camDist,
        std::cos(radPitch) * std::cos(radYaw) * m_camDist
    };
    camXf.rotation = { m_camPitch, m_camYaw + 180.0f, 0.0f };

    // Animate materials
    UpdateAnimatedMaterials(dt);
}

// ---------------------------------------------------------------------------
void MaterialLabModule::OnRender() { /* RenderSystem auto-draws */ }
void MaterialLabModule::OnResize(int, int) { }

// ---------------------------------------------------------------------------
// DLL exports
// ---------------------------------------------------------------------------

SPARK_IMPLEMENT_MODULE(MaterialLabModule)
