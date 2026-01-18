# API Endpoints Reference - Globals & Status

This document covers the Globals database endpoints and Status endpoint.

## Globals Overview

The Globals database stores server-wide mod data that isn't tied to specific players or objects. Useful for:
- Server-wide settings
- Economy totals
- Event states
- Shared counters

**Base URL Path**: `/Globals`

**MongoDB Collection**: `Globals`

---

## Globals Data Structure

Globals are stored with this structure:

```json
{
  "_id": "ObjectId(...)",
  "Mod": "EconomyMod",
  "Data": {
    "totalMoney": 5000000,
    "inflation": 1.05,
    "lastReset": "2025-01-10T00:00:00Z"
  }
}
```

Each mod has one global document identified by `Mod` name.

---

## Globals Endpoints

### POST /Globals/Load/:mod

Load global data for a mod. If data doesn't exist and a body is provided by a server, creates it.

**Authentication**: Player or Server auth

**URL Parameters**:
| Parameter | Type | Description |
|-----------|------|-------------|
| `mod` | string | Mod name/namespace |

**Request Body** (optional):
```json
{
  "defaultField": "defaultValue",
  "counter": 0
}
```

If provided by a server request and no global exists, this becomes the initial data.

**Response Codes**:
| Code | Meaning |
|------|---------|
| 200 | Global data found and returned |
| 203 | Data not found or creation failed |
| 500 | Server error |

**Response Body**:
```json
{
  "totalMoney": 5000000,
  "inflation": 1.05,
  "lastReset": "2025-01-10T00:00:00Z"
}
```

---

### POST /Globals/Save/:mod

Save global data for a mod. Creates or updates.

**Authentication**: Server auth only

**URL Parameters**:
| Parameter | Type | Description |
|-----------|------|-------------|
| `mod` | string | Mod name/namespace |

**Request Body**:
```json
{
  "totalMoney": 5000000,
  "inflation": 1.05,
  "lastReset": "2025-01-10T00:00:00Z"
}
```

**Response Codes**:
| Code | Meaning |
|------|---------|
| 200 | Update successful |
| 201 | New global created |
| 203 | Operation failed |
| 500 | Server error |

---

### POST /Globals/Update/:mod

Update a specific field in the global data.

**Authentication**: Server auth only

**Request Body**:
```json
{
  "Element": "inflation",
  "Operation": "set",
  "Value": 1.10
}
```

**Supported Operations**: `set`, `unset`, `push`, `pull`, `pullAll`, `mul`, `rename`

**Response Body**:
```json
{
  "Status": "Success",
  "Element": "inflation",
  "ID": "EconomyMod"
}
```

---

### POST /Globals/Transaction/:mod

Atomic increment/decrement on a global numeric field.

**Authentication**: Server auth only

**Request Body**:
```json
{
  "Element": "totalMoney",
  "Value": 10000
}
```

**Response Body**:
```json
{
  "Status": "Success",
  "ID": "EconomyMod",
  "Value": 5010000,
  "Element": "totalMoney"
}
```

---

## Status Endpoint

### GET /Status or POST /Status

Check the health and status of the UF Server Service.

**Authentication**: Optional (returns auth status in response)

**Rate Limit**: Uses `RequestLimitStatus` (default 100 per 10 seconds)

**Query Parameters**:
| Parameter | Type | Description |
|-----------|------|-------------|
| `noLog` | any | If present, suppresses logging of this status check |

**Response Body**:
```json
{
  "Status": "Success",
  "Error": "noerror",
  "Version": "2.0.0",
  "Discord": "Online",
  "OpenAI": "Online"
}
```

**Fields**:
| Field | Description |
|-------|-------------|
| `Status` | `Success` if database is writable, `Error` otherwise |
| `Error` | `noerror` if authenticated, `NoAuth` if not, or error description |
| `Version` | UF Server Service version |
| `Discord` | Discord bot status |
| `OpenAI` | OpenAI connection status |

**Discord Status Values**:
| Status | Meaning |
|--------|---------|
| `Online` | Bot is connected and ready |
| `Disabled` | Discord not configured |
| `Disconnected` | Bot was connected but disconnected |
| `Pending` | Bot is connecting |
| `Error` | Bot encountered an error |

**OpenAI Status Values**:
| Status | Meaning |
|--------|---------|
| `Online` | OpenAI API configured and ready |
| `Pending` | Not yet initialized |
| `Disabled` | OpenAI not configured |
| `Error` | Configuration or connection error |

**Health Check Behavior**:
The status endpoint writes a test record to MongoDB to verify database connectivity. This ensures:
- MongoDB is reachable
- Database is writable
- Connection credentials are valid

**Use Cases**:
- Load balancer health checks
- Monitoring systems (Prometheus, Grafana, etc.)
- DayZ server startup validation
- Debugging connection issues

**Example curl**:
```bash
# Basic health check
curl -X GET https://your-server:443/Status

# Authenticated check
curl -X POST https://your-server:443/Status \
  -H "Auth-Key: your-server-auth-token"

# Suppress logging
curl -X GET "https://your-server:443/Status?noLog=1"
```

---

## Common Use Cases

### Server-Wide Economy
```json
// Initialize economy
POST /Globals/Save/Economy
{
  "totalCirculation": 0,
  "bankReserve": 1000000,
  "interestRate": 0.02
}

// Add money to circulation
POST /Globals/Transaction/Economy
{
  "Element": "totalCirculation",
  "Value": 50000
}
```

### Event System
```json
// Save event state
POST /Globals/Save/Events
{
  "currentEvent": "winter_festival",
  "startTime": "2025-01-01T00:00:00Z",
  "endTime": "2025-01-15T00:00:00Z",
  "participants": 0
}

// Increment participants
POST /Globals/Transaction/Events
{
  "Element": "participants",
  "Value": 1
}
```

### Server Statistics
```json
// Initialize stats
POST /Globals/Save/ServerStats
{
  "totalPlaytime": 0,
  "uniquePlayers": 0,
  "zombieKills": 0
}

// Update stats atomically
POST /Globals/Transaction/ServerStats
{
  "Element": "zombieKills",
  "Value": 1
}
```

## Tags
`operators`, `api`, `endpoints`, `globals`, `status`, `reference`, `doc-usage`
