# API Endpoints Reference - Object Database

This document covers all REST API endpoints for the Object database operations.

## Overview

The Object database stores mod-created objects such as:
- Base building components
- Storage containers
- Custom items
- Any persistent mod data with unique identifiers

**Base URL Path**: `/Object`

**MongoDB Collection**: `Objects`

**Authentication**: Server auth required for write operations. Player or server auth for read operations.

---

## Endpoints

### POST /Object/Load/:ObjectId/:mod

Load an object from the database. If the object doesn't exist and a request body is provided by a server, the object will be created.

**Authentication**: Player or Server auth

**URL Parameters**:
| Parameter | Type | Description |
|-----------|------|-------------|
| `ObjectId` | string | Unique identifier for the object, or `"NewObject"` to generate one |
| `mod` | string | Mod name/namespace for organizing data |

**Request Body** (optional):
```json
{
  "field1": "value1",
  "field2": 123,
  "nested": { "key": "value" }
}
```

If the object doesn't exist and a body is provided from a server request, a new object is created with this data.

**Response Codes**:
| Code | Meaning |
|------|---------|
| 200 | Object found and returned |
| 201 | New object created (server only) |
| 204 | Object not found, no data in request body |
| 500 | Server error |

**Response Body** (200/201):
```json
{
  "ObjectId": "abc123",
  "field1": "value1",
  "field2": 123
}
```

**Example**:
```
POST /Object/Load/storage_001/MyMod
Auth-Key: your-server-auth-token
Content-Type: application/json

{}
```

---

### POST /Object/Save/:ObjectId/:mod

Save an object to the database. Creates or updates (upserts) the object.

**Authentication**: Server auth only

**URL Parameters**:
| Parameter | Type | Description |
|-----------|------|-------------|
| `ObjectId` | string | Unique identifier, or `"NewObject"` to generate one |
| `mod` | string | Mod name/namespace |

**Request Body**:
```json
{
  "field1": "value1",
  "field2": 123,
  "customData": { ... }
}
```

**Response Codes**:
| Code | Meaning |
|------|---------|
| 201 | Object saved successfully |
| 203 | Save operation returned unexpected result |
| 500 | Server error |

**Response Body**:
Returns the saved object data with `ObjectId` field populated.

**Example**:
```
POST /Object/Save/storage_001/MyMod
Auth-Key: your-server-auth-token
Content-Type: application/json

{
  "contents": ["item1", "item2"],
  "owner": "player123"
}
```

---

### POST /Object/Update/:ObjectId/:mod

Update a specific field in an object using a designated operation.

**Authentication**: Server auth only

**URL Parameters**:
| Parameter | Type | Description |
|-----------|------|-------------|
| `ObjectId` | string | Object identifier |
| `mod` | string | Mod name/namespace |

**Request Body**:
```json
{
  "Element": "fieldName",
  "Operation": "set",
  "Value": "newValue"
}
```

**Supported Operations**:
| Operation | Description |
|-----------|-------------|
| `set` | Set field to the specified value (default) |
| `unset` | Remove the field from the document |
| `push` | Add value to an array field |
| `pull` | Remove value from an array field |
| `pullAll` | Remove all matching values from array |
| `mul` | Multiply numeric field by value |
| `rename` | Rename the field |

**Response Codes**:
| Code | Meaning |
|------|---------|
| 200 | Update successful |
| 203 | Object not found or update failed |
| 500 | Server error |

**Response Body**:
```json
{
  "Status": "Success",
  "Element": "fieldName",
  "Mod": "MyMod",
  "ID": "storage_001"
}
```

---

### POST /Object/Transaction/:ObjectId/:mod

Perform an atomic increment/decrement operation on a numeric field. Supports optional value clamping.

**Authentication**: Server auth only

**URL Parameters**:
| Parameter | Type | Description |
|-----------|------|-------------|
| `ObjectId` | string | Object identifier |
| `mod` | string | Mod name/namespace |

**Request Body**:
```json
{
  "Element": "count",
  "Value": 5
}
```

With validation (clamping):
```json
{
  "Element": "count",
  "Value": -10,
  "Min": 0,
  "Max": 100
}
```

**Fields**:
| Field | Type | Description |
|-------|------|-------------|
| `Element` | string | Field name to modify |
| `Value` | number | Amount to add (positive) or subtract (negative) |
| `Min` | number | (Optional) Minimum allowed result value |
| `Max` | number | (Optional) Maximum allowed result value |

When `Min` and `Max` are provided and are different, a validated transaction is performed that ensures the result stays within bounds.

**Response Body**:
```json
{
  "Status": "Success",
  "ID": "storage_001",
  "Mod": "MyMod",
  "Value": 15,
  "Element": "count"
}
```

---

## Query Endpoint

### POST /Object/Query/:mod

Execute a MongoDB query against the Objects collection.

**Authentication**: Server or Player auth (player auth requires objects to be in the Objects collection)

**Rate Limit**: Uses `RequestLimitQuery` (default 400 per 10 seconds)

**URL Parameters**:
| Parameter | Type | Description |
|-----------|------|-------------|
| `mod` | string | Mod name/namespace (added to query filter automatically) |

**Request Body**:
```json
{
  "Query": "{\"owner\": \"player123\"}",
  "OrderBy": "{\"created\": -1}",
  "MaxResults": 10,
  "FixQuery": 0,
  "ReturnObject": ""
}
```

**Fields**:
| Field | Type | Description |
|-------|------|-------------|
| `Query` | string | JSON-encoded MongoDB query filter |
| `OrderBy` | string | JSON-encoded sort specification (1=asc, -1=desc) |
| `MaxResults` | number | (Optional) Maximum number of results to return |
| `FixQuery` | number | If 1, prefixes query fields with `data.` for nested queries |
| `ReturnObject` | string | (Optional) Specific sub-field to return from each result |

**Important**: `Query` and `OrderBy` must be **JSON strings**, not objects.

**Response Body**:
```json
{
  "Status": "Success",
  "Count": 3,
  "Results": [
    { "contents": ["item1"], "owner": "player123" },
    { "contents": [], "owner": "player123" },
    { "contents": ["item2", "item3"], "owner": "player123" }
  ]
}
```

**Status Values**:
| Status | Meaning |
|--------|---------|
| `Success` | Query executed, results found |
| `Empty` | Query executed, no results found |
| `Error` | Query failed |

---

### POST /Object/Query/Update/:mod

Update multiple objects matching a query.

**Authentication**: Server auth, or Player auth with `AllowClientWrite: true`

**Request Body**:
```json
{
  "Query": {
    "Query": "{\"type\": \"container\"}",
    "OrderBy": "{}",
    "FixQuery": 0
  },
  "Element": "locked",
  "Operation": "set",
  "Value": true
}
```

**Response Body**:
```json
{
  "Status": "Success",
  "Element": "locked",
  "Mod": "MyMod",
  "Count": 5
}
```

---

### POST /Object/Delete/:ObjectId/:mod

Delete an object from the database.

**Authentication**: Server auth only

**URL Parameters**:
| Parameter | Type | Description |
|-----------|------|-------------|
| `ObjectId` | string | Unique identifier of the object to delete |
| `mod` | string | Mod name/namespace |

**Request Body**: Empty or ignored (can send `{}`)

**Response Codes**:
| Code | Meaning |
|------|---------|
| 200 | Object deleted successfully |
| 404 | Object not found |
| 500 | Server error |

**Response Body** (200):
```json
{
  "success": true,
  "deleted": true,
  "deletedCount": 1
}
```

**Response Body** (404):
```json
{
  "error": "Object not found",
  "deleted": false,
  "deletedCount": 0
}
```

**Example**:
```
POST /Object/Delete/storage_001/MyMod
Auth-Key: your-server-auth-token
Content-Type: application/json

{}
```

**Mod Usage Example (Enforce Script)**:
```cpp
class MyDeleteCallback extends UFCallbackBase {
    override void OnSuccess(int cid, int status, string oid, string data) {
        Print("Object deleted: " + oid);
    }
    
    override void OnError(int cid, int ErrorCode, string oid, string error) {
        Print("Delete failed: " + error);
    }
}

// Delete with callback class
U().db().Delete("MyMod", "storage_001", new MyDeleteCallback());

// Or with instance/function callback
void OnObjectDeleted(int cid, int status, string oid, string data) {
    Print("Object deleted successfully");
}

U().db().Delete("MyMod", "storage_001", this, "OnObjectDeleted");
```

---

## Data Structure in MongoDB

Objects are stored with this structure:

```json
{
  "_id": "ObjectId(...)",
  "ObjectId": "storage_001",
  "Mod": "MyMod",
  "data": {
    "field1": "value1",
    "field2": 123,
    "nested": { "key": "value" }
  }
}
```

**Notes**:
- The `Mod` field is automatically added to all objects
- Your data is stored inside the `data` field
- When using `FixQuery: 1`, your query fields are prefixed with `data.`
- The `ObjectId` field is separate from MongoDB's `_id`

---

## Error Handling

All endpoints return errors in this format:
```json
{
  "error": "Error description"
}
```

Or for transaction/update operations:
```json
{
  "Status": "Error",
  "Error": "Error description",
  "ID": "object_id",
  "Mod": "mod_name"
}
```

Common errors:
- `Invalid Auth` - Authentication failed
- `Database Write Error` - MongoDB write operation failed
- `Invalid JSON in Query` - Query string is not valid JSON

## Tags
`operators`, `api`, `endpoints`, `objects`, `database`, `reference`, `doc-usage`
