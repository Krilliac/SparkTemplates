#pragma once
/**
 * @file DialogueDemoModule.h
 * @brief INTERMEDIATE — Branching dialogue system showcase.
 *
 * Demonstrates:
 *   - Dialogue trees with multiple nodes and branching choices
 *   - Speaker name, dialogue text, numbered choices
 *   - Conditions on choices (locked behind prior decisions)
 *   - Events triggered by dialogue choices (score, items)
 *   - 3 NPCs: Merchant, Guard, Wizard — each with unique dialogue tree
 *   - Proximity-based interaction (press E near NPC)
 *   - Dialogue UI rendered via DebugDraw
 *
 * Spark Engine systems used:
 *   Dialogue   — DialogueSystem, DialogueTree, DialogueNode
 *   ECS        — World, Transform, MeshRenderer, Camera, LightComponent, NameComponent
 *   Input      — InputManager
 *   Audio      — AudioEngine
 *   Events     — EventBus
 *   Debug      — DebugDraw (ScreenText, ScreenRect)
 */

#include <Core/ModuleManager.h>
#include <Core/EngineContext.h>
#include <Engine/ECS/Components.h>
#include <Engine/Debug/DebugDraw.h>
#include <Engine/Events/EventSystem.h>
#include <Engine/Dialogue/DialogueSystem.h>
#include <Engine/Audio/AudioEngine.h>
#include <vector>
#include <string>
#include <unordered_map>

// ---------------------------------------------------------------------------
// Events
// ---------------------------------------------------------------------------

struct DialogueStartedEvent
{
    std::string npcName;
};

struct DialogueEndedEvent
{
    std::string npcName;
};

struct DialogueChoiceMadeEvent
{
    std::string npcName;
    int         nodeID;
    int         choiceIndex;
    std::string choiceText;
};

// ---------------------------------------------------------------------------
// NPC data
// ---------------------------------------------------------------------------

struct NPCData
{
    entt::entity entity;
    std::string  name;
    float        x, y, z;
    std::string  treeID;        // identifier for the dialogue tree
};

// ---------------------------------------------------------------------------
// Module
// ---------------------------------------------------------------------------

class DialogueDemoModule final : public Spark::IModule
{
public:
    Spark::ModuleInfo GetModuleInfo() const override
    {
        return { "DialogueDemo", "0.2.0", SPARK_SDK_VERSION, 1000 };
    }

    void OnLoad(Spark::IEngineContext* ctx) override;
    void OnUnload() override;
    void OnUpdate(float dt) override;
    void OnRender() override;
    void OnResize(int w, int h) override;

private:
    // Scene setup
    void BuildScene();
    void BuildDialogueTrees();
    void BuildMerchantTree();
    void BuildGuardTree();
    void BuildWizardTree();

    // Gameplay
    void HandleInput(float dt);
    int  FindNearestNPC(float maxDist) const;
    void StartDialogueWith(int npcIndex);
    void AdvanceOrSelectChoice(int choiceNum);
    void EndDialogue();

    // Drawing
    void DrawDialogueUI();
    void DrawInteractionPrompt(const std::string& npcName);
    void DrawHUD();

    Spark::IEngineContext* m_ctx = nullptr;

    // Screen
    float m_screenW = 1920.0f;
    float m_screenH = 1080.0f;

    // Core entities
    entt::entity m_player = entt::null;
    entt::entity m_camera = entt::null;
    entt::entity m_light  = entt::null;
    std::vector<entt::entity> m_sceneEntities;

    // Player position and look
    float m_playerX = 0.0f;
    float m_playerY = 1.0f;
    float m_playerZ = -5.0f;
    float m_camYaw  = 0.0f;

    // NPCs
    std::vector<NPCData> m_npcs;

    // Dialogue state
    bool  m_inDialogue       = false;
    int   m_activeNPCIndex   = -1;
    std::string m_activeTreeID;

    // Dialogue trees (built manually since we define content)
    struct DialogueChoice
    {
        std::string text;
        int         nextNodeID;
        std::string conditionFlag; // empty = always available
    };

    struct DialogueNodeData
    {
        int         id;
        std::string speakerName;
        std::string text;
        std::vector<DialogueChoice> choices;
        std::string voiceClipPath;
    };

    struct DialogueTreeData
    {
        std::string treeID;
        int         startNodeID;
        std::unordered_map<int, DialogueNodeData> nodes;
    };

    std::unordered_map<std::string, DialogueTreeData> m_trees;

    // Current dialogue navigation
    int m_currentNodeID = -1;

    // Condition flags (set by dialogue choices)
    std::unordered_map<std::string, bool> m_flags;

    // Interaction detection
    int m_nearestNPC = -1;

    // Player stats affected by dialogue
    int  m_gold  = 50;
    int  m_score = 0;
    bool m_hasPass    = false;
    bool m_hasQuest   = false;

    // Event subscriptions
    uint64_t m_dialogueStartSubId = 0;
    uint64_t m_dialogueEndSubId   = 0;
    uint64_t m_choiceSubId        = 0;
};
