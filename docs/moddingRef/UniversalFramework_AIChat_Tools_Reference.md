# Universal Framework - AI Chat Tools Reference

## Tool System

Register tools that the AI can call to get information or perform actions. The framework uses **OpenAI's native function calling** feature and **automatic method dispatch** - just define methods matching your tool names!

### How Tool Calling Works

1. Register tools with `RegisterTools()` - each tool has a name, description, and parameters
2. Define methods matching the tool names on your agent class
3. When OpenAI calls a tool, the framework automatically invokes your method
4. Your method's return value is sent back to continue the conversation
5. This loop continues until OpenAI gives a final answer (up to 10 tool calls max)

**Key Benefits:**
- **No boilerplate** - Just define methods, no switch statements or dispatch code
- **Clean signatures** - Methods have proper parameter names matching the tool definition
- **Automatic dispatch** - Framework calls your methods by name using `CallFunctionParams`
- **Type hints** - Tell OpenAI what types to send (int, float, bool, vector)
- **Works with both agents** - `UFAIChatAgent` (string) and `UAIChatAgent<T>` (typed)

---

### Basic Tools (String Parameters)

The simplest way to define tools - all parameters are strings:

```enforce
class MyToolAgent extends UFAIChatAgent {
    
    override string SystemInstructions() {
        return "You are a helpful game assistant.";
    }
    
    // Step 1: Register your tools with names, descriptions, and parameter names
    override void RegisterTools(out array<autoptr UAIChatToolDef> tools) {
        // Tool with 1 parameter
        tools.Insert(new UAIChatToolDef("GetPlayerHealth", "Get a player's current health", {"playerName"}));
        
        // Tool with 2 parameters
        tools.Insert(new UAIChatToolDef("SetPlayerHealth", "Set a player's health", {"playerName", "health"}));
        
        // Tool with no parameters
        tools.Insert(new UAIChatToolDef("GetServerTime", "Get the current server time", NULL));
        
        // Tool with 3 parameters
        tools.Insert(new UAIChatToolDef("GiveItem", "Give items to a player", {"playerName", "itemClass", "quantity"}));
    }
    
    // Step 2: Define methods matching the tool names
    // The framework calls these automatically when the AI requests a tool
    
    string GetPlayerHealth(string playerName) {
        PlayerBase player = FindPlayerByName(playerName);
        if (!player) return "Player not found";
        
        float health = player.GetHealth("GlobalHealth", "Health");
        return "Player " + playerName + " has " + health.ToString() + "% health";
    }
    
    string SetPlayerHealth(string playerName, string health) {
        PlayerBase player = FindPlayerByName(playerName);
        if (!player) return "Player not found";
        
        float h = health.ToFloat();  // Convert string to float
        player.SetHealth("GlobalHealth", "Health", h);
        return "Set " + playerName + "'s health to " + h.ToString() + "%";
    }
    
    string GetServerTime() {
        return "Server time: " + UUtil.GetTimeStamp();
    }
    
    string GiveItem(string playerName, string itemClass, string quantity) {
        PlayerBase player = FindPlayerByName(playerName);
        if (!player) return "Player not found";
        
        int qty = quantity.ToInt();  // Convert string to int
        if (qty <= 0) qty = 1;
        
        for (int i = 0; i < qty; i++) {
            player.GetInventory().CreateInInventory(itemClass);
        }
        return "Gave " + qty + "x " + itemClass + " to " + playerName;
    }
}
```

---

### Typed Parameters

For better AI accuracy, you can specify parameter types. This tells OpenAI to format values correctly:

| Type | JSON Schema | OpenAI Sends | Example |
|------|-------------|--------------|---------|
| `"string"` | `{ type: "string" }` | `"John"` | Names, text |
| `"int"` | `{ type: "integer" }` | `100` | Counts, IDs |
| `"float"` | `{ type: "number" }` | `3.14` | Health, distance |
| `"bool"` | `{ type: "boolean" }` | `true` | Flags |
| `"vector"` | `{ type: "string" }` | `"123.5 51.0 45.2"` | Positions (x y z) |

#### Defining Typed Tools

```enforce
override void RegisterTools(out array<autoptr UAIChatToolDef> tools) {
    // Create typed parameters
    autoptr array<autoptr UAIChatToolParam> healParams = new array<autoptr UAIChatToolParam>;
    healParams.Insert(new UAIChatToolParam("playerName", "string", "Name of the player"));
    healParams.Insert(new UAIChatToolParam("amount", "int", "Health amount to restore"));
    tools.Insert(UAIChatToolDef.CreateTyped("HealPlayer", "Restore health to a player", healParams));
    
    // Teleport with vector position
    autoptr array<autoptr UAIChatToolParam> teleportParams = new array<autoptr UAIChatToolParam>;
    teleportParams.Insert(new UAIChatToolParam("playerName", "string", "Player to teleport"));
    teleportParams.Insert(new UAIChatToolParam("position", "vector", "Target position"));
    tools.Insert(UAIChatToolDef.CreateTyped("TeleportPlayer", "Teleport a player to a position", teleportParams));
    
    // Spawn with int quantity and bool flag
    autoptr array<autoptr UAIChatToolParam> spawnParams = new array<autoptr UAIChatToolParam>;
    spawnParams.Insert(new UAIChatToolParam("itemClass", "string", "Item class name"));
    spawnParams.Insert(new UAIChatToolParam("quantity", "int", "Number to spawn"));
    spawnParams.Insert(new UAIChatToolParam("pristine", "bool", "Spawn in pristine condition"));
    spawnParams.Insert(new UAIChatToolParam("position", "vector", "Spawn location"));
    tools.Insert(UAIChatToolDef.CreateTyped("SpawnItem", "Spawn items at a location", spawnParams));
}
```

---

### UAIChatToolParams Helper

Use the `UAIChatToolParams` helper class to parse parameters in your tool methods:

```enforce
class UAIChatToolParams {
    static int Int(string val, int defaultVal = 0);     // "100" -> 100
    static float Float(string val, float defaultVal = 0.0); // "3.14" -> 3.14
    static bool Bool(string val, bool defaultVal = false);  // "true"/"1"/"yes" -> true
    static vector Vec(string val);                      // "123.5 51.0 45.2" -> vector
    static vector Vec3(string x, string y, string z);   // 3 separate strings -> vector
}
```

#### Example Usage

```enforce
class GameMasterAI extends UFAIChatAgent {
    
    override void RegisterTools(out array<autoptr UAIChatToolDef> tools) {
        // Typed teleport tool
        autoptr array<autoptr UAIChatToolParam> params = new array<autoptr UAIChatToolParam>;
        params.Insert(new UAIChatToolParam("playerName", "string", "Player to teleport"));
        params.Insert(new UAIChatToolParam("position", "vector", "Target position as 'x y z'"));
        params.Insert(new UAIChatToolParam("notify", "bool", "Show notification to player"));
        tools.Insert(UAIChatToolDef.CreateTyped("TeleportPlayer", "Teleport a player", params));
        
        // Typed heal tool
        autoptr array<autoptr UAIChatToolParam> healParams = new array<autoptr UAIChatToolParam>;
        healParams.Insert(new UAIChatToolParam("playerName", "string"));
        healParams.Insert(new UAIChatToolParam("health", "float", "Health amount (0-100)"));
        tools.Insert(UAIChatToolDef.CreateTyped("SetHealth", "Set player health", healParams));
    }
    
    // Tool methods - use UAIChatToolParams for parsing
    
    string TeleportPlayer(string playerName, string position, string notify) {
        PlayerBase player = FindPlayerByName(playerName);
        if (!player) return "Player not found";
        
        // Parse vector from "123.5 51.0 45.2" format
        vector pos = UAIChatToolParams.Vec(position);
        if (pos == vector.Zero) return "Invalid position format";
        
        // Parse bool from "true"/"false"/"1"/"0"
        bool showNotify = UAIChatToolParams.Bool(notify);
        
        player.SetPosition(pos);
        
        if (showNotify) {
            // Send notification...
        }
        
        return "Teleported " + playerName + " to " + pos.ToString();
    }
    
    string SetHealth(string playerName, string health) {
        PlayerBase player = FindPlayerByName(playerName);
        if (!player) return "Player not found";
        
        // Parse float with default
        float h = UAIChatToolParams.Float(health, 100.0);
        h = Math.Clamp(h, 0.0, 100.0);
        
        player.SetHealth("GlobalHealth", "Health", h);
        return playerName + "'s health set to " + h.ToString();
    }
}
```

---

### Using Tool-Enabled Agents

```enforce
class GameMaster {
    protected autoptr GameMasterAI m_AI;
    
    void Init() {
        m_AI = new GameMasterAI();
    }
    
    void ProcessRequest(string playerRequest) {
        // Example: "Teleport John to 5000 0 5000 and heal him to full"
        // The AI will automatically:
        // 1. Call TeleportPlayer("John", "5000 0 5000", "true")
        // 2. Call SetHealth("John", "100")
        // 3. Respond: "Done! John has been teleported and healed."
        m_AI.Chat(playerRequest, this, "OnResponse");
    }
    
    void OnResponse(int cid, int status, string oid, string response) {
        if (status == UF_SUCCESS) {
            Print("Game Master: " + response);
        }
    }
}
```

---

### Tools with Typed Agents (UAIChatAgent<T>)

Tool calling also works with typed agents that return structured JSON:

```enforce
class NPCDecision {
    string Action;
    string Target;
    string Reason;
}

class SmartNPC extends UAIChatAgent<NPCDecision> {
    
    void SmartNPC() {
        string schema = "{\"type\":\"object\",\"properties\":{\"Action\":{\"type\":\"string\",\"enum\":[\"attack\",\"flee\",\"trade\",\"wait\"]},\"Target\":{\"type\":\"string\"},\"Reason\":{\"type\":\"string\"}},\"required\":[\"Action\",\"Target\",\"Reason\"],\"additionalProperties\":false}";
        SetSchema("NPCDecision", schema);
    }
    
    override string SystemInstructions() {
        return "You are an NPC decision AI. Use tools to gather info, then decide on an action.";
    }
    }
    
    override void RegisterTools(out array<autoptr UAIChatToolDef> tools) {
        tools.Insert(new UAIChatToolDef("AssessThreat", "Assess threat level of a target", {"targetName"}));
        tools.Insert(new UAIChatToolDef("CheckAmmo", "Check how much ammo NPC has", NULL));
    }
    
    // Define the tool methods
    string AssessThreat(string targetName) {
        // Check target's gear, weapons, behavior
        return "high - target is armed with military gear";
    }
    
    string CheckAmmo() {
        return "15 rounds remaining";
    }
}

// Usage
void OnNPCDecision(int cid, int status, string oid, NPCDecision decision) {
    if (status == UF_SUCCESS && decision) {
        Print("NPC decided to " + decision.Action + " because: " + decision.Reason);
    }
}
```

---

### Parameter Mapping

Parameters are passed to your methods in the order they appear in the tool definition.

**All parameters are passed as strings** - use `UAIChatToolParams` to convert:

| Tool Definition | Method Signature | Conversion |
|-----------------|------------------|------------|
| `{"playerName"}` | `string MyTool(string playerName)` | Direct use |
| `{"health"}` typed as `"int"` | `string MyTool(string health)` | `UAIChatToolParams.Int(health)` |
| `{"position"}` typed as `"vector"` | `string MyTool(string position)` | `UAIChatToolParams.Vec(position)` |
| `NULL` (no params) | `string MyTool()` | - |

**Limits:** Tools support up to 5 parameters.

---

### Tool Classes Reference

#### UAIChatToolDef

Tool definition class:

```enforce
// Simple constructor (all params are strings)
new UAIChatToolDef(string name, string description, array<string> paramNames)

// Typed constructor (via static method)
UAIChatToolDef.CreateTyped(string name, string description, array<autoptr UAIChatToolParam> params)
```

#### UAIChatToolParam

Parameter definition with type:

```enforce
// Constructor
new UAIChatToolParam(string name, string type = "string", string description = "")

// Supported types: "string", "int", "float", "bool", "vector"
```

#### UAIChatToolParams

Static helper for parsing parameters:

```enforce
// String to int (default: 0)
int val = UAIChatToolParams.Int("100");        // -> 100
int val = UAIChatToolParams.Int("", 50);       // -> 50 (default)

// String to float (default: 0.0)
float val = UAIChatToolParams.Float("3.14");   // -> 3.14

// String to bool (default: false)
// Accepts: "true", "1", "yes" (case-insensitive)
bool val = UAIChatToolParams.Bool("true");     // -> true
bool val = UAIChatToolParams.Bool("YES");      // -> true
bool val = UAIChatToolParams.Bool("0");        // -> false

// String to vector ("x y z" format)
vector pos = UAIChatToolParams.Vec("100 50 200");  // -> Vector(100, 50, 200)

// Three strings to vector
vector pos = UAIChatToolParams.Vec3("100", "50", "200");  // -> Vector(100, 50, 200)
```

---

### Best Practices

1. **Match Names Exactly** - Method name must match tool name (case-sensitive)
2. **Return Strings** - All tool methods must return `string`
3. **Descriptive Names** - Use clear names like `GetPlayerHealth` not `GPH`
4. **Detailed Descriptions** - Tell the AI what the tool does and when to use it
5. **Use Type Hints** - Specify `"int"`, `"float"`, `"vector"` for better AI accuracy
6. **Parameter Descriptions** - Use `UAIChatToolParam` to describe each parameter
7. **Return Useful Results** - Return actionable info, not just "OK"
8. **Handle Errors Gracefully** - Return helpful messages like "Player not found"
9. **Use UAIChatToolParams** - Parse typed values safely with defaults
10. **Vector Format** - Document that vectors use "x y z" space-separated format

---

## Tags
`ai`, `tools`, `reference`, `function-calling`, `schemas`, `parameters`, `doc-usage`, `modder`

