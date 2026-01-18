# API Endpoints Reference - Messages Queue

This document covers the Message Queue API endpoints for asynchronous communication between server and clients.

## Overview

The Message Queue system provides:
- Server-to-player messaging
- Player-to-server messaging (when enabled)
- FIFO (First In, First Out) or LIFO (Last In, First Out) ordering
- Read pointers per player
- Global queue resets

**Base URL Path**: `/Messages`

**MongoDB Collections**: Queue metadata and messages are stored in the database

**Authentication**: Varies by endpoint

---

## Concepts

### Queue Structure

Each queue is identified by:
- **Mod**: The mod name/namespace
- **Queue**: The queue name within that mod

Example: `MyMod/Announcements`, `MyMod/PlayerRequests`

### Read Pointers

Each player maintains a read pointer per queue:
- Tracks what messages they've already received
- Only messages after the pointer are returned
- Pointer advances on each read

### Queue Meta

Queues have metadata that controls behavior:
- **Order**: FIFO or LIFO
- **AllowPlayerWrites**: Whether players can write to the queue
- **ResetAt**: Timestamp after which all messages are considered "new"

---

## Endpoints

### POST /Messages/Read/:Mod/:Queue

Read messages from a queue for the authenticated user.

**Authentication**: Player or Server auth

**URL Parameters**:
| Parameter | Type | Description |
|-----------|------|-------------|
| `Mod` | string | Mod name/namespace |
| `Queue` | string | Queue name |

**Request Body**:
```json
{
  "Limit": 10,
  "SkipToLatest": 0
}
```

**Fields**:
| Field | Type | Description |
|-------|------|-------------|
| `Limit` | number | Max messages to return. Use -1 for all, 0 to just update pointer |
| `SkipToLatest` | number/boolean | If 1/true, skip old messages and return only latest N |

**Response Body**:
```json
{
  "Status": "Success",
  "Messages": [
    { "text": "Server restart in 10 minutes" },
    { "text": "Double XP weekend active!" }
  ]
}
```

**Status Values**:
| Status | Meaning |
|--------|---------|
| `Success` | Messages retrieved |
| `Empty` | No messages found |
| `Error` | Operation failed |

**Behavior**:
- For players: Returns messages after their personal pointer, updates pointer
- For servers: Returns messages after global reset, no pointer update
- `Limit: 0` updates pointer to current time without returning messages ("catch up")
- `SkipToLatest: 1` returns latest N messages regardless of read history

---

### POST /Messages/Write/:Mod/:Queue

Write a message to a queue.

**Authentication**: 
- Server auth: Always allowed
- Player auth: Only if `AllowPlayerWrites` is enabled for the queue

**URL Parameters**:
| Parameter | Type | Description |
|-----------|------|-------------|
| `Mod` | string | Mod name/namespace |
| `Queue` | string | Queue name |

**Request Body**:
```json
{
  "Message": {
    "text": "Player requesting admin assistance",
    "location": [5000, 200, 8000]
  }
}
```

The `Message` field can contain any JSON data.

**Response Body**:
```json
{
  "Status": "Success",
  "Error": ""
}
```

**Response Codes**:
| Code | Status | Meaning |
|------|--------|---------|
| 200 | Success | Message written |
| 401 | NoAuth | Player writes not allowed for this queue |
| 500 | Error | Write failed |

---

### POST /Messages/Meta/:Mod/:Queue

Update queue metadata/settings.

**Authentication**: Server auth only

**URL Parameters**:
| Parameter | Type | Description |
|-----------|------|-------------|
| `Mod` | string | Mod name/namespace |
| `Queue` | string | Queue name |

**Request Body**:
```json
{
  "Order": "FIFO",
  "AllowPlayerWrites": 1
}
```

**Fields**:
| Field | Type | Description |
|-------|------|-------------|
| `Order` | string | `FIFO` (oldest first) or `LIFO` (newest first) |
| `AllowPlayerWrites` | number | 1 to allow, 0 to deny player writes |

**Response Body**:
```json
{
  "Status": "Success",
  "Error": ""
}
```

---

### POST /Messages/Reset/:Mod/:Queue

Reset a queue by updating the global reset pointer. All previous messages are ignored.

**Authentication**: Server auth only

**URL Parameters**:
| Parameter | Type | Description |
|-----------|------|-------------|
| `Mod` | string | Mod name/namespace |
| `Queue` | string | Queue name |

**Response Body**:
```json
{
  "Status": "Success",
  "Error": ""
}
```

After reset:
- All players' next read will only see messages written after the reset
- Useful for clearing old announcements or event messages

---

## Common Use Cases

### Server Announcements

```json
// Server writes announcement
POST /Messages/Write/ServerCore/Announcements
{
  "Message": {
    "type": "warning",
    "text": "Server restart in 5 minutes",
    "priority": "high"
  }
}

// Player reads announcements
POST /Messages/Read/ServerCore/Announcements
{
  "Limit": 10
}
```

### Admin Request System

```json
// Enable player writes
POST /Messages/Meta/AdminTools/Requests
{
  "Order": "FIFO",
  "AllowPlayerWrites": 1
}

// Player submits request
POST /Messages/Write/AdminTools/Requests
{
  "Message": {
    "type": "help",
    "playerGUID": "76561198012345678",
    "location": [5000, 200, 8000],
    "description": "Stuck in terrain"
  }
}

// Admin reads requests
POST /Messages/Read/AdminTools/Requests
{
  "Limit": -1
}
```

### Event Notifications

```json
// Write event start
POST /Messages/Write/Events/Notifications
{
  "Message": {
    "event": "airdrop",
    "location": [10000, 150, 12000],
    "time": "2025-01-11T15:00:00Z"
  }
}

// Player catches up to latest
POST /Messages/Read/Events/Notifications
{
  "Limit": 5,
  "SkipToLatest": 1
}
```

### Daily Reset Pattern

```json
// At server restart, reset the daily queue
POST /Messages/Reset/DailyMod/DailyMessages

// Write new day's messages
POST /Messages/Write/DailyMod/DailyMessages
{
  "Message": { "text": "Welcome to a new day!" }
}
```

---

## Queue Naming Best Practices

1. **Use descriptive names**: `Announcements`, `AdminRequests`, `EventNotifications`
2. **Separate by purpose**: Don't mix admin messages with player notifications
3. **One queue per message type**: Easier to manage and reset
4. **Avoid special characters**: Stick to alphanumeric and underscores

---

## Performance Considerations

1. **Limit messages returned**: Use `Limit` parameter to avoid large responses
2. **Clean up old queues**: Use `Reset` periodically for high-volume queues
3. **Use SkipToLatest for joining players**: Avoid flooding new players with old messages
4. **LIFO for latest-only needs**: Use LIFO order when only recent messages matter

---

## Error Handling

**Error Response Format**:
```json
{
  "Status": "Error",
  "Error": "Error description"
}
```

**Common Errors**:
| Error | Cause |
|-------|-------|
| `Invalid mod or queue name` | Name contains invalid characters |
| `Invalid limit value` | Limit is not a valid number |
| Player writes not allowed | `AllowPlayerWrites` is 0 for this queue |

## Tags
`operators`, `api`, `endpoints`, `messages`, `queues`, `reference`, `doc-usage`
