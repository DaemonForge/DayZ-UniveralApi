# Universal Framework - AI Chat Advanced Examples

## Example 4: Typed Agent - Threat Assessment

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
        array<string> scanParams = new array<string>;
        scanParams.Insert("range");

        tools.Insert(new UAIChatToolDef("ScanArea", "Scan the area for threats within range", scanParams));
        tools.Insert(new UAIChatToolDef("GetPlayerStatus", "Check player's combat readiness", NULL));
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

## Tags
`ai`, `advanced`, `examples`, `tools`, `typed`, `patterns`, `doc-usage`, `modder`

## Example 5: Dynamic World Events

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
        tools.Insert(new UAIChatToolDef("GetPlayerLocation", "Get player's current location and surroundings", NULL));
        tools.Insert(new UAIChatToolDef("GetTimeOfDay", "Get current game time and weather", NULL));
        tools.Insert(new UAIChatToolDef("GetRecentEvents", "Get events that happened recently to avoid repetition", NULL));
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
        m_PlayerId = player.GetIdentity().GetId();
        
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
        tools.Insert(new UAIChatToolDef("CheckInventory", "Check what trade goods the player has", NULL));
        tools.Insert(new UAIChatToolDef("ExecuteTrade", "Execute a trade with the player", {"giving", "receiving"}));
        tools.Insert(new UAIChatToolDef("RememberFact", "Remember something important about this player", {"fact"}));
        tools.Insert(new UAIChatToolDef("RecallMemories", "Recall memories about this player", NULL));
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

