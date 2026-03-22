# Universal Framework - AI Chat Examples

Complete, ready-to-use examples demonstrating different AI Chat patterns.

**Key Features Demonstrated:**
- Simple NPC dialogue with model selection
- Server-client handler pattern for player chat
- Quest givers with tool calling
- Typed agents for structured responses
- Knowledge Base integration for document-backed NPCs

---

## Example 1: Simple NPC Dialogue (Server-Side Agent)

A basic AI-powered NPC that players can talk to. Uses Agent class pattern with model selection.

```enforce
// ============================================================
// Simple NPC - No tools, just conversation
// ============================================================

class NPCDialogueAgent extends UFAIChatAgent {
    protected string m_NPCName;
    protected string m_NPCRole;
    
    void NPCDialogueAgent(string npcName, string role) {
        m_NPCName = npcName;
        m_NPCRole = role;
    }
    
    // Optional: Specify a cheaper model for simple NPCs
    override string GetModel() {
        return "gpt-4o-mini";  // Fast and cost-effective
    }
    
    override string SystemInstructions() {
        string instr = "You are " + m_NPCName + ", a " + m_NPCRole + " in post-apocalyptic DayZ. Stay in character. Keep responses short (2-3 sentences). Be wary of strangers but warm to the friendly. Share survival knowledge but never break character.";
        return instr;
    }
}

// Usage
class NPCInteraction {
    protected autoptr NPCDialogueAgent m_Agent;
    protected PlayerBase m_Player;
    
    void StartConversation(PlayerBase player, string npcName, string role) {
        m_Player = player;
        m_Agent = new NPCDialogueAgent(npcName, role);
        
        // Add context about the player
        array<string> playerInfo = new array<string>;
        playerInfo.Insert("Name: " + player.GetIdentity().GetName());
        playerInfo.Insert("Appears " + GetPlayerCondition(player));
        m_Agent.AddStaticContext("Player Info", playerInfo);
        
        // Start with greeting
        m_Agent.Chat("*I approach the " + role + "*", this, "OnNPCResponse");
    }
    
    void Say(string message) {
        m_Agent.Chat(message, this, "OnNPCResponse");
    }
    
    void OnNPCResponse(int cid, int status, string oid, string response) {
        if (status == UF_SUCCESS && m_Player && m_Player.GetIdentity()) {
            // Display to player
            UUtil.SendNotification("NPC", response, m_Player.GetIdentity());
        }
    }
    
    protected string GetPlayerCondition(PlayerBase player) {
        float health = player.GetHealth("GlobalHealth", "Health");
        if (health < 30) return "injured and weak";
        if (health < 60) return "a bit roughed up";
        return "healthy";
    }
}
```

---

## Tags
`ai`, `examples`, `basic`, `quickstart`, `tutorial`, `npc`, `dialogue`, `doc-usage`, `modder`

## Example 2: Server-Client Handler Pattern

This example shows how to create an AI chat on the server and allow clients to interact with it.

**Key Concepts:**
- Constructor callback is for **MESSAGE responses**, not creation
- Use `NotifyOnCreated()` to get the ChatId when creation completes
- Messages can be queued immediately - they auto-send when chat is ready

```enforce
// ============================================================
// SERVER SIDE - Creates chat and sends ID to client
// ============================================================

class ServerNPCChatManager {
    protected ref map<string, autoptr UStringAIChatHandler> m_PlayerChats;
    protected ref map<string, PlayerBase> m_ChatPlayers;  // Map ChatId -> Player
    
    void ServerNPCChatManager() {
        m_PlayerChats = new map<string, autoptr UStringAIChatHandler>;
        m_ChatPlayers = new map<string, PlayerBase>;
    }
    
    // Called when player interacts with NPC
    void PlayerStartsNPCChat(PlayerBase player, string npcName) {
        if (!GetGame().IsServer()) return;
        
        string guid = player.GetIdentity().GetId();
        
        // Create new chat for this player
        string systemPrompt = "You are " + npcName + ", a survivor in DayZ. Keep responses short.";
        
        // Parameters: systemMessage, callbackTarget, callbackFunc, model, maxHistory
        UStringAIChatHandler handler = new UStringAIChatHandler(systemPrompt, this, "OnNPCMessage", "gpt-4o-mini", 25);
        
        // IMPORTANT: Set callback for when creation completes
        handler.NotifyOnCreated("OnChatCreated");
        
        // Store for later reference
        m_PlayerChats.Set(guid, handler);
        
        // We'll map ChatId -> Player when creation completes
    }
    
    // Called when chat creation completes (via NotifyOnCreated)
    // Signature: (int cid, int status, string chatId, bool success)
    void OnChatCreated(int cid, int status, string chatId, bool success) {
        if (!success || chatId == "") {
            Print("[NPC] Chat creation failed");
            return;
        }
        
        // Find which handler this belongs to by checking all handlers
        string playerGuid = "";
        foreach (string guid, UStringAIChatHandler handler : m_PlayerChats) {
            if (handler && handler.GetChatId() == chatId) {
                playerGuid = guid;
                break;
            }
        }
        
        if (playerGuid == "") return;
        
        PlayerBase player = GetPlayerByGUID(playerGuid);
        if (!player || !player.GetIdentity()) return;
        
        // Remember which player owns this chat
        m_ChatPlayers.Set(chatId, player);
        
        // Send the chat ID to the client via RPC
        GetGame().RPCSingleParam(player, RPC_AI_CHAT_CREATED, 
            new Param1<string>(chatId), true, player.GetIdentity());
    }
    
    // Called when any message response comes back (from constructor callback)
    void OnNPCMessage(int cid, int status, string chatId, string response) {
        if (status != UF_SUCCESS || response == "") return;
        
        // Forward response to the player who owns this chat
        PlayerBase player;
        if (m_ChatPlayers.Find(chatId, player) && player && player.GetIdentity()) {
            // Send response to client via RPC
            GetGame().RPCSingleParam(player, RPC_AI_NPC_RESPONSE,
                new Param2<string, string>(chatId, response), true, player.GetIdentity());
        }
    }
}

// ============================================================
// CLIENT SIDE - Connects to existing chat
// ============================================================

class ClientNPCChat {
    protected autoptr UStringAIChatHandler m_Handler;
    protected string m_ChatId;
    
    // Called when receiving RPC from server with chat ID
    void OnReceiveChatId(string chatId) {
        m_ChatId = chatId;
        
        // Connect to the existing chat (no system message = connect mode)
        m_Handler = new UStringAIChatHandler(chatId, this, "OnNPCResponse");
        
        // Enable polling for async responses
        m_Handler.SetPolling(true, 1);
    }
    
    // Player types a message to the NPC
    void SendToNPC(string message) {
        if (m_Handler) {
            m_Handler.SendMessage(message);
        }
    }
    
    // Callback when NPC responds
    void OnNPCResponse(int cid, int status, string chatId, string response) {
        if (status == UF_SUCCESS && response != "") {
            // Display in your UI
            ShowNPCDialogue(response);
        } else if (status == UF_AI_PENDING || status == UF_AI_PROCESSING) {
            // Still waiting - handler polls automatically
            ShowTypingIndicator();
        }
    }
    
    void ShowNPCDialogue(string text) {
        // Your UI code here
    }
    
    void ShowTypingIndicator() {
        // Show "..." or similar
    }
}
```

---

## Example 3: Quest Giver NPC

An NPC that gives quests and tracks player progress using tools.

```enforce
// ============================================================
// Quest NPC with tool calling
// ============================================================

class QuestGiverAgent extends UFAIChatAgent {
    protected PlayerBase m_Player;
    protected ref array<string> m_AvailableQuests;
    protected ref array<string> m_CompletedQuests;
    protected string m_ActiveQuest;
    
    void QuestGiverAgent(PlayerBase player) {
        m_Player = player;
        m_AvailableQuests = new array<string>;
        m_AvailableQuests.Insert("medical_supplies");
        m_AvailableQuests.Insert("clear_wolves");
        m_AvailableQuests.Insert("find_survivor");
        
        m_CompletedQuests = new array<string>;
        m_ActiveQuest = "";
        
        // Load player's quest history from database
        LoadPlayerQuests();
    }
    
    override string SystemInstructions() {
        return "You are Martha, a kind elderly survivor managing a trading post. You give quests. Be practical, survival-first. Stay in character and brief.";
    }
    
    override void RegisterTools(out array<autoptr UAIChatToolDef> tools) {
        // Helper to create string arrays for params
        array<string> questIdParam = new array<string>;
        questIdParam.Insert("questId");
        
        array<string> invParams = new array<string>;
        invParams.Insert("itemClass");
        invParams.Insert("quantity");

        tools.Insert(new UAIChatToolDef("CheckQuestStatus", "Check player's current quest and available quests", NULL));
        tools.Insert(new UAIChatToolDef("GiveQuest", "Give a quest to the player", questIdParam));
        tools.Insert(new UAIChatToolDef("CompleteQuest", "Mark the current quest as complete and give reward", NULL));
        tools.Insert(new UAIChatToolDef("CheckPlayerInventory", "Check if player has required items for quest", invParams));
    }
    
    // ===== TOOL IMPLEMENTATIONS =====
    
    string CheckQuestStatus() {
        string result = "";
        
        if (m_ActiveQuest != "") {
            result += "Active quest: " + GetQuestName(m_ActiveQuest) + "\n";
            result += "Progress: " + GetQuestProgress(m_ActiveQuest) + "\n";
        } else {
            result += "No active quest.\n";
        }
        
        result += "Available quests: ";
        foreach (string q : m_AvailableQuests) {
            if (m_CompletedQuests.Find(q) == -1) {
                result += GetQuestName(q) + ", ";
            }
        }
        
        result += "\nCompleted: " + m_CompletedQuests.Count() + " quests";
        
        return result;
    }
    
    string GiveQuest(string questId) {
        if (m_ActiveQuest != "") {
            return "Player already has active quest: " + GetQuestName(m_ActiveQuest);
        }
        
        if (m_CompletedQuests.Find(questId) != -1) {
            return "Player already completed this quest";
        }
        
        m_ActiveQuest = questId;
        SavePlayerQuests();
        
        string result = "Quest '" + GetQuestName(questId) + "' given. Objective: " + GetQuestObjective(questId);
        return result;
    }
    
    string CompleteQuest() {
        if (m_ActiveQuest == "") {
            return "Player has no active quest to complete";
        }
        
        // Check if requirements met
        if (!CheckQuestRequirements(m_ActiveQuest)) {
            return "Quest requirements not met. " + GetQuestObjective(m_ActiveQuest);
        }
        
        // Give reward
        string reward = GiveQuestReward(m_ActiveQuest);
        
        m_CompletedQuests.Insert(m_ActiveQuest);
        m_ActiveQuest = "";
        SavePlayerQuests();
        
        return "Quest completed! Player received: " + reward;
    }
    
    string CheckPlayerInventory(string itemClass, string quantity) {
        int needed = UAIChatToolParams.Int(quantity, 1);
        int found = 0;
        
        // Count items in player inventory
        array<EntityAI> items = new array<EntityAI>;
        m_Player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);
        
        foreach (EntityAI item : items) {
            if (item.GetType() == itemClass) {
                found++;
            }
        }
        
        if (found >= needed) {
            return "Player has " + found + "x " + itemClass + " (needs " + needed + "). SUFFICIENT.";
        } else {
            return "Player has " + found + "x " + itemClass + " (needs " + needed + "). INSUFFICIENT.";
        }
    }
    
    // ===== HELPER METHODS =====
    
    protected string GetQuestName(string questId) {
        if (questId == "medical_supplies") return "Medical Supplies";
        if (questId == "clear_wolves") return "Clear the Wolves";
        if (questId == "find_survivor") return "Find the Lost Survivor";
        return questId;
    }
    
    protected string GetQuestObjective(string questId) {
        if (questId == "medical_supplies") return "Bring 3 bandages and 1 morphine";
        if (questId == "clear_wolves") return "Kill 5 wolves near the camp";
        if (questId == "find_survivor") return "Find Jake who went missing near Elektro";
        return "Unknown objective";
    }
    
    protected string GetQuestProgress(string questId) {
        // In real implementation, track progress
        return "In progress";
    }
    
    protected bool CheckQuestRequirements(string questId) {
        // Check based on quest type
        return true; // Simplified
    }
    
    protected string GiveQuestReward(string questId) {
        if (questId == "medical_supplies") {
            m_Player.GetInventory().CreateInInventory("Rice");
            m_Player.GetInventory().CreateInInventory("Rice");
            return "2x Rice";
        }
        return "Thank you reward";
    }
    
    protected void LoadPlayerQuests() {
        // Load from UFramework database
        // UF().db().Load(...)
    }
    
    protected void SavePlayerQuests() {
        // Save to UFramework database
        // UF().db().Save(...)
    }
}

// Usage
class QuestNPCController {
    protected autoptr QuestGiverAgent m_Agent;
    
    void OnPlayerInteract(PlayerBase player) {
        m_Agent = new QuestGiverAgent(player);
        
        // Context about the trading post
        array<string> locationInfo = new array<string>;
        locationInfo.Insert("Martha's Trading Post, near Gorka");
        locationInfo.Insert("Small fortified building with supplies");
        m_Agent.AddStaticContext("Location", locationInfo);
        
        // Player initiates conversation
        m_Agent.Chat("Hello there!", this, "OnResponse");
    }
    
    void OnResponse(int cid, int status, string oid, string response) {
        Print("[Quest NPC] " + response);
    }
}
```

---

## Quick Reference Snippets

Copy-paste starting points for common patterns:

### Minimal String Agent

```enforce
class MyAgent extends UFAIChatAgent {
    override string SystemInstructions() {
        return "You are a helpful assistant.";
    }
}

void Use() {
    autoptr MyAgent agent = new MyAgent();
    agent.Chat("Hello", this, "OnReply");
}

void OnReply(int cid, int status, string oid, string response) {
    Print(response);
}
```

### Minimal Typed Agent

```enforce
class MyData : Managed {
    string action;
    int priority;
}

class MyTypedAgent extends UAIChatAgent<MyData> {
    override string SystemInstructions() { return "Decide an action."; }
    
    void MyTypedAgent() {
        SetSchema("MyData", "{\"type\":\"object\",\"properties\":{\"action\":{\"type\":\"string\"},\"priority\":{\"type\":\"integer\"}},\"required\":[\"action\",\"priority\"],\"additionalProperties\":false}");
    }
}

void OnData(int cid, int status, string oid, MyData data) {
    if (data) Print(data.action + " (priority: " + data.priority + ")");
}
```

### Agent with Tools

```enforce
class ToolAgent extends UFAIChatAgent {
    override string SystemInstructions() {
        return "You can get player info using tools.";
    }
    
    override void RegisterTools(out array<autoptr UAIChatToolDef> tools) {
        tools.Insert(new UAIChatToolDef("GetHealth", "Get player health", {"playerName"}));
    }
    
    // Method name MUST match tool name
    string GetHealth(string playerName) {
        return playerName + " has 85% health";
    }
}
```

### Dynamic Context

```enforce
class ContextAgent extends UFAIChatAgent {
    protected float m_Health = 100;
    protected string m_Location = "Cherno";
    
    override array<string> ExtraContext() {
        array<string> ctx = new array<string>;
        ctx.Insert("Player health: " + m_Health.ToString() + "%");
        ctx.Insert("Location: " + m_Location);
        return ctx;
    }
    
    void UpdateState(float health, string location) {
        m_Health = health;
        m_Location = location;
    }
}
```

### Error Handling

```enforce
void OnResponse(int cid, int status, string oid, string response) {
    switch (status) {
        case UF_SUCCESS:
            Print(response);
            break;
        case UF_TIMEOUT:
            Print("AI response timed out");
            break;
        case UF_JSONERROR:
            Print("Failed to parse AI response");
            break;
        case UF_ERROR:
        default:
            Print("AI Error: " + status);
            break;
    }
}
```

## Tags
`ai`, `examples`, `npc`, `tools`, `handlers`, `quickref`, `modder`
