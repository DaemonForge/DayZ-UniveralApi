# Universal Framework - File Utilities

## Overview

File operations, Base64 handling, and JSON serialization/deserialization utilities.

> **Related Documentation:**
> - [Core Utilities](UniversalFramework_Utilities.md) - Player, Time, Notifications, Config
> - [String Utilities](UniversalFramework_Utilities_Strings.md) - Formatting, Sanitization, Regex
> - [Map Utilities](UniversalFramework_Utilities_Map.md) - Map locations, nearest city

## Table of Contents

- [File Operations](#file-operations)
- [JSON Handler](#json-handler)

---

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

### Base64 Decoding Example

```enforce
// Decode base64 and save to file
string base64Data = "SGVsbG8gV29ybGQh";
array<int> bytes;
UUtil.DecodeBase64(base64Data, bytes);
UUtil.SaveBytesToFile(bytes, "$profile:MyMod\\decoded.bin");

// Or use the combined method
UUtil.SaveBase64ToFile(base64Data, "$profile:MyMod\\decoded.bin");
```

### Deferred Saving

For large files (like TTS audio), use `SaveBase64ToFileSplit` to avoid frame drops:

```enforce
void OnAudioReceived(string base64Audio, string filename) {
    // This splits the work across multiple frames
    UUtil.SaveBase64ToFileSplit(base64Audio, "$saves:" + filename + ".mp4");
}
```

---

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

### Basic Usage

```enforce
// Define a data class
class MyData extends Managed {
    string Name;
    int Value;
    bool Active;
}

// Serialize object to JSON
MyData data = new MyData();
data.Name = "Test";
data.Value = 100;
data.Active = true;

string json = UJSONHandler<MyData>.ToString(data);
// json = "{\"Name\":\"Test\",\"Value\":100,\"Active\":1}"

// Deserialize JSON to object
MyData loaded;
if (UJSONHandler<MyData>.FromString(json, loaded)) {
    Print(loaded.Name);    // "Test"
    Print(loaded.Value);   // 100
    Print(loaded.Active);  // true (deserialized from 1)
}
```

### File Operations

```enforce
// Save to file
UJSONHandler<MyData>.ToFile("$profile:mydata.json", data);

// Load from file
MyData fromFile;
UJSONHandler<MyData>.FromFile("$profile:mydata.json", fromFile);
```

### Error Handling

```enforce
string json = GetUserInput();
MyData parsed;

if (UJSONHandler<MyData>.FromString(json, parsed)) {
    // Success - use parsed data
    ProcessData(parsed);
} else {
    // Failed to parse - handle error
    Print("Invalid JSON format");
}
```

### Complex Objects

```enforce
class PlayerStats extends Managed {
    string GUID;
    int Kills;
    int Deaths;
    float PlayTime;
    autoptr array<string> Achievements;
}

// Save player stats
PlayerStats stats = new PlayerStats();
stats.GUID = "abc123";
stats.Kills = 50;
stats.Deaths = 10;
stats.PlayTime = 3600.5;
stats.Achievements = new array<string>();
stats.Achievements.Insert("FirstBlood");
stats.Achievements.Insert("Survivor");

UJSONHandler<PlayerStats>.ToFile("$profile:MyMod\\stats_abc123.json", stats);

// Load player stats
PlayerStats loaded;
UJSONHandler<PlayerStats>.FromFile("$profile:MyMod\\stats_abc123.json", loaded);
```

### Nested Objects

```enforce
class Inventory extends Managed {
    autoptr array<autoptr InventoryItem> Items;
}

class InventoryItem extends Managed {
    string ClassName;
    int Quantity;
    float Condition;
}

// Serialize nested structure
Inventory inv = new Inventory();
inv.Items = new array<autoptr InventoryItem>();

InventoryItem item = new InventoryItem();
item.ClassName = "Apple";
item.Quantity = 5;
item.Condition = 0.85;
inv.Items.Insert(item);

string json = UJSONHandler<Inventory>.ToString(inv);
// {"Items":[{"ClassName":"Apple","Quantity":5,"Condition":0.85}]}
```

### Database Integration

The JSON handler integrates seamlessly with the database system:

```enforce
class MyModData extends Managed {
    string PlayerName;
    int Score;
}

// Save to database
MyModData data = new MyModData();
data.PlayerName = "John";
data.Score = 1000;

string json = UJSONHandler<MyModData>.ToString(data);
U().db().Save("MyMod", playerGUID, json, this, "OnSaved");

// Load from database
void OnLoaded(int cid, int status, string oid, string data) {
    if (status == UF_SUCCESS) {
        MyModData loaded;
        if (UJSONHandler<MyModData>.FromString(data, loaded)) {
            Print("Loaded score: " + loaded.Score);
        }
    }
}
```
