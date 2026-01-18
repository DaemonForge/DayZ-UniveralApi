# AI Features & API Reference

This document provides a comprehensive reference for AI-powered features: AI Chat, Assistants, Text-to-Speech (TTS), Image Generation, and Knowledge Bases.

**Prerequisites**: 
- OpenAI API key in `config.json` (`OpenAIApi.ApiKey`)
- Sufficient credits/quota on OpenAI platform

---

## AI Chat Endpoints

Stateless chat system using OpenAI Responses API. Best for direct Q&A, NPCs, and simple interactions.

**Base URL**: `/AI/Chat`

### POST /AI/Chat/Create
Create a new chat session.

**Auth**: Server
**Body**:
```json
{
  "Model": "gpt-4o",
  "SystemMessage": "You are a helpful assistant.",
  "KnowledgeBaseId": "my_kb",
  "ResponseFormat": "Text", // or "JSON"
  "Tools": [{...}] // Optional OpenAI Tool definitions
}
```

### POST /AI/Chat/Send/:ChatId
Send a message to the chat.

**Auth**: Player or Server
**Body**:
```json
{
  "Message": "Hello", 
  "Context": [{"name": "PlayerName", "value": "Survivor"}]
}
```
**Response**:
```json
{
  "Status": "Success",
  "Message": "Hello Survivor!",
  "ToolCall": null 
}
```

### POST /AI/Chat/ToolResult/:MessageId
Submit the result of a tool call back to the AI.

**Auth**: Player or Server
**Body**:
```json
{
  "ToolCallId": "call_123",
  "Result": "Inventory: Apple, Bandage"
}
```

### POST /AI/Chat/Read/:ChatId
Get chat history.

**Auth**: Player or Server
**Response**: `{ Status: "Success", Messages: [...] }`

### POST /AI/Chat/Delete/:ChatId
Delete a chat session.

**Auth**: Server

### POST /AI/Chat/Summarize/:ChatId
Summarize the chat session.

**Auth**: Player or Server
**Body**: `{ "Focus": "Trade agreement" }` (Optional)

---

## AI Assistant Endpoints

Stateful Assistants API (Threads, runs). Best for complex tasks, long-term memory, and file search.

**Base URL**: `/AI/Assistant`

### POST /AI/Assistant/:Mod/Create
Create a new Assistant.

**Auth**: Server
**Body**:
```json
{
  "Name": "TraderBot",
  "Instructions": "You are a trader.",
  "Model": "gpt-4-turbo"
}
```

### POST /AI/Assistant/:Mod/CreateThread/:AssistantId
Create a thread (conversation) with an assistant.

**Auth**: Server
**Body**: `{ "InitialMessage": "Hello" }` (Optional)
**Response**: `{ "Status": "Success", "ThreadId": "thread_..." }`

### POST /AI/Assistant/Send/:ThreadId
Send a message to the assistant thread.

**Auth**: Player or Server
**Body**: `{ "Message": "What do you have?" }`

### POST /AI/Assistant/MsgStatus/:MessageId
Check status of a run/message.

**Auth**: Player or Server
**Response**: `{ "Status": "completed", "Content": "I have apples." }`

### POST /AI/Assistant/FunctionReturn/:MessageId
Submit function/tool output.

**Auth**: Server
**Body**: `{ "ToolCallId": "...", "Output": "..." }`

---

## Knowledge Base Endpoints

Vector search database for providing context to AI.

**Base URL**: `/KB`

### POST /KB/
Create a new Knowledge Base.

**Auth**: Server
**Body**: `{ "Name": "Wiki", "Description": "Game Wiki" }`
**Response**: `{ "Status": "Success", "KBId": "Wiki" }`

### DELETE /KB/:KBId
Delete a Knowledge Base.

**Auth**: Server

### POST /KB/:KBId/documents
Upload a document file (pdf, txt, md, json).

**Auth**: Server
**Form Data**: `file` (File), `name` (String), `contextHint` (String)

### POST /KB/:KBId/documents/text
Add raw text as a document.

**Auth**: Server
**Body**: `{ "Name": "Rules", "Content": "No KOS", "ContextHint": "Server Rules" }`

### POST /KB/:KBId/search
Search the Knowledge Base.

**Auth**: Server
**Body**: `{ "Query": "rules", "Limit": 3 }`

---

## Text-to-Speech (TTS) Endpoints

Generate audio from text using OpenAI.

**Base URL**: `/TTS`

### POST /TTS/Generate/:VoiceID
Generate audio.

**Auth**: Server
**URL Param**: `VoiceID` (alloy, echo, fable, onyx, nova, shimmer)
**Body**: `{ "Text": "Hello Survivor", "Format": "mp3" }`
**Response**: Binary Audio File

---

## Image Generation Endpoints

Generate images using DALL-E.

**Base URL**: `/Images`

### POST /Images/Generate
Generate an image.

**Auth**: Server
**Body**: `{ "Prompt": "A red apple", "Size": "1024x1024", "Quality": "standard" }`
**Response**: `{ "Status": "Success", "Url": "...", "Base64": "..." }`

### POST /Images/Download/:ImageId
Download a previously generated image.

**Auth**: Server

### POST /Images/Discord/:GUID
Get a player's Discord avatar converted to DDS (if supported) or PNG.

**Auth**: Server
**Body**: None (Uses GUID to look up Discord user)

---

## Configuration

In `config.json`:

```json
{
  "OpenAIApi": {
    "ApiKey": "sk-...",
    "enablePromptProtection": true
  }
}
```

## Tags
`operators`, `ai`, `openai`, `tts`, `images`, `knowledge-base`, `configuration`, `reference`, `doc-usage`
