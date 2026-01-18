# Universal Framework - AI Chat API Reference

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
| Create session | âœ… | âŒ | Use Agent or Handler with systemMessage |
| Send messages | âœ… | âœ… | Need ChatId |
| Delete session | âœ… | âŒ | Only server can clean up |
| Use Agents | âœ… | âŒ | Agents create sessions on first Chat() |
| Use Handlers | âœ… | âœ… | Use chatId constructor on client |

### Common Mistakes

```enforce
// âŒ WRONG: Assuming callback fires on creation
autoptr UStringAIChatHandler h = new UStringAIChatHandler("System prompt", this, "OnCreated", "gpt-4o-mini");  // OnCreated is for MESSAGES!
// âœ… CORRECT: Use NotifyOnCreated for creation callback
h.NotifyOnCreated("OnChatCreated");

// âŒ WRONG: Using Agent on client
autoptr MyAgent agent = new MyAgent();  // Creates session - clients can't!
agent.Chat("Hello", this, "OnReply");
// âœ… CORRECT: Client uses Handler with ChatId from server
autoptr UStringAIChatHandler h = new UStringAIChatHandler(chatIdFromRPC, this, "OnReply");

// âŒ WRONG: Not checking if chat is ready before getting ID
void CreateChat() {
    m_Handler = new UStringAIChatHandler("System prompt", this, "OnMsg");
    string chatId = m_Handler.GetChatId();  // Empty! Creation is async
    SendToClient(chatId);
}
// âœ… CORRECT: Wait for creation callback
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
// âŒ OLD (will cause compile error)
class MyAgent extends UFAIChatAgent {
    string GetModel() { return "gpt-4o"; }
}

// âœ… NEW (correct)
class MyAgent extends UFAIChatAgent {
    override string GetModel() { return "gpt-4o"; }
}
```

This change was made to provide a default implementation in the base class that returns an empty string (uses default model `gpt-4o-mini`).

## Tags
`ai`, `api`, `reference`, `endpoint`, `low-level`, `handlers`, `callbacks`, `doc-usage`, `modder`
