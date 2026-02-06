# Universal Framework - AI Chat Tool Calling

## Overview

Tool calling allows the AI to invoke functions on your agent class to gather information or perform actions. The framework uses **OpenAI's native function calling** feature with **automatic method dispatch** - just define methods matching your tool names!

## How It Works

```
1. You register tools with RegisterTools()
2. You define methods matching the tool names
3. AI receives your tools as available functions
4. AI decides to call a tool â†’ returns tool call request
5. Framework automatically invokes your method
6. Your method's return value is sent back to AI
7. AI continues (may call more tools) or gives final answer
```

**Key Features:**
- **Automatic dispatch** - Framework calls your methods by name
- **No boilerplate** - No switch statements or dispatch code
- **Type hints** - Tell OpenAI what types to send (int, float, vector)
- **Up to 10 tool calls** per conversation turn
- **Works with both agents** - `UFAIChatAgent` and `UAIChatAgent<T>`
- **Knowledge Base integration** - Combine with KB for document-backed tool responses

---

## Basic Tools (String Parameters)

The simplest way - all parameters are strings:

### Step 1: Register Tools

```enforce
class GameMasterAI extends UFAIChatAgent {
    
    override string SystemInstructions() {
        return "You are a game master assistant. Use tools to manage players.";
    }
    
    // Optional: Specify model
    override string GetModel() {
        return "gpt-4o";  // Tool-heavy agents benefit from smarter models
    }
    
    override void RegisterTools(out array<autoptr UAIChatToolDef> tools) {
        // Tool with 1 parameter: name, description, params array
        tools.Insert(new UAIChatToolDef("GetPlayerHealth", "Get a player's current health", {"playerName"}));
        
        // Tool with 2 parameters
        tools.Insert(new UAIChatToolDef("SetPlayerHealth", "Set a player's health value", {"playerName", "health"}));
        
        // Tool with no parameters
        tools.Insert(new UAIChatToolDef("GetServerTime", "Get the current server time", NULL));
        
        // Tool with 3 parameters
        tools.Insert(new UAIChatToolDef("GiveItem", "Give items to a player", {"playerName", "itemClass", "quantity"}));
    }
}
```

### Step 2: Define Matching Methods

```enforce
// Method names MUST match tool names exactly (case-sensitive)
// All methods MUST return string
// All parameters are strings - convert as needed

string GetPlayerHealth(string playerName) {
    PlayerBase player = FindPlayerByName(playerName);
    if (!player) return "Player '" + playerName + "' not found";
    
    float health = player.GetHealth("GlobalHealth", "Health");
    return playerName + " has " + health.ToString() + " health";
}

string SetPlayerHealth(string playerName, string health) {
    PlayerBase player = FindPlayerByName(playerName);
    if (!player) return "Player not found";
    
    float h = health.ToFloat();
    player.SetHealth("GlobalHealth", "Health", h);
    return "Set " + playerName + "'s health to " + h.ToString();
}

string GetServerTime() {
    return "Server time: " + UUtil.GetTimeStamp();
}

string GiveItem(string playerName, string itemClass, string quantity) {
    PlayerBase player = FindPlayerByName(playerName);
    if (!player) return "Player not found";
    
    int qty = quantity.ToInt();
    if (qty <= 0) qty = 1;
    
    for (int i = 0; i < qty; i++) {
        player.GetInventory().CreateInInventory(itemClass);
    }
    return "Gave " + qty + "x " + itemClass + " to " + playerName;
}
```

---

## Typed Parameters

For better AI accuracy, specify parameter types:

### Supported Types

| Type | JSON Schema | AI Sends | Convert With |
|------|-------------|----------|--------------|
| `"string"` | `string` | `"John"` | Direct use |
| `"int"` | `integer` | `100` | `UAIChatToolParams.Int()` |
| `"float"` | `number` | `3.14` | `UAIChatToolParams.Float()` |
| `"bool"` | `boolean` | `true` | `UAIChatToolParams.Bool()` |
| `"vector"` | `string` | `"100 50 200"` | `UAIChatToolParams.Vec()` |

### Defining Typed Tools

```enforce
override void RegisterTools(out array<autoptr UAIChatToolDef> tools) {
    // Heal player with typed amount
    autoptr array<autoptr UAIChatToolParam> healParams = new array<autoptr UAIChatToolParam>;
    healParams.Insert(new UAIChatToolParam("playerName", "string", "Name of the player"));
    healParams.Insert(new UAIChatToolParam("amount", "int", "Health amount to restore"));
    tools.Insert(UAIChatToolDef.CreateTyped("HealPlayer", "Restore health", healParams));
    
    // Teleport with vector position
    autoptr array<autoptr UAIChatToolParam> teleportParams = new array<autoptr UAIChatToolParam>;
    teleportParams.Insert(new UAIChatToolParam("playerName", "string", "Player to teleport"));
    teleportParams.Insert(new UAIChatToolParam("position", "vector", "Target position as 'x y z'"));
    teleportParams.Insert(new UAIChatToolParam("notify", "bool", "Show notification"));
    tools.Insert(UAIChatToolDef.CreateTyped("TeleportPlayer", "Teleport player", teleportParams));
    
    // Spawn with multiple typed params
    autoptr array<autoptr UAIChatToolParam> spawnParams = new array<autoptr UAIChatToolParam>;
    spawnParams.Insert(new UAIChatToolParam("itemClass", "string", "Item class name"));
    spawnParams.Insert(new UAIChatToolParam("quantity", "int", "Number to spawn"));
    spawnParams.Insert(new UAIChatToolParam("pristine", "bool", "Spawn pristine"));
    spawnParams.Insert(new UAIChatToolParam("position", "vector", "Spawn location"));
    tools.Insert(UAIChatToolDef.CreateTyped("SpawnItem", "Spawn items", spawnParams));
}
```

---

## UAIChatToolParams Helper

Parse string parameters to proper types safely:

```enforce
class UAIChatToolParams {
    // String to int (default: 0)
    static int Int(string val, int defaultVal = 0);
    
    // String to float (default: 0.0)
    static float Float(string val, float defaultVal = 0.0);
    
    // String to bool (accepts: "true", "1", "yes")
    static bool Bool(string val, bool defaultVal = false);
    
    // String "x y z" to vector
    static vector Vec(string val);
    
    // Three strings to vector
    static vector Vec3(string x, string y, string z);
}
```

### Usage Examples

```enforce
string TeleportPlayer(string playerName, string position, string notify) {
    PlayerBase player = FindPlayerByName(playerName);
    if (!player) return "Player not found";
    
    // Parse vector from "100 50 200" format
    vector pos = UAIChatToolParams.Vec(position);
    if (pos == vector.Zero) return "Invalid position";
    
    // Parse bool (handles "true", "false", "1", "0", "yes", "no")
    bool showNotify = UAIChatToolParams.Bool(notify, true);
    
    player.SetPosition(pos);
    
    if (showNotify) {
        UUtil.SendNotification("Teleported", "You were teleported", player.GetIdentity());
    }
    
    return "Teleported " + playerName + " to " + pos.ToString();
}

string HealPlayer(string playerName, string amount) {
    PlayerBase player = FindPlayerByName(playerName);
    if (!player) return "Player not found";
    
    // Parse int with default
    int healAmount = UAIChatToolParams.Int(amount, 100);
    healAmount = Math.Clamp(healAmount, 0, 100);
    
    player.SetHealth("GlobalHealth", "Health", healAmount);
    return playerName + " healed to " + healAmount.ToString() + " health";
}

string SpawnItem(string itemClass, string quantity, string pristine, string position) {
    int qty = UAIChatToolParams.Int(quantity, 1);
    bool isPristine = UAIChatToolParams.Bool(pristine, false);
    vector pos = UAIChatToolParams.Vec(position);
    
    for (int i = 0; i < qty; i++) {
        ItemBase item = ItemBase.Cast(GetGame().CreateObject(itemClass, pos));
        if (item && isPristine) {
            item.SetHealth("", "", item.GetMaxHealth("", ""));
        }
    }
    
    return "Spawned " + qty + "x " + itemClass + " at " + pos.ToString();
}
```

---

## Tool Classes Reference

### UAIChatToolDef

**Simple Constructor** - All parameters are strings:
```
UAIChatToolDef(name, description, params)
```
| Parameter | Type | Description |
|-----------|------|-------------|
| `name` | string | Tool name (must match your method name) |
| `description` | string | Description for the AI to understand when to use this tool |
| `params` | array<string> | Parameter names, or NULL for no parameters |

**Example:**
```enforce
new UAIChatToolDef("GetPlayerHealth", "Get a player's current health", {"playerName"})
new UAIChatToolDef("GetServerTime", "Get the current server time", NULL)
```

**Typed Constructor** - For detailed parameter types:
```
UAIChatToolDef.CreateTyped(name, description, params)
```
| Parameter | Type | Description |
|-----------|------|-------------|
| `name` | string | Tool name (must match your method name) |
| `description` | string | Description for the AI |
| `params` | array<autoptr UAIChatToolParam> | Array of typed parameter definitions |

### UAIChatToolParam

**Constructor:**
```
UAIChatToolParam(name, type, desc)
```
| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `name` | string | - | Parameter name |
| `type` | string | "string" | Type: "string", "int", "float", "bool", "vector" |
| `desc` | string | "" | Description for the AI |

**Example:**
```enforce
new UAIChatToolParam("playerName", "string", "The player's name")
new UAIChatToolParam("amount", "int", "Number of items (1-100)")
new UAIChatToolParam("position", "vector", "Position as 'x y z'")
```

---

## Complete Example: Admin Assistant

```enforce
class AdminAssistant extends UFAIChatAgent {
    
    override string SystemInstructions() {
        return "You are a DayZ server admin assistant. Help manage players. Always confirm actions. Be concise.";
    }
    
    override void RegisterTools(out array<autoptr UAIChatToolDef> tools) {
        array<string> pInfo = new array<string>;
        pInfo.Insert("playerName");
        tools.Insert(new UAIChatToolDef("GetPlayerInfo", "Get detailed player info", pInfo
        // Player info
        tools.Insert(new UAIChatToolDef("GetPlayerList", "Get list of online players", NULL));
        tools.Insert(new UAIChatToolDef("GetPlayerInfo", "Get detailed player info", {"playerName"}));
        
        // Player management (typed)
        autoptr array<autoptr UAIChatToolParam> healParams = new array<autoptr UAIChatToolParam>;
        healParams.Insert(new UAIChatToolParam("playerName", "string"));
        healParams.Insert(new UAIChatToolParam("fullHeal", "bool", "Heal to full if true"));
        tools.Insert(UAIChatToolDef.CreateTyped("HealPlayer", "Heal a player", healParams));
        
        autoptr array<autoptr UAIChatToolParam> tpParams = new array<autoptr UAIChatToolParam>;
        tpParams.Insert(new UAIChatToolParam("playerName", "string"));
        tpParams.Insert(new UAIChatToolParam("destination", "string", "Location name or 'x y z' coords"));
        tools.Insert(UAIChatToolDef.CreateTyped("TeleportPlayer", "Teleport player", tpParams));
        
        autoptr array<autoptr UAIChatToolParam> giveParams = new array<autoptr UAIChatToolParam>;
        giveParams.Insert(new UAIChatToolParam("playerName", "string"));
        giveParams.Insert(new UAIChatToolParam("itemClass", "string"));
        giveParams.Insert(new UAIChatToolParam("quantity", "int"));
        tools.Insert(UAIChatToolDef.CreateTyped("GiveItem", "Give items to player", giveParams));
        
        // Server management
        tools.Insert(new UAIChatToolDef("GetServerStats", "Get server statistics", NULL));
        tools.Insert(new UAIChatToolDef("BroadcastMessage", "Send message to all players", {"message"}));
    }
    
    // ============ TOOL IMPLEMENTATIONS ============
    
    string GetPlayerList() {
        array<Man> players = new array<Man>;
        GetGame().GetPlayers(players);
        
        if (players.Count() == 0) return "No players online";
        
        string result = "Online players (" + players.Count() + "):\n";
        foreach (Man man : players) {
            PlayerBase player = PlayerBase.Cast(man);
            if (player && player.GetIdentity()) {
                result += "- " + player.GetIdentity().GetName() + "\n";
            }
        }
        return result;
    }
    
    string GetPlayerInfo(string playerName) {
        PlayerBase player = FindPlayerByName(playerName);
        if (!player) return "Player '" + playerName + "' not found";
        
        float health = player.GetHealth("GlobalHealth", "Health");
        float blood = player.GetHealth("GlobalHealth", "Blood");
        vector pos = player.GetPosition();
        
        return "Player: " + playerName + "\n" +
               "Health: " + health.ToString() + "%\n" +
               "Blood: " + blood.ToString() + "\n" +
               "Position: " + pos.ToString();
    }
    
    string HealPlayer(string playerName, string fullHeal) {
        PlayerBase player = FindPlayerByName(playerName);
        if (!player) return "Player not found";
        
        bool full = UAIChatToolParams.Bool(fullHeal, true);
        
        if (full) {
            player.SetHealth("GlobalHealth", "Health", 100);
            player.SetHealth("GlobalHealth", "Blood", 5000);
            player.SetHealth("GlobalHealth", "Shock", 0);
            return playerName + " fully healed (health, blood, shock)";
        } else {
            player.SetHealth("GlobalHealth", "Health", 100);
            return playerName + " health restored to 100%";
        }
    }
    
    string TeleportPlayer(string playerName, string destination) {
        PlayerBase player = FindPlayerByName(playerName);
        if (!player) return "Player not found";
        
        vector pos;
        
        // Check for named locations
        destination.ToLower();
        if (destination == "cherno" || destination == "chernogorsk") {
            pos = "6649 0 2559";
        } else if (destination == "elektro" || destination == "elektrozavodsk") {
            pos = "10396 0 2113";
        } else if (destination == "nwaf" || destination == "airfield") {
            pos = "4523 0 10338";
        } else {
            // Try parsing as coordinates
            pos = UAIChatToolParams.Vec(destination);
            if (pos == vector.Zero) {
                return "Unknown destination. Use coords 'x y z' or: cherno, elektro, nwaf";
            }
        }
        
        // Get ground height
        pos[1] = GetGame().SurfaceY(pos[0], pos[2]);
        
        player.SetPosition(pos);
        return "Teleported " + playerName + " to " + pos.ToString();
    }
    
    string GiveItem(string playerName, string itemClass, string quantity) {
        PlayerBase player = FindPlayerByName(playerName);
        if (!player) return "Player not found";
        
        int qty = UAIChatToolParams.Int(quantity, 1);
        qty = Math.Clamp(qty, 1, 10);
        
        int given = 0;
        for (int i = 0; i < qty; i++) {
            EntityAI item = player.GetInventory().CreateInInventory(itemClass);
            if (item) given++;
        }
        
        if (given == 0) return "Failed to give items (invalid class or inventory full)";
        return "Gave " + given + "x " + itemClass + " to " + playerName;
    }
    
    string GetServerStats() {
        array<Man> players = new array<Man>;
        GetGame().GetPlayers(players);
        
        return "Server Statistics:\n" +
               "Players Online: " + players.Count() + "\n" +
               "Server Time: " + UUtil.GetTimeStamp() + "\n" +
               "Framework: Online";
    }
    
    string BroadcastMessage(string message) {
        // Send to all players
        array<Man> players = new array<Man>;
        GetGame().GetPlayers(players);
        
        foreach (Man man : players) {
            PlayerBase player = PlayerBase.Cast(man);
            if (player && player.GetIdentity()) {
                UUtil.SendNotification("Server", message, player.GetIdentity());
            }
        }
        
        return "Broadcast sent to " + players.Count() + " players";
    }
    
    // Helper
    PlayerBase FindPlayerByName(string name) {
        array<Man> players = new array<Man>;
        GetGame().GetPlayers(players);
        
        name.ToLower();
        foreach (Man man : players) {
            PlayerBase player = PlayerBase.Cast(man);
            if (player && player.GetIdentity()) {
                string pname = player.GetIdentity().GetName();
                pname.ToLower();
                if (pname.Contains(name)) {
                    return player;
                }
            }
        }
        return NULL;
    }
}

// Usage
class AdminPanel {
    protected autoptr AdminAssistant m_AI;
    
    void Init() {
        m_AI = new AdminAssistant();
    }
    
    void ProcessCommand(string input) {
        // "Teleport John to NWAF and give him an M4"
        m_AI.Chat(input, this, "OnResponse");
    }
    
    void OnResponse(int cid, int status, string oid, string response) {
        if (status == UF_SUCCESS) {
            Print("Admin AI: " + response);
        }
    }
}
```

---

## Tool Method Requirements

1. **Method name must exactly match tool name** (case-sensitive)
2. **Method must return `string`**
3. **All parameters are `string`** - use `UAIChatToolParams` to convert
4. **Maximum 5 parameters** per tool
5. **Return helpful messages** - AI uses them to continue

---

## Best Practices

### 1. Descriptive Tool Names

```enforce
// GOOD
"GetPlayerHealth"
"TeleportPlayerToLocation"
"SpawnItemAtPosition"

// BAD
"GPH"
"TP"
"Spawn"
```

### 2. Detailed Descriptions

```enforce
// GOOD - AI knows when to use it
new UAIChatToolDef("GetPlayerHealth", "Get a player's current health percentage. Returns health, blood, and shock values.", {"playerName"})

// BAD - Too vague
new UAIChatToolDef("GetPlayerHealth", "Gets health", {"playerName"})
```

### 3. Parameter Descriptions for Types

```enforce
// GOOD - AI sends correct format
new UAIChatToolParam("position", "vector", "Position as 'x y z' (e.g., '5000 0 8000')")
new UAIChatToolParam("quantity", "int", "Number of items (1-100)")

// BAD - AI might send wrong format
new UAIChatToolParam("position", "vector")
```

### 4. Return Actionable Information

```enforce
// GOOD - AI can use this info
string GetPlayerHealth(string name) {
    if (!player) return "Player '" + name + "' not found. Available players: John, Mike";
    return name + " has 75% health, 4500 blood, no shock damage";
}

// BAD - Not helpful
string GetPlayerHealth(string name) {
    if (!player) return "Error";
    return "75";
}
```

### 5. Handle Errors Gracefully

```enforce
string TeleportPlayer(string name, string pos) {
    PlayerBase player = FindPlayerByName(name);
    if (!player) {
        return "Player '" + name + "' not found. Did you mean one of: " + GetPlayerList();
    }
    
    vector position = UAIChatToolParams.Vec(pos);
    if (position == vector.Zero) {
        return "Invalid position format. Use 'x y z' like '5000 0 8000'";
    }
    
    // Success...
}
```

## Common Use Cases

### Server Management
- `KickPlayer(name, reason)`
- `BanPlayer(name, reason, time)`
- `RestartServer()`
- `SendMessage(text)`

### Gameplay Mechanics
- `SpawnEntity(class, location)`
- `Teleport(player, location)`
- `HealPlayer(player)`
- `GiveMoney(player, amount)`

### Investigation
- `GetPlayerLogs(player)`
- `CheckInventory(player)`
- `WhoKilled(player)`

## Tags
`ai`, `tools`, `function-calling`, `automation`, `game-master`, `plugins`, `how-to`, `reference`, `doc-usage`, `modder`

---

## Related Documentation

- [AI Chat - Overview](UniversalFramework_AIChat_Overview.md) - Basic concepts
- [AI Chat - Knowledge Base](UniversalFramework_AIChat_KnowledgeBase.md) - Document-backed AI responses
- [AI Chat - Typed Agents](UniversalFramework_AIChat_TypedAgents.md) - Structured responses
- [AI Chat - Examples](UniversalFramework_AIChat_Examples.md) - Complete examples
