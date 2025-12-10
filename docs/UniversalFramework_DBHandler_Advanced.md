# Universal Framework - Database Handler (Advanced)

## Overview

Advanced database operations beyond basic Save/Load. This covers query building, atomic transactions, partial updates, pagination, and the underlying query classes for complex data retrieval patterns.

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

// Usage
UDBQuery query = new UDBQuery("{ \"Level\": { \"$gte\": 10 } }");
UDBQuery sorted = new UDBQuery("{ \"Active\": true }", "{ \"Score\": -1 }", true, 100);
```

### UDBQueryObject

Alternative constructor order for convenience:

```enforce
class UDBQueryObject extends UDBQueryBase {
    void UDBQueryObject(string query = "{}", string orderBy = "{}", 
                        int maxResults = -1, string returnObject = "");
}
```

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
