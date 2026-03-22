# Universal Framework - Common Patterns & Tips

## Overview
This document outlines standard patterns for error handling, data management, and code organization when using UFramework.

---

## 1. The Callback Pattern
All API calls are **asynchronous**. Never assume data is available immediately after a call.

### [X] WRONG: Blocking/Synchronous Thinking
```enforce
string data = UF().db().Load("MyMod", "id"); // ERROR: Load returns void or call ID
Print(data); // Will be empty/null
```

### [YES] RIGHT: Async Callback
```enforce
// 1. Initiate Request
UF().db().Load("MyMod", "id", this, "OnLoaded");

// 2. Handle Response Later
void OnLoaded(int cid, int status, string oid, string data)
{
    if (status != UF_SUCCESS)
    {
        Print("Failed to load: " + status);
        return;
    }
    
    Print("Data received: " + data);
}
```

---

## 2. Using Typed Wrappers (UDBHandler)
Manually parsing JSON strings is tedious. Use `UDBHandler<T>` to automatically serialize/deserialize classes.

### Step 1: Define Your Data Class
```enforce
class MyPlayerData
{
    int coins;
    int kills;
    string clanName;

    void MyPlayerData()
    {
        coins = 0;
        kills = 0;
        clanName = "";
    }
}
```

### Step 2: Use the Handler
```enforce
class MyPlayerDB extends UDBHandler<MyPlayerData>
{
    // Optional: Add helper methods specific to your logic
    void AddCoin(int amount)
    {
        if (m_Data) m_Data.coins += amount;
    }
}

## Tags
`modder`, `patterns`, `callbacks`, `async`, `database`, `best-practice`, `how-to`, `doc-usage`

// Usage
MyPlayerDB db = new MyPlayerDB("MyMod", "MyPlayerDB", PLAYER_DB); // ModName, CollectionName, Type

// Load
// PLAYER_DB keys must use player.GetIdentity().GetId(), never GetPlainId()
db.Load(playerGuid, this, "OnStatsLoaded");

// Save
db.Save(playerGuid); // Uses internal m_Data automatically
```

### [!] CRITICAL: Always Use Class.CastTo() in Callbacks

When receiving typed data from callbacks, **never use direct assignment**. Always use `Class.CastTo()`:

[X] **WRONG - Can crash:**
```enforce
void OnStatsLoaded(int cid, int status, string oid, MyPlayerData data) {
    m_PlayerData = data;  // DANGEROUS!
}
```

[YES] **CORRECT:**
```enforce
void OnStatsLoaded(int cid, int status, string oid, MyPlayerData data) {
    if (status == UF_SUCCESS) {
        Class.CastTo(m_PlayerData, data);  // Safe extraction
        if (m_PlayerData) {
            // Now safe to use
        }
    }
}
```

**For query results, also use Class.CastTo():**
```enforce
typedef UDBQueryResult<MyPlayerData> UDBQueryResultMyPlayerData;

void OnQuery(int cid, int status, string oid, UDBQueryResultMyPlayerData result) {
    if (status == UF_SUCCESS) {
        array<autoptr MyPlayerData> items;
        Class.CastTo(items, result.GetResults());  // Required!
    }
}
```

### [!] CRITICAL: Callbacks Must Be Instance Methods (Not Static)

**Static functions cannot be used as callbacks:**

[X] **WRONG:**
```enforce
class MyManager {
    static void OnLoaded(int cid, int status, string oid, string data) {
        // Won't work!
    }
}
```

[YES] **CORRECT:**
```enforce
class MyManager {
    void OnLoaded(int cid, int status, string oid, string data) {
        // Instance method - works!
    }
    
    void DoLoad() {
        UF().db().Load("MyMod", "key", this, "OnLoaded");  // Pass 'this'
    }
}
```

---

## 3. Server vs Client Context
DayZ scripts run on both client and server. UFramework handles authentication automatically, but your logic must respect the destination.

| Operation | Server | Client | Note |
|-----------|--------|--------|------|
| `OBJECT_DB.Save()` | [YES] OK | [NO] Fail | Only servers can write object/global data. |
| `PLAYER_DB.Load()` | [YES] Any | [YES] Self | Clients can only load their *own* data, and the key must be the player's GUID from `GetIdentity().GetId()`. |
| `Discord.AddRole()` | [YES] OK | [NO] Fail | Admin actions are server-only. |

Use `GetGame().IsServer()` to guard logic:

```enforce
void GiveReward()
{
    // Logic that changes DB should only run on server
    if (GetGame().IsServer())
    {
        UF().db().Transaction("MyMod", "key", "coins", 100);
    }
}
```

---

## 4. Error Handling
Always check the `status` parameter in callbacks.

| Status Code | Meaning | Action |
|-------------|---------|--------|
| `UF_SUCCESS` (200) | OK | Process data. |
| `UF_EMPTY` (204) | No Content | Database record doesn't exist yet. Create default? |
| `UF_UNAUTHORIZED` (401) | Auth Failed | Check API keys or player login. |
| `UF_ERROR` (500) | Server Error | Check UFService logs. |

### Example Retry Logic
```enforce
void OnLoad(int cid, int status, string oid, string data)
{
    if (status == UF_TIMEOUT)
    {
        // Simple retry
        UF().db().Load("MyMod", oid, this, "OnLoad"); 
        return;
    }
    // ... handle other statuses
}
```
