# Universal Framework - Callback System

## Overview

All Universal Framework operations are **asynchronous**. When you make an API call (database, Discord, AI, etc.), the result comes back later via a callback. This document covers the callback architecture, patterns, and best practices.

## Callback Styles

There are two ways to handle callbacks:

### Style 1: Instance + Function Name (Recommended)

Pass an object instance and the name of the callback function as a string:

```enforce
// Make the call
U().db().Load("MyMod", "player123", this, "OnPlayerLoaded");

// Define the callback function on your class
void OnPlayerLoaded(int cid, int status, string oid, string data) {
    if (status == UF_SUCCESS) {
        Print("Loaded: " + data);
    }
}
```

### Style 2: UFCallbackBase Subclass

Create a callback class for more complex scenarios:

```enforce
// Define a typed callback
class MyLoadCallback extends UFCallback<MyPlayerData> {
    void MyLoadCallback(Class instance, string function) {
        UFCallbackBase(instance, function);
    }
}

// Use it
U().db().Load("MyMod", "player123", new MyLoadCallback(this, "OnLoaded"));
```

---

## Standard Callback Signature

All callbacks receive these 4 parameters:

```enforce
void MyCallback(int cid, int status, string oid, T data)
```

| Parameter | Type | Description |
|-----------|------|-------------|
| `cid` | `int` | Call ID - unique identifier for this request |
| `status` | `int` | Status code (see Status Codes section) |
| `oid` | `string` | Object ID - the key/identifier used in the request |
| `data` | `T` | The response data (type varies by operation) |

### Data Types by Operation

| Operation | Data Type | Example |
|-----------|-----------|---------|
| `db().Load()` | `string` (raw JSON) or `T` (typed) | `MyPlayerData` |
| `db().Save()` | `string` | Status message |
| `db().Query()` | `UDBQueryResult<T>` | Array of results |
| `ds().GetUser()` | `UDiscordUser` | Discord user info |
| `AI().Chat()` | `string` or `T` | AI response |
| `globals().Load()` | `string` | JSON string |

---

## Status Codes

Always check the status before using data:

```enforce
void OnDataLoaded(int cid, int status, string oid, string data) {
    switch (status) {
        case UF_SUCCESS:      // 200 - Success, data is valid
            ProcessData(data);
            break;
        case UF_EMPTY:        // 204 - No record found
            CreateDefault();
            break;
        case UF_UNAUTHORIZED: // 401 - Auth failed
            RefreshToken();
            break;
        case UF_NOTFOUND:     // 404 - Resource not found
            HandleNotFound();
            break;
        case UF_TIMEOUT:      // 408 - Request timed out
            RetryRequest();
            break;
        case UF_ERROR:        // 418 - General error
            LogError(data);
            break;
        case UF_SERVERERROR:  // 500 - Server error
            HandleServerError();
            break;
    }
}
```

### Quick Status Check Pattern

```enforce
void OnLoaded(int cid, int status, string oid, MyData data) {
    // Success with valid data
    if (status == UF_SUCCESS && data) {
        UseData(data);
        return;
    }
    
    // Empty result - create default
    if (status == UF_EMPTY) {
        data = new MyData();
        SaveDefault(data);
        return;
    }
    
    // Any error
    UFLog.Err("Load failed: " + status);
}
```

---

## Callback Classes

### UFCallbackBase

The base class for all custom callbacks:

```enforce
class UFCallbackBase extends Managed {
    protected Class Instance;    // Object to call back to
    protected string Function;   // Function name to call
    protected string OID;        // Object ID for context
    
    void UFCallbackBase(Class instance, string function, string oid = "");
    
    // Override these in subclasses
    void OnError(int errorCode, int cid);
    void OnSuccess(string jsonData, int cid);
}
```

### UFCallback<T> - Typed Callback

Automatically deserializes JSON to your class type:

```enforce
class MyPlayerData {
    string Name;
    int Level;
    float Experience;
}

// The callback automatically parses JSON to MyPlayerData
void OnPlayerLoaded(int cid, int status, string oid, MyPlayerData data) {
    if (status == UF_SUCCESS && data) {
        Print("Player: " + data.Name + " Level: " + data.Level);
    }
}
```

### UFCallbackLoader<T> - Load into Existing Object

Deserializes JSON into an existing object instance. This requires manually creating the callback or using `UDBHandler<T>`:

**Option A: Using UDBHandler (Recommended)**
```enforce
class MyConfig {
    int MaxPlayers = 50;
    bool PvPEnabled = true;
}

// Handler knows the type
autoptr UDBHandler<MyConfig> handler = new UDBHandler<MyConfig>("MyMod", OBJECT_DB);
autoptr MyConfig m_Config = new MyConfig();

// Load calls UFCallbackLoader internally
handler.Load("config", this, "OnConfigLoaded", m_Config);
```

**Option B: Manual Callback Construction**
```enforce
autoptr MyConfig m_Config = new MyConfig();

// Create loader manually
autoptr UFCallbackLoader<MyConfig> loader = new UFCallbackLoader<MyConfig>(this, "OnConfigLoaded");
loader.SetObject(m_Config);

// Use raw endpoint
U().db().Load("MyMod", "config", loader);
```

### UJSONCallback - Raw JSON

Returns raw JSON string without parsing:

```enforce
void OnRawData(int cid, int status, string oid, string jsonString) {
    if (status == UF_SUCCESS) {
        // Parse manually or forward elsewhere
        Print("Raw JSON: " + jsonString);
    }
}
```

---

## Call ID Management

Every async operation returns a **Call ID** (`cid`). Use this to:

1. **Track requests** - Know which request a callback belongs to
2. **Cancel pending calls** - Prevent callbacks after object deletion

### Tracking Call IDs

```enforce
class MyManager {
    protected int m_PendingLoadCid = -1;
    
    void LoadData() {
        m_PendingLoadCid = U().db().Load("MyMod", "data", this, "OnLoaded");
        Print("Started load with CID: " + m_PendingLoadCid);
    }
    
    void OnLoaded(int cid, int status, string oid, string data) {
        if (cid == m_PendingLoadCid) {
            Print("This is the load we requested");
            m_PendingLoadCid = -1;  // Clear tracked ID
        }
    }
}
```

### Cancelling Calls

**Critical:** Cancel pending calls when your object is destroyed to prevent crashes:

```enforce
class MyEntity extends ItemBase {
    protected int m_LoadCallId = -1;
    
    void LoadMyData() {
        m_LoadCallId = U().db().Load("MyMod", GetId(), this, "OnDataLoaded");
    }
    
    void ~MyEntity() {
        // CRITICAL: Cancel pending call to prevent crash
        if (m_LoadCallId != -1) {
            U().RequestCallCancel(m_LoadCallId);
        }
    }
    
    void OnDataLoaded(int cid, int status, string oid, string data) {
        m_LoadCallId = -1;  // Clear after callback
        // Process data...
    }
}
```

### UDBHandler Cancel Method

`UDBHandler<T>` has a built-in `Cancel()` method:

```enforce
class PlayerDataManager {
    protected autoptr UDBHandler<PlayerData> m_Handler;
    protected int m_LastCid = -1;
    
    void Init() {
        m_Handler = new UDBHandler<PlayerData>("MyMod", PLAYER_DB);
    }
    
    void LoadPlayer(string guid) {
        m_LastCid = m_Handler.Load(guid, this, "OnLoaded");
    }
    
    void ~PlayerDataManager() {
        // Cancel using handler method
        if (m_LastCid != -1) {
            m_Handler.Cancel(m_LastCid);
        }
    }
}
```

---

## UNestedCallBack

Internal wrapper that bridges `RestCallback` to `UFCallbackBase`. You typically don't use this directly, but it's how the framework chains callbacks:

```enforce
// How the framework uses it internally
int cid = -1;
Post(url, json, U().RegisterCall(new UNestedCallBack(myCallback), cid));
```

The `UNestedCallBack`:
- Handles REST errors, timeouts, and success
- Checks if the call was cancelled before invoking your callback
- Converts empty responses to `UF_EMPTY` status
- Passes data to your `UFCallbackBase` subclass

---

## Critical Quirks & Patterns

### âš ï¸ Always Use Class.CastTo() for Typed Callbacks

**Problem:** Directly assigning typed callback parameters can cause crashes when the framework passes unexpected types or null values.

âŒ **WRONG - Can crash:**
```enforce
void OnDataLoaded(int cid, int status, string oid, MyPlayerData data) {
    m_PlayerData = data;  // Direct assignment - can crash!
    m_PlayerData.SomeMethod();  // Crash on null or wrong type
}
```

âœ… **CORRECT - Use Class.CastTo():**
```enforce
void OnDataLoaded(int cid, int status, string oid, MyPlayerData data) {
    if (status == UF_SUCCESS) {
        Class.CastTo(m_PlayerData, data);  // Safe extraction
        if (m_PlayerData) {
            m_PlayerData.SomeMethod();  // Now safe
        }
    }
}
```

**Why this matters:** The callback system passes data through generic interfaces. `Class.CastTo()` performs runtime type resolution that direct assignment cannot, preventing crashes from type mismatches or null values.

### âš ï¸ Query Results Require Class.CastTo()

**Problem:** Getting results from `UDBQueryResult<T>.GetResults()` with direct assignment fails.

âŒ **WRONG:**
```enforce
void OnQueryComplete(int cid, int status, string oid, UDBQueryResult<MyData> result) {
    array<autoptr MyData> items = result.GetResults();  // Doesn't work!
}
```

âœ… **CORRECT:**
```enforce
void OnQueryComplete(int cid, int status, string oid, UDBQueryResult<MyData> result) {
    if (status == UF_SUCCESS) {
        array<autoptr MyData> items;
        Class.CastTo(items, result.GetResults());  // Always use CastTo!
        // Now items is populated
    }
}
```

### âš ï¸ UDBQueryResult<T> Requires Typedef

**Problem:** Using `UDBQueryResult<T>` directly in callback signatures causes "Undefined function" errors.

âŒ **WRONG:**
```enforce
void OnQuery(int cid, int status, string oid, UDBQueryResult<MyClass> result) {
    // ERROR: result.GetResults() shows "Undefined function"
}
```

âœ… **CORRECT - Create typedef in 3_Game:**
```enforce
// In scripts/3_Game/TypeDefs.c (or similar)
typedef UDBQueryResult<MyClass> UDBQueryResultMyClass;

// In callback - use typedef
void OnQuery(int cid, int status, string oid, UDBQueryResultMyClass result) {
    array<autoptr MyClass> items;
    Class.CastTo(items, result.GetResults());  // Now works!
}
```

**Why:** Enforce Script's compiler cannot resolve generic templates in callback parameters without typedef. The typedef provides early type resolution in the 3_Game module layer.

### âš ï¸ Boolean Values Are Stored as Integers

**Problem:** DayZ JSON serialization saves booleans as `0`/`1`, not `true`/`false`.

âŒ **WRONG - MongoDB query:**
```enforce
UDBQuery query = new UDBQuery("{ \"isActive\": { \"$ne\": true } }");
```

âœ… **CORRECT - Use integers:**
```enforce
UDBQuery query = new UDBQuery("{ \"isActive\": { \"$ne\": 1 } }");
```

**Affects:**
- All MongoDB queries with boolean fields
- Query filter comparisons
- Update operations on boolean properties

### âš ï¸ Static Functions Cannot Be Used as Callbacks

**Problem:** Callback functions must be instance methods, not static functions. The framework needs an object instance to call the method on.

âŒ **WRONG - Static method:**
```enforce
class MyManager {
    static void OnDataLoaded(int cid, int status, string oid, string data) {
        // This will NOT work!
    }
    
    void LoadData() {
        // ERROR: Cannot use static method as callback
        U().db().Load("MyMod", "key", MyManager, "OnDataLoaded");
    }
}
```

âœ… **CORRECT - Instance method:**
```enforce
class MyManager {
    void OnDataLoaded(int cid, int status, string oid, string data) {
        // Instance method - works!
        Print("Data loaded: " + data);
    }
    
    void LoadData() {
        // Pass 'this' for the instance
        U().db().Load("MyMod", "key", this, "OnDataLoaded");
    }
}
```

**Why:** Universal Framework callbacks use the pattern `callbackInstance.Call(callbackFunction, params)`. Static functions don't belong to an instance, so they cannot be invoked this way.

**Workaround for utility callbacks:**
```enforce
// Create a singleton instance instead of using static
class DataLoadHelper {
    static autoptr DataLoadHelper s_Instance = new DataLoadHelper();
    
    void OnLoaded(int cid, int status, string oid, string data) {
        // Handle callback
    }
}

// Use the singleton instance
DataLoadHelper.s_Instance.LoadSomething();
```

---

## Best Practices

### 1. Always Check Status Before Using Data

```enforce
void OnLoaded(int cid, int status, string oid, MyData data) {
    // BAD - data could be null!
    Print(data.Name);
    
    // GOOD - check first
    if (status == UF_SUCCESS && data) {
        Print(data.Name);
    }
}
```

### 2. Track and Cancel Pending Calls

```enforce
class MyClass {
    protected autoptr array<int> m_PendingCalls;
    
    void MyClass() {
        m_PendingCalls = new array<int>;
    }
    
    void MakeRequest() {
        int cid = U().db().Load("Mod", "id", this, "OnLoaded");
        m_PendingCalls.Insert(cid);
    }
    
    void OnLoaded(int cid, int status, string oid, string data) {
        m_PendingCalls.RemoveItem(cid);
        // Process...
    }
    
    void ~MyClass() {
        foreach (int cid : m_PendingCalls) {
            U().RequestCallCancel(cid);
        }
    }
}
```

### 3. Use Typed Callbacks for Complex Data

```enforce
// Instead of parsing JSON manually...
void OnLoaded(int cid, int status, string oid, string json) {
    MyData data;
    if (!UJSONHandler<MyData>.FromString(json, data)) {
        UFLog.Err("Parse failed");
        return;
    }
}

// Use a typed handler - parsing is automatic
autoptr UDBHandler<MyData> m_Handler = new UDBHandler<MyData>("MyMod", OBJECT_DB);
m_Handler.Load("id", this, "OnLoaded");

void OnLoaded(int cid, int status, string oid, MyData data) {
    // data is already parsed!
}
```

### 4. Handle All Relevant Status Codes

```enforce
void OnLoaded(int cid, int status, string oid, MyData data) {
    switch (status) {
        case UF_SUCCESS:
            UseData(data);
            break;
        case UF_EMPTY:
            CreateDefault();
            break;
        case UF_TIMEOUT:
            ScheduleRetry();
            break;
        case UF_UNAUTHORIZED:
            // Token expired - framework handles refresh
            UFLog.Info("Auth refresh in progress");
            break;
        default:
            UFLog.Err("Unexpected status: " + status);
            break;
    }
}
```

### 5. Don't Block Waiting for Callbacks

```enforce
// BAD - This will never work!
void LoadAndUse() {
    U().db().Load("Mod", "id", this, "OnLoaded");
    // Data is NOT available here - callback hasn't fired yet!
    UseData(m_Data);  // WRONG
}

// GOOD - Continue in callback
void LoadAndUse() {
    U().db().Load("Mod", "id", this, "OnLoaded");
}

void OnLoaded(int cid, int status, string oid, MyData data) {
    if (status == UF_SUCCESS) {
        UseData(data);  // Correct - data is now available
    }
}
```

---

## Complete Example

```enforce
class PlayerProfile {
    string GUID;
    string Name;
    int Level = 1;
    float PlayTime = 0;
    ref array<string> Achievements;
    
    void PlayerProfile() {
        Achievements = new array<string>;
    }
}

class PlayerProfileManager {
    protected autoptr UDBHandler<PlayerProfile> m_DB;
    protected ref map<string, autoptr PlayerProfile> m_Profiles;
    protected ref map<string, int> m_PendingLoads;  // GUID -> CID
    
    void PlayerProfileManager() {
        m_DB = new UDBHandler<PlayerProfile>("MyMod", PLAYER_DB);
        m_Profiles = new map<string, autoptr PlayerProfile>;
        m_PendingLoads = new map<string, int>;
    }
    
    void ~PlayerProfileManager() {
        // Cancel all pending loads
        foreach (string guid, int cid : m_PendingLoads) {
            m_DB.Cancel(cid);
        }
    }
    
    void LoadProfile(string guid) {
        // Don't load if already pending
        if (m_PendingLoads.Contains(guid)) return;
        
        // Don't load if already cached
        if (m_Profiles.Contains(guid)) return;
        
        int cid = m_DB.Load(guid, this, "OnProfileLoaded");
        if (cid != -1) {
            m_PendingLoads.Insert(guid, cid);
        }
    }
    
    void OnProfileLoaded(int cid, int status, string guid, PlayerProfile profile) {
        // Remove from pending
        m_PendingLoads.Remove(guid);
        
        if (status == UF_SUCCESS && profile) {
            m_Profiles.Insert(guid, profile);
            UFLog.Info("Loaded profile: " + profile.Name);
        } else if (status == UF_EMPTY) {
            // Create new profile
            profile = new PlayerProfile();
            profile.GUID = guid;
            profile.Name = "New Player";
            m_Profiles.Insert(guid, profile);
            SaveProfile(guid);
        } else {
            UFLog.Err("Failed to load profile: " + guid + " status: " + status);
        }
    }
    
    void SaveProfile(string guid) {
        if (!m_Profiles.Contains(guid)) return;
        
        m_DB.Save(guid, m_Profiles.Get(guid));
    }
    
    PlayerProfile GetProfile(string guid) {
        if (m_Profiles.Contains(guid)) {
            return m_Profiles.Get(guid);
        }
        return NULL;
    }
}
```

---

## Quick Reference

### Callback Base Classes

| Class | Description |
|-------|-------------|
| `UFCallbackBase` | Base class for custom callbacks |
| `UFCallback<T>` | Typed callback - auto-parses JSON to type T |
| `UFCallbackLoader<T>` | Loads JSON into existing object instance |
| `UJSONCallback` | Returns raw JSON string |
| `UNestedCallBack` | Internal REST-to-callback bridge |

### Key Methods

| Method | Description |
|--------|-------------|
| `U().RequestCallCancel(cid)` | Cancel a pending callback |
| `U().IsCallCanceled(cid)` | Check if call was cancelled |
| `handler.Cancel(cid)` | Cancel via UDBHandler |

### Status Code Summary

| Code | Constant | Meaning |
|------|----------|---------|
| 200 | `UF_SUCCESS` | Operation successful |
| 204 | `UF_EMPTY` | No data found |
| 400 | `UF_CLIENTERROR` | Client-side error |
| 401 | `UF_UNAUTHORIZED` | Auth failed |
| 404 | `UF_NOTFOUND` | Resource not found |
| 406 | `UF_JSONERROR` | JSON parsing failed |
| 408 | `UF_TIMEOUT` | Request timed out |
| 418 | `UF_ERROR` | General error |
| 424 | `UF_NOTSETUP` | Service not set up (Discord) |

## Best Practices

### Always Check Status
Never write code that assumes `status == UF_SUCCESS`.
*   **Must handle:** `UF_EMPTY` (database record not found -> create new default), `UF_TIMEOUT` (server busy/lag), `UF_JSONERROR` (corrupt data).

### Use Typed Callbacks
Avoid manual `JsonFileLoader` parsing. Use `UFCallback<MyClass>`.
*   **Why?** It handles the JSON deserialization on a separate thread (effectively) or cleaner logic, and you get a strongly typed object in your function.

### Cancellation
If you trigger a db load when a player opens a menu, and they close the menu 0.1s later, use `Cancel(cid)` to prevent the callback from trying to update a GUI widget that no longer exists (which would crash the game).

## Common Use Cases

### Chained Loading
Load Player Config -> Then Load Clan Data -> Then Load Global Settings.
*   **Pattern:** Fire the next `Load` call inside the `OnSuccess` of the previous callback.

### UI Updates
Show a "Loading..." spinner.
1. Show Spinner.
2. Call `Load(..., "OnFinished")`.
3. Inside `OnFinished`: Hide Spinner, Fill Data.

## Tags
`callbacks`, `async`, `rest-api`, `status-codes`, `patterns`, `events`, `response-handling`, `how-to`, `reference`, `doc-usage`, `modder`

