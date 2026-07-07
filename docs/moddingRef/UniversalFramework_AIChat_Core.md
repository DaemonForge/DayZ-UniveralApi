# Universal Framework - AI Chat SDK Reference

This document covers the core classes for integrating AI chat into your mod. For an introduction, see [AI Chat Overview](UniversalFramework_AIChat_Overview.md).

## Agent vs Handler Classes

| Class Type | Use When |
|-----------|----------|
| **Agent Classes** (`UFAIChatAgent`, `UAIChatAgent<T>`) | Server-side AI. Subclass to customize behavior. |
| **Handler Classes** (`UStringAIChatHandler`, `UAIChatHandler<T>`) | Client needs access, or you have an existing Chat ID. |

## Server-Client Architecture

The AI Chat system is server-authoritative:

1. **Server creates the chat session** - Only the server can call `Create()` or instantiate Agent classes
2. **Server sends Chat ID to client** - Use RPC to transmit the chat ID
3. **Client uses Handler classes** - Clients use `UStringAIChatHandler` or `UAIChatHandler<T>` with the received chat ID
4. **Both can send messages** - Once the session exists, either side can send messages

```
Server                                    Client
  |                                         |
  |  Agent.Chat() creates session           |
  +-----------------------------------------+
  |  ---- RPC: Send ChatId to client ---->  |
  |                                         |
  |                     Handler receives messages
  |                                         |
  |  <---- Both can send messages ---->     |
  |                                         |
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
autoptr UStringAIChatHandler serverHandler = new UStringAIChatHandler(
    "You are a helpful assistant", this, "OnMessageResponse", "gpt-4o-mini", 25);

// To get ChatId when creation completes, use NotifyOnCreated
serverHandler.NotifyOnCreated("OnChatCreated");

void OnChatCreated(int cid, int status, string chatId, bool success) {
    if (success) {
        // NOW send ChatId to client via RPC
    }
}

// CLIENT: Connect to existing chat using ChatId received from server
autoptr UStringAIChatHandler clientHandler = new UStringAIChatHandler(
    chatIdFromServer, this, "OnMessageResponse");
clientHandler.SendMessage("Hello from client!");
```

**Classes:** `UStringAIChatHandler` (string responses), `UAIChatHandler<T>` (typed responses)

### Pattern 3: Low-Level Endpoint (Full Control)

Best for **advanced scenarios** requiring direct API control.

```enforce
UFAIChatEndpoint ai = UF().AI();

// Create session - params: systemMessage, format, jsonSchema, model, maxHistory, callback, kbId, allowedPlayers
int cid = ai.Create("System message", "string", "", "gpt-4o-mini", 25, callback, "");

// Restrict who can access the session (server only, GUIDs or SteamID64s, empty = public)
ai.SetAccess(chatId, allowedPlayers);

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
| `allowedPlayers` | array\<string\> | Optional GUIDs/SteamID64s allowed to access this chat (NULL/empty = public) |

```enforce
autoptr UStringAIChatHandler handler = new UStringAIChatHandler(
    "You are a helpful NPC.", this, "OnMessage", "gpt-4o-mini");
```

> **Note:** When creating a new chat, explicitly pass the model parameter to distinguish from the "Existing Chat" constructor.

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
| `SetAccess(array<string> allowedPlayers)` | Replace the allowed players list (server only; empty = public) |
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

| Parameter | Type | Description |
|-----------|------|-------------|
| `systemMessage` | string | System prompt for the AI |
| `obj` | Class | Callback target for MESSAGE responses |
| `funcName` | string | Callback function for messages |
| `jsonSchema` | string | JSON schema for response format (REQUIRED) |
| `model` | string | AI model (optional - default: "gpt-4o-mini") |
| `maxHistory` | int | Max history entries (optional) |

```enforce
autoptr UAIChatHandler<MyResponse> handler = new UAIChatHandler<MyResponse>(
    "You are helpful.", this, "OnMessage", mySchema);
```

**Constructor 2: Connect to existing chat (SERVER or CLIENT)**

| Parameter | Type | Description |
|-----------|------|-------------|
| `chatId` | string | Existing chat ID received from server |
| `obj` | Class | Callback target |
| `funcName` | string | Callback function for messages |
| `jsonSchema` | string | Schema for parsing (optional if server set it) |

```enforce
autoptr UAIChatHandler<MyResponse> handler = new UAIChatHandler<MyResponse>(
    chatId, this, "OnMessage", mySchema);
```

**Message Callback Signature:**
```enforce
void OnMessageResponse(int cid, int status, string chatId, T response);
```

### Complete Server-Client Example

```enforce
// ======== SERVER SIDE ========
class ServerNPCManager {
    protected autoptr UStringAIChatHandler m_Handler;
    protected PlayerBase m_Player;
    
    void CreateNPCChat(PlayerBase player) {
        m_Player = player;
        
        // Create the chat on server
        m_Handler = new UStringAIChatHandler(
            "You are a friendly trader NPC.", this, "OnNPCMessage", "gpt-4o-mini", 25);
        
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
        return "You are a helpful survival guide for DayZ. Provide practical advice.";
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
    
    void OnAIResponse(int cid, int status, string chatId, string response) {
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
        string schema = "{\"type\":\"object\",\"properties\":{" +
            "\"Action\":{\"type\":\"string\",\"enum\":[\"attack\",\"flee\",\"trade\",\"ignore\"]}," +
            "\"Priority\":{\"type\":\"integer\",\"minimum\":1,\"maximum\":10}," +
            "\"Reason\":{\"type\":\"string\"}," +
            "\"Items\":{\"type\":\"array\",\"items\":{\"type\":\"string\"}}" +
            "},\"required\":[\"Action\",\"Priority\",\"Reason\",\"Items\"]," +
            "\"additionalProperties\":false}";
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
    
    void OnDecision(int cid, int status, string chatId, AIDecision decision) {
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

## Tags
`ai`, `core`, `architecture`, `handlers`, `agents`, `reference`, `how-to`, `modder`

