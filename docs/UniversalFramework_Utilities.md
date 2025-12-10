# Universal Framework - Utilities

## Overview

`UUtil` provides static helper functions for common operations.

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
```

### Usage

```enforce
int now = UUtil.GetUnixInt();
int utcNow = UUtil.GetUTCUnixInt();
string date = UUtil.GetDateStamp();  // "2024-01-15"
string time = UUtil.GetTimeStamp();  // "14:30:00"
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

## String Formatting

```enforce
// Format integer with commas: 1234567 -> "1,234,567"
static string ConvertIntToNiceString(int DollarAmount);
```

### Usage

```enforce
string formatted = UUtil.ConvertIntToNiceString(1000000);  // "1,000,000"
string negative = UUtil.ConvertIntToNiceString(-5000);     // "-5,000"
```

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

## File Operations

```enforce
// Find all files in directory
static TStringArray FindFilesInDirectory(string directory);

// Save base64 string to binary file
static void SaveBase64ToFile(string base64String, string filePath);

// Save base64 to file (deferred, less frame impact)
static void SaveBase64ToFileSplit(string base64String, string filePath);

// Decode base64 to byte array
static void DecodeBase64(string base64String, out array<int> decodedBytes);

// Save byte array to binary file
static void SaveBytesToFile(array<int> bytes, string filePath);
```

### Usage

```enforce
// List files in directory
TStringArray files = UUtil.FindFilesInDirectory("$profile:MyMod\\data");

// Save downloaded audio (deferred to reduce frame hit)
UUtil.SaveBase64ToFileSplit(audioData, "$saves:audio.mp4");
```

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

## JSON Handler

Templated JSON serialization/deserialization utility.

> **Note:** Boolean values are serialized as integers (0 = false, 1 = true) in JSON output and database storage. Deserialization handles this automatically.

```enforce
class UJSONHandler<Class T> {
    // Object to JSON string
    static string ToString(T data);
    
    // Object to JSON string (with success check)
    static bool GetString(T data, out string stringData);
    
    // JSON string to object
    static bool FromString(string stringData, out T data);
    
    // Load object from JSON file
    static void FromFile(string path, out T data);
    
    // Save object to JSON file
    static void ToFile(string path, T data);
}
```

### Usage

```enforce
// Serialize object to JSON
MyData data = new MyData();
data.Name = "Test";
data.Value = 100;

string json = UJSONHandler<MyData>.ToString(data);
// json = "{\"Name\":\"Test\",\"Value\":100}"

// Deserialize JSON to object
MyData loaded;
if (UJSONHandler<MyData>.FromString(json, loaded)) {
    Print(loaded.Name);  // "Test"
}

// Save to file
UJSONHandler<MyData>.ToFile("$profile:mydata.json", data);

// Load from file
MyData fromFile;
UJSONHandler<MyData>.FromFile("$profile:mydata.json", fromFile);
```

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
