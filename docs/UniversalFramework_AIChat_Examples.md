# Universal Framework - AI Chat Examples

Complete, ready-to-use examples demonstrating different AI Chat patterns.

**Key Features Demonstrated:**
- Simple NPC dialogue
- Quest givers with tool calling
- Typed agents for structured responses
- Knowledge Base integration for document-backed NPCs

---

## Example 1: Simple NPC Dialogue

A basic AI-powered NPC that players can talk to.

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

## Example 2: Quest Giver NPC

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
        m_AvailableQuests = {"medical_supplies", "clear_wolves", "find_survivor"};
        m_CompletedQuests = new array<string>;
        m_ActiveQuest = "";
        
        // Load player's quest history from database
        LoadPlayerQuests();
    }
    
    override string SystemInstructions() {
        return "You are Martha, a kind elderly survivor managing a trading post. You give quests. Be practical, survival-first. Stay in character and brief.";
    }
    
    override void RegisterTools(out array<autoptr UAIChatToolDef> tools) {
        tools.Insert(new UAIChatToolDef(
            "CheckQuestStatus",
            "Check player's current quest and available quests",
            NULL
        ));
        
        tools.Insert(new UAIChatToolDef(
            "GiveQuest",
            "Give a quest to the player",
            {"questId"}
        ));
        
        tools.Insert(new UAIChatToolDef(
            "CompleteQuest",
            "Mark the current quest as complete and give reward",
            NULL
        ));
        
        tools.Insert(new UAIChatToolDef(
            "CheckPlayerInventory",
            "Check if player has required items for quest",
            {"itemClass", "quantity"}
        ));
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
        // U().db().Load(...)
    }
    
    protected void SavePlayerQuests() {
        // Save to UFramework database
        // U().db().Save(...)
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

## Example 3: Typed Agent - Threat Assessment

AI that analyzes situations and returns structured threat data.

```enforce
// ============================================================
// Threat Assessment - Typed Agent with structured response
// ============================================================

class ThreatReport : Managed {
    int threat_level;       // 1-10
    string threat_type;     // "zombie", "player", "animal", "none"
    string recommendation;  // Action to take
    bool engage;            // Should player engage?
    array<string> targets;  // Priority targets
}

class ThreatAssessmentAgent extends UAIChatAgent<ThreatReport> {
    
    void ThreatAssessmentAgent() {
        string schema = "{\"type\":\"object\",\"properties\":{\"threat_level\":{\"type\":\"integer\",\"minimum\":1,\"maximum\":10},\"threat_type\":{\"type\":\"string\",\"enum\":[\"zombie\",\"player\",\"animal\",\"environmental\",\"none\"]},\"recommendation\":{\"type\":\"string\"},\"engage\":{\"type\":\"boolean\"},\"targets\":{\"type\":\"array\",\"items\":{\"type\":\"string\"}}},\"required\":[\"threat_level\",\"threat_type\",\"recommendation\",\"engage\",\"targets\"],\"additionalProperties\":false}";
        SetSchema("ThreatReport", schema);
    }
    
    override string SystemInstructions() {
        return "You are a tactical AI analyzing threats for survivors. Assess situations and prioritize survival.";
    }
    
    override void RegisterTools(out array<autoptr UAIChatToolDef> tools) {
        tools.Insert(new UAIChatToolDef(
            "ScanArea",
            "Scan the area for threats within range",
            {"range"}
        ));
        
        tools.Insert(new UAIChatToolDef(
            "GetPlayerStatus",
            "Check player's combat readiness",
            NULL
        ));
    }
    
    string ScanArea(string range) {
        // Simulate threat scan
        int r = UAIChatToolParams.Int(range, 100);
        
        // In reality, you'd scan for actual entities
        return "Scan results (range " + r + "m):\n" +
               "- 3 zombies, 50m north\n" +
               "- 1 wolf, 80m east\n" +
               "- Unknown player, 120m south (armed)";
    }
    
    string GetPlayerStatus() {
        return "Player status:\n" +
               "Health: 85%\n" +
               "Ammo: 24 rounds (AK)\n" +
               "Bandages: 2\n" +
               "Combat ready: Yes";
    }
}

// Usage
class TacticalAdvisor {
    protected autoptr ThreatAssessmentAgent m_Agent;
    
    void AssessSituation(string situation) {
        m_Agent = new ThreatAssessmentAgent();
        
        m_Agent.Chat("Assess this situation: " + situation, this, "OnAssessment");
    }
    
    void OnAssessment(int cid, int status, string oid, ThreatReport report) {
        if (status != UF_SUCCESS || !report) {
            Print("[Tactical] Assessment failed");
            return;
        }
        
        Print("[Tactical] Threat Level: " + report.threat_level);
        Print("[Tactical] Type: " + report.threat_type);
        Print("[Tactical] Recommendation: " + report.recommendation);
        
        if (report.engage) {
            Print("[Tactical] ENGAGE - Priority targets:");
            foreach (string target : report.targets) {
                Print("  > " + target);
            }
        } else {
            Print("[Tactical] AVOID ENGAGEMENT");
        }
    }
}

// Example call
void TestThreat() {
    autoptr TacticalAdvisor advisor = new TacticalAdvisor();
    advisor.AssessSituation("I hear zombies nearby and see a player in the distance");
}
```

---

## Example 4: Dynamic World Events

AI that creates dynamic events based on player actions.

```enforce
// ============================================================
// Dynamic Event Generator
// ============================================================

class WorldEvent : Managed {
    string event_type;      // "encounter", "discovery", "weather", "quest"
    string description;     // What happens
    string location;        // Where (relative to player)
    int difficulty;         // 1-5
    array<string> rewards;  // Potential rewards
    bool immediate;         // Happens now vs requires travel
}

class EventGeneratorAgent extends UAIChatAgent<WorldEvent> {
    
    void EventGeneratorAgent() {
        // SetSchema MUST be called before Chat()
        SetSchema("WorldEvent", "{" +
            "\"type\":\"object\"," +
            "\"properties\":{" +
                "\"event_type\":{\"type\":\"string\",\"enum\":[\"encounter\",\"discovery\",\"weather\",\"quest\"]}," +
                "\"description\":{\"type\":\"string\"}," +
                "\"location\":{\"type\":\"string\"}," +
                "\"difficulty\":{\"type\":\"integer\",\"minimum\":1,\"maximum\":5}," +
                "\"rewards\":{\"type\":\"array\",\"items\":{\"type\":\"string\"}}," +
                "\"immediate\":{\"type\":\"boolean\"}" +
            "}," +
            "\"required\":[\"event_type\",\"description\",\"location\",\"difficulty\",\"rewards\",\"immediate\"]," +
            "\"additionalProperties\":false" +
        "}");
    }
    
    override string SystemInstructions() {
        return "You are a game master creating dynamic DayZ events. Create immersive, balanced events fitting the post-apocalyptic setting. Consider player health, gear, and location.";
    }
    
    override void RegisterTools(out array<autoptr UAIChatToolDef> tools) {
        tools.Insert(new UAIChatToolDef(
            "GetPlayerLocation",
            "Get player's current location and surroundings",
            NULL
        ));
        
        tools.Insert(new UAIChatToolDef(
            "GetTimeOfDay",
            "Get current game time and weather",
            NULL
        ));
        
        tools.Insert(new UAIChatToolDef(
            "GetRecentEvents",
            "Get events that happened recently to avoid repetition",
            NULL
        ));
    }
    
    string GetPlayerLocation() {
        return "Location: Forest north of Gorka\n" +
               "Terrain: Dense trees, small clearing nearby\n" +
               "Structures: Abandoned cabin 200m east\n" +
               "Last known threats: Cleared, quiet area";
    }
    
    string GetTimeOfDay() {
        return "Time: Dusk (18:30)\n" +
               "Weather: Light fog rolling in\n" +
               "Visibility: Decreasing";
    }
    
    string GetRecentEvents() {
        return "Recent events:\n" +
               "- Player found a camp (2 hours ago)\n" +
               "- Zombie encounter (30 min ago)\n" +
               "Avoid: More zombie encounters, camp discoveries";
    }
}

// Event Manager
class DynamicEventManager {
    protected autoptr EventGeneratorAgent m_Agent;
    protected PlayerBase m_Player;
    
    void GenerateEvent(PlayerBase player, string trigger) {
        m_Player = player;
        m_Agent = new EventGeneratorAgent();
        
        // Add player context
        array<string> playerState = new array<string>;
        playerState.Insert("Health: " + player.GetHealth("GlobalHealth", "Health").ToString() + "%");
        playerState.Insert("Armed: Yes (hunting rifle)");
        playerState.Insert("Supplies: Low on food");
        playerState.Insert("Experience: Intermediate survivor");
        m_Agent.AddStaticContext("Player State", playerState);
        
        m_Agent.Chat("Generate an event. Trigger: " + trigger, this, "OnEventGenerated");
    }
    
    void OnEventGenerated(int cid, int status, string oid, WorldEvent event) {
        if (status != UF_SUCCESS || !event) {
            Print("[Events] Failed to generate event");
            return;
        }
        
        Print("[Events] New Event: " + event.event_type);
        Print("[Events] " + event.description);
        
        // Execute the event
        ExecuteEvent(event);
    }
    
    protected void ExecuteEvent(WorldEvent event) {
        switch (event.event_type) {
            case "encounter":
                // Spawn NPCs or threats
                break;
            case "discovery":
                // Spawn loot or points of interest
                break;
            case "weather":
                // Trigger weather change
                break;
            case "quest":
                // Create quest marker
                break;
        }
    }
}
```

---

## Example 5: Persistent NPC Memory

An NPC that remembers past conversations and player reputation.

```enforce
// ============================================================
// NPC with persistent memory
// ============================================================

class PersistentTraderAgent extends UFAIChatAgent {
    protected PlayerBase m_Player;
    protected string m_PlayerId;
    protected int m_Reputation;           // -100 to 100
    protected int m_TotalTrades;
    protected string m_LastVisit;
    
    void PersistentTraderAgent(PlayerBase player) {
        m_Player = player;
        m_PlayerId = player.GetIdentity().GetPlainId();
        
        // Load memory from database
        LoadMemory();
    }
    
    override string SystemInstructions() {
        string attitude;
        if (m_Reputation > 50) attitude = "very friendly, gives discounts";
        else if (m_Reputation > 0) attitude = "cautiously friendly";
        else if (m_Reputation > -50) attitude = "suspicious and cold";
        else attitude = "hostile, may refuse service";
        
        return "You are Viktor, a gruff trader at the Cherno market. " +
               "You remember past interactions with customers. " +
               "Current attitude toward this player: " + attitude + ". " +
               "Reputation: " + m_Reputation + "/100. " +
               "This player has made " + m_TotalTrades + " trades. " +
               "Last visit: " + m_LastVisit + ". " +
               "Stay in character. Reference past dealings if relevant.";
    }
    
    override void RegisterTools(out array<autoptr UAIChatToolDef> tools) {
        tools.Insert(new UAIChatToolDef(
            "CheckInventory",
            "Check what trade goods the player has",
            NULL
        ));
        
        tools.Insert(new UAIChatToolDef(
            "ExecuteTrade",
            "Execute a trade with the player",
            {"giving", "receiving"}
        ));
        
        tools.Insert(new UAIChatToolDef(
            "RememberFact",
            "Remember something important about this player",
            {"fact"}
        ));
        
        tools.Insert(new UAIChatToolDef(
            "RecallMemories",
            "Recall memories about this player",
            NULL
        ));
    }
    
    string CheckInventory() {
        string result = "Player inventory (trade goods):\n";
        
        array<EntityAI> items = new array<EntityAI>;
        m_Player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);
        
        foreach (EntityAI item : items) {
            // List valuable trade items
            string type = item.GetType();
            if (IsTradeGood(type)) {
                result += "- " + GetDisplayName(type) + "\n";
            }
        }
        
        return result;
    }
    
    string ExecuteTrade(string giving, string receiving) {
        // Validate trade is fair based on reputation
        float discount = m_Reputation * 0.01; // Up to 100% discount at max rep
        
        // Execute trade logic...
        m_TotalTrades++;
        m_Reputation = Math.Clamp(m_Reputation + 5, -100, 100);
        
        SaveMemory();
        
        return "Trade completed: Gave " + giving + ", received " + receiving + 
               ". Reputation increased.";
    }
    
    string RememberFact(string fact) {
        // Store in database for future sessions
        SaveFact(fact);
        return "Noted: " + fact;
    }
    
    string RecallMemories() {
        string memories = GetStoredFacts();
        if (memories == "") {
            return "No specific memories about this player.";
        }
        return "I remember about this player:\n" + memories;
    }
    
    // ===== PERSISTENCE =====
    
    protected void LoadMemory() {
        // Load from UFramework database
        U().db().Load("NPCMemory", "viktor_" + m_PlayerId, this, "OnMemoryLoaded");
    }
    
    void OnMemoryLoaded(int cid, int status, string oid, string data) {
        if (status == UF_SUCCESS && data != "") {
            // Parse JSON data
            JsonSerializer js = new JsonSerializer();
            string error;
            // ... deserialize reputation, trades, etc.
        } else {
            // New player
            m_Reputation = 0;
            m_TotalTrades = 0;
            m_LastVisit = "Never";
        }
    }
    
    protected void SaveMemory() {
        m_LastVisit = UUtil.GetTimeStamp();
        
        string data = "{" +
            "\"reputation\":" + m_Reputation + "," +
            "\"trades\":" + m_TotalTrades + "," +
            "\"lastVisit\":\"" + m_LastVisit + "\"" +
        "}";
        
        U().db().Save("NPCMemory", "viktor_" + m_PlayerId, data, NULL);
    }
    
    protected void SaveFact(string fact) {
        // Append to facts array in database
    }
    
    protected string GetStoredFacts() {
        // Return stored facts from database
        return "";
    }
    
    protected bool IsTradeGood(string type) {
        return type.Contains("Ammo") || type.Contains("Food") || type.Contains("Med");
    }
    
    protected string GetDisplayName(string type) {
        return type; // In reality, get localized name
    }
}
```

---

## Example 6: Minimal Quick Reference

For copy-paste starting points:

### String Agent (Simplest)

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

### Typed Agent (Structured Response)

```enforce
class MyData : Managed {
    string action;
    int priority;
}

class MyTypedAgent extends UAIChatAgent<MyData> {
    override string SystemInstructions() { return "Decide an action."; }
    
    void MyTypedAgent() {
        // SetSchema MUST be called before Chat()
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
    override string SystemInstructions() { return "Use tools to help."; }
    
    override void RegisterTools(out array<autoptr UAIChatToolDef> tools) {
        tools.Insert(new UAIChatToolDef("GetTime", "Get server time", NULL));
        tools.Insert(new UAIChatToolDef("AddNumbers", "Add two numbers", {"a", "b"}));
    }
    
    string GetTime() {
        return "12:00 PM";
    }
    
    string AddNumbers(string a, string b) {
        int result = a.ToInt() + b.ToInt();
        return "Result: " + result.ToString();
    }
}
```

### Agent with Knowledge Base

```enforce
// Agent backed by a document collection
class KnowledgeableNPC extends UFAIChatAgent {
    
    void KnowledgeableNPC(string kbId) {
        // Attach Knowledge Base - AI will search it automatically
        SetKBId(kbId);
    }
    
    override string SystemInstructions() {
        return "You are a helpful guide. Answer questions using the provided knowledge base. If unsure, say you don't know.";
    }
}

// Usage - NPC that knows server lore
void SetupLoreNPC() {
    autoptr KnowledgeableNPC npc = new KnowledgeableNPC("kb_server_lore");
    npc.Chat("Tell me about the history of this place", this, "OnResponse");
}
```

### Typed Agent with Knowledge Base

```enforce
// Structured responses backed by documents
class FactResponse : Managed {
    string answer;
    bool isConfident;
    ref array<string> sources;
    
    void FactResponse() {
        sources = new array<string>;
    }
}

class FactAgent extends UAIChatAgent<FactResponse> {
    
    void FactAgent(string kbId) {
        SetKBId(kbId);
        SetSchema("FactResponse", "{\"type\":\"object\",\"properties\":{\"answer\":{\"type\":\"string\"},\"isConfident\":{\"type\":\"boolean\"},\"sources\":{\"type\":\"array\",\"items\":{\"type\":\"string\"}}},\"required\":[\"answer\",\"isConfident\",\"sources\"],\"additionalProperties\":false}");
    }
    
    override string SystemInstructions() {
        return "Answer questions from the knowledge base. Be confident only if the answer is clear.";
    }
}
```

---

## Common Patterns

### Cancellation Check

```enforce
class CancellableAgent extends UFAIChatAgent {
    override string SystemInstructions() { ... }
    
    override void RegisterTools(out array<autoptr UAIChatToolDef> tools) {
        tools.Insert(new UAIChatToolDef("LongOperation", "Do something slow", NULL));
    }
    
    string LongOperation() {
        // Note: Tool methods cannot check cancellation mid-execution
        // Cancellation is handled at the callback level by the framework
        
        // Do expensive work...
        return "Done";
    }
}

// Cancel from outside using call ID
void Cancel(int callId) {
    U().RequestCallCancel(callId);
}
```

### Dynamic Context via Override

```enforce
// Use ExtraContext() override for dynamic context that changes each message
class MyAgent extends UFAIChatAgent {
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

// Use AddStaticContext() for permanent context added once
array<string> rules = new array<string>;
rules.Insert("Be helpful");
rules.Insert("Stay in character");
agent.AddStaticContext("Rules", rules);
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

---

## Related Documentation

- [AI Chat - Overview](UniversalFramework_AIChat_Overview.md) - Basic concepts
- [AI Chat - Knowledge Base](UniversalFramework_AIChat_KnowledgeBase.md) - Document-backed AI responses
- [AI Chat - Typed Agents](UniversalFramework_AIChat_TypedAgents.md) - Structured responses
- [AI Chat - Tool Calling](UniversalFramework_AIChat_Tools.md) - Function calling
- [Callbacks](UniversalFramework_Callbacks.md) - Callback system
