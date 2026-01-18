# Universal Framework - AI Chat Quick Reference

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

## Tags
`ai`, `quickref`, `cheatsheet`, `examples`, `snippets`, `reference`, `doc-usage`, `modder`
