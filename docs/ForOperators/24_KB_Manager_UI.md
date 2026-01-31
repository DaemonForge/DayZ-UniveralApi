# Knowledge Base Manager UI

The Knowledge Base (KB) Manager provides a graphical interface for creating and managing document collections that power AI-assisted responses. Knowledge Bases enable your AI agents to answer questions based on your own documents, lore, rules, or any text content.

---

## Accessing KB Manager

1. Right-click the **UF Service** system tray icon
2. Select **Options â†’ KB Manager**

---

## Interface Overview

The KB Manager has a two-panel layout:

| Panel | Description |
|-------|-------------|
| **Left Sidebar** | Lists all Knowledge Bases with search functionality |
| **Main Panel** | Shows KB details, documents, or document editor |

---

## Creating a Knowledge Base

1. Click **+ New KB** in the sidebar (or **+ Create Knowledge Base** from the empty state)

2. Fill in the creation form:

| Field | Required | Description |
|-------|----------|-------------|
| **KB ID** | [YES] | Unique identifier used in code (letters, numbers, underscores only) |
| **Name** | [YES] | Human-readable name |
| **Description** | âŒ | Optional description of the KB's purpose |
| **Enable Shorter Answers** | âŒ | Uses AI to extract only relevant excerpts instead of full chunks |
| **Extract Model** | âŒ | AI model for extraction (default: `gpt-5-mini`) |

3. Click **Create**

**Example KB ID Patterns:**
```
server_rules
game_lore
trader_prices
faction_info
custom_items
```

> **Important**: The KB ID cannot be changed after creation. Choose a descriptive, permanent identifier.

---

## Knowledge Base Settings

After selecting a KB, click **âš™ï¸ Settings** to modify:

- **Name**: Display name
- **Description**: Purpose description
- **Enable Shorter Answers**: Toggle AI-powered excerpt extraction
- **Extract Model**: Model for excerpt extraction

---

## Managing Documents

### Uploading Documents

1. Select a Knowledge Base from the sidebar
2. Click **ðŸ“¤ Upload Document**
3. Configure the upload:

| Field | Description |
|-------|-------------|
| **Select File** | Choose a file to upload |
| **Document Name** | Optional override for the filename |
| **Context Hint** | Description to help AI understand the document |

**Supported File Formats:**
- `.txt` - Plain text
- `.md` - Markdown
- `.xml` - XML documents
- `.json` - JSON data
- `.pdf` - PDF documents
- `.doc` / `.docx` - Microsoft Word documents

4. Click **Upload**

The document will be processed, chunked, and embedded for vector search.

---

### Creating Text Documents

For smaller content or manual entry:

1. Click **ðŸ“ New Text Document**
2. Enter document details:

| Field | Description |
|-------|-------------|
| **Document Name** | Name for the document |
| **Context Hint** | Optional context description |
| **Content** | The actual text content |

3. Click **ðŸ’¾ Save**

---

### Editing Documents

1. Click on any document in the list
2. The document editor opens with:
   - **Document Name** input
   - **Context Hint** input
   - **Content Editor** (syntax-highlighted)

3. Make your changes
4. Click **ðŸ’¾ Save**

**Size Warnings:**
- âš ï¸ **Yellow warning** at ~16,000 characters: Large document, consider splitting
- ðŸ”´ **Red warning** at ~22,000 characters: Document may be too large

---

### Deleting Documents

1. Hover over a document in the list
2. Click the **ðŸ—‘ï¸** delete icon
3. Confirm deletion

> **Warning**: Deleted documents cannot be recovered.

---

### Re-uploading Documents

To replace a document's content while keeping the same entry:

1. Hover over the document
2. Click the **Re-upload** icon
3. Select a new file
4. Confirm the replacement

This preserves the document ID and any references to it.

---

## Context Hints

Context hints help the AI understand how to use document content. Good context hints:

| Good Context Hint | Why It's Good |
|------------------|---------------|
| "Server rules and punishments for rule violations" | Describes content and use case |
| "Trader prices for weapons, updated January 2026" | Includes temporal context |
| "Faction lore for the Northern Alliance faction" | Specifies the subject |
| "Custom item crafting recipes and requirements" | Indicates document purpose |

**Bad Examples:**
- "rules" (too vague)
- "document 1" (not descriptive)
- Empty (AI has no context)

---

## Deleting a Knowledge Base

1. Select the KB from the sidebar
2. Click **ðŸ—‘ï¸ Delete**
3. Confirm deletion

> **Warning**: This permanently deletes the KB and ALL its documents. This action cannot be undone.

---

## Using Knowledge Bases in Code

### From the DayZ Mod (Enforce Script)

```cpp
// Create an AI agent with a Knowledge Base
class LoreNPC extends UFAIChatAgent {
    void LoreNPC() {
        SetKBId("game_lore");  // Use the KB ID you created
    }
    
    override string SystemInstructions() {
        return "You are a storyteller NPC. Answer questions about the world's history using the provided lore.";
    }
}

// Usage
autoptr LoreNPC npc = new LoreNPC();
npc.Chat("Tell me about the great war", this, "OnResponse");
```

### Via REST API

```bash
# Create a chat session with KB attached
curl -X POST https://your-server:3000/AI/Chat/Create \
  -H "Authorization: Bearer YOUR_AUTH_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "Model": "gpt-4o",
    "SystemMessage": "Answer using the knowledge base.",
    "KnowledgeBaseId": "server_rules"
  }'
```

---

## Shorter Answers Feature

When **Enable Shorter Answers** is turned on:

1. When a user asks a question, relevant document chunks are retrieved
2. Before sending to the main AI, an extraction model processes each chunk
3. Only the most relevant sentences/paragraphs are sent to the AI
4. Results in more focused, concise responses

**When to Use:**
- [YES] Documents with mixed content (some relevant, some not)
- [YES] Large documents where only snippets are needed
- [YES] FAQ-style content
- âŒ Small, focused documents
- âŒ When full context is always needed

**Extract Model Options:**
- `gpt-5-mini` - Fast and cost-effective (default)
- Other OpenAI models as available

---

## Best Practices

### Document Organization

1. **One topic per document**: Keep documents focused on a single subject
2. **Reasonable sizes**: 1,000-10,000 characters is ideal
3. **Clear structure**: Use headings, lists, and clear formatting
4. **Descriptive names**: "Trader_Weapon_Prices_2026" not "prices.txt"

### Knowledge Base Organization

1. **Purpose-based KBs**: Create separate KBs for different use cases
   - `server_rules` - Server rules and punishments
   - `game_lore` - World lore and history
   - `trader_info` - Trader locations and prices
   
2. **Agent-specific KBs**: Each AI agent type can have its own KB

3. **Update regularly**: Keep documents current with game changes

### Performance Tips

1. **Chunk size matters**: Documents are automatically chunked, but very long documents may have less relevant chunks
2. **Remove outdated content**: Delete documents that are no longer accurate
3. **Test your KB**: Use the search endpoint to verify relevant content is returned

---

## Troubleshooting

### Documents Not Being Found

1. **Check the query**: Ensure search terms match document content
2. **Verify embedding**: Re-upload the document to regenerate embeddings
3. **Context hint**: Add or improve the context hint

### Upload Fails

1. **Check file format**: Ensure it's a supported format
2. **File size**: Very large files may timeout
3. **Check logs**: View Console for detailed error messages

### AI Not Using KB Content

1. **Verify KB ID**: Ensure the correct KB ID is set in your agent
2. **System prompt**: Include instructions to use the knowledge base
3. **Check search**: Manually search the KB to verify content exists

### Embeddings Error

If you see embedding-related errors:
1. Verify your OpenAI API key is configured
2. Check OpenAI API quota/credits
3. Ensure MongoDB is running and accessible

---

## API Reference

For programmatic KB management, see [AI Features - Knowledge Base Endpoints](15_AI_Features.md#knowledge-base-endpoints).

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/KB/` | POST | Create a new KB |
| `/KB/:KBId` | DELETE | Delete a KB |
| `/KB/:KBId/documents` | POST | Upload a document file |
| `/KB/:KBId/documents/text` | POST | Add raw text |
| `/KB/:KBId/search` | POST | Search the KB |

---

## Related Documentation

- [AI Features](15_AI_Features.md) - Complete AI and KB API reference
- [AI Chat for Modders](../moddingRef/UniversalFramework_AIChat_KnowledgeBase.md) - Using KB in your mod
- [Configuration Editor](23_Config_Editor_UI.md) - Service settings UI

## Tags
`operators`, `kb`, `knowledge-base`, `ui`, `ai`, `how-to`, `doc-usage`
