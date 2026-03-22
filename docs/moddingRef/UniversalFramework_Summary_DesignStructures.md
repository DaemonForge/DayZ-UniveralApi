# Universal Framework - Summary & Design Structures

## Overview
A summary of Universal API and it's uses and what other context's exsist

## About

The Universal Framework (UFramework) is a backend service integration for DayZ modding that provides cloud-based database, Discord integration, AI chat, messaging queues, and utility services. It connects a DayZ Enforce Script mod (`_UFramework`) to a Node.js REST API service (`UFServerServiceV2`) backed by MongoDB. All operations are asynchronous using callbacks.

Access all features through the `UF()` singleton: `UF().db()` for database, `UF().ds()` for Discord, `UF().AI()` for AI chat, `UF().Msg()` for message queues, `UF().globals()` for global state, `UF().Api()` for external APIs, and `UF().Cron()` for scheduled tasks. Two database types exist: `OBJECT_DB` for shared data and `PLAYER_DB` for player-specific data. Configuration loads from `$profile:UF/UFramework.json` with ServerURL, ServerID, and ServerAuth fields.

### Status Checks

```enforce
UF().IsOnline()          // True if service is connected
UF().IsDiscordEnabled()  // True if Discord bot configured
UF().IsOpenAIEnabled()   // True if OpenAI API configured
UF().HasValidAuth()      // True if auth token is valid
UF().GetServerID()       // Returns the ServerID (available on client and server)
```

All REST operations use callbacks - either pass an object instance + function name string, or pass a `UFCallbackBase` subclass. Always check the status code (e.g., `UF_SUCCESS`, `UF_EMPTY`, `UF_ERROR`) before using response data. Use `autoptr` for memory management and cancel pending callbacks in destructors.

---

## Authentication & Permissions

The API uses two authentication types:

### Server Auth
The DayZ server uses a static API key (`ServerAuth` in config). Has full read/write access to all endpoints.

### Player Auth  
Players receive a JWT token issued by the server on connect. Limited access - primarily read-only for their own data.

### Permission Matrix

| Feature | Server | Player | Notes |
|---------|--------|--------|-------|
| **Object DB** ||||
| Load | [YES] Read + Create | [YES] Read only | Server can create if not exists |
| Save | [YES] | [NO] | |
| Update/Transaction | [YES] | [NO] | |
| Query | [YES] | [YES] | Both can query |
| **Player DB** ||||
| Load | [YES] Any player | [YES] Own GUID only | Player token restricts to own data |
| Save | [YES] | [NO] | |
| Update/Transaction | [YES] | [NO] | |
| Query | [YES] | [NO] | Server only |
| PublicLoad | [YES] | [YES] (no auth) | Anyone can read `Public.{mod}` data |
| PublicSave | [YES] | [NO] | |
| **Globals** ||||
| Load | [YES] | [YES] | Both can read |
| Save/Update/Transaction | [YES] | [NO] | |
| **Discord** ||||
| Get / GetChannel | [YES] | [YES] Own GUID | Player can check own Discord |
| AddRole / RemoveRole | [YES] | [NO] | |
| Mute / Kick / Move | [YES] | [NO] | |
| Send (DM) / SetNickname | [YES] | [NO] | |
| Check / CheckRole | [YES] (no auth) | [YES] (no auth) | Public endpoints |
| Channel Create/Delete/Edit | [YES] | [NO] | |
| Channel Send/Messages | [YES] | [YES] | Both can interact |
| **AI Chat** ||||
| Create | [YES] | [NO] | Server creates sessions |
| Send / Read / Reset | [YES] | [YES] | Both can use existing chats |
| MessageStatus / Summarize | [YES] | [YES] | |
| Delete | [YES] | [NO] | |
| **Message Queues** ||||
| Read | [YES] | [YES] | Per-reader pointers |
| Write | [YES] | [YES]* | *Controlled by queue `AllowPlayerWrites` setting |
| Meta / Reset / Purge | [YES] | [NO] | |
| **External APIs** ||||
| TTS Generate/Status/Download | [YES] | [YES] | Rate limited |
| ServerQuery | [YES] | [YES] | |
| Crypto prices | [YES] | [YES] | |
| Random numbers | [YES] | [YES] | |

### Key Rules

1. **Player DB is per-player isolated** - A player's auth token only allows access to their own GUID. Server can access any player. Always use `player.GetIdentity().GetId()` for `PLAYER_DB` keys, never `GetPlainId()`.

2. **Object DB is shared** - Any authenticated request can read. Only server can write.

3. **Write operations are server-only** - Save, Update, Transaction always require server auth.

4. **Public endpoints exist** - `PublicLoad` for player data and Discord `Check` work without auth.

5. **Message queue writes are configurable** - Queue metadata controls if players can write.

---

## Documentation Index

### StatusCodes.md
All status code constants (`UF_SUCCESS`, `UF_EMPTY`, `UF_ERROR`, etc.) with values and descriptions. Also covers REST error codes and the `UUtil.StatusToString()` / `UUtil.RestErrorToString()` helper functions for debugging.

### DBHandler_Basics.md
Core database operations using `UDBHandler<T>`. Covers typed Save/Load operations with automatic JSON serialization, the `T Load()` and `void Save(T data)` methods, callback signatures, and basic CRUD patterns. Shows how to extend `UDBHandler` for custom data types.

### DBHandler_Advanced.md
Advanced database features: Query operations with field/comparison/value filters, Transaction for atomic numeric increments, Update for partial field updates, QueryPaged for paginated results, and Count for counting matching documents. Includes comparison operators reference.

### Discord.md
Discord endpoint operations via `UF().ds()`. Role management (AddRole, RemoveRole, HasRole, GetRoles), direct messaging (UserSend, UserSendEmbed), channel messaging (ChannelSend, ChannelSendEmbed), voice operations (GetVoiceChannel, MoveVoiceChannel), and user info lookup (GetUserById, GetLinkedUser).

### DiscordObjects.md
Data classes for Discord integration: `UDiscordEmbed` (rich embeds with fields, author, footer, images), `UDiscordField`, `UDiscordAuthor`, `UDiscordFooter`, `UDiscordUser` (Discord user with roles, voice channel), `UDiscordObject` (webhook message), and `UChannelOptions` (channel create/update options).

### AIChat.md
AI chat system using OpenAI integration. `UFAIChatAgent` for simple string responses, `UAIChatAgent<T>` for typed JSON responses. Covers system instructions, context blocks with `UAIChatContext`, tool definitions with `UAIChatToolDef`, session management, and async polling for responses.

### AIChat_KnowledgeBase.md
Knowledge Base integration for document-backed AI responses. Attach a KB to agents with `SetKBId()`. Automatic vector search for relevant documents. KB Manager UI for document upload. REST API for embedding generation and search. Supports PDF, TXT, MD, and DOCX files.

### AIVoice.md
Text-to-Speech via OpenAI. Use `UF().Api().TTSGenerate()` with `UTTSMessage` objects, `TTSStatus()` to check completion, `TTSDownload()` to save audio. Voice constants in `UTTSVoice` class. Files saved to `$saves:{id}.mp4`.

### MessageQueues.md
Message queue system for async communication. `UQueueHandler<T>` for typed message handling, `UFMsgEndpoint` for raw operations. Supports send/receive/peek patterns, named queues, and automatic polling with configurable intervals.

### GlobalHandler.md
Global state management via `UF().globals()`. Get/Set operations for server-wide key-value storage with `UDBGlobalEndpoint`. Supports string, int, float, and JSON object values with atomic operations.

### CronManager.md
Scheduled task system via `UF().Cron()`. Register periodic callbacks with `AddCronJob()`, specify intervals in seconds, automatic execution on schedule. Use for maintenance tasks, periodic syncs, and timed game events.

### ItemStore.md
Entity serialization via `UEntityStore`. Persist entity data through `OnUFSave()`/`OnUFLoad()` callbacks. Use `Write(key, value)` and `Read(key, out value)` methods for custom data. Supports primitives, strings, vectors, and JSON objects.

### TextureSystem.md
Dynamic texture/material system via `modded class ItemBase`. Override `InitSkins()` to register textures with `RegisterTextureAndMaterial()`. Use `SetTexture(index)`, `CanPaint()`, `GetTextureName()`. Supports per-player restrictions and network sync.

### PlayerMoneyHandling.md
Physical currency system using inventory items. Register currency types via `UCurrency.Register()`. PlayerBase methods: `UGetPlayerBalance(key)`, `UAddMoney(key, amount)`, `URemoveMoney(key, amount)`. Synchronous, handles making change automatically.

### Utilities.md
Static helper functions in `UUtil` class. Player finding (FindPlayer, FindPlayerByIdentity), time functions (GetUnixInt, GetUTCUnixInt, GetDateStamp, GetTimeStamp, UnixToDateTime, UnixToDateTimeString, GetTimezoneOffsetSeconds, UTCToLocalUnix), notifications (SendNotification), config access (GetConfigInt/Float/String/Array), file I/O (SaveBase64ToFile), and status code formatting.

### Logger.md
Logging system via `UFLog` static class. Methods: `UFLog.Info()`, `UFLog.Debug()`, `UFLog.Err()`. Extend `ULoggerBase` for custom categories. Server-side file logging to `$profile:`. Configure levels with `SetLogLevels()`.

### API.md
External service integrations via `UF().Api()`. Steam server queries (`SteamQuery()` returns `UFServerStatus`), cryptocurrency prices (`CryptoPrice()`, `CryptoConvert()`, `Crypto()`), random numbers (`RandomNumbers()`), and TTS functions.

### Context.md
AI-friendly quick reference for documentation sections. Use to quickly identify which file covers a topic. Includes section lookup table, core concepts summary, and "when to use" decision guide.

## Tags
`overview`, `architecture`, `summary`, `reference`, `navigation`, `doc-usage`, `how-to`, `modder`
