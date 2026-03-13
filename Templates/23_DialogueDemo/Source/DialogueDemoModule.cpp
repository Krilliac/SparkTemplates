/**
 * @file DialogueDemoModule.cpp
 * @brief INTERMEDIATE — Branching dialogue system showcase.
 */

#include "DialogueDemoModule.h"
#include <Input/InputManager.h>
#include <cmath>
#include <algorithm>
#include <cstdio>

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void DialogueDemoModule::OnLoad(Spark::IEngineContext* ctx)
{
    m_ctx = ctx;

    // Subscribe to dialogue events
    auto* bus = m_ctx->GetEventBus();
    if (bus)
    {
        m_dialogueStartSubId = bus->Subscribe<DialogueStartedEvent>(
            [this](const DialogueStartedEvent& evt)
            {
                auto* audio = m_ctx->GetAudioEngine();
                if (audio)
                    audio->PlayOneShot("Assets/Audio/dialogue_open.wav", 0.5f);
            });

        m_dialogueEndSubId = bus->Subscribe<DialogueEndedEvent>(
            [this](const DialogueEndedEvent& evt)
            {
                auto* audio = m_ctx->GetAudioEngine();
                if (audio)
                    audio->PlayOneShot("Assets/Audio/dialogue_close.wav", 0.4f);
            });

        m_choiceSubId = bus->Subscribe<DialogueChoiceMadeEvent>(
            [this](const DialogueChoiceMadeEvent& evt)
            {
                auto* audio = m_ctx->GetAudioEngine();
                if (audio)
                    audio->PlayOneShot("Assets/Audio/dialogue_select.wav", 0.3f);
            });
    }

    BuildDialogueTrees();
    BuildScene();
}

void DialogueDemoModule::OnUnload()
{
    auto* bus = m_ctx ? m_ctx->GetEventBus() : nullptr;
    if (bus)
    {
        bus->Unsubscribe<DialogueStartedEvent>(m_dialogueStartSubId);
        bus->Unsubscribe<DialogueEndedEvent>(m_dialogueEndSubId);
        bus->Unsubscribe<DialogueChoiceMadeEvent>(m_choiceSubId);
    }

    auto* world = m_ctx ? m_ctx->GetWorld() : nullptr;
    if (!world) { m_ctx = nullptr; return; }

    for (auto e : { m_player, m_camera, m_light })
        if (e != entt::null) world->DestroyEntity(e);

    for (auto& npc : m_npcs)
        if (npc.entity != entt::null) world->DestroyEntity(npc.entity);

    for (auto e : m_sceneEntities)
        if (e != entt::null) world->DestroyEntity(e);

    m_npcs.clear();
    m_sceneEntities.clear();
    m_ctx = nullptr;
}

// ---------------------------------------------------------------------------
// Scene construction
// ---------------------------------------------------------------------------

void DialogueDemoModule::BuildScene()
{
    auto* world = m_ctx->GetWorld();

    // --- Player entity ------------------------------------------------------
    m_player = world->CreateEntity();
    world->AddComponent<NameComponent>(m_player, NameComponent{ "Player" });
    world->AddComponent<Transform>(m_player, Transform{
        { m_playerX, m_playerY, m_playerZ },
        { 0.0f, 0.0f, 0.0f },
        { 0.8f, 1.8f, 0.8f }
    });
    world->AddComponent<MeshRenderer>(m_player, MeshRenderer{
        "Assets/Models/cube.obj", "Assets/Materials/player.mat",
        true, true, true
    });

    // --- Camera (third-person overhead) -------------------------------------
    m_camera = world->CreateEntity();
    world->AddComponent<NameComponent>(m_camera, NameComponent{ "MainCamera" });
    world->AddComponent<Transform>(m_camera, Transform{
        { 0.0f, 12.0f, -12.0f },
        { -35.0f, 0.0f, 0.0f },
        { 1.0f, 1.0f, 1.0f }
    });
    world->AddComponent<Camera>(m_camera, Camera{
        55.0f, 0.1f, 500.0f, true
    });

    // --- Light --------------------------------------------------------------
    m_light = world->CreateEntity();
    world->AddComponent<NameComponent>(m_light, NameComponent{ "SunLight" });
    world->AddComponent<Transform>(m_light, Transform{
        { 0.0f, 20.0f, 0.0f },
        { -50.0f, 25.0f, 0.0f },
        { 1.0f, 1.0f, 1.0f }
    });
    world->AddComponent<LightComponent>(m_light, LightComponent{
        LightComponent::Type::Directional,
        { 1.0f, 0.95f, 0.85f }, 2.0f, 0.0f,
        0.0f, 0.0f, true, 2048
    });

    // --- Ground (village square) --------------------------------------------
    auto ground = world->CreateEntity();
    world->AddComponent<NameComponent>(ground, NameComponent{ "Ground" });
    world->AddComponent<Transform>(ground, Transform{
        { 0.0f, -0.1f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 30.0f, 0.2f, 30.0f }
    });
    world->AddComponent<MeshRenderer>(ground, MeshRenderer{
        "Assets/Models/cube.obj", "Assets/Materials/cobblestone.mat",
        false, true, true
    });
    m_sceneEntities.push_back(ground);

    // --- NPC: Merchant (green) ----------------------------------------------
    {
        auto e = world->CreateEntity();
        world->AddComponent<NameComponent>(e, NameComponent{ "Merchant" });
        world->AddComponent<Transform>(e, Transform{
            { -5.0f, 1.0f, 3.0f },
            { 0.0f, 45.0f, 0.0f },
            { 1.0f, 2.0f, 1.0f }
        });
        world->AddComponent<MeshRenderer>(e, MeshRenderer{
            "Assets/Models/cube.obj", "Assets/Materials/green.mat",
            true, true, true
        });
        m_npcs.push_back({ e, "Merchant", -5.0f, 1.0f, 3.0f, "merchant_tree" });
    }

    // --- NPC: Guard (red) ---------------------------------------------------
    {
        auto e = world->CreateEntity();
        world->AddComponent<NameComponent>(e, NameComponent{ "Guard" });
        world->AddComponent<Transform>(e, Transform{
            { 5.0f, 1.0f, 3.0f },
            { 0.0f, -45.0f, 0.0f },
            { 1.2f, 2.2f, 1.2f }
        });
        world->AddComponent<MeshRenderer>(e, MeshRenderer{
            "Assets/Models/cube.obj", "Assets/Materials/red.mat",
            true, true, true
        });
        m_npcs.push_back({ e, "Guard", 5.0f, 1.0f, 3.0f, "guard_tree" });
    }

    // --- NPC: Wizard (blue) -------------------------------------------------
    {
        auto e = world->CreateEntity();
        world->AddComponent<NameComponent>(e, NameComponent{ "Wizard" });
        world->AddComponent<Transform>(e, Transform{
            { 0.0f, 1.0f, 8.0f },
            { 0.0f, 180.0f, 0.0f },
            { 1.0f, 2.4f, 1.0f }
        });
        world->AddComponent<MeshRenderer>(e, MeshRenderer{
            "Assets/Models/cube.obj", "Assets/Materials/blue.mat",
            true, true, true
        });
        m_npcs.push_back({ e, "Wizard", 0.0f, 1.0f, 8.0f, "wizard_tree" });
    }

    // --- Market stall (decorative) ------------------------------------------
    auto stall = world->CreateEntity();
    world->AddComponent<NameComponent>(stall, NameComponent{ "MarketStall" });
    world->AddComponent<Transform>(stall, Transform{
        { -5.0f, 1.5f, 5.0f }, { 0.0f, 0.0f, 0.0f }, { 3.0f, 3.0f, 2.0f }
    });
    world->AddComponent<MeshRenderer>(stall, MeshRenderer{
        "Assets/Models/cube.obj", "Assets/Materials/wood.mat",
        false, true, true
    });
    m_sceneEntities.push_back(stall);

    // --- Gate (decorative) --------------------------------------------------
    auto gate = world->CreateEntity();
    world->AddComponent<NameComponent>(gate, NameComponent{ "Gate" });
    world->AddComponent<Transform>(gate, Transform{
        { 5.0f, 2.5f, 5.0f }, { 0.0f, 0.0f, 0.0f }, { 4.0f, 5.0f, 0.5f }
    });
    world->AddComponent<MeshRenderer>(gate, MeshRenderer{
        "Assets/Models/cube.obj", "Assets/Materials/stone.mat",
        false, true, true
    });
    m_sceneEntities.push_back(gate);
}

// ---------------------------------------------------------------------------
// Dialogue tree construction
// ---------------------------------------------------------------------------

void DialogueDemoModule::BuildDialogueTrees()
{
    BuildMerchantTree();
    BuildGuardTree();
    BuildWizardTree();

    // Also register with engine DialogueSystem
    auto* dialogueSys = DialogueSystem::GetInstance();
    if (dialogueSys)
    {
        for (auto& [treeID, treeData] : m_trees)
        {
            DialogueTree engineTree;
            for (auto& [nodeID, nodeData] : treeData.nodes)
            {
                DialogueNode engineNode;
                engineNode.speakerName  = nodeData.speakerName;
                engineNode.text         = nodeData.text;
                engineNode.voiceClipPath = nodeData.voiceClipPath;

                for (auto& choice : nodeData.choices)
                {
                    engineNode.choices.push_back({
                        choice.text, choice.nextNodeID, choice.conditionFlag
                    });
                }

                engineTree.AddNode(nodeID, engineNode);
            }
            engineTree.SetStartNode(treeData.startNodeID);
            dialogueSys->LoadTree(treeID, engineTree);
        }
    }
}

void DialogueDemoModule::BuildMerchantTree()
{
    DialogueTreeData tree;
    tree.treeID      = "merchant_tree";
    tree.startNodeID = 0;

    // Node 0: Greeting
    tree.nodes[0] = {
        0, "Merchant",
        "Welcome, traveller! I have the finest wares in all the land. What brings you to my stall?",
        {
            { "I'd like to buy something.", 1, "" },
            { "I'd like to sell something.", 2, "" },
            { "Just looking. Goodbye.", 99, "" }
        },
        "Assets/Audio/merchant_greet.wav"
    };

    // Node 1: Buy branch
    tree.nodes[1] = {
        1, "Merchant",
        "Excellent! I have potions for 10 gold, and a special city pass for 25 gold. What'll it be?",
        {
            { "Buy a potion (10 gold).", 10, "" },
            { "Buy the city pass (25 gold).", 11, "" },
            { "Never mind, let me think.", 0, "" }
        },
        ""
    };

    // Node 2: Sell branch
    tree.nodes[2] = {
        2, "Merchant",
        "Hmm, let me see what you've got... I can offer 5 gold for any trinkets you have.",
        {
            { "Sell a trinket (+5 gold).", 20, "" },
            { "Actually, no thanks.", 0, "" }
        },
        ""
    };

    // Node 10: Buy potion
    tree.nodes[10] = {
        10, "Merchant",
        "A fine potion! That'll be 10 gold. Your health will thank you!",
        {
            { "Thanks! (Continue shopping)", 1, "" },
            { "That's all for now. Goodbye.", 99, "" }
        },
        ""
    };

    // Node 11: Buy pass
    tree.nodes[11] = {
        11, "Merchant",
        "Ah, the city pass! Very wise. The guard at the gate won't let you through without one. Here you go.",
        {
            { "Thanks! (Continue shopping)", 1, "" },
            { "That's all for now. Goodbye.", 99, "" }
        },
        ""
    };

    // Node 20: Sell trinket
    tree.nodes[20] = {
        20, "Merchant",
        "Here's your 5 gold. Pleasure doing business with you!",
        {
            { "Anything else?", 0, "" },
            { "Goodbye.", 99, "" }
        },
        ""
    };

    // Node 99: Goodbye
    tree.nodes[99] = {
        99, "Merchant",
        "Safe travels, friend! Come back any time!",
        {},
        ""
    };

    m_trees[tree.treeID] = tree;
}

void DialogueDemoModule::BuildGuardTree()
{
    DialogueTreeData tree;
    tree.treeID      = "guard_tree";
    tree.startNodeID = 0;

    // Node 0: Halt
    tree.nodes[0] = {
        0, "Guard",
        "Halt! Nobody passes through this gate without proper authorization. State your business.",
        {
            { "I have a city pass. (Show pass)", 1, "has_pass" },
            { "Perhaps some gold would change your mind? (Bribe: 15 gold)", 2, "" },
            { "I'll fight my way through!", 3, "" },
            { "Sorry, I'll come back later.", 99, "" }
        },
        "Assets/Audio/guard_halt.wav"
    };

    // Node 1: Show pass
    tree.nodes[1] = {
        1, "Guard",
        "A city pass! Very well, you may enter. Move along, citizen.",
        {
            { "Thank you, sir.", 99, "" }
        },
        ""
    };

    // Node 2: Bribe
    tree.nodes[2] = {
        2, "Guard",
        "Hmm... well, I suppose I could look the other way. Just this once. Don't tell anyone.",
        {
            { "Your secret is safe with me. (Pay 15 gold)", 20, "" },
            { "On second thought, never mind.", 0, "" }
        },
        ""
    };

    // Node 20: Bribe accepted
    tree.nodes[20] = {
        20, "Guard",
        "Pleasure doing business. Now move along before my captain sees.",
        {
            { "Thanks.", 99, "" }
        },
        ""
    };

    // Node 3: Fight option
    tree.nodes[3] = {
        3, "Guard",
        "Ha! You think you can take on a city guard? I admire your courage, but you'd best reconsider. I'll let this slide... this time.",
        {
            { "You're right, sorry about that.", 0, "" },
            { "Fine, I'll find another way.", 99, "" }
        },
        ""
    };

    // Node 99: Goodbye
    tree.nodes[99] = {
        99, "Guard",
        "Move along now.",
        {},
        ""
    };

    m_trees[tree.treeID] = tree;
}

void DialogueDemoModule::BuildWizardTree()
{
    DialogueTreeData tree;
    tree.treeID      = "wizard_tree";
    tree.startNodeID = 0;

    // Node 0: Quest offer
    tree.nodes[0] = {
        0, "Wizard",
        "Ah, a visitor! I am Aldric the Wise. I sense great potential in you. I have a quest, if you are willing.",
        {
            { "I'm interested. Tell me more.", 1, "" },
            { "No thanks, I'm busy.", 2, "" }
        },
        "Assets/Audio/wizard_greet.wav"
    };

    // Node 1: Quest details
    tree.nodes[1] = {
        1, "Wizard",
        "Deep in the forest lies the Crystal of Eternity. Bring it to me, and I shall reward you with great power. Do you accept?",
        {
            { "I accept the quest!", 10, "" },
            { "That sounds dangerous. Let me think about it.", 2, "" }
        },
        ""
    };

    // Node 2: Decline
    tree.nodes[2] = {
        2, "Wizard",
        "I understand. The path is not for everyone. Return if you change your mind.",
        {
            { "Actually, tell me about the quest.", 1, "" },
            { "Goodbye, Wizard.", 99, "" }
        },
        ""
    };

    // Node 10: Accept quest
    tree.nodes[10] = {
        10, "Wizard",
        "Wonderful! Take this enchanted compass. It will guide you to the Crystal. May the stars light your path!",
        {
            { "I won't let you down!", 11, "" }
        },
        ""
    };

    // Node 11: Follow-up after accepting
    tree.nodes[11] = {
        11, "Wizard",
        "One more thing — beware the forest guardian. It protects the Crystal fiercely. You may need allies. Speak to the Merchant for supplies.",
        {
            { "I'll prepare well. Thank you.", 99, "" }
        },
        ""
    };

    // Node 99: Goodbye
    tree.nodes[99] = {
        99, "Wizard",
        "Until we meet again, brave one. The stars watch over you.",
        {},
        ""
    };

    m_trees[tree.treeID] = tree;
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

void DialogueDemoModule::OnUpdate(float dt)
{
    HandleInput(dt);

    auto* world = m_ctx->GetWorld();
    if (!world) return;

    // Update player transform
    if (m_player != entt::null)
    {
        auto& xf = world->GetComponent<Transform>(m_player);
        xf.position = { m_playerX, m_playerY, m_playerZ };
        xf.rotation.y = m_camYaw;
    }

    // Camera follow (top-down-ish)
    if (m_camera != entt::null)
    {
        auto& camXf = world->GetComponent<Transform>(m_camera);
        float t = 1.0f - std::exp(-4.0f * dt);
        camXf.position.x += (m_playerX - camXf.position.x) * t;
        camXf.position.y += (m_playerY + 12.0f - camXf.position.y) * t;
        camXf.position.z += (m_playerZ - 12.0f - camXf.position.z) * t;
    }

    // Find nearest NPC (when not in dialogue)
    if (!m_inDialogue)
        m_nearestNPC = FindNearestNPC(3.0f);
    else
        m_nearestNPC = -1;
}

// ---------------------------------------------------------------------------
// Input handling
// ---------------------------------------------------------------------------

void DialogueDemoModule::HandleInput(float dt)
{
    auto* input = m_ctx->GetInput();
    if (!input) return;

    if (m_inDialogue)
    {
        // --- Dialogue input -------------------------------------------------
        // Number keys to select choices
        if (input->WasKeyPressed('1')) AdvanceOrSelectChoice(0);
        if (input->WasKeyPressed('2')) AdvanceOrSelectChoice(1);
        if (input->WasKeyPressed('3')) AdvanceOrSelectChoice(2);

        // Escape to exit dialogue
        if (input->WasKeyPressed(VK_ESCAPE))
            EndDialogue();

        return; // No movement during dialogue
    }

    // --- WASD movement (top-down) -------------------------------------------
    float moveX = 0.0f, moveZ = 0.0f;
    if (input->IsKeyDown('W')) moveZ += 1.0f;
    if (input->IsKeyDown('S')) moveZ -= 1.0f;
    if (input->IsKeyDown('A')) moveX -= 1.0f;
    if (input->IsKeyDown('D')) moveX += 1.0f;

    float speed = 5.0f;
    m_playerX += moveX * speed * dt;
    m_playerZ += moveZ * speed * dt;

    // Update facing direction
    if (std::abs(moveX) > 0.01f || std::abs(moveZ) > 0.01f)
        m_camYaw = std::atan2(moveX, moveZ) * (180.0f / 3.14159f);

    // --- E to start dialogue with nearby NPC --------------------------------
    if (input->WasKeyPressed('E') && m_nearestNPC >= 0)
        StartDialogueWith(m_nearestNPC);
}

// ---------------------------------------------------------------------------
// NPC proximity
// ---------------------------------------------------------------------------

int DialogueDemoModule::FindNearestNPC(float maxDist) const
{
    int closest = -1;
    float closestDist = maxDist;

    for (int i = 0; i < static_cast<int>(m_npcs.size()); ++i)
    {
        float dx = m_npcs[i].x - m_playerX;
        float dz = m_npcs[i].z - m_playerZ;
        float dist = std::sqrt(dx * dx + dz * dz);
        if (dist < closestDist)
        {
            closestDist = dist;
            closest = i;
        }
    }
    return closest;
}

// ---------------------------------------------------------------------------
// Dialogue management
// ---------------------------------------------------------------------------

void DialogueDemoModule::StartDialogueWith(int npcIndex)
{
    if (npcIndex < 0 || npcIndex >= static_cast<int>(m_npcs.size())) return;

    auto& npc = m_npcs[npcIndex];
    m_inDialogue    = true;
    m_activeNPCIndex = npcIndex;
    m_activeTreeID  = npc.treeID;

    auto it = m_trees.find(m_activeTreeID);
    if (it == m_trees.end()) { m_inDialogue = false; return; }

    m_currentNodeID = it->second.startNodeID;

    // Notify engine dialogue system
    auto* dialogueSys = DialogueSystem::GetInstance();
    if (dialogueSys)
        dialogueSys->StartDialogue(m_activeTreeID);

    // Play voice clip if available
    auto& node = it->second.nodes[m_currentNodeID];
    if (!node.voiceClipPath.empty())
    {
        auto* audio = m_ctx->GetAudioEngine();
        if (audio)
            audio->PlayOneShot(node.voiceClipPath.c_str(), 0.7f);
    }

    auto* bus = m_ctx->GetEventBus();
    if (bus)
        bus->Publish(DialogueStartedEvent{ npc.name });
}

void DialogueDemoModule::AdvanceOrSelectChoice(int choiceNum)
{
    auto it = m_trees.find(m_activeTreeID);
    if (it == m_trees.end()) return;

    auto nodeIt = it->second.nodes.find(m_currentNodeID);
    if (nodeIt == it->second.nodes.end()) return;

    auto& node = nodeIt->second;

    // If no choices, dialogue is complete
    if (node.choices.empty())
    {
        EndDialogue();
        return;
    }

    // Build list of available choices (filter by conditions)
    std::vector<int> availableIndices;
    for (int i = 0; i < static_cast<int>(node.choices.size()); ++i)
    {
        auto& choice = node.choices[i];
        if (choice.conditionFlag.empty() || m_flags[choice.conditionFlag])
            availableIndices.push_back(i);
    }

    if (choiceNum < 0 || choiceNum >= static_cast<int>(availableIndices.size()))
        return;

    int actualIdx = availableIndices[choiceNum];
    auto& chosen  = node.choices[actualIdx];

    // --- Apply side effects based on dialogue choice ------------------------
    // Merchant: buy potion
    if (m_activeTreeID == "merchant_tree" && m_currentNodeID == 1 && actualIdx == 0)
    {
        if (m_gold >= 10) { m_gold -= 10; m_score += 10; }
    }
    // Merchant: buy pass
    if (m_activeTreeID == "merchant_tree" && m_currentNodeID == 1 && actualIdx == 1)
    {
        if (m_gold >= 25) { m_gold -= 25; m_hasPass = true; m_flags["has_pass"] = true; m_score += 25; }
    }
    // Merchant: sell trinket
    if (m_activeTreeID == "merchant_tree" && m_currentNodeID == 2 && actualIdx == 0)
    {
        m_gold += 5; m_score += 5;
    }
    // Guard: bribe accepted
    if (m_activeTreeID == "guard_tree" && m_currentNodeID == 2 && actualIdx == 0)
    {
        if (m_gold >= 15) { m_gold -= 15; m_score += 20; }
    }
    // Wizard: accept quest
    if (m_activeTreeID == "wizard_tree" && m_currentNodeID == 1 && actualIdx == 0)
    {
        m_hasQuest = true; m_flags["has_quest"] = true; m_score += 50;
    }

    // Publish choice event
    auto* bus = m_ctx->GetEventBus();
    if (bus && m_activeNPCIndex >= 0)
    {
        bus->Publish(DialogueChoiceMadeEvent{
            m_npcs[m_activeNPCIndex].name,
            m_currentNodeID,
            choiceNum,
            chosen.text
        });
    }

    // Notify engine dialogue system
    auto* dialogueSys = DialogueSystem::GetInstance();
    if (dialogueSys)
        dialogueSys->SelectChoice(choiceNum);

    // Navigate to next node
    int nextID = chosen.nextNodeID;
    auto nextIt = it->second.nodes.find(nextID);
    if (nextIt == it->second.nodes.end())
    {
        EndDialogue();
        return;
    }

    m_currentNodeID = nextID;

    // Play voice clip for new node
    auto& nextNode = nextIt->second;
    if (!nextNode.voiceClipPath.empty())
    {
        auto* audio = m_ctx->GetAudioEngine();
        if (audio)
            audio->PlayOneShot(nextNode.voiceClipPath.c_str(), 0.7f);
    }

    // If new node has no choices, it's a terminal — user can press any key to close
}

void DialogueDemoModule::EndDialogue()
{
    if (!m_inDialogue) return;

    auto* bus = m_ctx->GetEventBus();
    if (bus && m_activeNPCIndex >= 0)
        bus->Publish(DialogueEndedEvent{ m_npcs[m_activeNPCIndex].name });

    auto* dialogueSys = DialogueSystem::GetInstance();
    if (dialogueSys)
        dialogueSys->AdvanceDialogue(); // signals end

    m_inDialogue     = false;
    m_activeNPCIndex = -1;
    m_currentNodeID  = -1;
}

// ---------------------------------------------------------------------------
// Render
// ---------------------------------------------------------------------------

void DialogueDemoModule::OnRender()
{
    DrawHUD();

    if (m_inDialogue)
        DrawDialogueUI();
    else if (m_nearestNPC >= 0)
        DrawInteractionPrompt(m_npcs[m_nearestNPC].name);
}

// ---------------------------------------------------------------------------
// Dialogue UI
// ---------------------------------------------------------------------------

void DialogueDemoModule::DrawDialogueUI()
{
    auto* dd = Spark::DebugDraw::GetInstance();
    if (!dd) return;

    auto it = m_trees.find(m_activeTreeID);
    if (it == m_trees.end()) return;

    auto nodeIt = it->second.nodes.find(m_currentNodeID);
    if (nodeIt == it->second.nodes.end()) return;

    auto& node = nodeIt->second;

    // Dialogue box background (lower portion of screen)
    float boxX = m_screenW * 0.1f;
    float boxY = m_screenH * 0.55f;
    float boxW = m_screenW * 0.8f;
    float boxH = m_screenH * 0.4f;

    dd->ScreenRect({ boxX, boxY }, { boxX + boxW, boxY + boxH },
                    { 0.05f, 0.05f, 0.1f, 0.85f });

    // Border
    dd->ScreenLine({ boxX, boxY }, { boxX + boxW, boxY },
                    { 0.4f, 0.4f, 0.6f, 0.9f });
    dd->ScreenLine({ boxX + boxW, boxY }, { boxX + boxW, boxY + boxH },
                    { 0.4f, 0.4f, 0.6f, 0.9f });
    dd->ScreenLine({ boxX + boxW, boxY + boxH }, { boxX, boxY + boxH },
                    { 0.4f, 0.4f, 0.6f, 0.9f });
    dd->ScreenLine({ boxX, boxY + boxH }, { boxX, boxY },
                    { 0.4f, 0.4f, 0.6f, 0.9f });

    // Speaker name (top of box)
    float textX = boxX + 20.0f;
    float textY = boxY + 15.0f;

    dd->ScreenRect({ textX - 5.0f, textY - 5.0f },
                    { textX + static_cast<float>(node.speakerName.size()) * 10.0f + 10.0f,
                      textY + 20.0f },
                    { 0.2f, 0.2f, 0.35f, 0.9f });

    dd->ScreenText({ textX, textY }, node.speakerName.c_str(),
                    { 1.0f, 0.85f, 0.3f, 1.0f });

    // Dialogue text (centre of box)
    float dialogueY = textY + 35.0f;

    // Simple word wrapping: split text into lines of ~80 chars
    std::string fullText = node.text;
    float lineY = dialogueY;
    int lineLen = 80;
    for (size_t pos = 0; pos < fullText.size(); pos += lineLen)
    {
        size_t end = std::min(pos + static_cast<size_t>(lineLen), fullText.size());
        // Try to break at a space
        if (end < fullText.size() && fullText[end] != ' ')
        {
            size_t space = fullText.rfind(' ', end);
            if (space > pos) end = space + 1;
        }
        std::string line = fullText.substr(pos, end - pos);
        dd->ScreenText({ textX, lineY }, line.c_str(),
                        { 1.0f, 1.0f, 1.0f, 1.0f });
        lineY += 20.0f;

        // Adjust pos for word-break
        if (end != pos + static_cast<size_t>(lineLen))
            pos = end - lineLen; // will be incremented by lineLen in loop
    }

    // Choices (bottom of box)
    float choiceY = boxY + boxH - 30.0f;

    if (node.choices.empty())
    {
        dd->ScreenText({ textX, choiceY }, "[Press 1 to continue]",
                        { 0.6f, 0.6f, 0.8f, 0.9f });
    }
    else
    {
        // Build available choices
        int displayNum = 1;
        float choiceStartY = choiceY - static_cast<float>(node.choices.size()) * 22.0f;

        for (int i = 0; i < static_cast<int>(node.choices.size()); ++i)
        {
            auto& choice = node.choices[i];

            // Check condition
            bool available = choice.conditionFlag.empty() || m_flags[choice.conditionFlag];

            if (available)
            {
                char buf[256];
                std::snprintf(buf, sizeof(buf), "%d. %s", displayNum, choice.text.c_str());
                dd->ScreenText({ textX, choiceStartY + static_cast<float>(displayNum - 1) * 22.0f },
                                buf, { 0.8f, 1.0f, 0.8f, 1.0f });
                displayNum++;
            }
            else
            {
                char buf[256];
                std::snprintf(buf, sizeof(buf), "   [Locked] %s", choice.text.c_str());
                dd->ScreenText({ textX, choiceStartY + static_cast<float>(i) * 22.0f },
                                buf, { 0.4f, 0.4f, 0.4f, 0.6f });
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Interaction prompt
// ---------------------------------------------------------------------------

void DialogueDemoModule::DrawInteractionPrompt(const std::string& npcName)
{
    auto* dd = Spark::DebugDraw::GetInstance();
    if (!dd) return;

    float cx = m_screenW * 0.5f;
    float y  = m_screenH * 0.7f;

    char buf[128];
    std::snprintf(buf, sizeof(buf), "Press E to talk to %s", npcName.c_str());
    float textW = static_cast<float>(std::strlen(buf)) * 8.0f + 20.0f;

    dd->ScreenRect({ cx - textW * 0.5f, y },
                    { cx + textW * 0.5f, y + 28.0f },
                    { 0.0f, 0.0f, 0.0f, 0.6f });

    dd->ScreenText({ cx - textW * 0.5f + 10.0f, y + 6.0f }, buf,
                    { 1.0f, 1.0f, 0.5f, 1.0f });
}

// ---------------------------------------------------------------------------
// HUD
// ---------------------------------------------------------------------------

void DialogueDemoModule::DrawHUD()
{
    auto* dd = Spark::DebugDraw::GetInstance();
    if (!dd) return;

    // Controls (top-left)
    dd->ScreenText({ 20.0f, 10.0f }, "WASD: Move  E: Talk  1/2/3: Choose  ESC: Exit dialogue",
                    { 0.6f, 0.6f, 0.6f, 0.7f });

    // Player stats (top-right)
    char buf[64];
    std::snprintf(buf, sizeof(buf), "Gold: %d", m_gold);
    dd->ScreenText({ m_screenW - 150.0f, 10.0f }, buf,
                    { 1.0f, 0.85f, 0.2f, 1.0f });

    std::snprintf(buf, sizeof(buf), "Score: %d", m_score);
    dd->ScreenText({ m_screenW - 150.0f, 30.0f }, buf,
                    { 1.0f, 1.0f, 1.0f, 0.9f });

    // Inventory flags
    float flagY = 55.0f;
    if (m_hasPass)
    {
        dd->ScreenText({ m_screenW - 150.0f, flagY }, "[ City Pass ]",
                        { 0.3f, 1.0f, 0.3f, 0.9f });
        flagY += 20.0f;
    }
    if (m_hasQuest)
    {
        dd->ScreenText({ m_screenW - 150.0f, flagY }, "[ Quest Active ]",
                        { 0.5f, 0.5f, 1.0f, 0.9f });
        flagY += 20.0f;
    }
}

// ---------------------------------------------------------------------------

void DialogueDemoModule::OnResize(int w, int h)
{
    m_screenW = static_cast<float>(w);
    m_screenH = static_cast<float>(h);
}

// ---------------------------------------------------------------------------
// DLL exports
// ---------------------------------------------------------------------------

SPARK_IMPLEMENT_MODULE(DialogueDemoModule)
