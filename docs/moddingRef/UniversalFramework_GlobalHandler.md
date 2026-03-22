# Universal Framework - Global Handler

## Overview

The Global Handler (`UDBGlobalEndpoint`) provides a key-value store for server-wide configuration and state that persists across restarts. Unlike the object/player databases, globals are accessed by mod name rather than object ID.

## Use Cases

- Server configuration
- Global counters and statistics
- Shared state between all players
- Server-wide event flags
- Economy totals

## Accessing the Global Endpoint

```enforce
UDBGlobalEndpoint globals = UF().globals();
```

## Permissions

| Operation | Server | Player (Client) |
|-----------|--------|----------------|
| Load | [YES] | [YES] |
| Save | [YES] | âŒ |
| Update | [YES] | âŒ |
| Transaction | [YES] | âŒ |

> **Note:** Players can read global state but cannot modify it. All writes are server-only.

## Save Operations

### Save Full State

```enforce
// Define your global state class
class MyModGlobals {
    int TotalPlayerCount;
    int TotalDeaths;
    float EconomyMultiplier;
    ref array<string> BannedItems;
    string LastWipeDate;
}

// Save the entire state
MyModGlobals state = new MyModGlobals();
state.TotalPlayerCount = 150;
state.TotalDeaths = 2500;
state.EconomyMultiplier = 1.5;
state.BannedItems = {"AWM", "LAW"};
state.LastWipeDate = "2024-01-15";

string json;
if (UJSONHandler<MyModGlobals>.GetString(state, json)) {
    UF().globals().Save("MyMod", json);
}
```

### Save with Callback

```enforce
UF().globals().Save("MyMod", json, this, "OnSaveComplete");

void OnSaveComplete(int cid, int status, string oid, string data) {
    if (status == UF_SUCCESS) {
        Print("Global state saved for: " + oid);
    }
}
```

### Save with Typed Callback

```enforce
UF().globals().Save("MyMod", json, new UFCallback<StatusObject>(this, "OnSaved"));

void OnSaved(int cid, int status, string oid, StatusObject result) {
    if (status == UF_SUCCESS) {
        Print("Save successful");
    }
}
```

## Load Operations

### Load with Callback

```enforce
UF().globals().Load("MyMod", this, "OnGlobalsLoaded");

void OnGlobalsLoaded(int cid, int status, string oid, string json) {
    if (status == UF_SUCCESS) {
        MyModGlobals state;

## Best Practices

### Use Generic Wrappers
Instead of calling `UF().globals()` directly, use the `UFGlobalHandler<T>` wrapper class. This handles JSON serialization/deserialization automatically and provides a cleaner API.

```enforce
class MyGlobalState {
    int ServerLevel;
    string LastWinner;
}

// In your init
UFGlobalHandler<MyGlobalState> m_Globals = new UFGlobalHandler<MyGlobalState>("MyMod", this, "OnGlobalUpdate");
m_Globals.Load(); // Automatically calls OnGlobalUpdate with typed data
```

### Atomic Transactions
For values that change frequently (like counters or economy balances), use `.Transaction()` or `.Update()` instead of strict Load/Save cycles to prevent race conditions.

### Shared Configuration
Use Globals to store configuration that should be identical across all servers in a cluster. This allows you to update one JSON object and have changes propagate to all servers on their next reload/poll.

## Common Use Cases

### Server Cluster Configuration
Store a single configuration object in Globals that all your servers load on startup.
- **Benefit:** Centralized management of settings (ban lists, MOTD, event schedules).

### Global Economy
Track a "Global Bank" or "Server Tax" where a percentage of all transactions goes into a global pot.
- **Implementation:** Use `Transaction` to increment the global pot safely.

### Cross-Server Events
Use a global flag to trigger events across all servers simultaneously.
- **Implementation:** One server (or Discord bot) sets `EventActive = true`, all servers poll this value and start the event.

## Tags
`globals`, `configuration`, `persistence`, `UDBGlobalEndpoint`, `shared-state`, `cluster-management`, `server-config`, `how-to`, `reference`, `doc-usage`, `modder`

}
```

### Load with Typed Callback

```enforce
UF().globals().Load("MyMod", new UFCallback<MyModGlobals>(this, "OnLoaded"));

void OnLoaded(int cid, int status, string oid, MyModGlobals state) {
    if (status == UF_SUCCESS && state) {
        m_State = state;
    }
}
```

## Increment Operations

Atomic increment for numeric fields.

```enforce
// Increment by 1 (default)
UF().globals().Increment("MyMod", "TotalDeaths");

// Increment by specific amount
UF().globals().Increment("MyMod", "TotalKills", 5);
```

## Transaction Operations

Atomic numeric operations with optional callbacks.

```enforce
// Simple transaction
UF().globals().Transaction("MyMod", "ActivePlayers", 1);
UF().globals().Transaction("MyMod", "ActivePlayers", -1);

// Transaction with callback
UF().globals().Transaction("MyMod", "TotalMoney", 1000, this, "OnTransaction");

void OnTransaction(int cid, int status, string oid, string data) {
    if (status == UF_SUCCESS) {
        Print("Transaction completed for: " + oid);
    }
}

// Typed callback
UF().globals().Transaction("MyMod", "Economy", 500, 
    new UFCallback<UDBTransactionResponse>(this, "OnEconomyUpdate"));

void OnEconomyUpdate(int cid, int status, string oid, UDBTransactionResponse resp) {
    if (status == UF_SUCCESS && resp) {
        Print("New economy value: " + resp.NewValue);
    }
}
```

## Update Operations

Update specific fields within the global state.

```enforce
// Update operations available
class UpdateOpts {
    static const string SET = "set";
    static const string PUSH = "push";
    static const string PULL = "pull";
    static const string UNSET = "unset";
}

// Set a value (strings must be quoted)
UF().globals().Update("MyMod", "LastWipeDate", "\"2024-02-01\"", UpdateOpts.SET);

// Set a number
UF().globals().Update("MyMod", "EconomyMultiplier", "2.0", UpdateOpts.SET);

// Push to array
UF().globals().Update("MyMod", "BannedItems", "\"RPG\"", UpdateOpts.PUSH);

// Pull from array
UF().globals().Update("MyMod", "BannedItems", "\"AWM\"", UpdateOpts.PULL);

// Update with callback
UF().globals().Update("MyMod", "ServerStatus", "\"online\"", UpdateOpts.SET, this, "OnUpdate");
```

## Complete Example

### Server Statistics Tracker

```enforce
class ServerStats {
    int UniqueLogins;
    int TotalPlaytime;    // In minutes
    int TotalDeaths;
    int TotalKills;
    int TotalZombieKills;
    ref array<string> TopPlayers;
    string LastRestart;
}

class ServerStatsManager {
    protected autoptr ServerStats m_Stats;
    protected bool m_IsLoaded = false;
    
    void Init() {
        UF().globals().Load("ServerStats", new UFCallback<ServerStats>(this, "OnStatsLoaded"));
    }
    
    void OnStatsLoaded(int cid, int status, string oid, ServerStats stats) {
        if (status == UF_SUCCESS && stats) {
            m_Stats = stats;
        } else {
            m_Stats = new ServerStats();
            m_Stats.TopPlayers = new array<string>;
        }
        m_IsLoaded = true;
        
        // Update restart timestamp
        UF().globals().Update("ServerStats", "LastRestart", 
            "\"" + UUtil.GetDateStamp() + " " + UUtil.GetTimeStamp() + "\"", UpdateOpts.SET);
    }
    
    void RecordDeath(string killerType) {
        if (!m_IsLoaded) return;
        
        UF().globals().Increment("ServerStats", "TotalDeaths");
        
        if (killerType == "zombie") {
            UF().globals().Increment("ServerStats", "TotalZombieKills");
        } else if (killerType == "player") {
            UF().globals().Increment("ServerStats", "TotalKills");
        }
    }
    
    void RecordLogin(string playerId) {
        if (!m_IsLoaded) return;
        UF().globals().Increment("ServerStats", "UniqueLogins");
    }
    
    void AddTopPlayer(string playerName) {
        UF().globals().Update("ServerStats", "TopPlayers", 
            "\"" + playerName + "\"", UpdateOpts.PUSH);
    }
    
    void SavePlaytime(int minutes) {
        UF().globals().Transaction("ServerStats", "TotalPlaytime", minutes);
    }
}
```

## Best Practices

1. **Use for server-wide state only** - player-specific data should use `PLAYER_DB`
2. **Use transactions for counters** - ensures atomic updates
3. **Use Update for partial changes** - more efficient than full Save
4. **Cache loaded state** - don't reload on every access
5. **Schedule periodic saves** - for complex state that changes frequently
6. **Handle UF_EMPTY status** - initialize defaults on first run
