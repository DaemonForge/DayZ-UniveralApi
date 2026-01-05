# Universal Framework - AI Chat Typed Agents

## Overview

`UAIChatAgent<T>` is a template agent that returns structured JSON responses parsed into your class type. This ensures consistent, type-safe responses from the AI - perfect for game commands, NPC decisions, and data extraction.

## When to Use Typed Agents

| Use Case | Agent Type |
|----------|------------|
| Chat, dialogue, text responses | `UFAIChatAgent` (string) |
| Game commands, structured data | `UAIChatAgent<T>` (typed) |
| NPC decisions with specific fields | `UAIChatAgent<T>` (typed) |
| Extracting data from text | `UAIChatAgent<T>` (typed) |

---

## Creating a Typed Agent

### Step 1: Define Your Response Class

```enforce
class NPCDecision {
    string Action;           // "attack", "flee", "trade", "wait"
    string Target;           // Who/what to act on
    int Priority;            // 1-10 urgency
    string Reason;           // Explanation
    ref array<string> Items; // Related items (optional)
    
    void NPCDecision() {
        Items = new array<string>;
    }
}
```

### Step 2: Create the Agent with Schema

```enforce
class DecisionAI extends UAIChatAgent<NPCDecision> {
    
    void DecisionAI() {
        // REQUIRED: Set JSON schema for response format
        SetSchema("NPCDecision", BuildSchema());
    }
    
    override string SystemInstructions() {
        return "You are an NPC decision-making AI. Analyze situations and decide on actions.";
    }
    
    // Optional: Specify model (default is gpt-4o-mini)
    override string GetModel() {
        return "gpt-4o";  // Use more capable model for complex decisions
    }
    
    protected string BuildSchema() {
        return "{\"type\":\"object\",\"properties\":{\"Action\":{\"type\":\"string\",\"enum\":[\"attack\",\"flee\",\"trade\",\"wait\"]},\"Target\":{\"type\":\"string\"},\"Priority\":{\"type\":\"integer\",\"minimum\":1,\"maximum\":10},\"Reason\":{\"type\":\"string\"},\"Items\":{\"type\":\"array\",\"items\":{\"type\":\"string\"}}},\"required\":[\"Action\",\"Target\",\"Priority\",\"Reason\"],\"additionalProperties\":false}";
    }
}
```

### Step 3: Use the Agent

```enforce
class NPCBrain {
    protected autoptr DecisionAI m_AI;
    
    void Init() {
        m_AI = new DecisionAI();
    }
    
    void EvaluateSituation(string context) {
        m_AI.Chat(context, this, "OnDecision");
    }
    
    // Callback receives TYPED object, not string!
    void OnDecision(int cid, int status, string oid, NPCDecision decision) {
        if (status == UF_SUCCESS && decision) {
            Print("Action: " + decision.Action);
            Print("Priority: " + decision.Priority);
            Print("Reason: " + decision.Reason);
            
            ExecuteDecision(decision);
        }
    }
    
    void ExecuteDecision(NPCDecision decision) {
        switch (decision.Action) {
            case "attack":
                InitiateCombat(decision.Target);
                break;
            case "flee":
                RunAway();
                break;
            case "trade":
                OfferTrade(decision.Target, decision.Items);
                break;
            case "wait":
                // Do nothing
                break;
        }
    }
}
```

---

## JSON Schema Requirements

### OpenAI Strict Mode

OpenAI's structured output uses **strict mode**, which requires:

1. `"additionalProperties": false` in the schema
2. All expected fields listed in `"required"`
3. Valid JSON Schema syntax

### Schema Property Types

| Enforce Type | JSON Schema Type | Example |
|--------------|------------------|---------|
| `string` | `"type": "string"` | `"Action": {"type": "string"}` |
| `int` | `"type": "integer"` | `"Priority": {"type": "integer"}` |
| `float` | `"type": "number"` | `"Distance": {"type": "number"}` |
| `bool` | `"type": "boolean"` | `"Hostile": {"type": "boolean"}` |
| `array<string>` | `"type": "array", "items": {"type": "string"}` | See below |
| `enum` | `"type": "string", "enum": [...]` | See below |

### Array Example

```enforce
// Enforce class
class LootList {
    ref array<string> Items;
}

// Schema for SetSchema()
string schema = "{\"type\":\"object\",\"properties\":{\"Items\":{\"type\":\"array\",\"items\":{\"type\":\"string\"}}},\"required\":[\"Items\"],\"additionalProperties\":false}";
```

### Enum Example

```enforce
// Enforce class
class WeaponChoice {
    string WeaponType;  // Will be one of the enum values
}

// Schema for SetSchema()
string schema = "{\"type\":\"object\",\"properties\":{\"WeaponType\":{\"type\":\"string\",\"enum\":[\"rifle\",\"pistol\",\"melee\",\"none\"]}},\"required\":[\"WeaponType\"],\"additionalProperties\":false}";
```

### Numeric Constraints

```enforce
// Schema with min/max
"\"Priority\":{\"type\":\"integer\",\"minimum\":1,\"maximum\":10}"
"\"Health\":{\"type\":\"number\",\"minimum\":0,\"maximum\":100}"
```

---

## SetSchema Method

```enforce
void SetSchema(string name, string jsonSchemaString);
```

| Parameter | Description |
|-----------|-------------|
| `name` | Schema name (for OpenAI, typically your class name) |
| `jsonSchemaString` | Valid JSON Schema as a string |

**Must be called before `Chat()`** - typically in the constructor.

```enforce
class MyTypedAgent extends UAIChatAgent<MyResponse> {
    void MyTypedAgent() {
        SetSchema("MyResponse", mySchemaString);
    }
}

bool HasSchema();  // Check if schema is set
```

---

## Complete Examples

### Game Command Parser

```enforce
class GameCommand {
    string Command;     // "spawn", "teleport", "give", "heal"
    string Target;      // Player name or "self"
    string ItemClass;   // For spawn/give commands
    int Quantity;       // Amount
    ref array<float> Position;  // For teleport [x, y, z]
    
    void GameCommand() {
        Position = new array<float>;
    }
}

class CommandParserAI extends UAIChatAgent<GameCommand> {
    
    void CommandParserAI() {
        SetSchema("GameCommand", "{" +
            "\"type\":\"object\"," +
            "\"properties\":{" +
                "\"Command\":{\"type\":\"string\",\"enum\":[\"spawn\",\"teleport\",\"give\",\"heal\",\"unknown\"]}," +
                "\"Target\":{\"type\":\"string\"}," +
                "\"ItemClass\":{\"type\":\"string\"}," +
                "\"Quantity\":{\"type\":\"integer\",\"minimum\":1,\"maximum\":100}," +
                "\"Position\":{\"type\":\"array\",\"items\":{\"type\":\"number\"}}" +
            "}," +
            "\"required\":[\"Command\",\"Target\"]," +
            "\"additionalProperties\":false" +
        "}");
    }
    
    override string SystemInstructions() {
        return "Parse player requests into game commands. " +
               "Extract the command type, target player, items, and quantities.";
    }
}

// Usage
class AdminTool {
    protected autoptr CommandParserAI m_Parser;
    
    void ProcessRequest(string playerInput) {
        // "Give John 5 AK rifles" -> parsed GameCommand
        m_Parser.Chat(playerInput, this, "OnCommandParsed");
    }
    
    void OnCommandParsed(int cid, int status, string oid, GameCommand cmd) {
        if (status == UF_SUCCESS && cmd) {
            switch (cmd.Command) {
                case "spawn":
                    SpawnItem(cmd.ItemClass, cmd.Quantity, cmd.Position);
                    break;
                case "give":
                    GiveToPlayer(cmd.Target, cmd.ItemClass, cmd.Quantity);
                    break;
                case "teleport":
                    TeleportPlayer(cmd.Target, cmd.Position);
                    break;
                case "heal":
                    HealPlayer(cmd.Target);
                    break;
            }
        }
    }
}
```

### Threat Assessment

```enforce
class ThreatAssessment {
    string ThreatLevel;    // "none", "low", "medium", "high", "critical"
    bool ShouldEngage;
    bool ShouldFlee;
    string PrimaryThreat;
    ref array<string> Factors;
    
    void ThreatAssessment() {
        Factors = new array<string>;
    }
}

class ThreatAnalyzerAI extends UAIChatAgent<ThreatAssessment> {
    
    void ThreatAnalyzerAI() {
        SetSchema("ThreatAssessment", "{" +
            "\"type\":\"object\"," +
            "\"properties\":{" +
                "\"ThreatLevel\":{\"type\":\"string\",\"enum\":[\"none\",\"low\",\"medium\",\"high\",\"critical\"]}," +
                "\"ShouldEngage\":{\"type\":\"boolean\"}," +
                "\"ShouldFlee\":{\"type\":\"boolean\"}," +
                "\"PrimaryThreat\":{\"type\":\"string\"}," +
                "\"Factors\":{\"type\":\"array\",\"items\":{\"type\":\"string\"}}" +
            "}," +
            "\"required\":[\"ThreatLevel\",\"ShouldEngage\",\"ShouldFlee\",\"PrimaryThreat\",\"Factors\"]," +
            "\"additionalProperties\":false" +
        "}");
    }
    
    override string SystemInstructions() {
        return "Analyze combat situations and assess threat levels. " +
               "Consider weapons, numbers, terrain, and health.";
    }
    
    override array<string> ExtraContext() {
        array<string> ctx = new array<string>;
        ctx.Insert("NPC Health: " + m_NPCHealth.ToString() + "%");
        ctx.Insert("NPC Ammo: " + m_NPCAmmo.ToString() + " rounds");
        ctx.Insert("NPC Weapon: " + m_NPCWeapon);
        return ctx;
    }
}
```

### Dialogue Choice

```enforce
class DialogueChoice {
    string Response;           // What the NPC says
    string Emotion;            // "friendly", "neutral", "hostile", "scared"
    bool OffersQuest;
    bool OffersTrade;
    string NextTopic;          // Suggested follow-up topic
}

class DialogueAI extends UAIChatAgent<DialogueChoice> {
    protected string m_NPCName;
    protected string m_NPCRole;
    
    void DialogueAI(string name, string role) {
        m_NPCName = name;
        m_NPCRole = role;
        SetSchema("DialogueChoice", "{" +
            "\"type\":\"object\"," +
            "\"properties\":{" +
                "\"Response\":{\"type\":\"string\"}," +
                "\"Emotion\":{\"type\":\"string\",\"enum\":[\"friendly\",\"neutral\",\"hostile\",\"scared\"]}," +
                "\"OffersQuest\":{\"type\":\"boolean\"}," +
                "\"OffersTrade\":{\"type\":\"boolean\"}," +
                "\"NextTopic\":{\"type\":\"string\"}" +
            "}," +
            "\"required\":[\"Response\",\"Emotion\",\"OffersQuest\",\"OffersTrade\"]," +
            "\"additionalProperties\":false" +
        "}");
    }
    
    override string SystemInstructions() {
        string instr = "You are " + m_NPCName + ", a " + m_NPCRole + ". Respond in character. Keep under 50 words per response.";
        return instr;
    }
}

// Usage
void OnDialogue(int cid, int status, string oid, DialogueChoice choice) {
    if (status == UF_SUCCESS && choice) {
        // Display response with appropriate emotion
        ShowNPCDialogue(choice.Response, choice.Emotion);
        
        // Show trade button if applicable
        if (choice.OffersTrade) {
            ShowTradeButton();
        }
        
        // Show quest marker if applicable
        if (choice.OffersQuest) {
            ShowQuestAvailable();
        }
    }
}
```

---

## Tools with Typed Agents

Typed agents can also use tools. The AI gathers information via tools, then returns a structured response:

```enforce
class IntelReport {
    string Situation;
    int ThreatCount;
    string Recommendation;
}

class ScoutAI extends UAIChatAgent<IntelReport> {
    
    void ScoutAI() {
        SetSchema("IntelReport", "{" +
            "\"type\":\"object\"," +
            "\"properties\":{" +
                "\"Situation\":{\"type\":\"string\"}," +
                "\"ThreatCount\":{\"type\":\"integer\"}," +
                "\"Recommendation\":{\"type\":\"string\"}" +
            "}," +
            "\"required\":[\"Situation\",\"ThreatCount\",\"Recommendation\"]," +
            "\"additionalProperties\":false" +
        "}");
    }
    
    override void RegisterTools(out array<autoptr UAIChatToolDef> tools) {
        tools.Insert(new UAIChatToolDef("ScanArea", "Scan for threats", {"radius"}));
        tools.Insert(new UAIChatToolDef("CheckAmmunition", "Check ammo levels", NULL));
    }
    
    // Tool methods
    string ScanArea(string radius) {
        int r = radius.ToInt();
        // Scan logic...
        return "Found 3 infected and 1 player within " + r + "m";
    }
    
    string CheckAmmunition() {
        return "45 rounds remaining";
    }
}

// The AI will:
// 1. Call ScanArea and CheckAmmunition tools
// 2. Analyze the results
// 3. Return a structured IntelReport
```

---

## Best Practices

### 1. Keep Schemas Simple

```enforce
// GOOD - Flat structure, clear types
class SimpleDecision {
    string Action;
    string Target;
    int Priority;
}

// AVOID - Deeply nested structures
class ComplexDecision {
    ref SubObject Details;  // Nested objects are harder
    ref map<string, int> Scores;  // Maps not well supported
}
```

### 2. Use Enums for Fixed Choices

```enforce
// Forces AI to pick from valid options
"\"Action\":{\"type\":\"string\",\"enum\":[\"attack\",\"flee\",\"wait\"]}"
```

### 3. Provide Clear System Instructions

```enforce
override string SystemInstructions() {
    return "Decide NPC actions. " +
           "Priority 1-3 is low, 4-6 is medium, 7-10 is urgent. " +
           "Always provide a reason for your decision.";
}
```

### 4. Handle Parse Failures

```enforce
void OnResponse(int cid, int status, string oid, MyType data) {
    if (status == UF_JSONERROR) {
        UFLog.Err("AI returned invalid JSON");
        UseDefaultBehavior();
        return;
    }
    
    if (status == UF_SUCCESS && data) {
        // Use typed data
    }
}
```

### 5. Validate Response Data

```enforce
void OnDecision(int cid, int status, string oid, NPCDecision decision) {
    if (status != UF_SUCCESS || !decision) return;
    
    // Validate even typed responses
    if (decision.Priority < 1) decision.Priority = 1;
    if (decision.Priority > 10) decision.Priority = 10;
    if (decision.Target == "") decision.Target = "none";
    
    ExecuteDecision(decision);
}
```

---

## Typed Handler Class (UAIChatHandler<T>)

For client-side usage or direct instantiation without subclassing, use `UAIChatHandler<T>`:

```enforce
// SERVER: Create typed chat with schema
string schema = "{\"type\":\"object\",\"properties\":{\"action\":{\"type\":\"string\"}},\"required\":[\"action\"]}";

// Parameters: systemMessage, callbackTarget, callbackFunc, jsonSchema, model, maxHistory
// NOTE: The callback is for MESSAGE responses, not creation!
autoptr UAIChatHandler<MyResponse> handler = new UAIChatHandler<MyResponse>("You are an AI that returns JSON.", this, "OnTypedMessage", schema, "gpt-4o-mini", 25);

// IMPORTANT: To get ChatId for sending to clients, use NotifyOnCreated
handler.NotifyOnCreated("OnChatCreated");

void OnChatCreated(int cid, int status, string chatId, bool success) {
    if (success) {
        // Send chatId to client via RPC
    }
}

// CLIENT: Connect to existing typed chat
autoptr UAIChatHandler<MyResponse> clientHandler = new UAIChatHandler<MyResponse>(chatIdFromServer, this, "OnTypedMessage", schema);
```

> **Note:** Like `UStringAIChatHandler`, the constructor callback is for message responses. Use `NotifyOnCreated()` if you need to know when the chat is created.
