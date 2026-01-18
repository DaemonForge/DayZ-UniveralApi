# Universal Framework - Core Utilities

## Overview

`UUtil` provides static helper functions for common operations including player lookups, time handling, notifications, config access, and more.

> **Utilities are split across multiple documents for easier navigation:**
> - **This Document** - Core utilities (Player, Time, Notifications, Config)
> - [String Utilities](UniversalFramework_Utilities_Strings.md) - Formatting, Sanitization, Regex patterns
> - [File Utilities](UniversalFramework_Utilities_Files.md) - File operations, JSON handling
> - [Map Utilities](UniversalFramework_Utilities_Map.md) - Map locations, nearest city/town

## Table of Contents

- [Player Functions](#player-functions)
- [Time Functions](#time-functions)
- [Random ID](#random-id)
- [Notifications](#notifications)
- [Status/Error Conversion](#statuserror-conversion)
- [Config Getters](#config-getters)
- [Framework Configuration](#framework-configuration)
- [Common Patterns](#common-patterns)

## Player Functions

```enforce
// Find player by GUID (server-side)
static DayZPlayer FindPlayer(string GUID);

// Find player by identity
static DayZPlayer FindPlayerByIdentity(PlayerIdentity identity);

// Get Steam ID (client-side)
static string GetSteamId();
```

### Usage

```enforce
// Server: find player by GUID
DayZPlayer player = UUtil.FindPlayer("abc123xyz");

// Server: find by identity
DayZPlayer player = UUtil.FindPlayerByIdentity(identity);

// Client: get own Steam ID
string myId = UUtil.GetSteamId();
```

## Time Functions

```enforce
// Get Unix timestamp (local time)
static int GetUnixInt();

// Get Unix timestamp (UTC)
static int GetUTCUnixInt();

// Get days since Jan 1, 1970 (local)
static int GetDateInt();

// Get days since Jan 1, 1970 (UTC)
static int GetUTCDateInt();

// Get date string "YYYY-MM-DD"
static string GetDateStamp();

// Get time string "HH:MM:SS"
static string GetTimeStamp();

// Convert Unix timestamp to components
static void UnixToDateTime(int unixTime, out int year, out int month, out int day, out int hour, out int minute, out int second);
static void UnixToDate(int unixTime, out int year, out int month, out int day);
static void UnixToTime(int unixTime, out int hour, out int minute, out int second);

// Convert Unix timestamp to formatted strings
static string UnixToDateTimeString(int unixTime);  // "YYYY-MM-DD HH:MM:SS"
static string UnixToDateString(int unixTime);      // "YYYY-MM-DD"
static string UnixToTimeString(int unixTime);      // "HH:MM:SS"

// Timezone functions
static int GetTimezoneOffsetSeconds();   // Offset in seconds (e.g., 7200 for UTC+2)
static int GetTimezoneOffsetHours();     // Offset in hours (e.g., 2 for UTC+2)
static string GetTimezoneString();       // Formatted string (e.g., "UTC+02:00")

// UTC to Local conversion
static int UTCToLocalUnix(int utcUnixTime);        // Convert UTC timestamp to local
static int LocalToUTCUnix(int localUnixTime);      // Convert local timestamp to UTC
static void UTCToLocalDateTime(int utcUnixTime, out int year, out int month, out int day, out int hour, out int minute, out int second);
static string UTCToLocalDateTimeString(int utcUnixTime);
```

### Usage

```enforce
int now = UUtil.GetUnixInt();
int utcNow = UUtil.GetUTCUnixInt();
string date = UUtil.GetDateStamp();  // "2024-01-15"
string time = UUtil.GetTimeStamp();  // "14:30:00"

// Convert Unix timestamp to components
int year, month, day, hour, minute, second;
UUtil.UnixToDateTime(now, year, month, day, hour, minute, second);

// Get just the date components
int yr, mth, dy;
UUtil.UnixToDate(now, yr, mth, dy);

// Get just the time components
int hr, min, sec;
UUtil.UnixToTime(now, hr, min, sec);

// Convert to formatted strings
string datetime = UUtil.UnixToDateTimeString(now);  // "2024-01-15 14:30:00"
string dateStr = UUtil.UnixToDateString(now);       // "2024-01-15"
string timeStr = UUtil.UnixToTimeString(now);       // "14:30:00"

// Timezone info
int offsetSec = UUtil.GetTimezoneOffsetSeconds();   // e.g., 7200 for UTC+2
int offsetHrs = UUtil.GetTimezoneOffsetHours();     // e.g., 2 for UTC+2
string tz = UUtil.GetTimezoneString();              // e.g., "UTC+02:00"

// Convert UTC timestamp to local time
int utcTimestamp = 1735689600;
int localTimestamp = UUtil.UTCToLocalUnix(utcTimestamp);
string localStr = UUtil.UTCToLocalDateTimeString(utcTimestamp);

// Convert local timestamp to UTC
int myLocalTime = UUtil.GetUnixInt();
int myUtcTime = UUtil.LocalToUTCUnix(myLocalTime);

// Get local date/time components from UTC timestamp
int lYear, lMonth, lDay, lHour, lMinute, lSecond;
UUtil.UTCToLocalDateTime(utcTimestamp, lYear, lMonth, lDay, lHour, lMinute, lSecond);
```

## Random ID

```enforce
// Generate random alphanumeric string (length = number + 1)
static string GetRandomId(int number);
```

### Usage

```enforce
string id = UUtil.GetRandomId(15);  // 16 character string
```

## Notifications

```enforce
// Send notification to player
static void SendNotification(string Header, string Text, PlayerIdentity player, string Icon = "_UFramework\\images\\info.edds");

// Extended version (also works if player is null on client)
static void SendNotificationEx(string Header, string Text, PlayerIdentity player, string Icon = "_UFramework\\images\\info.edds");
```

### Usage

```enforce
UUtil.SendNotification("Alert", "Airdrop incoming!", player.GetIdentity());

// Custom icon
UUtil.SendNotification("VIP", "Welcome!", identity, "MyMod\\icons\\vip.edds");
```

---

## Status/Error Conversion

```enforce
// Convert UF status code to string
static string StatusToString(int StatusCode);


// Convert REST error code to string
static string RestErrorToString(int ErrorCode);
```

### Usage

```enforce
void OnCallback(int cid, int status, string oid, string data) {
    if (status != UF_SUCCESS) {
        Print("Error: " + UUtil.StatusToString(status));
    }
}
```

---

## Config Getters

Read values from game config (CfgMagazines, CfgWeapons, CfgVehicles):

```enforce
// Get single values
static bool GetConfigInt(string type, string variable, out int value);
static bool GetConfigFloat(string type, string variable, out float value);
static bool GetConfigString(string type, string variable, out string value);

// Get arrays
static bool GetConfigTStringArray(string type, string variable, out TStringArray value);
static bool GetConfigTFloatArray(string type, string variable, out TFloatArray value);
static bool GetConfigTIntArray(string type, string variable, out TIntArray value);
```

### Usage

```enforce
int magSize;
if (UUtil.GetConfigInt("Mag_AKM_30Rnd", "count", magSize)) {
    Print("Magazine capacity: " + magSize);
}

TStringArray chambers;
if (UUtil.GetConfigTStringArray("AKM", "chamberableFrom", chambers)) {
    // chambers contains compatible magazine types
}
```

## Framework Configuration

Access the Universal Framework's server configuration via the `UFConfig()` singleton. Configuration is loaded from `$profile:UF/UFramework.json`.

### Configuration Fields

| Field | Type | Description |
|-------|------|-------------|
| `ServerURL` | string | Base URL of the UFServerService (e.g., `"https://api.example.com/"`) |
| `ServerID` | string | Unique identifier for this server (used in logging, globals) |
| `ServerAuth` | string | Authentication key for server-to-service communication (server-only) |
| `EnableBuiltinLogging` | int | Enable/disable built-in logging (0 = off, 1 = on) |
| `PromptDiscordOnConnect` | int | Prompt players to link Discord on connect (0 = off, 1 = on) |

### Configuration Methods

| Method | Returns | Description |
|--------|---------|-------------|
| `GetBaseURL()` | string | Returns the ServerURL |
| `GetServerID()` | string | Returns the ServerID (available on client and server) |
| `GetAuth()` | string | Returns the ServerAuth (server-only, returns "ERROR" on client) |

### Accessing Configuration

The `ServerID` is available on **both client and server**. On the client, it becomes available after the auth token is received from the server.

```enforce
// Preferred: Via UFramework singleton (works on client and server)
string serverId = U().GetServerID();

// Alternative: Via config directly
string serverId = UFConfig().GetServerID();

// Get the base URL
string baseUrl = UFConfig().GetBaseURL();

// Check if Discord prompt is enabled (server-only check recommended)
if (GetGame().IsServer() && UFConfig().PromptDiscordOnConnect == 1) {
    // Prompt player to link Discord
}
```

### Example: Server-Specific Logging

```enforce
void LogPlayerAction(PlayerIdentity identity, string action) {
    if (!GetGame().IsServer()) return;
    
    string serverName = U().GetServerID();
    string logMsg = "[" + serverName + "] " + identity.GetName() + ": " + action;
    
    U().Logger().Log(logMsg);
}
```

### Example: Server-Aware Welcome Message

```enforce
void OnPlayerConnect(PlayerIdentity identity) {
    if (!GetGame().IsServer()) return;
    
    string serverName = U().GetServerID();
    string welcomeMsg = "Welcome to " + serverName + "!";
    
    UUtil.SendNotification("Welcome", welcomeMsg, identity);
}
```

### Example: Client-Side Server ID Usage

```enforce
// On client, ServerID is available after auth token is received
void ShowServerInfo() {
    string serverId = U().GetServerID();
    if (serverId != "") {
        Print("Connected to server: " + serverId);
    }
}
```

> **Note:** The `ServerID`, `ServerURL`, and non-sensitive config fields are synced to the client after authentication. The `ServerAuth` field is **never** sent to the client for security reasons - `GetAuth()` returns "ERROR" on the client.

---

## Common Patterns

### Schedule with Unix Time

```enforce
int eventTime = UUtil.GetUnixInt() + 300;  // 5 minutes from now
U().Cron().runOnce(eventTime, this, "OnEvent");
```

### Generate Unique Keys

```enforce
string saveKey = "item_" + UUtil.GetRandomId(7);  // 8 char suffix
U().db().Save("MyMod", saveKey, itemJson);
```

### Notify All Players

```enforce
void NotifyAllPlayers(string header, string message) {
    array<Man> players = new array<Man>;
    GetGame().GetPlayers(players);
    
    foreach (Man man : players) {
        DayZPlayer player = DayZPlayer.Cast(man);
        if (player && player.GetIdentity()) {
            UUtil.SendNotification(header, message, player.GetIdentity());
        }
    }
}
```

## Best Practices

### Reliable Timestamps
When storing dates in a database, **ALWAYS** use `GetUTCUnixInt()` or `GetUTCDateInt()`.
*   **Why?** If you move your server to a different timezone, or have players from different zones, UTC ensures sorting works correctly. Only convert to local time when displaying to the user (`UTCToLocalDateTimeString`).

### Player Lookups
Use `FindPlayer` carefully in loops. It iterates through the player list.
*   **Optimization**: If you need to message all players, iterate the player list yourself (`GetGame().GetPlayers()`) instead of calling `FindPlayer` for every ID.

## Common Use Cases

### Log Files
Use `GetTimeStamp()` to prefix your custom log files.
`[14:30:15] Player X killed Player Y`

### Daily Login Rewards
Use `GetUTCDateInt()` to track if a player has claimed their reward today.
*   `if (lastClaimDate < UUtil.GetUTCDateInt()) { GiveReward(); }`

### Server Messages
Use `UUtilNotification` (see below) to send standardized, colored messages to chat without re-inventing the wheel every time.

## Tags
`utilities`, `helpers`, `time`, `date`, `player-lookup`, `utc`, `timestamps`, `timezone`, `how-to`, `reference`, `doc-usage`, `modder`

