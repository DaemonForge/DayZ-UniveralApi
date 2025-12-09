# Universal Framework - Message Queues

## Overview

The Message Queue system provides asynchronous communication between server and players, or between different mods/systems. Each queue maintains per-reader pointers, supporting both FIFO and LIFO ordering.

## Key Features

- **Per-Reader Pointers**: Each reader maintains their own "last read" position
- **FIFO/LIFO Ordering**: Configure queue order via metadata
- **Player Write Control**: Enable/disable player write permissions per queue
- **Queue Reset**: Mark all existing messages as "read" for all readers
- **Auto-Polling**: Handlers can automatically poll for new messages

## UQueueHandler<T> - Typed Queue Handler

### Initialization

```enforce
// Create a typed queue handler with auto-polling
autoptr UQueueHandler<MyMessage> m_Queue = new UQueueHandler<MyMessage>(
    "MyMod",           // Mod identifier
    "notifications",   // Queue name
    this,              // Callback instance
    "OnMessage",       // Callback function
    NULL,              // Queue metadata (optional)
    -1,                // Limit (-1 = all messages)
    3                  // Poll frequency in seconds
);
```

### Writing Messages

```enforce
// Define your message class
class MyMessage {
    string Title;
    string Content;
    int Priority;
}

// Write a typed message
MyMessage msg = new MyMessage();
msg.Title = "Server Alert";
msg.Content = "Airdrop incoming!";
msg.Priority = 1;

int cid = m_Queue.Write(msg);
```

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
autoptr UStringQueueHandler m_StringQueue = new UStringQueueHandler(
    "MyMod",
    "chat",
    this,
    "OnChatMessage"
);

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
autoptr UQueueHandler<MyMessage> m_Queue = new UQueueHandler<MyMessage>(
    "MyMod",
    "feedback",
    this,
    "OnFeedback",
    meta
);
```

## UFMsgEndpoint - Low-Level API

Direct endpoint access for advanced usage.

### Read Messages

```enforce
UFMsgEndpoint msg = U().Msg();

// Read all unread messages
msg.Read("MyMod", "notifications", new UFMsgCallback<MyMessage>(this, "OnMessage", "notifications"));

// Read with limit
msg.Read("MyMod", "notifications", 10, callback);
```

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
        
        m_Queue = new UQueueHandler<Announcement>(
            "Announcements", "global", this, "OnDummy", meta
        );
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
        m_Queue = new UQueueHandler<Announcement>(
            "Announcements", "global", this, "OnAnnouncement", NULL, -1, 5
        );
    }
    
    void OnAnnouncement(int cid, int status, string oid, Announcement ann) {
        if (status == UF_SUCCESS && ann) {
            // Show notification to player
            NotificationSystem.AddNotificationExtended(
                ann.Duration, ann.Title, ann.Message, ann.Icon
            );
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
        
        m_Queue = new UQueueHandler<FeedbackEntry>(
            "Feedback", "submissions", this, "OnFeedback", meta, -1, 30
        );
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
