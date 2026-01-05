# Universal Framework - AI Chat

## Overview

The AI Chat system integrates OpenAI's GPT models into DayZ, enabling intelligent NPCs, player assistants, and automated systems. It supports both simple string responses (`UFAIChatAgent`) and structured JSON responses with type-safe parsing (`UAIChatAgent<T>`).

### Key Features

- **Two Agent Types**: String responses (`UFAIChatAgent`) or typed JSON (`UAIChatAgent<T>`)
- **Tool Calling**: Let the AI call functions on your agent to gather information
- **Knowledge Base Integration**: Attach document collections for AI-powered retrieval
- **Context System**: Provide dynamic and static context to guide AI responses
- **Conversation History**: Automatic history management with configurable limits

## Related Documentation

- [AI Chat Overview](UniversalFramework_AIChat_Overview.md) - Quick start guide
- [AI Chat Tools](UniversalFramework_AIChat_Tools.md) - Tool calling system
- [AI Chat Typed Agents](UniversalFramework_AIChat_TypedAgents.md) - Structured responses
- [AI Chat Knowledge Base](UniversalFramework_AIChat_KnowledgeBase.md) - Document retrieval

## Permissions

| Operation | Server | Player (Client) |
|-----------|--------|----------------|
| Create session | ✅ | ❌ |
| Send message | ✅ | ✅ |
| Check message status | ✅ | ✅ |
| Read history | ✅ | ✅ |
| Reset chat | ✅ | ✅ |
| Summarize | ✅ | ✅ |
| Delete session | ✅ | ❌ |

> **Note:** The server must create chat sessions. Once created, both server and players can send messages and interact with the chat.

### Server-Client Architecture

The AI Chat system is designed for server-authoritative usage:

1. **Server creates the chat session** - Only the server can call `Create()` or instantiate Agent classes
2. **Server sends Chat ID to client** - Use RPC to transmit the chat ID
3. **Client uses Handler classes** - Clients use `UStringAIChatHandler` or `UAIChatHandler<T>` with the received chat ID
4. **Both can send messages** - Once the session exists, either side can send messages

```
Server                                    Client
  │                                         │
  │  Agent.Chat() creates session           │
  ├─────────────────────────────────────────┤
  │  ──── RPC: Send ChatId to client ────>  │
  │                                         │
  │                     Handler receives messages
  │                                         │
  │  <──── Both can send messages ────>     │
  │                                         │
```

---

## Usage Patterns

There are **three** ways to use the AI Chat system, each suited for different scenarios:

### Pattern 1: Agent Classes (Server-Only, Subclass)

Best for **server-side AI** like NPC brains, game masters, or automated systems.

```enforce
// Define your agent by subclassing
class MyNPC extends UFAIChatAgent {
    override string SystemInstructions() {
        return "You are a helpful NPC.";
    }
    
    override string GetModel() {
        return "gpt-4o-mini";  // Optional: specify model
    }
}

// Use on SERVER
autoptr MyNPC npc = new MyNPC();
npc.Chat("Hello!", this, "OnResponse");
```

**Classes:** `UFAIChatAgent` (string responses), `UAIChatAgent<T>` (typed responses)

### Pattern 2: Handler Classes (Server or Client, Direct Instantiation)

Best for **client-side chat** or when you have an existing chat ID from the server.

```enforce
// SERVER: Create chat
// Parameters: systemMessage, callbackTarget, callbackFunc, model, maxHistory
// NOTE: The callback is for MESSAGE responses, not creation!
autoptr UStringAIChatHandler serverHandler = new UStringAIChatHandler("You are a helpful assistant", this, "OnMessageResponse", "gpt-4o-mini", 25);

// To get ChatId when creation completes, use NotifyOnCreated
serverHandler.NotifyOnCreated("OnChatCreated");

void OnChatCreated(int cid, int status, string chatId, bool success) {
    if (success) {
        // NOW send ChatId to client via RPC
    }
}

// CLIENT: Connect to existing chat using ChatId received from server
autoptr UStringAIChatHandler clientHandler = new UStringAIChatHandler(chatIdFromServer, this, "OnMessageResponse");
clientHandler.SendMessage("Hello from client!");
```

**Classes:** `UStringAIChatHandler` (string responses), `UAIChatHandler<T>` (typed responses)

### Pattern 3: Low-Level Endpoint (Full Control)

Best for **advanced scenarios** requiring direct API control.

```enforce
UFAIChatEndpoint ai = U().AI();

// Create session - params: systemMessage, format, jsonSchema, model, maxHistory, callback, kbId
int cid = ai.Create("System message", "string", "", "gpt-4o-mini", 25, callback, "");

// Send message
ai.Send(chatId, "Hello", callback, context, tools);
```

---

## Handler Classes (Client-Compatible)

Handler classes allow clients to interact with AI chats after the server creates the session.

### UStringAIChatHandler - String Responses

**Constructor 1: Create new chat (SERVER ONLY)**

Chat creation happens automatically - messages can be sent immediately (they queue until ready).

| Parameter | Type | Description |
|-----------|------|-------------|
| `systemMessage` | string | System prompt for the AI |
| `obj` | Class | Callback target for MESSAGE responses (not creation) |
| `funcName` | string | Callback function name for messages |
| `model` | string | AI model (optional - default: "gpt-4o-mini") |
| `maxHistory` | int | Max history entries (optional - default: -1 unlimited) |

```enforce
autoptr UStringAIChatHandler handler = new UStringAIChatHandler("You are a helpful NPC.", this, "OnMessage");
```

**Constructor 2: Connect to existing chat (SERVER or CLIENT)**

| Parameter | Type | Description |
|-----------|------|-------------|
| `chatId` | string | Existing chat ID received from server |
| `obj` | Class | Callback target |
| `funcName` | string | Callback function for messages |

```enforce
autoptr UStringAIChatHandler handler = new UStringAIChatHandler(chatId, this, "OnMessage");
```

**Methods:**

| Method | Description |
|--------|-------------|
| `SendMessage(string message, context)` | Send a message (auto-queues if chat not ready) |
| `SetPolling(bool enabled, int frequency)` | Configure auto-polling |
| `NotifyOnCreated(string callbackFunc)` | Set callback for when chat creation completes |
| `Summarize(callback)` | Get conversation summary |
| `Reset()` | Clear chat history |
| `Delete()` | Delete the session (server only) |
| `GetChatId()` | Get the chat ID (empty until created) |
| `GetQueueCount()` | Get number of messages waiting to be sent |

**Message Callback Signature:**
```enforce
// Called for each message response
void OnMessageResponse(int cid, int status, string chatId, string response);
```

**Creation Callback Signature (via NotifyOnCreated):**
```enforce
// Called once when chat creation completes
void OnChatCreated(int cid, int status, string chatId, bool success);
```

> **Important:** The constructor callback is for **message responses only**. To be notified when chat creation completes (to get the ChatId for RPC), use `NotifyOnCreated()`.

### UAIChatHandler<T> - Typed Responses

**Constructor 1: Create new chat (SERVER ONLY)**

Chat creation happens automatically - messages queue until ready.

| Parameter | Type | Description |
|-----------|------|-------------|
| `systemMessage` | string | System prompt for the AI |
| `obj` | Class | Callback target for MESSAGE responses |
| `funcName` | string | Callback function for messages |
| `jsonSchema` | string | JSON schema for response format (REQUIRED) |
| `model` | string | AI model (optional - default: "gpt-4o-mini") |
| `maxHistory` | int | Max history entries (optional) |

```enforce
autoptr UAIChatHandler<MyResponse> handler = new UAIChatHandler<MyResponse>("You are helpful.", this, "OnMessage", mySchema);
```

**Constructor 2: Connect to existing chat (SERVER or CLIENT)**

| Parameter | Type | Description |
|-----------|------|-------------|
| `chatId` | string | Existing chat ID received from server |
| `obj` | Class | Callback target |
| `funcName` | string | Callback function for messages |
| `jsonSchema` | string | Schema for parsing (optional if server set it) |

```enforce
autoptr UAIChatHandler<MyResponse> handler = new UAIChatHandler<MyResponse>(chatId, this, "OnMessage", mySchema);
```

**Message Callback Signature:**
```enforce
void OnMessageResponse(int cid, int status, string chatId, T response);
```

> Same behavior as `UStringAIChatHandler` - use `NotifyOnCreated()` if you need to know when the chat is created.

### Complete Server-Client Example

```enforce
// ======== SERVER SIDE ========
class ServerNPCManager {
    protected autoptr UStringAIChatHandler m_Handler;
    protected PlayerBase m_Player;
    
    void CreateNPCChat(PlayerBase player) {
        m_Player = player;
        
        // Create the chat on server
        // Parameters: systemMessage, callbackTarget, callbackFunc, model, maxHistory
        // NOTE: The callback ("OnNPCMessage") is for MESSAGE responses
        m_Handler = new UStringAIChatHandler("You are a friendly trader NPC.", this, "OnNPCMessage", "gpt-4o-mini", 25);
        
        // Set a SEPARATE callback for when creation completes
        m_Handler.NotifyOnCreated("OnChatCreated");
    }
    
    // Called when chat creation completes (via NotifyOnCreated)
    void OnChatCreated(int cid, int status, string chatId, bool success) {
        if (success && chatId != "") {
            // Send chat ID to client via RPC
            if (m_Player && m_Player.GetIdentity()) {
                GetGame().RPCSingleParam(m_Player, RPC_NPC_CHAT_ID, 
                    new Param1<string>(chatId), true, m_Player.GetIdentity());
            }
        }
    }
    
    // Called for each message response
    void OnNPCMessage(int cid, int status, string chatId, string response) {
        if (status == UF_SUCCESS) {
            Print("NPC says: " + response);
        }
    }
}

// ======== CLIENT SIDE ========
class ClientNPCChat {
    protected autoptr UStringAIChatHandler m_Handler;
    
    // Called when receiving RPC from server with chat ID
    void OnReceiveChatId(string chatId) {
        // Connect to the existing chat - no creation needed
        m_Handler = new UStringAIChatHandler(chatId, this, "OnNPCResponse");
        
        // Enable auto-polling for async responses
        m_Handler.SetPolling(true, 1);
    }
    
    void SayToNPC(string message) {
        if (m_Handler) {
            m_Handler.SendMessage(message);
        }
    }
    
    void OnNPCResponse(int cid, int status, string chatId, string response) {
        if (status == UF_SUCCESS) {
            // Display NPC response to player
            ShowNPCDialogue(response);
        } else if (status == UF_AI_PENDING || status == UF_AI_PROCESSING) {
            // Still processing - polling will retry automatically
            ShowTypingIndicator();
        }
    }
}
```

> **Key Pattern:** Use `NotifyOnCreated()` to get the ChatId, then RPC it to clients. The constructor callback is for message responses only.

---

## UFAIChatAgent - String Response Agent

High-level agent for AI chat with string responses.

### Creating an Agent

```enforce
class MyAIAssistant extends UFAIChatAgent {
    
    override string SystemInstructions() {
        return "You are a helpful survival guide for DayZ. Provide practical advice. Never break character.";
    }
    
    // Optional: Add dynamic context
    override array<string> ExtraContext() {
        array<string> ctx = new array<string>;
        ctx.Insert("Current server time: " + UUtil.GetTimeStamp());
        ctx.Insert("Weather: Clear");
        return ctx;
    }
    
    // Optional: Add static context
    override array<string> SystemContext() {
        array<string> ctx = new array<string>;
        ctx.Insert("The game takes place in post-apocalyptic Chernarus");
        ctx.Insert("Zombies are called 'infected' in DayZ");
        return ctx;
    }
}
```

### Using the Agent

```enforce
class SurvivalGuide {
    protected autoptr MyAIAssistant m_AI;
    
    void Init() {
        m_AI = new MyAIAssistant();
        m_AI.SetIncludeHistory(true, 25);  // Keep last 25 messages
    }
    
    void AskQuestion(string question) {
        m_AI.Chat(question, this, "OnAIResponse");
    }
    
    void OnAIResponse(int cid, int status, string oid, string response) {
        if (status == UF_SUCCESS) {
            Print("AI says: " + response);
        } else {
            Print("AI error: " + UUtil.StatusToString(status));
        }
    }
}
```

### Static Context Blocks

Add structured context that persists across messages:

```enforce
class ContextualAI extends UFAIChatAgent {
    void Init(array<string> rules) {
        AddStaticContext("Server Rules", rules);
        
        array<string> tips = new array<string>;
        tips.Insert("Fresh spawns start on the coast");
        tips.Insert("Military bases have best loot");
        AddStaticContext("Game Tips", tips);
    }
}
```

## UAIChatAgent<T> - Typed Response Agent

Template agent that returns typed objects parsed from JSON responses.

### Defining Response Class

```enforce
class AIDecision {
    string Action;          // "attack", "flee", "trade", "ignore"
    int Priority;           // 1-10 urgency
    string Reason;          // Explanation
    ref array<string> Items; // Related items
}
```

### Creating Typed Agent

```enforce
class DecisionAI extends UAIChatAgent<AIDecision> {
    
    void DecisionAI() {
        string schema = "{\"type\":\"object\",\"properties\":{\"Action\":{\"type\":\"string\",\"enum\":[\"attack\",\"flee\",\"trade\",\"ignore\"]},\"Priority\":{\"type\":\"integer\",\"minimum\":1,\"maximum\":10},\"Reason\":{\"type\":\"string\"},\"Items\":{\"type\":\"array\",\"items\":{\"type\":\"string\"}}},\"required\":[\"Action\",\"Priority\",\"Reason\",\"Items\"],\"additionalProperties\":false}";
        SetSchema("AIDecision", schema);
    }
    
    override string SystemInstructions() {
        return "You are an NPC decision-making AI. Analyze situations and make tactical decisions.";
    }
}
```

### Using Typed Agent

```enforce
class NPCBrain {
    protected autoptr DecisionAI m_DecisionAI;
    
    void Init() {
        m_DecisionAI = new DecisionAI();
    }
    
    void EvaluateSituation(string context) {
        m_DecisionAI.Chat(context, this, "OnDecision");
    }
    
    void OnDecision(int cid, int status, string oid, AIDecision decision) {
        if (status == UF_SUCCESS && decision) {
            Print("Action: " + decision.Action);
            Print("Priority: " + decision.Priority);
            Print("Reason: " + decision.Reason);
            
            switch (decision.Action) {
                case "attack":
                    InitiateCombat();
                    break;
                case "flee":
                    RunAway();
                    break;
                case "trade":
                    OfferTrade();
                    break;
            }
        }
    }
}
```

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

## UFAIChatEndpoint - Low-Level API

Direct endpoint access for advanced control.

### Create Session

```enforce
UFAIChatEndpoint ai = U().AI();

// Parameters: systemMessage, format, jsonSchema, model, maxHistory, callback
int cid = ai.Create("You are a helpful assistant", "string", "", "gpt-4o-mini", 25, new UFCallback<UAIChatCreateResponse>(this, "OnSessionCreated"));

void OnSessionCreated(int cid, int status, string oid, UAIChatCreateResponse resp) {
    if (status == UF_SUCCESS && resp) {
        m_ChatId = resp.ChatId;
    }
}
```

### Send Message

```enforce
// Build context
array<autoptr UAIChatContext> ctx = new array<autoptr UAIChatContext>;
autoptr UAIChatContext gameCtx = new UAIChatContext("game_state");
gameCtx.AddContext("Player is in Cherno");
gameCtx.AddContext("Time is night");
ctx.Insert(gameCtx);

ai.Send(m_ChatId, "Where can I find food?", 
    new UFCallback<UAIChatMessageResponse>(this, "OnResponse"), ctx);
```

### Check Message Status

```enforce
ai.MessageStatus(messageId, new UFCallback<UAIChatMessageResponse>(this, "OnStatus"));
```

### Read Chat History

```enforce
ai.Read(chatId, new UFCallback<UAIChatHistory>(this, "OnHistory"));
```

### Reset Chat

```enforce
ai.Reset(chatId);  // Clears history, keeps system message
```

## Response Status Handling

```enforce
void OnAIResponse(int cid, int status, string oid, string data) {
    switch (status) {
        case UF_SUCCESS:
            // Response received
            ProcessResponse(data);
            break;
            
        case UF_AI_PENDING:
        case UF_AI_PROCESSING:
            // Still waiting - agent handles polling automatically
            break;
            
        case UF_TIMEOUT:
            // Response took too long (2 minute default)
            HandleTimeout();
            break;
            
        case UF_NOTFOUND:
            // Chat session not found
            RecreateSession();
            break;
            
        case UF_ERROR:
            // OpenAI or service error
            HandleError(data);
            break;
    }
}
```

## Chat History

The agent automatically manages conversation history:

```enforce
// Configure history
m_Agent.SetIncludeHistory(true, 25);   // Enable, max 25 entries

// Access history
array<autoptr UAIChatHistoryEntry> history = m_Agent.GetHistory();
foreach (UAIChatHistoryEntry entry : history) {
    Print(entry.Role + ": " + entry.Message);
}
```

## Complete NPC Example

```enforce
// NPC Personality class
class NPCPersonality extends UFAIChatAgent {
    protected string m_NPCName;
    protected string m_Profession;
    
    void NPCPersonality(string name, string profession) {
        m_NPCName = name;
        m_Profession = profession;
        SetIncludeHistory(true, 10);
    }
    
    override string SystemInstructions() {
        string instr = "You are " + m_NPCName + ", a " + m_Profession + " in post-apocalyptic Chernarus. Stay in character. Give short, natural responses. Reference your profession and local knowledge.";
        return instr;
    }
    
    override array<string> SystemContext() {
        array<string> ctx = new array<string>;
        ctx.Insert("Location: Elektrozavodsk");
        ctx.Insert("You've been surviving here for 3 years");
        if (m_Profession == "doctor") {
            ctx.Insert("You can treat wounds and illness");
            ctx.Insert("You need medical supplies");
        } else if (m_Profession == "trader") {
            ctx.Insert("You buy and sell goods");
            ctx.Insert("You value rare items");
        }
        return ctx;
    }
}

// Usage
class NPCManager {
    protected autoptr map<string, autoptr NPCPersonality> m_NPCs;
    
    void Init() {
        m_NPCs = new map<string, autoptr NPCPersonality>;
        m_NPCs.Insert("doc_ivan", new NPCPersonality("Dr. Ivan", "doctor"));
        m_NPCs.Insert("trader_bob", new NPCPersonality("Bob", "trader"));
    }
    
    void PlayerSpeaks(string npcId, string message, PlayerBase player) {
        if (m_NPCs.Contains(npcId)) {
            m_NPCs.Get(npcId).Chat(message, this, "OnNPCResponse");
        }
    }
    
    void OnNPCResponse(int cid, int status, string oid, string response) {
        if (status == UF_SUCCESS) {
            // Display to player via UI or chat
        }
    }
}
```


## Best Practices

1. **Check OpenAI status**: Use `U().IsOpenAIEnabled()` before making calls
2. **Keep system prompts concise**: Shorter prompts = faster responses
3. **Limit history size**: Large histories increase token usage and latency
4. **Use typed agents** for structured data to ensure consistent responses
5. **Handle all status codes**: AI responses can timeout or fail
6. **Use context blocks** for dynamic information instead of modifying system prompts
7. **Use typed parameters** for tools to improve AI accuracy
8. **Use UAIChatToolParams** for safe parameter parsing with defaults
9. **Use Knowledge Base** for document-backed responses instead of hardcoding information

---

## Quick Reference

### Agent Classes (Server-Only, Subclass Pattern)

| Class | Response Type | Use `GetModel()` Override |
|-------|---------------|---------------------------|
| `UFAIChatAgent` | `string` | Yes |
| `UAIChatAgent<T>` | Typed object | Yes |

### Handler Classes (Server or Client, Direct Instantiation)

| Class | Response Type | Pass Model in Constructor |
|-------|---------------|---------------------------|
| `UStringAIChatHandler` | `string` | Yes |
| `UAIChatHandler<T>` | Typed object | Yes |

### When to Use Which

| Scenario | Use |
|----------|-----|
| Server-only AI (NPC brains, game master) | Agent classes |
| Client needs to send messages | Handler classes |
| You want to subclass with overrides | Agent classes |
| You have an existing Chat ID | Handler classes |
| Simple direct instantiation | Handler classes |

### Configuration Methods

| Method | Description |
|--------|-------------|
| `SetIncludeHistory(bool, int)` | Enable/disable history, set max messages |
| `SetKBId(string)` | Attach a Knowledge Base for document retrieval |
| `AddStaticContext(string, array<string>)` | Add persistent context blocks |

### Tool Classes

| Class | Description |
|-------|-------------|
| `UAIChatToolDef` | Tool definition (name, description, parameters) |
| `UAIChatToolParam` | Parameter definition with type and description |
| `UAIChatToolParams` | Static helper for parsing string parameters |

### Supported Parameter Types

| Type | JSON Schema | OpenAI Sends | Parse With |
|------|-------------|--------------|------------|
| `"string"` | `string` | `"John"` | Direct use |
| `"int"` | `integer` | `100` | `UAIChatToolParams.Int()` |
| `"float"` | `number` | `3.14` | `UAIChatToolParams.Float()` |
| `"bool"` | `boolean` | `true` | `UAIChatToolParams.Bool()` |
| `"vector"` | `string` | `"100 50 200"` | `UAIChatToolParams.Vec()` |

### Agent Methods to Override

| Method | Purpose |
|--------|---------|
| `SystemInstructions()` | Return system prompt for the AI |
| `GetModel()` | Return AI model name (default: "gpt-4o-mini") |
| `RegisterTools()` | Register tool definitions |
| `ExtraContext()` | Add dynamic context lines |
| `SystemContext()` | Add static context lines |
| `GetHistory()` | Access/transform history |

### Model Selection

Override `GetModel()` to specify which OpenAI model to use:

```enforce
class EfficientNPC extends UFAIChatAgent {
    override string GetModel() {
        return "gpt-4o-mini";  // Cheaper and faster for simple NPCs
    }
    
    override string SystemInstructions() {
        return "You are a villager. Keep responses brief.";
    }
}
```

#### Available Models

| Model | Best For | Speed | Cost |
|-------|----------|-------|------|
| `"gpt-4o"` | Complex tasks | Fast | Medium |
| `"gpt-4o-mini"` | General use (default) | Very Fast | Low |
| `"gpt-4-turbo"` | Legacy complex tasks | Medium | High |
| `"gpt-3.5-turbo"` | Simple tasks | Very Fast | Very Low |
| `"o1"` | Advanced reasoning | Slow | Very High |
| `"o1-mini"` | Balanced reasoning | Medium | High |
| `"o3-mini"` | Best reasoning/cost | Medium | Medium |

> **Tip:** Return empty string `""` to use the default model (`gpt-4o-mini`).

### Tool Method Requirements

- Method name must **exactly match** tool name (case-sensitive)
- Method must return `string`
- Parameters are always passed as `string` - convert with `UAIChatToolParams`
- Maximum 5 parameters per tool

---

## Gotchas & Limitations

### Handler Callback Flow

| Issue | Correct Approach |
|-------|-----------------|
| Constructor callback is for messages only | Use `NotifyOnCreated()` to get ChatId on creation |
| Callback fires for every message | Handle multiple responses in your callback |
| ChatId is empty until created | Check `GetChatId() != ""` or use `NotifyOnCreated()` |

### Tool Calling Limits

| Limit | Value | What Happens |
|-------|-------|--------------|
| Max tool calls per turn | 10 | Returns `UF_ERROR` with "Maximum tool call depth reached" |
| Max poll retries | 120 (~2 min) | Returns `UF_TIMEOUT` with "AI response timed out" |
| Max parameters per tool | 5 | Additional params ignored |

### Polling Behavior

- Handlers with `SetPolling(true)` automatically retry pending messages
- Default poll frequency: 1 second
- Agents use `CallLater` with 1 second delay for polling

### Server vs Client

| Operation | Server | Client | Notes |
|-----------|--------|--------|-------|
| Create session | ✅ | ❌ | Use Agent or Handler with systemMessage |
| Send messages | ✅ | ✅ | Need ChatId |
| Delete session | ✅ | ❌ | Only server can clean up |
| Use Agents | ✅ | ❌ | Agents create sessions on first Chat() |
| Use Handlers | ✅ | ✅ | Use chatId constructor on client |

### Common Mistakes

```enforce
// ❌ WRONG: Assuming callback fires on creation
autoptr UStringAIChatHandler h = new UStringAIChatHandler("System prompt", this, "OnCreated", "gpt-4o-mini");  // OnCreated is for MESSAGES!
// ✅ CORRECT: Use NotifyOnCreated for creation callback
h.NotifyOnCreated("OnChatCreated");

// ❌ WRONG: Using Agent on client
autoptr MyAgent agent = new MyAgent();  // Creates session - clients can't!
agent.Chat("Hello", this, "OnReply");
// ✅ CORRECT: Client uses Handler with ChatId from server
autoptr UStringAIChatHandler h = new UStringAIChatHandler(chatIdFromRPC, this, "OnReply");

// ❌ WRONG: Not checking if chat is ready before getting ID
void CreateChat() {
    m_Handler = new UStringAIChatHandler("System prompt", this, "OnMsg");
    string chatId = m_Handler.GetChatId();  // Empty! Creation is async
    SendToClient(chatId);
}
// ✅ CORRECT: Wait for creation callback
void CreateChat() {
    m_Handler = new UStringAIChatHandler("System prompt", this, "OnMsg");
    m_Handler.NotifyOnCreated("OnReady");
}
void OnReady(int cid, int status, string chatId, bool success) {
    if (success) SendToClient(chatId);  // Now chatId is valid
}
```

### Message Queue Behavior

When using Handlers with the creation constructor:
- Messages sent before creation completes are **automatically queued**
- Queued messages are sent in order once creation completes
- Use `GetQueueCount()` to check pending messages

```enforce
autoptr UStringAIChatHandler h = new UStringAIChatHandler("Prompt", this, "OnMsg");
h.SendMessage("Hello");  // Queued (chat not ready yet)
h.SendMessage("World");  // Queued
// Both messages send automatically when chat is created
```

---

## Migration Notes

### v2.x: GetModel() Now Requires Override

If you have an existing mod that extends `UFAIChatAgent` or `UAIChatAgent<T>` and defines a `GetModel()` method, you must add the `override` keyword:

```enforce
// ❌ OLD (will cause compile error)
class MyAgent extends UFAIChatAgent {
    string GetModel() { return "gpt-4o"; }
}

// ✅ NEW (correct)
class MyAgent extends UFAIChatAgent {
    override string GetModel() { return "gpt-4o"; }
}
```

This change was made to provide a default implementation in the base class that returns an empty string (uses default model `gpt-4o-mini`).