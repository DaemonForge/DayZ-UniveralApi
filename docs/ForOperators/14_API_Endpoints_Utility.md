# API Endpoints - Utility Endpoints

This document covers utility endpoints: Logger, Server Query, Random Numbers, Cryptocurrency, and Authentication.

## Logger Endpoints

Store log entries from DayZ servers and clients in the database.

**Base URL Path**: `/Logger`

**MongoDB Collection**: `Logs`

### POST /Logger/One/:ServerID

Log a single entry.

**Authentication**: Player or Server auth

**URL Parameters**:
| Parameter | Type | Description |
|-----------|------|-------------|
| `ServerID` | string | Server identifier |

**Request Body**:
```json
{
  "Level": "info",
  "Message": "Player connected",
  "Details": {
    "playerName": "Survivor",
    "location": [5000, 200, 8000]
  }
}
```

Any JSON structure is accepted. The service adds:
- `ServerId`: From URL parameter
- `LoggedDateTime`: Server timestamp
- `ClientId`: Hashed IP (SHA256, first 32 chars)
- `ClientType`: "Server" or "Client"

**Response Body**:
```json
{
  "Status": "Success",
  "Error": ""
}
```

### POST /Logger/Many/:ServerID

Log multiple entries at once.

**Authentication**: Player or Server auth

**URL Parameters**:
| Parameter | Type | Description |
|-----------|------|-------------|
| `ServerID` | string | Server identifier |

**Request Body**:
```json
[
  { "Level": "info", "Message": "Event 1" },
  { "Level": "debug", "Message": "Event 2" }
]
```

**Response Body**:
```json
{
  "Status": "Success",
  "Error": ""
}
```

---

## Server Query Endpoint

Query DayZ server status via Steam Query protocol.

**Base URL Path**: `/ServerQuery`

### POST /ServerQuery/Status/:IP/:Port

Query a DayZ server's status.

**Authentication**: Player or Server auth

**URL Parameters**:
| Parameter | Type | Description |
|-----------|------|-------------|
| `IP` | string | Server IP address |
| `Port` | string | Steam Query port (usually Game Port + 1 or same as Game Port depending on config) |

**Response Body** (Online):
```json
{
  "Status": "Online",
  "Error": "",
  "IP": "192.168.1.100",
  "GamePort": 2302,
  "QueryPort": 27016,
  "Name": "My DayZ Server",
  "ServerVersion": "1.25.156789",
  "Players": 45,
  "QueuePlayers": 3,
  "MaxPlayers": 60,
  "GameTime": "14:30",
  "GameMap": "chernarusplus",
  "Password": 0,
  "FirstPerson": 1
}
```

**Response Body** (Offline):
```json
{
  "Status": "offline",
  "Error": "Server is offline or wrong ip/query port",
  "IP": "192.168.1.100",
  "GamePort": 27016, 
  "QueryPort": 27016,
  "Name": "",
  "ServerVersion": "",
  "Players": 0,
  "QueuePlayers": 0,
  "MaxPlayers": 0,
  "GameTime": "",
  "GameMap": "",
  "Password": 0,
  "FirstPerson": 0
}
```

---

## Random Number Generator

Generate random numbers from ANU's Quantum Random Number Generator (QRNG) with JavaScript fallback.

**Base URL Path**: `/Random`

### POST /Random

Generate random integers.

**Authentication**: Player or Server auth

**Request Body**:
```json
{
  "Count": 100
}
```

**Fields**:
| Field | Type | Description |
|-------|------|-------------|
| `Count` | number | Number of integers to generate (1-4096). Default 4096. |

**Response Body**:
```json
{
  "Status": "Success",
  "Error": "",
  "Numbers": [1234567, -987654, 456789, ...]
}
```

**Number Range**: -2,147,483,647 to 2,147,483,647 (32-bit signed integer range)

---

## Cryptocurrency Endpoints

Convert cryptocurrency values or check prices.

**Base URL Path**: `/Crypto`

### POST /Crypto/Convert/:From/:To

Convert a specific value from one currency to another.

**Authentication**: Player or Server auth

**URL Parameters**:
| Parameter | Type | Description |
|-----------|------|-------------|
| `From` | string | Source Currency Symbol (e.g., BTC, ETH) |
| `To` | string | Target Currency Symbol (e.g., USD, EUR) |

**Request Body**:
```json
{
  "Value": 1.5
}
```

**Response Body**:
```json
{
  "Status": "Success",
  "Error": "",
  "Value": 12345.67 // The converted amount
}
```

### POST /Crypto/Price/:From/:To

Get the price of 1 unit of the source currency in the target currency.

**Authentication**: Player or Server auth

**Response Body**:
```json
{
  "Status": "Success",
  "Error": "",
  "Value": 50000.00 // Price of 1 From in To
}
```

### POST /Crypto/:From

Bulk convert 1 unit of the source currency to multiple target currencies.

**Authentication**: Player or Server auth

**URL Parameters**:
| Parameter | Type | Description |
|-----------|------|-------------|
| `From` | string | Source Currency Symbol |

**Request Body**:
```json
{
  "To": ["USD", "EUR", "GBP"]
}
```
*Note: You can also use "From" key in body which acts the same as To.*

**Response Body**:
```json
{
  "Status": "Success",
  "Error": "",
  "Values": {
      "USD": 50000.00,
      "EUR": 45000.00,
      "GBP": 40000.00
  }
}
```

---

## Authentication Endpoint

Get authentication tokens for player clients.

**Base URL Path**: `/GetAuth`

### POST /GetAuth/:GUID

Generate a player authentication token.

**Authentication**: Server auth only

**URL Parameters**:
| Parameter | Type | Description |
|-----------|------|-------------|
| `GUID` | string | Player's Steam ID |

**Response Body**:
```json
{
  "GUID": "76561198012345678",
  "AUTH": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9..."
}
```
*Note: If error, returns `AUTH: "ERROR"`.*

## Tags
`operators`, `api`, `endpoints`, `utility`, `logger`, `auth`, `reference`, `doc-usage`
