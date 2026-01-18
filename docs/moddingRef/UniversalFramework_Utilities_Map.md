# Universal Framework - Map Utilities

## Overview

Map location utilities for retrieving named locations (cities, towns, villages, etc.) from the current map's CfgWorlds configuration.

> **Related Documentation:**
> - [Core Utilities](UniversalFramework_Utilities.md) - Player, Time, Notifications, Config
> - [String Utilities](UniversalFramework_Utilities_Strings.md) - Formatting, Sanitization, Regex
> - [File Utilities](UniversalFramework_Utilities_Files.md) - File operations, JSON handling

## Table of Contents

- [UMapLocation Class](#umaplocation-class)
- [Common Location Types](#common-location-types)
- [Functions](#functions)
- [Usage Examples](#usage-examples)
- [Practical Examples](#practical-examples)

---

## UMapLocation Class

```enforce
class UMapLocation
{
    string ClassName;  // Config class name
    string Name;       // Display name (e.g., "Chernogorsk")
    string Type;       // Location type (e.g., "City", "Village")
    vector Position;   // 3D world position (includes terrain height)
}
```

---

## Common Location Types

| Type | Description |
|------|-------------|
| `"Capital"` | Major cities |
| `"City"` | Large towns/cities |
| `"Village"` | Small villages |
| `"Local"` | Local landmarks |
| `"Marine"` | Marine/coastal points |
| `"Hill"` | Hills and elevated areas |
| `"Ruin"` | Ruins and historical sites |
| `"ViewPoint"` | Scenic viewpoints |

---

## Functions

```enforce
// Get all map locations (optionally filtered by type)
static array<autoptr UMapLocation> GetMapLocations(array<string> typeFilters = NULL);

// Find the nearest location to a position
static UMapLocation GetNearestMapLocation(vector position, array<string> typeFilters = NULL);

// Find the nearest location name (convenience wrapper)
static string GetNearestMapLocationName(vector position, array<string> typeFilters = NULL);

// Find all locations within a radius
static array<autoptr UMapLocation> GetMapLocationsInRadius(vector position, float radius, array<string> typeFilters = NULL);
```

---

## Usage Examples

### Get All Towns

```enforce
// Get all cities and villages on the map
array<string> filters = new array<string>();
filters.Insert("City");
filters.Insert("Village");

array<autoptr UMapLocation> towns = UUtil.GetMapLocations(filters);
foreach (UMapLocation loc : towns)
{
    Print("Found: " + loc.Name + " (" + loc.Type + ") at " + loc.Position.ToString());
}
```

### Get All Locations

```enforce
// Get ALL locations (no filter)
array<autoptr UMapLocation> allLocations = UUtil.GetMapLocations();
Print("Total locations on map: " + allLocations.Count());
```

### Find Nearest City

```enforce
// Find the nearest city to a player
vector playerPos = player.GetPosition();
array<string> cityFilter = new array<string>();
cityFilter.Insert("City");
cityFilter.Insert("Capital");

UMapLocation nearest = UUtil.GetNearestMapLocation(playerPos, cityFilter);
if (nearest)
{
    float distance = vector.Distance(playerPos, nearest.Position);
    Print("Nearest city: " + nearest.Name + " (" + distance.ToString() + "m away)");
}
```

### Quick Name Lookup

```enforce
// Quick way to get just the name (returns "unknown" if not found)
string nearestName = UUtil.GetNearestMapLocationName(playerPos);
Print("Player is near: " + nearestName);
```

### Locations in Radius

```enforce
// Find all locations within 5km of a position
array<autoptr UMapLocation> nearby = UUtil.GetMapLocationsInRadius(playerPos, 5000);
Print("Locations within 5km: " + nearby.Count());

foreach (UMapLocation loc : nearby)
{
    float dist = vector.Distance(playerPos, loc.Position);
    Print("  - " + loc.Name + " (" + loc.Type + "): " + dist.ToString() + "m");
}
```

---

## Practical Examples

### Spawn Near Random Town

```enforce
// Spawn an item near a random village
array<string> villageFilter = new array<string>();
villageFilter.Insert("Village");

array<autoptr UMapLocation> villages = UUtil.GetMapLocations(villageFilter);
if (villages.Count() > 0)
{
    int randomIndex = Math.RandomInt(0, villages.Count());
    UMapLocation village = villages.Get(randomIndex);
    
    // Offset position slightly
    vector spawnPos = village.Position + Vector(Math.RandomFloat(-50, 50), 0, Math.RandomFloat(-50, 50));
    spawnPos[1] = GetGame().SurfaceY(spawnPos[0], spawnPos[2]);
    
    GetGame().CreateObject("Barrel_Green", spawnPos, false, false, true);
    Print("Spawned barrel near " + village.Name);
}
```

### Location-Based Death Message

```enforce
void OnPlayerDeath(DayZPlayer player, Object killer) {
    vector deathPos = player.GetPosition();
    string location = UUtil.GetNearestMapLocationName(deathPos);
    
    string message = player.GetIdentity().GetName() + " died near " + location;
    
    // Send to Discord
    U().ds().ChannelSend("deaths-channel-id", message);
}
```

### Airdrop Zone Selection

```enforce
// Select a random city for airdrop
UMapLocation GetRandomAirdropLocation() {
    array<string> filters = new array<string>();
    filters.Insert("City");
    filters.Insert("Capital");
    
    array<autoptr UMapLocation> cities = UUtil.GetMapLocations(filters);
    if (cities.Count() == 0)
        return NULL;
    
    int idx = Math.RandomInt(0, cities.Count());
    return cities.Get(idx);
}

void StartAirdrop() {
    UMapLocation target = GetRandomAirdropLocation();
    if (target) {
        Print("Airdrop incoming to " + target.Name + "!");
        SpawnAirdropAt(target.Position);
    }
}
```

### Player Location Tracking

```enforce
class PlayerLocationTracker {
    protected string m_LastLocation;
    protected DayZPlayer m_Player;
    
    void Update() {
        if (!m_Player) return;
        
        string currentLocation = UUtil.GetNearestMapLocationName(m_Player.GetPosition());
        
        if (currentLocation != m_LastLocation) {
            Print(m_Player.GetIdentity().GetName() + " entered " + currentLocation);
            m_LastLocation = currentLocation;
            
            // Could trigger events based on location
            OnLocationChanged(currentLocation);
        }
    }
    
    void OnLocationChanged(string newLocation) {
        // Custom logic when player enters new area
    }
}
```

### Safe Zone Check

```enforce
bool IsInSafeZone(vector position) {
    // Define safe zone centers (could load from config)
    array<string> safeZoneNames = {"Chernogorsk", "Elektrozavodsk"};
    float safeZoneRadius = 500; // meters
    
    array<string> cityFilter = new array<string>();
    cityFilter.Insert("City");
    cityFilter.Insert("Capital");
    
    UMapLocation nearest = UUtil.GetNearestMapLocation(position, cityFilter);
    if (!nearest)
        return false;
    
    // Check if it's a designated safe zone
    foreach (string safeName : safeZoneNames) {
        if (nearest.Name == safeName) {
            float dist = vector.Distance(position, nearest.Position);
            return dist <= safeZoneRadius;
        }
    }
    
    return false;
}
```

## Tags
`map`, `locations`, `utilities`, `navigation`, `cfgworlds`, `reference`, `doc-usage`, `modder`
