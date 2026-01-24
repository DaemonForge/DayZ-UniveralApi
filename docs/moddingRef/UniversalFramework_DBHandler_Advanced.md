# Universal Framework - Database Handler (Advanced)

## Overview

Advanced database operations beyond basic Save/Load. This covers query building, atomic transactions, partial updates, pagination, and the underlying query classes for complex data retrieval patterns.

---

## Method Signatures

All advanced `UDBHandlerBase` methods with exact parameter types:

```enforce
// Query for multiple documents
// Returns: int callId (-1 on error)
int Query(UDBQueryBase query, Class cbInstance, string cbFunction);
int Query(string query, Class cbInstance, string cbFunction);

// Atomic transaction (increment/decrement numeric field)
// Returns: int callId
int Transaction(string oid, string element, float value);
int Transaction(string oid, string element, float value, Class cbInstance, string cbFunction);
int Transaction(string oid, string element, float value, float min, float max, Class cbInstance, string cbFunction);

// Fire-and-forget increment
int Increment(string oid, string element, float value = 1);

// Update specific field with operation
// Returns: int callId
int Update(string oid, string element, string value, string operation = UpdateOpts.SET);
int Update(string oid, string element, string value, string operation, Class cbInstance, string cbFunction);

// Update all documents matching query
int QueryUpdate(UDBQueryBase query, string element, string value, string operation = UpdateOpts.SET);
int QueryUpdate(UDBQueryBase query, string element, string value, string operation, Class cbInstance, string cbFunction);
```

### Callback Signatures

```enforce
// Query callback
void OnQuery(int cid, int status, string oid, UDBQueryResult<T> results);

// Transaction callback  
void OnTransaction(int cid, int status, string oid, UDBTransactionResponse resp);

// Update callback
void OnUpdate(int cid, int status, string oid, string response);

// QueryUpdate callback
void OnQueryUpdate(int cid, int status, string oid, UDBQueryUpdateResponse resp);
```

### Parameter Reference

| Parameter | Type | Description |
|-----------|------|-------------|
| `oid` | `string` | Object ID - unique identifier for the record |
| `element` | `string` | Field name (supports dot notation: `"Stats.Kills"`) |
| `value` | `float`/`string` | Value for transaction or update |
| `min` / `max` | `float` | Bounds for transaction (clamp result) |
| `operation` | `string` | Update operation constant (see UpdateOpts) |
| `query` | `UDBQueryBase` | Query object or JSON string |

---

## Query Classes

### UDBQuery

Standard query class for database searches:

```enforce
class UDBQuery extends UDBQueryBase {
    string Query;        // MongoDB query JSON
    string OrderBy;      // Sort order JSON
    bool FixQuery;       // Auto-fix query structure (default: true)
    int MaxResults;      // Limit results (-1 = no limit)
    string ReturnObject; // Return specific field only
    
    void UDBQuery(string query = "{}", string orderBy = "{}", 
                  bool fixQuery = true, int maxResults = -1, 
                  string returnObject = "");
}


## Best Practices

### Use Transactions for Economy
When handling money or resources that can be modified by multiple sources simultaneously (e.g., spending money while receiving a passive income check), **ALWAYS** use `Transaction()`.
*   **Why?** `Save()` overwrites the whole document. If two processes `Load()` -> modify -> `Save()` at the same time, one update will be lost. `Transaction()` is atomic on the database server.

### Queries must be Efficient
Avoid querying with `{}` (empty query) unless you really want ALL records. Always try to filter by at least one indexed field if possible (though you don't control indexes here, reducing result set size is critical).

### Partial Updates
Use `Update()` when you only want to change one field (like "LastLoginTime") without loading the entire 50KB player profile object first.

## Common Use Cases

### Leaderboards
Use `Query` with `orderBy` to find top players.
*   **Query**: `"{}"` (All stats)
*   **OrderBy**: `"{ \"Kills\": -1 }"` (Descending Kills)
*   **Limit**: `10`

### Offline Base Maintenance
Write a script that runs every restart, queries all base objects, and runs a `Update` to decrement their "Health" or "Upkeep" variable without actually spawning them in the world.

### Faction Renaming
Use `QueryUpdate` to find all members of "Clan A" and update their `ClanName` field to "Clan B" in one operation.

## Tags
`database`, `query`, `transaction`, `atomic`, `updates`, `leaderboards`, `economy`, `queryupdate`, `how-to`, `reference`, `doc-usage`, `modder`


### UDBQueryResult<T>

Query response wrapper:

```enforce
class UDBQueryResult<T> extends StatusObject {
    array<autoptr T> GetResults();  // Get typed results array
}
```

---

## Queries

Search for documents matching MongoDB query criteria.

### Basic Query

```enforce
// Find all players with level >= 10
UDBQuery query = new UDBQuery("{ \"Level\": { \"$gte\": 10 } }");
g_PlayerHandler.Query(query, this, "OnQueryResult");

void OnQueryResult(int cid, int status, string oid, UDBQueryResult<MyPlayerData> result) {
    if (status == UF_SUCCESS && result) {
        array<autoptr MyPlayerData> players = result.GetResults();
        Print("Found " + players.Count() + " players");
    }
}
```

### Query Operators

| Operator | Usage | Description |
|----------|-------|-------------|
| `$eq` | `{ "field": { "$eq": value } }` | Equals |
| `$ne` | `{ "field": { "$ne": value } }` | Not equals |
| `$gt` | `{ "field": { "$gt": 5 } }` | Greater than |
| `$gte` | `{ "field": { "$gte": 5 } }` | Greater or equal |
| `$lt` | `{ "field": { "$lt": 10 } }` | Less than |
| `$lte` | `{ "field": { "$lte": 10 } }` | Less or equal |
| `$in` | `{ "field": { "$in": [1,2,3] } }` | In array |
| `$nin` | `{ "field": { "$nin": [1,2] } }` | Not in array |
| `$exists` | `{ "field": { "$exists": true } }` | Field exists |
| `$regex` | `{ "field": { "$regex": "pattern" } }` | Regex match |

### Query Examples

```enforce
// Exact match
"{ \"Faction\": \"survivor\" }"

// Multiple conditions (AND)
"{ \"Level\": { \"$gte\": 5 }, \"IsOnline\": true }"

// Nested object field (dot notation)
"{ \"Stats.Kills\": { \"$gt\": 10 } }"

// Array contains value
"{ \"Achievements\": \"first_blood\" }"

// Array contains any of values
"{ \"Achievements\": { \"$in\": [\"first_blood\", \"veteran\"] } }"

// Field exists check
"{ \"PremiumExpiry\": { \"$exists\": true } }"

// Regex pattern
"{ \"Nickname\": { \"$regex\": \"^Admin\" } }"
```

## Transactions

Atomic increment/decrement operations on numeric fields. The operation is atomic on the database - no race conditions.

### Basic Transaction

```enforce
// Add 100 to Balance field
g_PlayerHandler.Transaction("player123", "Balance", 100, this, "OnTransaction");

void OnTransaction(int cid, int status, string oid, UDBTransactionResponse resp) {
    if (status == UF_SUCCESS) {
        Print("Transaction successful for element: " + resp.Element);
    }
}
```

### Transaction with Bounds

Prevent value from going below/above limits:

```enforce
// Remove 50, but don't go below 0 or above 10000
g_PlayerHandler.Transaction("player123", "Balance", -50, 0, 10000, this, "OnTransaction");
```

### Increment Shorthand

Fire-and-forget increment (no callback):

```enforce
g_PlayerHandler.Increment("player123", "PlayTime", 60);  // Add 60 seconds
g_PlayerHandler.Increment("player123", "Deaths", 1);      // Increment deaths
```

### Nested Field Transaction

Use dot notation for nested fields:

```enforce
g_PlayerHandler.Transaction("player123", "Stats.Kills", 1, this, "OnKillAdded");
```

## Updates

Modify specific fields without replacing the entire document.

### Update Operations

| Operation | Constant | Description |
|-----------|----------|-------------|
| Set | `UpdateOpts.SET` | Set field value |
| Push | `UpdateOpts.PUSH` | Add to array |
| Pull | `UpdateOpts.PULL` | Remove from array |
| Unset | `UpdateOpts.UNSET` | Delete field |

### Set Field

```enforce
// Set string value (note: strings need quotes in JSON)
g_PlayerHandler.Update("player123", "Nickname", "\"NewName\"", UpdateOpts.SET);

// Set number
g_PlayerHandler.Update("player123", "Level", "10", UpdateOpts.SET);

// Set boolean
g_PlayerHandler.Update("player123", "IsVIP", "true", UpdateOpts.SET);

// Set nested field
g_PlayerHandler.Update("player123", "Stats.LastLogin", "\"2024-01-15\"", UpdateOpts.SET);
```

### Array Operations

```enforce
// Add to array
g_PlayerHandler.Update("player123", "Achievements", "\"sharpshooter\"", UpdateOpts.PUSH);

// Remove from array
g_PlayerHandler.Update("player123", "Achievements", "\"noob\"", UpdateOpts.PULL);

// Add object to array (stringify the object)
string itemJson = "{\"id\": \"item1\", \"count\": 5}";
g_PlayerHandler.Update("player123", "Inventory", itemJson, UpdateOpts.PUSH);
```

### Remove Field

```enforce
g_PlayerHandler.Update("player123", "TempData", "", UpdateOpts.UNSET);
```

## Query Updates

Update all documents matching a query.

```enforce
// Set all bandits to "wanted" status
UDBQuery query = new UDBQuery("{ \"Faction\": \"bandit\", \"Kills\": { \"$gt\": 5 } }");
g_Handler.QueryUpdate(query, "Status", "\"wanted\"", UpdateOpts.SET, this, "OnQueryUpdate");

void OnQueryUpdate(int cid, int status, string oid, UDBQueryUpdateResponse resp) {
    if (status == UF_SUCCESS && resp) {
        Print("Updated " + resp.Count + " documents");
    }
}
```

## Advanced Patterns

### Upsert on Load

Create if not exists in a single operation:

```enforce
void LoadOrCreate(string playerId) {
    MyPlayerData defaultData = new MyPlayerData();
    defaultData.Level = 1;
    g_PlayerHandler.Load(playerId, this, "OnLoaded", defaultData);
}

void OnLoaded(int cid, int status, string oid, MyPlayerData data) {
    // data will never be null if default was provided
    m_Data = data;
}
```

### Conditional Save

Only save if field meets criteria:

```enforce
void SafeWithdraw(string playerId, int amount) {
    // First load current balance
    g_PlayerHandler.Load(playerId, this, "OnBalanceLoaded");
    m_PendingWithdraw = amount;
}

void OnBalanceLoaded(int cid, int status, string oid, MyPlayerData data) {
    if (status == UF_SUCCESS && data && data.Balance >= m_PendingWithdraw) {
        g_PlayerHandler.Transaction(oid, "Balance", -m_PendingWithdraw, 0, 999999, this, "OnWithdraw");
    } else {
        NotifyInsufficientFunds();
    }
}
```

### Batch Updates

Multiple field updates in sequence:

```enforce
void OnPlayerLevelUp(string playerId, int newLevel) {
    // Update multiple fields
    g_PlayerHandler.Update(playerId, "Level", newLevel.ToString(), UpdateOpts.SET);
    g_PlayerHandler.Update(playerId, "Experience", "0", UpdateOpts.SET);
    g_PlayerHandler.Update(playerId, "LevelUpHistory", "\"" + UUtil.GetTimestamp() + "\"", UpdateOpts.PUSH);
}
```

## MongoDB Document Structure

Documents stored in service:

```json
{
    "_id": "ObjectId",
    "ServerID": "server-123",
    "Mod": "MyMod",
    "OID": "player123",
    "Data": {
        "Level": 10,
        "Experience": 5000,
        "Stats": {
            "Kills": 25,
            "Deaths": 5
        },
        "Achievements": ["survivor", "killer"]
    },
    "LastUpdate": "2024-01-15T10:30:00Z"
}
```

**Note:** Query operators and dot notation reference fields inside `Data` object. The service automatically wraps/unwraps the `Data` field.
