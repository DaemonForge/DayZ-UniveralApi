# Universal Framework - AI Chat Overview

## Introduction

The AI Chat system integrates OpenAI's GPT models into DayZ, enabling intelligent NPCs, player assistants, game masters, and automated systems. The framework handles session management, message polling, tool execution, and response parsing automatically.

## Key Features

- **String & Typed Responses** - Get plain text or structured JSON objects
- **Tool Calling** - AI can invoke your methods to gather information or perform actions
- **Knowledge Base Integration** - Attach document collections for automatic context retrieval
- **Context System** - Provide dynamic and static context to the AI
- **Conversation History** - Automatic history tracking with configurable limits

## Architecture

```
Your Mod Code
     ↓
UFAIChatAgent / UAIChatAgent<T>  (High-level agents)
     ↓
UFAIChatEndpoint                  (Low-level API)
     ↓
UFServerService                   (Node.js REST API)
     ↓
OpenAI API                        (GPT models)
```

## Agent Types

| Class | Response Type | Use Case |
|-------|---------------|----------|
| `UFAIChatAgent` | `string` | General chat, NPCs, simple assistants |
| `UAIChatAgent<T>` | Typed object | Structured decisions, game commands, data extraction |

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

> **Note:** The server must create chat sessions. Once created, both server and players can send messages.

---

## Quick Start

### 1. Create an Agent Class

```enforce
class MyAssistant extends UFAIChatAgent {
    
    override string SystemInstructions() {
        return "You are a helpful survival guide for DayZ. Provide practical advice. Never break character.";
    }
}
```

### 2. Use the Agent

```enforce
class SurvivalGuide {
    protected autoptr MyAssistant m_AI;
    
    void Init() {
        m_AI = new MyAssistant();
    }
    
    void AskQuestion(string question) {
        m_AI.Chat(question, this, "OnResponse");
    }
    
    void OnResponse(int cid, int status, string oid, string response) {
        if (status == UF_SUCCESS) {
            Print("AI: " + response);
        }
    }
}
```

---

## UFAIChatAgent - String Response Agent

The base agent class for AI chat. Returns string responses.

### Methods to Override

| Method | Purpose | Required |
|--------|---------|----------|
| `SystemInstructions()` | Return the system prompt | Yes |
| `RegisterTools()` | Register callable tools | No |
| `ExtraContext()` | Add dynamic context each message | No |
| `SystemContext()` | Add static context | No |
| `GetHistory()` | Access/transform history | No |

### Configuration Methods

```enforce
// Enable/disable history, set max messages
void SetIncludeHistory(bool include, int maxHistory = 25);

// Attach a Knowledge Base for document-backed responses
void SetKBId(string kbId);

// Add persistent context blocks
void AddStaticContext(string description, array<string> items);
```

### Main API

```enforce
// Send a message - callback receives string response
void Chat(string input, Class handler, string handlerFn);
```

### Callback Signature

```enforce
void OnResponse(int cid, int status, string oid, string response);
```

---

## Context System

Context provides the AI with information about the current situation.

### Dynamic Context (ExtraContext)

Called every message - use for current state:

```enforce
override array<string> ExtraContext() {
    array<string> ctx = new array<string>;
    ctx.Insert("Current time: " + UUtil.GetTimeStamp());
    ctx.Insert("Player health: " + GetPlayerHealth());
    ctx.Insert("Location: " + GetPlayerLocation());
    return ctx;
}
```

### Static Context (SystemContext)

Called once during setup - use for permanent knowledge:

```enforce
override array<string> SystemContext() {
    array<string> ctx = new array<string>;
    ctx.Insert("The game is DayZ, a survival game");
    ctx.Insert("Zombies are called 'infected'");
    ctx.Insert("The map is Chernarus");
    return ctx;
}
```

### Context Blocks (AddStaticContext)

Add structured context programmatically:

```enforce
void Init() {
    array<string> rules = new array<string>;
    rules.Insert("No combat logging");
    rules.Insert("No hacking");
    rules.Insert("Respect other players");
    AddStaticContext("Server Rules", rules);
    
    array<string> locations = new array<string>;
    locations.Insert("Cherno - large city, south coast");
    locations.Insert("Elektro - industrial city, south coast");
    locations.Insert("NWAF - military airfield, northwest");
    AddStaticContext("Key Locations", locations);
}
```

---

## Chat History

The agent automatically tracks conversation history.

### Configuration

```enforce
// Keep last 25 messages (default)
m_AI.SetIncludeHistory(true, 25);

// Disable history (each message is independent)
m_AI.SetIncludeHistory(false);

// Keep more history for complex conversations
m_AI.SetIncludeHistory(true, 50);
```

### Accessing History

```enforce
override array<autoptr UAIChatHistoryEntry> GetHistory() {
    array<autoptr UAIChatHistoryEntry> history = m_History;
    
    // Optionally filter or transform
    // Each entry has: Role ("user" or "assistant"), Message
    
    return history;
}
```

---

## Response Status Handling

```enforce
void OnResponse(int cid, int status, string oid, string response) {
    switch (status) {
        case UF_SUCCESS:
            // Response received successfully
            DisplayToPlayer(response);
            break;
            
        case UF_AI_PENDING:
        case UF_AI_PROCESSING:
            // Still waiting - agent handles polling automatically
            break;
            
        case UF_TIMEOUT:
            // Response took too long (2 minute limit)
            DisplayError("AI is taking too long, please try again");
            break;
            
        case UF_NOTFOUND:
            // Session expired or not found
            // Agent will recreate on next Chat() call
            break;
            
        case UF_ERROR:
            // OpenAI or service error
            UFLog.Err("AI Error: " + response);
            break;
    }
}
```

---

## Low-Level API (UFAIChatEndpoint)

For advanced control, use the endpoint directly:

```enforce
UFAIChatEndpoint ai = U().AI();

// Create session
int cid = ai.Create(
    "You are a helpful assistant",  // System message
    "string",                        // Response format
    "",                              // JSON schema (for typed)
    "gpt-4o-mini",                   // Model (optional)
    25,                              // Max history
    new UFCallback<UAIChatCreateResponse>(this, "OnCreated")
);

// Send message with context
array<autoptr UAIChatContext> ctx = new array<autoptr UAIChatContext>;
autoptr UAIChatContext gameCtx = new UAIChatContext("game_state");
gameCtx.AddContext("Player is in Cherno");
ctx.Insert(gameCtx);

ai.Send(chatId, "Where is food?", myCallback, ctx);

// Other operations
ai.Read(chatId, callback);           // Get history
ai.Reset(chatId);                    // Clear history
ai.Delete(chatId);                   // Delete session
ai.Summarize(chatId, callback);      // Get summary
ai.MessageStatus(messageId, callback); // Check status
```

---

## Best Practices

### 1. Check OpenAI Status First

```enforce
void StartAI() {
    if (!U().IsOpenAIEnabled()) {
        Print("OpenAI service not available");
        return;
    }
    m_AI = new MyAssistant();
}
```

### 2. Keep System Prompts Concise

```enforce
// GOOD - Short and focused
override string SystemInstructions() {
    return "You are a trader NPC. Offer to buy/sell items. Be brief.";
}

// BAD - Multi-line concatenation (invalid Enforce Script)
override string SystemInstructions() {
    // This will NOT compile in Enforce Script
    string msg = "Long text " +
                 "on multiple lines";
    return msg;
}
```

### 3. Limit History Size

```enforce
// For simple NPCs - short memory
SetIncludeHistory(true, 10);

// For complex assistants - longer memory
SetIncludeHistory(true, 50);

// Large histories = more tokens = higher cost + latency
```

### 4. Handle All Status Codes

```enforce
void OnResponse(int cid, int status, string oid, string response) {
    if (status != UF_SUCCESS) {
        HandleError(status);
        return;
    }
    // Use response...
}
```

### 5. Use Context for Dynamic Info

```enforce
// DON'T modify system instructions per-message
// DO use ExtraContext for dynamic information

override array<string> ExtraContext() {
    array<string> ctx = new array<string>;
    ctx.Insert("Time: " + GetServerTime());
    ctx.Insert("Weather: " + GetWeather());
    return ctx;
}
```

---

## Related Documentation

- [AI Chat - Knowledge Base](UniversalFramework_AIChat_KnowledgeBase.md) - Document-backed AI responses
- [AI Chat - Typed Agents](UniversalFramework_AIChat_TypedAgents.md) - Structured JSON responses
- [AI Chat - Tool Calling](UniversalFramework_AIChat_Tools.md) - Function calling
- [AI Chat - Examples](UniversalFramework_AIChat_Examples.md) - Complete examples
- [Callbacks](UniversalFramework_Callbacks.md) - Callback system reference
- [Status Codes](UniversalFramework_StatusCodes.md) - Status code reference
