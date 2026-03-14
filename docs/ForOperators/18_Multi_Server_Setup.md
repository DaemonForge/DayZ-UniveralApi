# Multi-Server Setup Guide

This document covers running multiple DayZ servers with a single UF Server Service instance.

## Overview

One UF Server Service can handle multiple DayZ servers with:
- Separate authentication per server
- Shared or isolated data per your choice
- Centralized management
- Reduced resource requirements

---

## Architecture Options

### Option 1: Shared Data

All servers share the same data in MongoDB.

**Use Cases**:
- Cross-server economies
- Unified player progression
- Linked base building
- Shared bans/reputation

**Data Separation**: By mod name only
- Players on Server A and Server B see the same economy data
- Changes on one server reflect on all others

### Option 2: Isolated Data

Each server has its own namespace using different mod names.

**Use Cases**:
- Independent server clusters
- Different server types (PvP, PvE, RP)
- Separate leaderboards
- Server-specific economies

**Data Separation**: By mod name with server prefix
- `US1_Economy` vs `EU1_Economy`
- Each server's data is independent

### Option 3: Hybrid

Some data shared, some isolated.

**Use Cases**:
- Shared player identity but separate inventories
- Global bans with local reputation
- Shared Discord linking, separate game data

---

## Configuration

### Service Configuration

In `config.json`, configure multiple auth tokens:

```json
{
  "ServerAuth": [
    "token_for_server_us_east",
    "token_for_server_us_west",
    "token_for_server_eu"
  ],
  "ServerAuthLabels": [
    "US-East",
    "US-West",
    "EU"
  ]
}
```

**ServerAuth**: Array of authentication tokens
**ServerAuthLabels**: Descriptive labels (for logging/management)

### Per-Server Mod Configuration

Each DayZ server's `$profile/UF/UFramework.json`:

**Server US-East**:
```json
{
  "ServerURL": "https://your-uf-service.com:443/",
  "ServerID": "US-East-1",
  "ServerAuth": "token_for_server_us_east",
  "PromptDiscordOnConnect": true,
  "EnableBuiltinLogging": true
}
```

**Server US-West**:
```json
{
  "ServerURL": "https://your-uf-service.com:443/",
  "ServerID": "US-West-1",
  "ServerAuth": "token_for_server_us_west",
  "PromptDiscordOnConnect": true,
  "EnableBuiltinLogging": true
}
```

**Server EU**:
```json
{
  "ServerURL": "https://your-uf-service.com:443/",
  "ServerID": "EU-1",
  "ServerAuth": "token_for_server_eu",
  "PromptDiscordOnConnect": true,
  "EnableBuiltinLogging": true
}
```

---

## Data Isolation Strategies

### Strategy 1: Mod Name Prefixing

Configure mods to use server-specific prefixes:

```cpp
// On US-East server
string modName = "US1_Economy";
U().db().Save(modName, playerId, data, this, "OnSaved");

// On EU server
string modName = "EU1_Economy";
U().db().Save(modName, playerId, data, this, "OnSaved");
```

**MongoDB Result**:
- `Objects` collection has documents with `Mod: "US1_Economy"` and `Mod: "EU1_Economy"`
- Players collection has `US1_Economy` and `EU1_Economy` keys

### Strategy 2: Separate Databases

Use different MongoDB databases per server cluster:

**config.json for US cluster**:
```json
{
  "DB": "DayZ_US"
}
```

**config.json for EU cluster**:
```json
{
  "DB": "DayZ_EU"
}
```

**Note**: This requires separate UF Service instances.

### Strategy 3: ServerID in Data

Include ServerID in stored data for filtering:

```cpp
// Store with ServerID
string serverID = UFConfig().ServerID;
data.ServerID = serverID;
U().db().Save("Economy", playerId, data, this, "OnSaved");

// Query for specific server
string query = "{\"ServerID\": \"US-East-1\"}";
U().db().Query("Economy", query, "{}", this, "OnResults");
```

---

## Shared Features

### Shared Player Linking

Discord linking is inherently shared:
- Player links Discord once
- Linked status visible from all servers
- Roles managed from any server

### Shared Bans

Implement a global ban system:

```cpp
// Check ban on connect
void OnPlayerConnect(PlayerBase player) {
  string guid = player.GetIdentity().GetId();
    U().db().Load("GlobalBans", guid, this, "OnBanCheck");
}

void OnBanCheck(int cid, int status, string oid, string data) {
    if (status == 200) {
        // Player is banned
        KickPlayer(oid, "You are banned from all servers");
    }
}
```

### Cross-Server Communication

Use the Message Queue for cross-server messaging:

```cpp
// Server A writes event
U().Msg().Write("CrossServer", "Events", eventData, this, "OnWritten");

// All servers poll for events
U().Msg().Read("CrossServer", "Events", 10, this, "OnEvents");
```

---

## Load Balancing Considerations

### Rate Limits

With multiple servers, consider total request volume:

```json
{
  "RequestLimit": 1000,
  "RequestLimitQuery": 800,
  "RateLimitWhiteList": [
    "10.0.0.10",
    "10.0.0.11",
    "10.0.0.12"
  ]
}
```

Add all your DayZ servers to the whitelist to prevent rate limiting.

### Clustering

Enable clustering for multi-core utilization:

```json
{
  "cpuCount": 4
}
```

This spawns worker processes to handle requests.

### Resource Planning

**Per-server resource estimates**:
| Metric | Light | Medium | Heavy |
|--------|-------|--------|-------|
| Requests/min | 100 | 500 | 2000 |
| Connections | 10 | 30 | 60 |
| Peak players | 20 | 60 | 120 |

**Service requirements** (approximate, combined):
| Servers | RAM | CPU Cores |
|---------|-----|-----------|
| 1-3 | 2 GB | 1 |
| 4-10 | 4 GB | 2 |
| 10+ | 8 GB | 4 |

---

## Monitoring Multi-Server

### Status Endpoint per Server

Each server can identify itself in status checks:

```bash
# Check service health
curl -H "Auth-Key: token_for_server_us_east" https://service:443/Status
```

### Log Analysis

Logs include mod name and client ID:

```bash
# Find requests from specific server (by mod prefix)
grep "US1_" /var/lib/ufserverservice/logs/UF-*.log
```

### Alerts

Set up alerts for:
- Any server's requests failing
- Rate limiting on any IP
- Database connection issues

---

## Example: Two-Server Economy

### Scenario

- Server A: PvP server
- Server B: PvE server
- Shared player balances
- Separate server inventories

### Implementation

**Mod Configuration**:
```cpp
// Shared economy mod name
const string ECONOMY_MOD = "SharedEconomy";

// Server-specific inventory mod name
string GetInventoryMod() {
    return UFConfig().ServerID + "_Inventory";
}
```

**Load Player Data**:
```cpp
void OnPlayerJoin(string guid) {
    // Load shared economy
    U().db().Load(ECONOMY_MOD, guid, this, "OnEconomyLoaded");
    
    // Load server-specific inventory
    U().db().Load(GetInventoryMod(), guid, this, "OnInventoryLoaded");
}
```

**Save Player Data**:
```cpp
void OnPlayerLeave(string guid) {
    // Save to shared economy
    U().db().Save(ECONOMY_MOD, guid, economyData, this, "OnSaved");
    
    // Save to server-specific inventory
    U().db().Save(GetInventoryMod(), guid, inventoryData, this, "OnSaved");
}
```

---

## Troubleshooting

### Server Can't Connect

1. Verify `ServerAuth` token matches between service and mod config
2. Check service is reachable from server's network
3. Verify firewall allows connection from server IP
4. Check rate limiting isn't blocking the server

### Data Appearing on Wrong Server

1. Verify mod names include server identifiers
2. Check no hardcoded mod names in mod code
3. Ensure ServerID is configured correctly

### Performance Issues

1. Add server IPs to rate limit whitelist
2. Increase rate limits
3. Enable clustering
4. Consider dedicated UF Service per region
5. Optimize query patterns

### Auth Token Security

1. Use unique tokens per server
2. Rotate tokens periodically
3. Store tokens securely
4. Monitor for unauthorized access

## Tags
`operators`, `multi-server`, `cluster`, `scaling`, `auth`, `how-to`, `doc-usage`
