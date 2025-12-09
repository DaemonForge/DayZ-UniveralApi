# Universal Framework - Database Handler (Basics)

## Overview

`UDBHandler<T>` is a templated, type-safe wrapper for MongoDB operations. It automatically serializes/deserializes your data classes to JSON.

## Database Types

| Constant | Collection | Access |
|----------|------------|--------|
| `OBJECT_DB` | Objects | Shared - any client can access |
| `PLAYER_DB` | Players | Per-player - client only accesses own data |

## Creating a Handler

```enforce
// Define your data class
class MyPlayerData {
    int Level;
    float Experience;
    ref array<string> Achievements;
    
    void MyPlayerData() {
        Level = 1;
        Experience = 0;
        Achievements = new array<string>;
    }
}

// Create handler (static singleton recommended)
static autoptr UDBHandler<MyPlayerData> g_PlayerHandler = new UDBHandler<MyPlayerData>("MyMod", PLAYER_DB);
```

## Save

```enforce
// Save data
MyPlayerData data = new MyPlayerData();
data.Level = 5;
data.Experience = 1250.5;

g_PlayerHandler.Save("player123", data, this, "OnSaved");

void OnSaved(int cid, int status, string oid, MyPlayerData data) {
    if (status == UF_SUCCESS) {
        Print("Saved successfully");
    } else {
        Print("Save failed: " + status);
    }
}
```

## Load

```enforce
g_PlayerHandler.Load("player123", this, "OnLoaded");

void OnLoaded(int cid, int status, string oid, MyPlayerData data) {
    if (status == UF_SUCCESS && data) {
        // Data exists
        Print("Level: " + data.Level);
    } else if (status == UF_EMPTY || !data) {
        // No data found - create defaults
        data = new MyPlayerData();
        g_PlayerHandler.Save(oid, data);
    } else {
        // Error occurred
        Print("Load error: " + status);
    }
}
```

### Load with Default Template

Provide a default object if no record exists:

```enforce
MyPlayerData defaultData = new MyPlayerData();
defaultData.Level = 1;

g_PlayerHandler.Load("player123", this, "OnLoaded", defaultData);
```

Or as JSON string:

```enforce
g_PlayerHandler.Load("player123", this, "OnLoaded", "{\"Level\": 1, \"Experience\": 0}");
```

## Handling Empty Responses

**Critical:** Always check for both status and null data.

```enforce
void OnLoaded(int cid, int status, string oid, MyPlayerData data) {
    // Case 1: Success with data
    if (status == UF_SUCCESS && data) {
        UseData(data);
        return;
    }
    
    // Case 2: Empty - no record exists
    if (status == UF_EMPTY) {
        CreateNewRecord(oid);
        return;
    }
    
    // Case 3: Success but null data (can happen with malformed JSON)
    if (status == UF_SUCCESS && !data) {
        Print("Warning: Success but null data for " + oid);
        CreateNewRecord(oid);
        return;
    }
    
    // Case 4: Error
    Print("Database error: " + status);
}

void CreateNewRecord(string oid) {
    MyPlayerData newData = new MyPlayerData();
    g_PlayerHandler.Save(oid, newData, this, "OnSaved");
}
```

## Status Codes

| Code | Constant | Meaning |
|------|----------|---------|
| 200 | `UF_SUCCESS` | Operation successful |
| 204 | `UF_EMPTY` | No record found |
| 400 | `UF_CLIENTERROR` | Bad request |
| 401 | `UF_UNAUTHORIZED` | Auth failed |
| 418 | `UF_ERROR` | General error |
| 500 | `UF_SERVERERROR` | Server error |

## LoadJson - Raw String Response

When you need the raw JSON instead of parsed object:

```enforce
g_PlayerHandler.LoadJson("player123", this, "OnJsonLoaded", "{}");

void OnJsonLoaded(int cid, int status, string oid, string jsonData) {
    if (status == UF_SUCCESS && jsonData != "" && jsonData != "{}") {
        // Parse manually if needed
        MyPlayerData obj;
        if (UJSONHandler<MyPlayerData>.FromString(jsonData, obj) && obj) {
            UseData(obj);
        }
    } else {
        // Empty or error
        CreateNewRecord(oid);
    }
}
```

## Cancel Pending Calls

Prevent callbacks on destroyed objects:

```enforce
class MyManager {
    protected int m_PendingCallId = -1;
    
    void LoadData(string id) {
        m_PendingCallId = g_PlayerHandler.Load(id, this, "OnLoaded");
    }
    
    void ~MyManager() {
        if (m_PendingCallId > 0) {
            g_PlayerHandler.Cancel(m_PendingCallId);
        }
    }
}
```

## Complete Example

```enforce
class PlayerDataManager {
    protected string m_PlayerId;
    protected autoptr MyPlayerData m_Data;
    protected int m_LoadCallId = -1;
    
    void Init(PlayerBase player) {
        m_PlayerId = player.GetIdentity().GetPlainId();
        m_LoadCallId = g_PlayerHandler.Load(m_PlayerId, this, "OnDataLoaded");
    }
    
    void OnDataLoaded(int cid, int status, string oid, MyPlayerData data) {
        m_LoadCallId = -1;
        
        if (status == UF_SUCCESS && data) {
            m_Data = data;
        } else {
            // No existing data - create new
            m_Data = new MyPlayerData();
            g_PlayerHandler.Save(m_PlayerId, m_Data);
        }
    }
    
    void AddExperience(float amount) {
        if (!m_Data) return;
        m_Data.Experience += amount;
        g_PlayerHandler.Save(m_PlayerId, m_Data);
    }
    
    void ~PlayerDataManager() {
        if (m_LoadCallId > 0) {
            g_PlayerHandler.Cancel(m_LoadCallId);
        }
    }
}
```
