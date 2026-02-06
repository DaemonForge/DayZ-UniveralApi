# Universal Framework - AI Chat Knowledge Base

## Overview

The Knowledge Base (KB) system enhances AI Chat agents with contextual document retrieval. When a KB is attached to an agent, the AI automatically gains access to a `__kb_search` tool that queries your document collection using **vector search** - allowing the AI to find and reference relevant information without you writing any search code.

## Key Features

- **Vector Search**: Uses OpenAI's `text-embedding-3-large` model for semantic similarity search
- **Automatic Tool Injection**: KB search tool is automatically available to the AI
- **Shorter Answers**: Optional AI-powered excerpt extraction for concise responses
- **Document Chunking**: Large documents are automatically split with overlap for better retrieval
- **Multiple File Formats**: Supports TXT, MD, JSON, XML, PDF, DOC, and DOCX

## Architecture

```
Your Agent â†’ SetKBId("my_kb") â†’ AI Chat Session Created with KB
                                        â†“
                User asks question â†’ AI decides to search KB
                                        â†“
                        AI calls __kb_search tool internally
                                        â†“
            Service generates embedding â†’ Vector search â†’ Results returned
                                        â†“
                    AI uses results to formulate response
```

The KB tool call is **intercepted internally** by the service - you never see it in tool callbacks. The AI simply uses the information seamlessly.

---

## Quick Start

### 1. Create a Knowledge Base (Service Side)

First, create a KB using the service's KB Manager UI or REST API:

```javascript
// REST API - Create KB
POST /KB
{
    "kbId": "survival_guide",
    "name": "DayZ Survival Guide",
    "description": "Guides and tips for DayZ survival",
    "shorterAnswers": true,
    "extractModel": "gpt-4o-mini"
}
```

### 2. Upload Documents

Add documents to your KB:

```javascript
// REST API - Upload document
POST /KB/survival_guide/documents
Content-Type: multipart/form-data
file: [your_document.pdf]
name: "Beginner's Guide"
contextHint: "Basic survival tips for new players"
```

### 3. Use KB in Your Agent

```enforce
class SurvivalGuideAI extends UFAIChatAgent {
    
    void SurvivalGuideAI() {
        // Attach the knowledge base
        SetKBId("survival_guide");
    }
    
    override string SystemInstructions() {
        return "You are a DayZ survival expert. Use the knowledge base to provide accurate information. Always cite your sources when using KB results.";
    }
}
```

### 4. Chat with KB-Enhanced Agent

```enforce
class SurvivalHelper {
    protected autoptr SurvivalGuideAI m_AI;
    
    void Init() {
        m_AI = new SurvivalGuideAI();
    }
    
    void AskQuestion(string question) {
        m_AI.Chat(question, this, "OnResponse");
    }
    
    void OnResponse(int cid, int status, string oid, string response) {
        if (status == UF_SUCCESS) {
            // Response includes information from the KB!
            Print("AI: " + response);
        }
    }
}
```

---

## KB Configuration Options

### Shorter Answers

When enabled, the service uses AI to extract only the most relevant excerpts from search results, reducing token usage and improving response quality.

| Setting | Description |
|---------|-------------|
| `shorterAnswers: false` | Return full document chunks (default) |
| `shorterAnswers: true` | AI extracts relevant sentences only |

### Extract Model

The model used for excerpt extraction when `shorterAnswers` is enabled:

| Model | Speed | Quality | Cost |
|-------|-------|---------|------|
| `gpt-4o-mini` | Fast | Good | Low |
| `gpt-4o` | Slower | Better | Higher |

---

## API Reference

### SDK Methods

#### SetKBId

Attach a Knowledge Base to an agent before the first `Chat()` call.

```enforce
void SetKBId(string kbId);
```

**Parameters:**
- `kbId` - The KB identifier (alphanumeric and underscores only)

**Example:**
```enforce
m_AI.SetKBId("my_knowledge_base");
```

#### GetKBId

Get the currently attached KB ID.

```enforce
string GetKBId();
```

**Returns:** The KB ID or empty string if not set.

---

### REST API Endpoints

#### Knowledge Base Management

| Method | Endpoint | Description |
|--------|----------|-------------|
| `GET` | `/KB` | List all KBs |
| `POST` | `/KB` | Create a new KB |
| `GET` | `/KB/:kbId` | Get KB details |
| `PUT` | `/KB/:kbId` | Update KB settings |
| `DELETE` | `/KB/:kbId` | Delete KB and all documents |

#### Document Management

| Method | Endpoint | Description |
|--------|----------|-------------|
| `GET` | `/KB/:kbId/documents` | List all documents |
| `POST` | `/KB/:kbId/documents` | Upload a document (multipart) |
| `POST` | `/KB/:kbId/documents/text` | Add document from text |
| `GET` | `/KB/:kbId/documents/:docId` | Get document content |
| `PUT` | `/KB/:kbId/documents/:docId` | Update document |
| `DELETE` | `/KB/:kbId/documents/:docId` | Delete document |

#### Search

| Method | Endpoint | Description |
|--------|----------|-------------|
| `POST` | `/KB/:kbId/search` | Search KB documents |

---

## Document Management

### Uploading Documents

**Via File Upload:**
```javascript
POST /KB/my_kb/documents
Content-Type: multipart/form-data

file: [document.pdf]
name: "My Document"
contextHint: "Description to help retrieval"
```

**Via Text Content:**
```javascript
POST /KB/my_kb/documents/text
{
    "name": "Quick Reference",
    "content": "Your document text content here...",
    "contextHint": "Quick reference for common commands"
}
```

### Supported File Types

| Extension | MIME Type | Notes |
|-----------|-----------|-------|
| `.txt` | text/plain | Plain text |
| `.md` | text/markdown | Markdown |
| `.json` | application/json | JSON files |
| `.xml` | application/xml | XML files |
| `.pdf` | application/pdf | PDF documents |
| `.doc` | application/msword | Word (legacy) |
| `.docx` | application/vnd.openxmlformats... | Word (modern) |

### Document Chunking

Large documents are automatically split into chunks for better retrieval:

| Constant | Value | Description |
|----------|-------|-------------|
| `RECOMMENDED_CHUNK_SIZE` | 16,000 chars | Warning threshold |
| `MAX_CHUNK_SIZE` | 22,000 chars | Auto-split threshold |
| `OVERLAP_SIZE` | 1,200 chars | Overlap between chunks |

Documents under 22,000 characters are stored as a single chunk. Larger documents are split at natural break points (paragraphs, sentences, words) with overlap to preserve context.

---

## Vector Search

### How It Works

1. **Embedding Generation**: When a document is uploaded, the service generates embeddings using `text-embedding-3-large` (3072 dimensions)
2. **Query Embedding**: When the AI searches, the query is also embedded
3. **Cosine Similarity**: MongoDB Atlas performs vector search to find semantically similar content
4. **Deduplication**: Results are deduplicated by document ID, keeping the highest-scoring chunk

### Search Parameters

```javascript
POST /KB/my_kb/search
{
    "query": "how do I find water",
    "limit": 5,
    "useShorterAnswers": true  // Override KB setting
}
```

### Fallback to Text Search

If vector search fails (e.g., index not available), the service automatically falls back to MongoDB's text search.

---

## Examples

### Basic KB-Enhanced NPC

```enforce
class NPCGuide extends UFAIChatAgent {
    
    void NPCGuide() {
        SetKBId("npc_knowledge");
        SetIncludeHistory(true, 20);
    }
    
    override string SystemInstructions() {
        return "You are an NPC guide. Use the knowledge base to answer questions about the area. If you don't find relevant information, say so honestly.";
    }
}

// Usage
autoptr NPCGuide guide = new NPCGuide();
guide.Chat("What can you tell me about this town?", this, "OnGuideResponse");
```

### KB with Tools

You can combine KB access with custom tools:

```enforce
class SmartAssistant extends UFAIChatAgent {
    
    void SmartAssistant() {
        SetKBId("game_manual");
    }
    
    override string SystemInstructions() {
        return "You are a game assistant. Use the knowledge base for game information. Use tools to check player status.";
    }
    
    override void RegisterTools(out array<autoptr UAIChatToolDef> tools) {
        tools.Insert(new UAIChatToolDef("GetPlayerHealth", "Get current player health", {"playerName"}));
        tools.Insert(new UAIChatToolDef("GetPlayerLocation", "Get player's current location", {"playerName"}));
    }
    
    string GetPlayerHealth(string playerName) {
        // Your implementation
        return "100%";
    }
    
    string GetPlayerLocation(string playerName) {
        // Your implementation  
        return "Cherno";
    }
}
```

### Typed Agent with KB

KB also works with typed agents:

```enforce
class GameCommand {
    string Command;
    string Target;
    ref array<string> Parameters;
}

class CommandParser extends UAIChatAgent<GameCommand> {
    
    void CommandParser() {
        SetKBId("command_reference");
        SetSchema("GameCommand", "{\"type\":\"object\",\"properties\":{\"Command\":{\"type\":\"string\"},\"Target\":{\"type\":\"string\"},\"Parameters\":{\"type\":\"array\",\"items\":{\"type\":\"string\"}}},\"required\":[\"Command\"],\"additionalProperties\":false}");
    }
    
    override string SystemInstructions() {
        return "Parse natural language into game commands. Use the knowledge base to understand valid command formats.";
    }
}
```

---

## KB Manager UI

The service includes a built-in KB Manager accessible through the Electron app:

### Features

- **Create/Edit/Delete KBs** - Manage your knowledge bases
- **Upload Documents** - Drag & drop file upload
- **Edit Documents** - In-browser document editor
- **Test Search** - Test vector search queries
- **View Embeddings** - See embedding status for each document

### Accessing the UI

The KB Manager is available in the service's Electron app navigation menu.

---

## Best Practices

### 1. Meaningful Context Hints

Always provide context hints when uploading documents:

```javascript
// Good
contextHint: "Vehicle repair guide for cars and trucks"

// Bad
contextHint: "document1"
```

### 2. Prompt the AI to Use KB

Include instructions in your system prompt:

```enforce
override string SystemInstructions() {
    return "You are an expert assistant. ALWAYS search the knowledge base before answering questions about game mechanics. Cite your sources.";
}
```

### 3. Handle Missing Information

Guide the AI on what to do when KB doesn't have answers:

```enforce
override string SystemInstructions() {
    return "Search the knowledge base for answers. If no relevant information is found, clearly state that you don't have that information rather than making something up.";
}
```

### 4. Organize Documents by Topic

Create separate KBs for different domains:

| KB ID | Purpose |
|-------|---------|
| `game_lore` | World building, story, factions |
| `mechanics` | Game mechanics, crafting, survival |
| `commands` | Admin commands, server rules |
| `locations` | Map locations, points of interest |

### 5. Monitor Token Usage

With `shorterAnswers: true`, the AI extracts relevant excerpts, reducing the tokens sent in the KB search results. Enable this for large documents.

---

## Debugging

### Debug Logging

Both SDK and service include debug logging for KB operations:

**SDK (Enforce Script):**
- `[AIChatAgent] SetKBId: my_kb`
- `[AIChatAgent] CreateSession - KBId: my_kb`
- `[AI Chat] Creating session with KB: my_kb`

**Service (Node.js):**
- `internalKBSearch called` - KB search started
- `generateEmbeddings: Starting` - Embedding generation
- `vectorSearch: Starting` - Vector search execution
- `executeWithKBInterception: Tool call detected` - KB tool intercepted

### Common Issues

| Issue | Cause | Solution |
|-------|-------|----------|
| KB not searched | KB ID not set | Call `SetKBId()` before `Chat()` |
| No results | No embeddings | Regenerate embeddings in KB Manager |
| Wrong results | Poor context hints | Add better context hints to documents |
| Slow search | Large documents | Enable `shorterAnswers` |

---

## Technical Details

### Embedding Model

- **Model**: `text-embedding-3-large`
- **Dimensions**: 3072
- **Similarity**: Cosine

### MongoDB Requirements

For vector search, you need:
- MongoDB Atlas (M10+ cluster)
- Atlas Search enabled
- Vector search index created (auto-created by service)

If vector search is unavailable, the service falls back to text search.

### Search Limits

| Limit | Value |
|-------|-------|
| Max KB tool loops | 5 per message |
| Default result limit | 5 documents |
| Max file size | 50 MB |

---

## See Also

- [AI Chat Overview](UniversalFramework_AIChat_Overview.md)
- [AI Chat Tools](UniversalFramework_AIChat_Tools.md)
- [AI Chat Typed Agents](UniversalFramework_AIChat_TypedAgents.md)
- [Best Practices](UniversalFramework_BestPractices.md)

## Tags
`ai`, `knowledge-base`, `embeddings`, `retrieval`, `vector-search`, `how-to`, `reference`, `doc-usage`, `modder`
