# Universal Framework - Message Queues

## Overview

The Message Queue system provides asynchronous communication between server and players, or between different mods/systems. Each queue maintains per-reader pointers, supporting both FIFO and LIFO ordering.

## Key Features

- **Per-Reader Pointers**: Each reader maintains their own "last read" position
- **FIFO/LIFO Ordering**: Configure queue order via metadata
- **Player Write Control**: Enable/disable player write permissions per queue
- **Queue Reset**: Mark all existing messages as "read" for all readers
- **Auto-Polling**: Handlers can automatically poll for new messages
- **Skip to Latest**: Read only the most recent N messages, skipping older ones

## How Limit Works with Queue Ordering

When reading messages with a limit, the behavior depends on the queue order:

### FIFO (First In, First Out) - Default

Messages are returned **oldest first**. With a limit of 15 and 100 unread messages:
- **First read**: Returns messages 1-15 (oldest 15 unread)
- **Second read**: Returns messages 16-30
- **...and so on**

This is useful for processing messages in order.

### LIFO (Last In, First Out)

Messages are returned **newest first**. With a limit of 15 and 100 unread messages:
- **First read**: Returns messages 86-100 (newest 15 unread)
- The pointer still advances to the newest message, so subsequent reads continue from older messages

### Skip to Latest Mode

Use `ReadLatest()` when you want **only the most recent N messages** and want to **skip older unread messages**:
- Reads the newest N messages from the queue
- Updates your pointer to mark all older messages as "read"
- Perfect for "catch up" scenarios where old messages are no longer relevant

```enforce
// Example: 100 messages in queue, you want only the latest 15
U().Msg().ReadLatest("MyMod", "notifications", 15, callback);
// Returns: messages 86-100 (newest 15)
// Your pointer is updated to skip messages 1-85
// Next regular Read() would return nothing (all caught up)
```

## Permissions

| Operation | Server | Player (Client) |
|-----------|--------|----------------|
| Read | [YES] | [YES] |
| ReadLatest | [YES] | [YES] |
| Write | [YES] | [YES]* |
| Meta (configure queue) | [YES] | âŒ |
| Reset | [YES] | âŒ |
| Purge | [YES] | âŒ |

*Player writes are controlled by the queue's `AllowPlayerWrites` metadata setting. Server must enable this for players to write.

## UQueueHandler<T> - Typed Queue Handler

### Initialization

**Constructor Parameters:**
| Parameter | Type | Description |
|-----------|------|-------------|
| `modName` | string | Mod identifier |
| `queueName` | string | Queue name |
| `callbackObj` | Class | Callback instance |
| `callbackFunc` | string | Callback function name |
| `meta` | UQueueMeta | Queue metadata (optional, NULL for defaults) |
| `limit` | int | Message limit (-1 = all messages) |
| `pollFreq` | int | Poll frequency in seconds |

```enforce
// Create a typed queue handler with auto-polling
autoptr UQueueHandler<MyMessage> m_Queue = new UQueueHandler<MyMessage>("MyMod", "notifications", this, "OnMessage", NULL, -1, 3);
```

### Writing Messages

```enforce
// Define your message class
class MyMessage {
    string Title;
    string Content;
    int Priority;
}


## Best Practices

### Use Typed Handlers
Always prefer `UQueueHandler<T>` over raw strings. This ensures your message structure is consistent and handles serialization for you.

### Polling Frequency
Set your polling frequency based on urgency:
- **Chat:** 1-3 seconds (needs to feel responsive)
- **Commands/Events:** 5-10 seconds
- **Low priority logs:** 30+ seconds

### ReadLatest for "Live" Feeds
For systems like Chat or Killfeeds, use `ReadLatest()` or configure the handler with a limit. This prevents processing thousands of old messages if the server was offline for a while.

## Common Use Cases

### Cross-Server Chat
Connect multiple DayZ servers together.
1. Server A writes message to queue "GlobalChat".
2. Server B polls "GlobalChat" and displays new messages to players.
3. Use `AllowPlayerWrites: true` metadata so players can talk directly.

### Discord-to-Game Bridge
Allow Discord users to trigger specialized in-game events.
1. Discord Bot writes command object `{ "command": "spawn_zombies", "loc": "Cherno" }` to queue "AdminCommands".
2. Server polls "AdminCommands", executes the logic, and maybe writes a response back to a "DiscordLog" queue.

### Offline Notifications
Send messages to players who aren't currently online.
- Since queues persist, you can write a "Welcome Back" gift notification to a player's personal queue, which they will receive (poll) next time they log in.

## Tags
`messaging`, `queues`, `async`, `UQueueHandler`, `UFMsgEndpoint`, `cross-server`, `chat-system`, `discord-integration`, `fifo`, `lifo`, `polling`, `how-to`, `doc-usage`, `modder`


### Reading Messages (Callback)

```enforce
// Messages are delivered via callback (one per message)
void OnMessage(int cid, int status, string oid, MyMessage msg) {
    if (status == UF_SUCCESS && msg) {
        Print("Received: " + msg.Title + " - " + msg.Content);
    }
}
```

### Manual Read

```enforce
// Trigger a manual read (in addition to auto-polling)
m_Queue.Read();

// Read with limit
m_Queue.Read(5);  // Get up to 5 messages
```

## UStringQueueHandler - String Queue Handler

For simple string messages without typed objects.

```enforce
// Create a string queue handler
autoptr UStringQueueHandler m_StringQueue = new UStringQueueHandler("MyMod", "chat", this, "OnChatMessage");

// Write a string message
m_StringQueue.Write("Hello, world!");

// Callback
void OnChatMessage(int cid, int status, string oid, string message) {
    if (status == UF_SUCCESS) {
        Print("Chat: " + message);
    }
}
```

## Queue Metadata

Configure queue behavior using `UQueueMeta`:

```enforce
// Create metadata configuration
autoptr UQueueMeta meta = new UQueueMeta();
meta.Order = "FIFO";           // or "LIFO"
meta.AllowPlayerWrites = 1;    // 1 = allow, 0 = deny

// Apply during handler creation (server-only)
autoptr UQueueHandler<MyMessage> m_Queue = new UQueueHandler<MyMessage>("MyMod", "feedback", this, "OnFeedback", meta);
```

## UFMsgEndpoint - Low-Level API

Direct endpoint access for advanced usage.

### Read Messages

```enforce
UFMsgEndpoint msg = U().Msg();

// Read all unread messages
msg.Read("MyMod", "notifications", new UFMsgCallback<MyMessage>(this, "OnMessage", "notifications"));

// Read with limit (FIFO: oldest N unread, LIFO: newest N unread)
msg.Read("MyMod", "notifications", 10, callback);
```

### Read Latest (Skip Older Messages)

Skip older unread messages and read only the most recent N:

```enforce
UFMsgEndpoint msg = U().Msg();

// Read only the latest 15 messages, skip/discard older ones
msg.ReadLatest("MyMod", "notifications", 15, callback);
```

This is useful when:
- A client reconnects and has many old messages queued
- You only care about recent state, not history
- Catching up quickly without processing stale data

### Write Messages

```enforce
// Write using message object
autoptr UMessage<MyMessage> wrapped = new UMessage<MyMessage>(myMessage);
msg.Write("MyMod", "notifications", wrapped.ToJson());

// Write raw JSON
msg.Write("MyMod", "notifications", "{\"Message\": {\"Title\": \"Test\"}}");
```

### Reset Queue

Marks all existing messages as "read" for all readers.

```enforce
// Server-only operation
msg.Reset("MyMod", "notifications");

// With callback
msg.Reset("MyMod", "notifications", new UFCallback<StatusObject>(this, "OnReset"));
```

### Set Metadata

```enforce
autoptr UQueueMeta meta = new UQueueMeta();
meta.Order = "LIFO";
meta.AllowPlayerWrites = 0;

msg.SetMeta("MyMod", "admin_queue", meta);
```

### Purge Old Messages

Delete messages older than specified days.

```enforce
// Purge messages older than 7 days
msg.Purge("MyMod", "logs", 7);
```

## Write-Only Queue Handler

Create a queue handler for writing only (no polling).

```enforce
autoptr UQueueMeta meta = new UQueueMeta();
meta.AllowPlayerWrites = 0;

// No callback = no polling
autoptr UQueueHandlerBase m_WriteOnlyQueue = new UQueueHandlerBase("MyMod", "events", meta);
```

## Complete Example

### Server-Side Announcement System

```enforce
// Announcement message class
class Announcement {
    string Title;
    string Message;
    string Icon;
    int Duration;
}

// Server-side: Send announcements
class AnnouncementServer {
    protected autoptr UQueueHandler<Announcement> m_Queue;
    
    void Init() {
        autoptr UQueueMeta meta = new UQueueMeta();
        meta.Order = "FIFO";
        meta.AllowPlayerWrites = 0;  // Server only
        
        m_Queue = new UQueueHandler<Announcement>("Announcements", "global", this, "OnDummy", meta);
    }
    
    void Announce(string title, string message, int duration = 5) {
        Announcement ann = new Announcement();
        ann.Title = title;
        ann.Message = message;
        ann.Icon = "_UFramework\\images\\info.edds";
        ann.Duration = duration;
        
        m_Queue.Write(ann);
    }
    
    void OnDummy(int cid, int status, string oid, Announcement data) {
        // Server doesn't need to receive
    }
}

// Client-side: Receive announcements
class AnnouncementClient {
    protected autoptr UQueueHandler<Announcement> m_Queue;
    
    void Init() {
        m_Queue = new UQueueHandler<Announcement>("Announcements", "global", this, "OnAnnouncement", NULL, -1, 5);
    }
    
    void OnAnnouncement(int cid, int status, string oid, Announcement ann) {
        if (status == UF_SUCCESS && ann) {
            // Show notification to player
            NotificationSystem.AddNotificationExtended(ann.Duration, ann.Title, ann.Message, ann.Icon);
        }
    }
}
```

### Player Feedback Queue

```enforce
class FeedbackEntry {
    string PlayerName;
    string Category;
    string Message;
    int Timestamp;
}

// Allow players to submit feedback
class FeedbackSystem {
    protected autoptr UQueueHandler<FeedbackEntry> m_Queue;
    
    void Init() {
        autoptr UQueueMeta meta = new UQueueMeta();
        meta.AllowPlayerWrites = 1;  // Players can write
        
        m_Queue = new UQueueHandler<FeedbackEntry>("Feedback", "submissions", this, "OnFeedback", meta, -1, 30);
    }
    
    void SubmitFeedback(string category, string message, PlayerBase player) {
        FeedbackEntry entry = new FeedbackEntry();
        entry.PlayerName = player.GetIdentity().GetName();
        entry.Category = category;
        entry.Message = message;
        entry.Timestamp = UUtil.GetUnixInt();
        
        m_Queue.Write(entry);
    }
    
    void OnFeedback(int cid, int status, string oid, FeedbackEntry entry) {
        if (status == UF_SUCCESS && entry && GetGame().IsServer()) {
            Print("[Feedback] " + entry.PlayerName + ": " + entry.Message);
        }
    }
}
```

## Best Practices

1. **Use typed handlers** for structured data
2. **Set appropriate poll frequencies** - don't poll too frequently
3. **Disable player writes** for server-only queues
4. **Purge old messages** periodically to manage storage
5. **Handle UF_EMPTY status** - it's normal when no new messages exist
6. **Cancel pending reads** in destructors
7. **Use ReadLatest() for reconnection** - skip stale messages when a client reconnects
8. **Choose FIFO for ordered processing** - when message order matters
9. **Choose LIFO for latest-first** - when you want newest messages first

## Special Limit Values

| Limit Value | Behavior |
|-------------|----------|
| `-1` | Return all unread messages (up to 1000 max) |
| `0` | Update pointer to current time, return empty array (catch up without reading) |
| `N` (positive) | Return up to N messages based on queue order |
