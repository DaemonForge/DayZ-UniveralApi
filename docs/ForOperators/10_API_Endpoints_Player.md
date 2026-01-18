# API Endpoints Reference - Player Database

This document covers all REST API endpoints for Player database operations.

## Overview

The Player database stores per-player data organized by mod namespace. Each player is identified by their Steam GUID (17-digit Steam ID).

**Base URL Path**: `/Player`

**MongoDB Collection**: `Players`

**Authentication**: Server auth required for write operations. Player or server auth for read operations.

---

## Data Structure

Players are stored with this structure:

```json
{
  "_id": "ObjectId(...)",
  "GUID": "76561198012345678",
  "DiscordId": "123456789012345678",
  "ModA": {
    "field1": "value1",
    "field2": 123
  },
  "ModB": {
    "balance": 5000,
    "reputation": 75
  },
  "Public.ModA": {
    "publicField": "visible to all"
  }
}
```

**Key Points**:
- Each player has one document in the `Players` collection
- Each mod stores its data under its mod name as a key
- Public data uses the `Public.ModName` prefix
- Discord linking adds `DiscordId` to the player document

---

## Endpoints

### POST /Player/Load/:GUID/:mod

Load a player's mod-specific data.

**Authentication**: Player or Server auth

**URL Parameters**:
| Parameter | Type | Description |
|-----------|------|-------------|
| `GUID` | string | Player's 17-digit Steam ID |
| `mod` | string | Mod name/namespace |

**Request Body**: Empty or ignored

**Response Codes**:
| Code | Meaning |
|------|---------|
| 200 | Player data found and returned |
| 404 | Player or mod data not found |
| 500 | Server error |

**Response Body** (200):
```json
{
  "balance": 5000,
  "reputation": 75,
  "inventory": ["item1", "item2"]
}
```

Returns only the data stored under the specified mod key.

---

### POST /Player/Save/:GUID/:mod

Save a player's mod-specific data. Creates player record if it doesn't exist.

**Authentication**: Server auth only

**URL Parameters**:
| Parameter | Type | Description |
|-----------|------|-------------|
| `GUID` | string | Player's 17-digit Steam ID |
| `mod` | string | Mod name/namespace |

**Request Body**:
```json
{
  "balance": 5000,
  "reputation": 75,
  "inventory": ["item1", "item2"]
}
```

**Response Codes**:
| Code | Meaning |
|------|---------|
| 200 | Player data saved successfully |
| 500 | Server error |

**Response Body**: Returns the saved data or operation result.

---

### POST /Player/Update/:GUID/:mod

Update a specific field in a player's mod data.

**Authentication**: Server auth only

**URL Parameters**:
| Parameter | Type | Description |
|-----------|------|-------------|
| `GUID` | string | Player's Steam ID |
| `mod` | string | Mod name/namespace |

**Request Body**:
```json
{
  "Element": "balance",
  "Operation": "set",
  "Value": 7500
}
```

**Supported Operations**:
| Operation | Description |
|-----------|-------------|
| `set` | Set field to the specified value |
| `unset` | Remove the field |
| `push` | Add value to an array field |
| `pull` | Remove value from an array |
| `pullAll` | Remove all matching values from array |
| `mul` | Multiply numeric field by value |
| `rename` | Rename the field |

**Response Body**:
```json
{
  "Status": "Success",
  "Element": "balance",
  "GUID": "76561198012345678",
  "Mod": "EconomyMod"
}
```

---

### POST /Player/Transaction/:GUID/:mod

Perform an atomic increment/decrement on a numeric field.

**Authentication**: Server auth only

**URL Parameters**:
| Parameter | Type | Description |
|-----------|------|-------------|
| `GUID` | string | Player's Steam ID |
| `mod` | string | Mod name/namespace |

**Request Body**:
```json
{
  "Element": "balance",
  "Value": -500
}
```

With validation:
```json
{
  "Element": "balance",
  "Value": -500,
  "Min": 0,
  "Max": 1000000
}
```

**Response Body**:
```json
{
  "Status": "Success",
  "ID": "76561198012345678",
  "Mod": "EconomyMod",
  "Value": 4500,
  "Element": "balance"
}
```

---

## Public Endpoints

Public endpoints allow storing data that can be accessed without authentication. Useful for leaderboards, public profiles, etc.

### POST /Player/PublicLoad/:GUID/:mod

Load a player's public mod data. No authentication required.

**URL Parameters**:
| Parameter | Type | Description |
|-----------|------|-------------|
| `GUID` | string | Player's Steam ID |
| `mod` | string | Mod name (will be prefixed with `Public.`) |

**Response**: Returns data stored under `Public.ModName`.

---

### POST /Player/PublicSave/:GUID/:mod

Save a player's public mod data.

**Authentication**: Server auth only

**URL Parameters**:
| Parameter | Type | Description |
|-----------|------|-------------|
| `GUID` | string | Player's Steam ID |
| `mod` | string | Mod name (will be prefixed with `Public.`) |

**Request Body**: The data to save as the player's public data for this mod.

---

## Query Endpoint

### POST /Player/Query/:mod

Execute a MongoDB query against the Players collection.

**Authentication**: Server auth only

**Rate Limit**: Uses `RequestLimitQuery` (default 400 per 10 seconds)

**URL Parameters**:
| Parameter | Type | Description |
|-----------|------|-------------|
| `mod` | string | Mod name (used as the return column) |

**Request Body**:
```json
{
  "Query": "{\"EconomyMod.balance\": {\"$gt\": 10000}}",
  "OrderBy": "{\"EconomyMod.balance\": -1}",
  "MaxResults": 10,
  "FixQuery": 1,
  "ReturnObject": ""
}
```

**Fields**:
| Field | Type | Description |
|-------|------|-------------|
| `Query` | string | JSON-encoded MongoDB query |
| `OrderBy` | string | JSON-encoded sort specification |
| `MaxResults` | number | (Optional) Limit results |
| `FixQuery` | number | If 1, prefixes fields with mod name |
| `ReturnObject` | string | (Optional) Specific sub-field to return |

**FixQuery Behavior**:
When `FixQuery: 1`, your query fields are automatically prefixed with the mod name. So `balance` becomes `EconomyMod.balance`.

**Response Body**:
```json
{
  "Status": "Success",
  "Count": 10,
  "Results": [
    { "balance": 50000, "reputation": 100 },
    { "balance": 45000, "reputation": 85 }
  ]
}
```

**Example Query - Top 10 Players by Balance**:
```json
{
  "Query": "{}",
  "OrderBy": "{\"balance\": -1}",
  "MaxResults": 10,
  "FixQuery": 1
}
```

---

### POST /Player/Delete/:GUID/:mod

Delete a mod's data from a player document. **Note**: This removes only the mod's data field, not the entire player record.

**Authentication**: Server auth only

**URL Parameters**:
| Parameter | Type | Description |
|-----------|------|-------------|
| `GUID` | string | Player's 17-digit Steam ID |
| `mod` | string | Mod name/namespace to delete |

**Request Body**: Empty or ignored (can send `{}`)

**Response Codes**:
| Code | Meaning |
|------|------------|
| 200 | Mod data deleted successfully |
| 404 | Player or mod data not found |
| 500 | Server error |

**Response Body** (200):
```json
{
  "success": true,
  "deleted": true,
  "modifiedCount": 1
}
```

**Response Body** (404):
```json
{
  "error": "Player or mod data not found",
  "deleted": false
}
```

**Important Notes**:
- Only the specified mod's data is removed from the player document
- The player record itself remains in the database
- Other mods' data on the same player is not affected
- If the player has no other mod data, the player document remains with just the GUID field

**Example**:
```
POST /Player/Delete/76561198012345678/Economy
Auth-Key: your-server-auth-token
Content-Type: application/json

{}
```

**Before Delete**:
```json
{
  "GUID": "76561198012345678",
  "Economy": { "balance": 5000 },
  "Reputation": { "score": 100 }
}
```

**After Delete** (Economy mod data removed):
```json
{
  "GUID": "76561198012345678",
  "Reputation": { "score": 100 }
}
```

**Mod Usage Example (Enforce Script)**:
```cpp
class MyDeleteCallback extends UFCallbackBase {
    override void OnSuccess(int cid, int status, string oid, string data) {
        Print("Player mod data deleted: " + oid);
    }
    
    override void OnError(int cid, int ErrorCode, string oid, string error) {
        Print("Delete failed: " + error);
    }
}

// Delete player mod data with callback class
U().db(PLAYER_DB).Delete("Economy", playerGUID, new MyDeleteCallback());

// Or with instance/function callback
void OnPlayerDataDeleted(int cid, int status, string oid, string data) {
    Print("Player data deleted successfully");
}

U().db(PLAYER_DB).Delete("Economy", playerGUID, this, "OnPlayerDataDeleted");
```

---

## GUID Normalization

The service automatically normalizes GUIDs:
- Leading zeros are preserved
- Non-numeric characters are stripped
- Steam ID format (76561198...) is expected

**Valid GUID formats**:
- `76561198012345678` (standard Steam ID64)
- `12345678901234567` (17 digits)

---

## Indexes

The service automatically creates indexes on the Players collection:
- `GUID` (unique)
- `DiscordId` (for Discord linking)

These ensure fast lookups by player ID or Discord ID.

---

## Common Use Cases

### Economy System
```json
// Load player balance
POST /Player/Load/76561198012345678/Economy

// Deduct balance safely
POST /Player/Transaction/76561198012345678/Economy
{
  "Element": "balance",
  "Value": -500,
  "Min": 0
}
```

### Reputation System
```json
// Add reputation
POST /Player/Transaction/76561198012345678/Reputation
{
  "Element": "score",
  "Value": 10,
  "Max": 1000
}
```

### Inventory Management
```json
// Add item to inventory array
POST /Player/Update/76561198012345678/Inventory
{
  "Element": "items",
  "Operation": "push",
  "Value": "rare_weapon_001"
}

// Remove item from inventory
POST /Player/Update/76561198012345678/Inventory
{
  "Element": "items",
  "Operation": "pull",
  "Value": "rare_weapon_001"
}
```

### Leaderboard Query
```json
POST /Player/Query/Economy
{
  "Query": "{}",
  "OrderBy": "{\"balance\": -1}",
  "MaxResults": 100,
  "FixQuery": 1
}
```

---

## Error Handling

**Error Response Format**:
```json
{
  "error": "Error description"
}
```

Or for transaction operations:
```json
{
  "Status": "Error",
  "ID": "76561198012345678",
  "Mod": "Economy",
  "Error": "Invalid transaction payload"
}
```

**Common Errors**:
| Error | Cause |
|-------|-------|
| `Player or mod data not found` | GUID doesn't exist or has no data for this mod |
| `Invalid Auth` | Authentication failed |
| `Invalid transaction payload` | Missing `Element` or `Value` in transaction request |
| `Invalid update payload` | Missing required fields in update request |

## Tags
`operators`, `api`, `endpoints`, `players`, `database`, `reference`, `doc-usage`
