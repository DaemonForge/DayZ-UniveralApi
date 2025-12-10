# Universal Framework - AI Chat

## Overview

The AI Chat system integrates OpenAI's GPT models into DayZ, enabling intelligent NPCs, player assistants, and automated systems. It supports both simple string responses (`UFAIChatAgent`) and structured JSON responses with type-safe parsing (`UAIChatAgent<T>`).

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

## UFAIChatAgent - String Response Agent

High-level agent for AI chat with string responses.

### Creating an Agent

```enforce
class MyAIAssistant extends UFAIChatAgent {
    
    override string SystemInstructions() {
        return "You are a helpful survival guide for DayZ. " +
               "Provide short, practical advice. " +
               "Never break character.";
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
    
    override string SystemInstructions() {
        return "You are an NPC decision-making AI. " +
               "Analyze situations and provide structured decisions.";
    }
    
    void UAIChatAgent() {
        // REQUIRED: Set JSON schema for response format
        // OpenAI strict mode requires additionalProperties: false
        SetSchema("AIDecision", "{" +
            "\"type\":\"object\"," +
            "\"properties\":{" +
                "\"Action\":{\"type\":\"string\",\"enum\":[\"attack\",\"flee\",\"trade\",\"ignore\"]}," +
                "\"Priority\":{\"type\":\"integer\",\"minimum\":1,\"maximum\":10}," +
                "\"Reason\":{\"type\":\"string\"}," +
                "\"Items\":{\"type\":\"array\",\"items\":{\"type\":\"string\"}}" +
            "}," +
            "\"required\":[\"Action\",\"Priority\",\"Reason\",\"Items\"]," +
            "\"additionalProperties\":false" +
        "}");
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

Register tools that the AI can call to get information or perform actions.

### Defining Tools

```enforce
class ToolEnabledAI extends UFAIChatAgent {
    
    override string SystemInstructions() {
        return "You are a game master. Use available tools to check player status.";
    }
    
    override void RegisterTools(out array<autoptr UAIChatToolDef> tools) {
        array<string> params1 = new array<string>;
        params1.Insert("playerName");
        tools.Insert(new UAIChatToolDef("GetPlayerHealth", "Get a player's current health", params1));
        
        array<string> params2 = new array<string>;
        params2.Insert("playerName");
        params2.Insert("itemType");
        tools.Insert(new UAIChatToolDef("CheckInventory", "Check if player has an item", params2));
    }
    
    override string OnToolCall(string toolName, string p1, string p2, string p3, string p4, string p5) {
        if (toolName == "GetPlayerHealth") {
            return GetPlayerHealthImpl(p1);
        }
        if (toolName == "CheckInventory") {
            return CheckInventoryImpl(p1, p2);
        }
        return "Unknown tool";
    }
    
    protected string GetPlayerHealthImpl(string playerName) {
        // Implementation
        return "Player " + playerName + " has 85% health";
    }
    
    protected string CheckInventoryImpl(string playerName, string itemType) {
        // Implementation
        return "Player " + playerName + " has 3x " + itemType;
    }
}
```

## UFAIChatEndpoint - Low-Level API

Direct endpoint access for advanced control.

### Create Session

```enforce
UFAIChatEndpoint ai = U().AI();

int cid = ai.Create(
    "You are a helpful assistant",  // System message
    "string",                        // Response format: "string" or "JSON"
    "",                              // JSON schema (if format is "JSON")
    "gpt-4o-mini",                  // Model (optional)
    25,                              // Max history (optional)
    new UFCallback<UAIChatCreateResponse>(this, "OnSessionCreated")
);

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
        return "You are " + m_NPCName + ", a " + m_Profession + " in post-apocalyptic Chernarus. " +
               "Stay in character. Give short, natural responses. " +
               "You may reference your profession and local knowledge.";
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
