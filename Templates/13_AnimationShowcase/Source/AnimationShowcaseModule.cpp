/**
 * @file AnimationShowcaseModule.cpp
 * @brief INTERMEDIATE — Skeletal animation, state machines, blending, layers, and IK.
 */

#include "AnimationShowcaseModule.h"
#include <Audio/AudioEngine.h>
#include <Input/InputManager.h>
#include <cmath>

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void AnimationShowcaseModule::OnLoad(Spark::IEngineContext* ctx)
{
    m_ctx = ctx;
    BuildScene();
    SetupPlayerAnimation();
    SetupAnimationEvents();
}

void AnimationShowcaseModule::OnUnload()
{
    // Unsubscribe animation events
    if (m_animEventSub)
    {
        auto* bus = m_ctx ? m_ctx->GetEventBus() : nullptr;
        if (bus) bus->Unsubscribe(m_animEventSub);
        m_animEventSub = 0;
    }

    auto* world = m_ctx ? m_ctx->GetWorld() : nullptr;
    if (!world) { m_ctx = nullptr; return; }

    for (auto e : { m_floor, m_light, m_camera, m_player })
        if (e != entt::null) world->DestroyEntity(e);

    for (auto e : m_npcs)
        if (e != entt::null) world->DestroyEntity(e);
    m_npcs.clear();

    m_ctx = nullptr;
}

// ---------------------------------------------------------------------------
// Scene construction
// ---------------------------------------------------------------------------

void AnimationShowcaseModule::BuildScene()
{
    auto* world = m_ctx->GetWorld();

    // Ground plane
    m_floor = world->CreateEntity();
    world->AddComponent<NameComponent>(m_floor, NameComponent{ "Ground" });
    world->AddComponent<Transform>(m_floor, Transform{
        { 0.f, -0.05f, 0.f }, { 0.f, 0.f, 0.f }, { 40.f, 0.1f, 40.f }
    });
    world->AddComponent<MeshRenderer>(m_floor, MeshRenderer{
        "Assets/Models/cube.obj", "Assets/Materials/grass.mat",
        false, true, true
    });

    // Directional light
    m_light = world->CreateEntity();
    world->AddComponent<NameComponent>(m_light, NameComponent{ "SunLight" });
    world->AddComponent<Transform>(m_light, Transform{
        { 0.f, 20.f, 0.f }, { -45.f, 30.f, 0.f }, { 1.f, 1.f, 1.f }
    });
    world->AddComponent<LightComponent>(m_light, LightComponent{
        LightComponent::Type::Directional,
        { 1.f, 0.95f, 0.85f }, 2.0f, 0.f,
        0.f, 0.f, true, 2048
    });

    // Camera
    m_camera = world->CreateEntity();
    world->AddComponent<NameComponent>(m_camera, NameComponent{ "MainCamera" });
    world->AddComponent<Transform>(m_camera, Transform{
        { 0.f, 5.f, -12.f }, { -20.f, 0.f, 0.f }, { 1.f, 1.f, 1.f }
    });
    world->AddComponent<Camera>(m_camera, Camera{
        60.0f, 0.1f, 500.0f, true
    });

    // Player character (animated mesh)
    m_player = world->CreateEntity();
    world->AddComponent<NameComponent>(m_player, NameComponent{ "Player" });
    world->AddComponent<Transform>(m_player, Transform{
        { 0.f, 0.f, 0.f }, { 0.f, 0.f, 0.f }, { 1.f, 1.f, 1.f }
    });
    world->AddComponent<MeshRenderer>(m_player, MeshRenderer{
        "Assets/Models/character.fbx", "Assets/Materials/character.mat",
        true, true, true
    });

    // Load audio for animation events
    auto* audio = m_ctx->GetAudio();
    if (audio)
    {
        audio->LoadSound("footstep_l", "Assets/Audio/footstep_left.wav");
        audio->LoadSound("footstep_r", "Assets/Audio/footstep_right.wav");
        audio->LoadSound("attack_whoosh", "Assets/Audio/attack_whoosh.wav");
        audio->LoadSound("jump_land", "Assets/Audio/jump_land.wav");
    }
}

// ---------------------------------------------------------------------------
// Player animation setup
// ---------------------------------------------------------------------------

void AnimationShowcaseModule::SetupPlayerAnimation()
{
    auto* world = m_ctx->GetWorld();

    // --- Animation component with clips ------------------------------------
    world->AddComponent<AnimationComponent>(m_player, AnimationComponent{
        "Assets/Animations/character_anims.fbx",  // skeleton + clip source
        true   // autoPlay
    });

    auto& anim = world->GetComponent<AnimationComponent>(m_player);
    anim.AddClip("idle",   "Assets/Animations/idle.anim",   true);   // looping
    anim.AddClip("walk",   "Assets/Animations/walk.anim",   true);
    anim.AddClip("run",    "Assets/Animations/run.anim",    true);
    anim.AddClip("attack", "Assets/Animations/attack.anim", false);  // one-shot
    anim.AddClip("jump",   "Assets/Animations/jump.anim",   false);

    // --- Animation state machine -------------------------------------------
    world->AddComponent<AnimationStateMachine>(m_player, AnimationStateMachine{});
    auto& sm = world->GetComponent<AnimationStateMachine>(m_player);

    // Define states
    sm.AddState("Idle",   "idle",   1.0f);
    sm.AddState("Walk",   "walk",   1.0f);
    sm.AddState("Run",    "run",    1.0f);
    sm.AddState("Attack", "attack", 1.2f);  // slightly faster playback
    sm.AddState("Jump",   "jump",   1.0f);

    // Define transitions with crossfade durations
    sm.AddTransition("Idle", "Walk", AnimationStateMachine::Condition{
        "speed", AnimationStateMachine::Op::GreaterThan, 0.1f
    }, kBlendTime);

    sm.AddTransition("Walk", "Idle", AnimationStateMachine::Condition{
        "speed", AnimationStateMachine::Op::LessThan, 0.1f
    }, kBlendTime);

    sm.AddTransition("Walk", "Run", AnimationStateMachine::Condition{
        "speed", AnimationStateMachine::Op::GreaterThan, 5.0f
    }, kBlendTime);

    sm.AddTransition("Run", "Walk", AnimationStateMachine::Condition{
        "speed", AnimationStateMachine::Op::LessThan, 5.0f
    }, kBlendTime);

    sm.AddTransition("Idle", "Attack", AnimationStateMachine::Condition{
        "attacking", AnimationStateMachine::Op::Equals, 1.0f
    }, 0.1f);

    sm.AddTransition("Attack", "Idle", AnimationStateMachine::Condition{
        "attackDone", AnimationStateMachine::Op::Equals, 1.0f
    }, kBlendTime);

    sm.AddTransition("Idle", "Jump", AnimationStateMachine::Condition{
        "jumping", AnimationStateMachine::Op::Equals, 1.0f
    }, 0.1f);

    sm.AddTransition("Walk", "Jump", AnimationStateMachine::Condition{
        "jumping", AnimationStateMachine::Op::Equals, 1.0f
    }, 0.1f);

    sm.AddTransition("Jump", "Idle", AnimationStateMachine::Condition{
        "grounded", AnimationStateMachine::Op::Equals, 1.0f
    }, kBlendTime);

    sm.SetDefaultState("Idle");

    // Initialize parameters
    sm.SetFloat("speed", 0.0f);
    sm.SetFloat("attacking", 0.0f);
    sm.SetFloat("attackDone", 0.0f);
    sm.SetFloat("jumping", 0.0f);
    sm.SetFloat("grounded", 1.0f);

    // --- Animation layers --------------------------------------------------
    // Layer 0: Full-body base layer (managed by state machine above)
    // Layer 1: Upper-body override for attack while moving
    world->AddComponent<AnimationLayer>(m_player, AnimationLayer{});
    auto& layers = world->GetComponent<AnimationLayer>(m_player);

    layers.AddLayer("BaseBody", 0, AnimationLayer::BlendMode::Override, 1.0f);
    layers.SetLayerMask("BaseBody", AnimationLayer::Mask::FullBody);

    layers.AddLayer("UpperBody", 1, AnimationLayer::BlendMode::Override, 0.0f);
    layers.SetLayerMask("UpperBody", AnimationLayer::Mask::UpperBody);

    // --- IK component for foot placement and hand targeting ----------------
    world->AddComponent<IKComponent>(m_player, IKComponent{});
    auto& ik = world->GetComponent<IKComponent>(m_player);

    ik.AddTarget(IKTarget{
        "LeftFoot",                         // name
        IKTarget::Type::FootPlacement,      // type
        "mixamorig:LeftFoot",               // bone name
        { 0.f, 0.f, 0.f },                 // target position (updated at runtime)
        1.0f,                               // weight
        2                                   // chain length
    });

    ik.AddTarget(IKTarget{
        "RightFoot",
        IKTarget::Type::FootPlacement,
        "mixamorig:RightFoot",
        { 0.f, 0.f, 0.f },
        1.0f,
        2
    });

    ik.AddTarget(IKTarget{
        "LeftHand",
        IKTarget::Type::HandReach,
        "mixamorig:LeftHand",
        { 0.f, 0.f, 0.f },
        0.0f,  // disabled by default
        2
    });

    ik.AddTarget(IKTarget{
        "RightHand",
        IKTarget::Type::HandReach,
        "mixamorig:RightHand",
        { 0.f, 0.f, 0.f },
        0.0f,
        2
    });

    ik.SetEnabled(m_ikEnabled);

    // --- Animation events (footstep sounds at keyframes) -------------------
    anim.AddEvent("walk", 0.25f, "footstep_left");   // left foot hits ground
    anim.AddEvent("walk", 0.75f, "footstep_right");  // right foot hits ground
    anim.AddEvent("run",  0.20f, "footstep_left");
    anim.AddEvent("run",  0.70f, "footstep_right");
    anim.AddEvent("attack", 0.4f, "attack_whoosh");
    anim.AddEvent("jump", 0.85f, "jump_land");
}

// ---------------------------------------------------------------------------
// Animation event subscription
// ---------------------------------------------------------------------------

void AnimationShowcaseModule::SetupAnimationEvents()
{
    auto* bus = m_ctx->GetEventBus();
    if (!bus) return;

    m_animEventSub = bus->Subscribe<Spark::AnimationEventFired>(
        [this](const Spark::AnimationEventFired& evt)
        {
            auto* audio = m_ctx->GetAudio();
            if (!audio) return;

            // Play the sound named by the event payload
            auto* world = m_ctx->GetWorld();
            if (!world) return;

            // Get the entity position for 3D audio
            if (world->HasComponent<Transform>(evt.entity))
            {
                auto& xf = world->GetComponent<Transform>(evt.entity);
                audio->Play3DSound(evt.eventName, xf.position, 1.0f);
            }
            else
            {
                audio->PlaySound(evt.eventName);
            }
        });
}

// ---------------------------------------------------------------------------
// NPC spawning
// ---------------------------------------------------------------------------

void AnimationShowcaseModule::SpawnPatrolNPC(float x, float z)
{
    auto* world = m_ctx->GetWorld();
    m_npcCounter++;

    std::string name = "PatrolNPC_" + std::to_string(m_npcCounter);

    auto npc = world->CreateEntity();
    world->AddComponent<NameComponent>(npc, NameComponent{ name.c_str() });
    world->AddComponent<Transform>(npc, Transform{
        { x, 0.f, z }, { 0.f, 0.f, 0.f }, { 1.f, 1.f, 1.f }
    });
    world->AddComponent<MeshRenderer>(npc, MeshRenderer{
        "Assets/Models/character.fbx", "Assets/Materials/npc_blue.mat",
        true, true, true
    });

    // Animation component with clips
    world->AddComponent<AnimationComponent>(npc, AnimationComponent{
        "Assets/Animations/character_anims.fbx", true
    });
    auto& anim = world->GetComponent<AnimationComponent>(npc);
    anim.AddClip("idle", "Assets/Animations/idle.anim", true);
    anim.AddClip("walk", "Assets/Animations/walk.anim", true);

    // Simple state machine: Idle <-> Walk
    world->AddComponent<AnimationStateMachine>(npc, AnimationStateMachine{});
    auto& sm = world->GetComponent<AnimationStateMachine>(npc);
    sm.AddState("Idle", "idle", 1.0f);
    sm.AddState("Walk", "walk", 1.0f);

    sm.AddTransition("Idle", "Walk", AnimationStateMachine::Condition{
        "speed", AnimationStateMachine::Op::GreaterThan, 0.1f
    }, kBlendTime);

    sm.AddTransition("Walk", "Idle", AnimationStateMachine::Condition{
        "speed", AnimationStateMachine::Op::LessThan, 0.1f
    }, kBlendTime);

    sm.SetDefaultState("Walk");
    sm.SetFloat("speed", kWalkSpeed);

    // IK for foot placement
    world->AddComponent<IKComponent>(npc, IKComponent{});
    auto& ik = world->GetComponent<IKComponent>(npc);
    ik.AddTarget(IKTarget{
        "LeftFoot", IKTarget::Type::FootPlacement,
        "mixamorig:LeftFoot", { 0.f, 0.f, 0.f }, 1.0f, 2
    });
    ik.AddTarget(IKTarget{
        "RightFoot", IKTarget::Type::FootPlacement,
        "mixamorig:RightFoot", { 0.f, 0.f, 0.f }, 1.0f, 2
    });
    ik.SetEnabled(m_ikEnabled);

    // Footstep events
    anim.AddEvent("walk", 0.25f, "footstep_left");
    anim.AddEvent("walk", 0.75f, "footstep_right");

    m_npcs.push_back(npc);
}

void AnimationShowcaseModule::SpawnDanceNPC(float x, float z)
{
    auto* world = m_ctx->GetWorld();
    m_npcCounter++;

    std::string name = "DanceNPC_" + std::to_string(m_npcCounter);

    auto npc = world->CreateEntity();
    world->AddComponent<NameComponent>(npc, NameComponent{ name.c_str() });
    world->AddComponent<Transform>(npc, Transform{
        { x, 0.f, z }, { 0.f, 0.f, 0.f }, { 1.f, 1.f, 1.f }
    });
    world->AddComponent<MeshRenderer>(npc, MeshRenderer{
        "Assets/Models/character.fbx", "Assets/Materials/npc_red.mat",
        true, true, true
    });

    // Animation with dance clip
    world->AddComponent<AnimationComponent>(npc, AnimationComponent{
        "Assets/Animations/character_anims.fbx", true
    });
    auto& anim = world->GetComponent<AnimationComponent>(npc);
    anim.AddClip("idle",  "Assets/Animations/idle.anim",  true);
    anim.AddClip("dance", "Assets/Animations/dance.anim", true);

    // State machine — always dancing
    world->AddComponent<AnimationStateMachine>(npc, AnimationStateMachine{});
    auto& sm = world->GetComponent<AnimationStateMachine>(npc);
    sm.AddState("Idle",  "idle",  1.0f);
    sm.AddState("Dance", "dance", 1.0f);

    sm.AddTransition("Idle", "Dance", AnimationStateMachine::Condition{
        "dancing", AnimationStateMachine::Op::Equals, 1.0f
    }, 0.5f); // slow crossfade into dance

    sm.SetDefaultState("Dance");
    sm.SetFloat("dancing", 1.0f);

    // Randomise playback speed for variety
    float speedVariation = 0.8f + static_cast<float>(rand() % 40) * 0.01f;
    anim.SetPlaybackSpeed(speedVariation);

    m_npcs.push_back(npc);
}

// ---------------------------------------------------------------------------
// Player movement and state machine parameter updates
// ---------------------------------------------------------------------------

void AnimationShowcaseModule::UpdatePlayerMovement(float dt)
{
    auto* input = m_ctx->GetInput();
    auto* world = m_ctx->GetWorld();
    if (!input || !world) return;

    auto& xf = world->GetComponent<Transform>(m_player);
    auto& sm = world->GetComponent<AnimationStateMachine>(m_player);
    auto& layers = world->GetComponent<AnimationLayer>(m_player);

    // --- Directional movement (WASD) ---------------------------------------
    float moveX = 0.f, moveZ = 0.f;
    if (input->IsKeyDown('W')) moveZ += 1.f;
    if (input->IsKeyDown('S')) moveZ -= 1.f;
    if (input->IsKeyDown('A')) moveX -= 1.f;
    if (input->IsKeyDown('D')) moveX += 1.f;

    float inputMag = std::sqrt(moveX * moveX + moveZ * moveZ);
    if (inputMag > 0.01f)
    {
        moveX /= inputMag;
        moveZ /= inputMag;

        // Determine if running (Shift held)
        bool running = input->IsKeyDown(VK_SHIFT);
        float targetSpeed = running ? kRunSpeed : kWalkSpeed;
        m_playerSpeed = targetSpeed;

        // Rotate player to face movement direction
        float targetYaw = std::atan2(moveX, moveZ) * (180.0f / 3.14159f);
        float yawDiff = targetYaw - m_playerYaw;
        // Normalize angle
        while (yawDiff > 180.f)  yawDiff -= 360.f;
        while (yawDiff < -180.f) yawDiff += 360.f;
        m_playerYaw += yawDiff * std::min(1.f, kRotateSpeed * dt / 180.f);
        xf.rotation.y = m_playerYaw;

        // Translate
        xf.position.x += moveX * m_playerSpeed * dt;
        xf.position.z += moveZ * m_playerSpeed * dt;
    }
    else
    {
        m_playerSpeed = 0.0f;
    }

    // Update state machine parameters
    sm.SetFloat("speed", m_playerSpeed);

    // --- Jump (Space) ------------------------------------------------------
    if (input->WasKeyPressed(VK_SPACE))
    {
        sm.SetFloat("jumping", 1.0f);
        sm.SetFloat("grounded", 0.0f);
    }

    // Auto-reset jump after clip finishes (simplified: timer-based)
    auto& anim = world->GetComponent<AnimationComponent>(m_player);
    if (sm.GetCurrentState() == "Jump" && anim.IsClipFinished("jump"))
    {
        sm.SetFloat("jumping", 0.0f);
        sm.SetFloat("grounded", 1.0f);
    }

    // --- Attack (LMB) — plays on upper-body layer while moving -------------
    if (input->IsMouseButtonDown(0))
    {
        // If moving, use upper-body layer overlay so legs keep walking
        if (m_playerSpeed > 0.1f)
        {
            layers.SetLayerWeight("UpperBody", 1.0f);
            layers.PlayClipOnLayer("UpperBody", "attack", 0.1f);
        }
        else
        {
            sm.SetFloat("attacking", 1.0f);
        }
    }

    // Reset attack state when clip done
    if (sm.GetCurrentState() == "Attack" && anim.IsClipFinished("attack"))
    {
        sm.SetFloat("attacking", 0.0f);
        sm.SetFloat("attackDone", 1.0f);
    }
    else
    {
        sm.SetFloat("attackDone", 0.0f);
    }

    // Fade out upper-body layer when attack finishes
    if (layers.GetLayerWeight("UpperBody") > 0.0f && anim.IsClipFinished("attack"))
    {
        float w = layers.GetLayerWeight("UpperBody") - dt * 4.0f;
        layers.SetLayerWeight("UpperBody", std::max(0.0f, w));
    }
}

// ---------------------------------------------------------------------------
// Camera follow
// ---------------------------------------------------------------------------

void AnimationShowcaseModule::UpdateCamera(float dt)
{
    auto* input = m_ctx->GetInput();
    auto* world = m_ctx->GetWorld();
    if (!input || !world) return;

    // Right-click drag to orbit
    if (input->IsMouseButtonDown(1))
    {
        auto md = input->GetMouseDelta();
        m_camYaw   += md.x * 0.3f;
        m_camPitch -= md.y * 0.3f;
        m_camPitch  = std::max(-80.0f, std::min(10.0f, m_camPitch));
    }

    float scroll = input->GetMouseScrollDelta();
    m_camDist -= scroll * 1.0f;
    m_camDist  = std::max(4.0f, std::min(25.0f, m_camDist));

    auto& playerXf = world->GetComponent<Transform>(m_player);
    float radYaw   = m_camYaw   * (3.14159f / 180.0f);
    float radPitch = m_camPitch * (3.14159f / 180.0f);

    auto& camXf = world->GetComponent<Transform>(m_camera);
    camXf.position = {
        playerXf.position.x + std::cos(radPitch) * std::sin(radYaw) * m_camDist,
        playerXf.position.y + 2.0f - std::sin(radPitch) * m_camDist,
        playerXf.position.z + std::cos(radPitch) * std::cos(radYaw) * m_camDist
    };
    camXf.rotation = { m_camPitch, m_camYaw + 180.0f, 0.0f };
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

void AnimationShowcaseModule::OnUpdate(float dt)
{
    auto* input = m_ctx->GetInput();
    if (!input) return;

    // P — Pause / resume all animations
    if (input->WasKeyPressed('P'))
    {
        m_animPaused = !m_animPaused;
        auto* world = m_ctx->GetWorld();
        if (world)
        {
            // Set playback speed to 0 or 1 on all animated entities
            auto setSpeed = [&](entt::entity e, float speed) {
                if (world->HasComponent<AnimationComponent>(e))
                    world->GetComponent<AnimationComponent>(e).SetPlaybackSpeed(speed);
            };

            float speed = m_animPaused ? 0.0f : 1.0f;
            setSpeed(m_player, speed);
            for (auto npc : m_npcs)
                setSpeed(npc, speed);
        }
    }

    if (m_animPaused) return;

    // I — Toggle IK
    if (input->WasKeyPressed('I'))
    {
        m_ikEnabled = !m_ikEnabled;
        auto* world = m_ctx->GetWorld();
        if (world)
        {
            auto toggleIK = [&](entt::entity e) {
                if (world->HasComponent<IKComponent>(e))
                    world->GetComponent<IKComponent>(e).SetEnabled(m_ikEnabled);
            };
            toggleIK(m_player);
            for (auto npc : m_npcs)
                toggleIK(npc);
        }
    }

    // 1 — Spawn patrol NPC at random offset from player
    if (input->WasKeyPressed('1'))
    {
        auto* world = m_ctx->GetWorld();
        auto& pxf = world->GetComponent<Transform>(m_player);
        float ox = (static_cast<float>(rand() % 100) - 50.f) * 0.1f;
        float oz = (static_cast<float>(rand() % 100) - 50.f) * 0.1f;
        SpawnPatrolNPC(pxf.position.x + 5.f + ox, pxf.position.z + oz);
    }

    // 2 — Spawn dance NPC
    if (input->WasKeyPressed('2'))
    {
        auto* world = m_ctx->GetWorld();
        auto& pxf = world->GetComponent<Transform>(m_player);
        float ox = (static_cast<float>(rand() % 100) - 50.f) * 0.1f;
        float oz = (static_cast<float>(rand() % 100) - 50.f) * 0.1f;
        SpawnDanceNPC(pxf.position.x - 5.f + ox, pxf.position.z + oz);
    }

    // Update patrol NPC movement (simple circular patrol)
    {
        auto* world = m_ctx->GetWorld();
        static float patrolTimer = 0.0f;
        patrolTimer += dt;

        for (auto npc : m_npcs)
        {
            if (!world->HasComponent<AnimationStateMachine>(npc)) continue;
            auto& sm = world->GetComponent<AnimationStateMachine>(npc);

            // Patrol NPCs walk in a circle
            if (sm.GetCurrentState() == "Walk" || sm.GetFloat("speed") > 0.1f)
            {
                auto& xf = world->GetComponent<Transform>(npc);
                float angle = patrolTimer * 0.5f + static_cast<float>(
                    reinterpret_cast<uintptr_t>(&xf) % 100) * 0.1f;
                xf.position.x += std::cos(angle) * kWalkSpeed * 0.3f * dt;
                xf.position.z += std::sin(angle) * kWalkSpeed * 0.3f * dt;
                xf.rotation.y = angle * (180.0f / 3.14159f);
            }
        }
    }

    UpdatePlayerMovement(dt);
    UpdateCamera(dt);
}

// ---------------------------------------------------------------------------
void AnimationShowcaseModule::OnRender() { /* RenderSystem auto-draws */ }
void AnimationShowcaseModule::OnResize(int, int) { }

// ---------------------------------------------------------------------------
// DLL exports
// ---------------------------------------------------------------------------

SPARK_IMPLEMENT_MODULE(AnimationShowcaseModule)
