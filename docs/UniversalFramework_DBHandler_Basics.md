# Universal Framework - Database Handler (Basics)

## Overview

`UDBHandler<T>` is a templated, type-safe wrapper for MongoDB operations. It automatically serializes/deserializes your data classes to JSON.

## Database Types

| Constant | Collection | Access |
|----------|------------|--------|
| `OBJECT_DB` | Objects | Shared - any client can access |
| `PLAYER_DB` | Players | Per-player - client only accesses own data |

## Permissions

| Operation | Server | Player (Client) |
|-----------|--------|----------------|
| **OBJECT_DB** |||
| Load | ✅ Read + Create | ✅ Read only |
| Save | ✅ | ❌ |
| Update/Transaction | ✅ | ❌ |
| Query | ✅ | ✅ |
| **PLAYER_DB** |||
| Load | ✅ Any player | ✅ Own GUID only |
| Save | ✅ | ❌ |
| Update/Transaction | ✅ | ❌ |
| Query | ✅ | ❌ |
| PublicLoad | ✅ | ✅ (no auth) |
| PublicSave | ✅ | ❌ |

> **Note:** Player auth tokens are GUID-specific. A player can only load their own data from `PLAYER_DB`. The server can access any player's data.

## Creating a Handler

### Constructor Signature

```enforce
// UDBHandler constructor
// Params:
//   mod (string): Your mod's unique identifier - used as collection prefix
//   database (int): PLAYER_DB or OBJECT_DB constant (default: PLAYER_DB)
UDBHandler<Class T>(string mod, int database = PLAYER_DB);
```

### Example

```enforce
// Define your data class
class MyPlayerData {
    int Level;
    float Experience;
    bool IsVIP;
    ref array<string> Achievements;
    
    void MyPlayerData() {
        Level = 1;
        Experience = 0;
        IsVIP = false;
        Achievements = new array<string>;
    }
}

// Create handler - MUST specify template type, mod name, and database
// Params: ("ModName", PLAYER_DB or OBJECT_DB)
static autoptr UDBHandler<MyPlayerData> g_PlayerHandler = new UDBHandler<MyPlayerData>("MyMod", PLAYER_DB);

// For shared/global data, use OBJECT_DB
static autoptr UDBHandler<MyConfigData> g_ConfigHandler = new UDBHandler<MyConfigData>("MyMod", OBJECT_DB);
```

> **Note:** Boolean values are stored as integers in the database (0 = false, 1 = true). This is automatic - you still use `bool` in your classes.

---

## Method Signatures

All `UDBHandler<T>` methods with exact parameter types:

```enforce
class UDBHandler<Class T> extends UDBHandlerBase {
    // Save object to database
    // Returns: int callId (-1 on error)
    int Save(string oid, Class object);
    int Save(string oid, Class object, Class cbInstance, string cbFunction);
    
    // Load object from database  
    // Returns: int callId (-1 on error)
    int Load(string oid, Class cbInstance, string cbFunction);
    int Load(string oid, Class cbInstance, string cbFunction, string defaultJson);
    int Load(string oid, Class cbInstance, string cbFunction, Class inObject);
    
    // Load raw JSON string (not parsed to object)
    int LoadJson(string oid, Class cbInstance, string cbFunction, string defaultJson);
    
    // Query for multiple records
    int Query(UDBQueryBase query, Class cbInstance, string cbFunction);
    int Query(string query, Class cbInstance, string cbFunction);
    
    // Cancel a pending callback to prevent crashes on object deletion
    void Cancel(int cid);
}
```

### Callback Signatures

```enforce
// Typed callback (Save/Load with UDBHandler<T>)
void MyCallback(int cid, int status, string oid, T data);

// JSON callback (LoadJson)
void MyJsonCallback(int cid, int status, string oid, string jsonData);

// Query callback
void MyQueryCallback(int cid, int status, string oid, UDBQueryResult<T> results);
```

### Parameter Types

| Parameter | Type | Description |
|-----------|------|-------------|
| `oid` | `string` | Object ID - unique identifier for the record |
| `object` | `Class` | The data object to save (must match template type T) |
| `cbInstance` | `Class` | Object instance containing the callback method (`this`) |
| `cbFunction` | `string` | Name of callback method as string (`"OnLoaded"`) |
| `defaultJson` | `string` | JSON string to use if record doesn't exist |
| `inObject` | `Class` | Template object for defaults |
| `cid` | `int` | Call ID returned by async operations |

---

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

Prevent callbacks on destroyed objects. **CRITICAL:** If an object is deleted while an API call is pending, the callback will crash when it tries to invoke a method on the deleted object.

### Method Signature

```enforce
// UDBHandler<T>.Cancel - wraps U().RequestCallCancel(cid)
void Cancel(int cid);

// Parameter:
//   cid (int): The call ID returned by Load/Save/Query operations
//              Only valid if cid > 0 (cid of -1 means the call failed to start)
```

### Usage Pattern

```enforce
class MyManager {
    protected int m_PendingCallId = -1;  // Track the call ID
    
    void LoadData(string id) {
        // Load returns int callId (-1 on error)
        m_PendingCallId = g_PlayerHandler.Load(id, this, "OnLoaded");
    }
    
    void OnLoaded(int cid, int status, string oid, MyPlayerData data) {
        m_PendingCallId = -1;  // Clear tracking - call completed
        // Process data...
    }
    
    void ~MyManager() {
        // Cancel pending call in destructor to prevent crash
        if (m_PendingCallId > 0) {
            g_PlayerHandler.Cancel(m_PendingCallId);
        }
    }
}
```

> **Note:** You can also use `U().RequestCallCancel(cid)` directly instead of going through the handler.

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
