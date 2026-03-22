# Universal Framework - Cron Manager

## Overview

The Cron Manager (`UCronManager`) provides scheduled task execution using Unix timestamps as timing references. It enables periodic function calls, one-time scheduled executions, and various scheduling patterns.

## Key Features

- **Unix Time Based**: Uses Unix timestamps for accurate timing
- **Multiple Scheduling Modes**: Endless, end-time, end-count, and one-time
- **Automatic Cleanup**: Removes null/invalid cron functions periodically
- **Function Parameters**: Pass custom parameters to scheduled functions

## Accessing the Cron Manager

```enforce
UCronManager cron = UF().Cron();
```

---

## Method Signatures

```enforce
// All UCronManager methods with exact parameter types:

// Run forever at fixed interval
// Params: (int freqSeconds, Class obj, string fnName, Param params = NULL)
void runEndless(int freqSeconds, Class obj, string fnName, Param params = NULL);

// Run at interval until absolute Unix timestamp is reached
// Params: (int freqSeconds, int endCallUnix, Class obj, string fnName, Param params = NULL)
// NOTE: endCallUnix is ABSOLUTE Unix timestamp, NOT relative seconds!
void runEndTime(int freqSeconds, int endCallUnix, Class obj, string fnName, Param params = NULL);

// Run at interval for exactly N executions
// Params: (int freqSeconds, int maxCount, Class obj, string fnName, Param params = NULL)
void runEndCount(int freqSeconds, int maxCount, Class obj, string fnName, Param params = NULL);

// Run once at absolute Unix timestamp
// Params: (int nextRunUnix, Class obj, string fnName, Param params = NULL)
// NOTE: nextRunUnix is ABSOLUTE Unix timestamp, NOT relative seconds!
void runOnce(int nextRunUnix, Class obj, string fnName, Param params = NULL);

// Remove a scheduled task by object and function name
void Remove(Class obj, string fnName);
```

### Parameter Reference

| Parameter | Type | Description |
|-----------|------|-------------|
| `freqSeconds` | `int` | Interval in seconds between executions |
| `endCallUnix` | `int` | **Absolute** Unix timestamp when task should stop |
| `nextRunUnix` | `int` | **Absolute** Unix timestamp when task should execute |
| `maxCount` | `int` | Maximum number of executions before removal |
| `obj` | `Class` | Object instance containing the callback method (`this`) |
| `fnName` | `string` | Name of method to call as string (`"MyMethod"`) |
| `params` | `Param` | Optional parameters passed to callback (default: `NULL`) |

> **CRITICAL:** `runEndTime` and `runOnce` use **absolute Unix timestamps**!  
> Use `UUtil.GetUnixInt()` to get current time, then add your offset.
> Example: `UUtil.GetUnixInt() + 3600` = one hour from now

---

## Scheduling Methods

### runEndless

Execute a function repeatedly at a fixed interval forever.

```enforce
// Run every 60 seconds forever
// Params: (int freqSeconds, Class obj, string fnName, Param params)
UF().Cron().runEndless(60, this, "OnMinuteTick", NULL);

// With parameters
Param1<string> params = new Param1<string>("hello");
UF().Cron().runEndless(30, this, "OnHalfMinute", params);

void OnMinuteTick() {
    Print("One minute has passed");
}

void OnHalfMinute(string message) {
    Print("Message: " + message);
}
```

### runEndTime

Execute repeatedly until a specific Unix timestamp.

```enforce
// Params: (int freqSeconds, int endCallUnix, Class obj, string fnName, Param params)
// NOTE: endCallUnix must be an ABSOLUTE Unix timestamp!

// Run every 5 seconds until midnight UTC
int midnight = GetMidnightUnix();
UF().Cron().runEndTime(5, midnight, this, "OnPoll", NULL);

// Run every 10 seconds for the next hour
int oneHourFromNow = UUtil.GetUnixInt() + 3600;  // Current time + 3600 seconds
UF().Cron().runEndTime(10, oneHourFromNow, this, "HourlyTask", NULL);

void OnPoll() {
    Print("Polling...");
}
```

### runEndCount

Execute a specific number of times then stop.

```enforce
// Params: (int freqSeconds, int maxCount, Class obj, string fnName, Param params)

// Run 5 times total, every 10 seconds
UF().Cron().runEndCount(10, 5, this, "OnCountedRun", NULL);

// Run 3 times with params
Param2<int, string> params = new Param2<int, string>(100, "bonus");
UF().Cron().runEndCount(60, 3, this, "GiveBonus", params);

void OnCountedRun() {
    Print("Counted execution");
}

void GiveBonus(int amount, string type) {
    Print("Giving " + type + " of " + amount);
}
```

### runOnce

Execute a function once at a specific Unix timestamp.

```enforce
// Params: (int nextRunUnix, Class obj, string fnName, Param params)
// NOTE: nextRunUnix must be an ABSOLUTE Unix timestamp!

// Run 5 minutes from now
int fiveMinutes = UUtil.GetUnixInt() + 300;  // Current time + 300 seconds
UF().Cron().runOnce(fiveMinutes, this, "DelayedAction", NULL);

// Schedule for specific time
int targetTime = CalculateNextEventTime();
UF().Cron().runOnce(targetTime, this, "ScheduledEvent", NULL);

void DelayedAction() {
    Print("Delayed action executed");
}
```

## Removing Scheduled Tasks

```enforce
// Remove a specific scheduled function
UF().Cron().Remove(this, "OnMinuteTick");

// Always clean up in destructor
void ~MyClass() {
    UF().Cron().Remove(this, "DoWork");
}
```

## Practical Examples

### Auto-Save System

```enforce
class AutoSaveManager {
    protected autoptr UDBHandler<PlayerData> m_DB;
    
    void Init() {
        // Save all players every 5 minutes
        UF().Cron().runEndless(300, this, "SaveAllPlayers");
    }
    
    void ~AutoSaveManager() {
        UF().Cron().Remove(this, "SaveAllPlayers");
    }
    
    void SaveAllPlayers() {
        array<Man> players = new array<Man>;
        GetGame().GetPlayers(players);
        
        foreach (Man man : players) {
            PlayerBase player = PlayerBase.Cast(man);
            if (player && player.GetIdentity()) {
                SavePlayer(player);
            }
        }
        Print("[AutoSave] Saved " + players.Count() + " players");
    }
    
    protected void SavePlayer(PlayerBase player) {
        // Save logic
    }
}
```

### Event Scheduler

```enforce
class EventScheduler {
    void ScheduleAirdrop(vector position, int delaySeconds) {
        int executeAt = UUtil.GetUnixInt() + delaySeconds;
        
        Param1<vector> params = new Param1<vector>(position);
        UF().Cron().runOnce(executeAt, this, "SpawnAirdrop", params);
        
        // Announce 1 minute before
        int announceAt = executeAt - 60;
        UF().Cron().runOnce(announceAt, this, "AnnounceAirdrop", params);
    }
    
    void AnnounceAirdrop(vector position) {
        // Notify all players
        Print("Airdrop incoming near " + position.ToString());
    }
    
    void SpawnAirdrop(vector position) {
        // Spawn airdrop at position
        Print("Spawning airdrop at " + position.ToString());
    }
}
```

## Best Practices

### Performance
Cron jobs run on the main thread (usually). Keep the code inside your cron function **fast**. If you need to do heavy work, split it up or use an async worker.
*   **Bad:** Loop through 5000 objects every 1 second.
*   **Good:** Loop through 5000 objects every 5 minutes (300s), or process 50 objects every 1 second.

### Object Lifetime
If the object (`this`) passed to the cron manager is deleted (like a player disconnecting), the Cron Manager will detect `obj == null` and remove the task automatically. However, explicitly calling `Remove()` is cleaner.

## Common Use Cases

### Server Announcements
Send a message to chat every 15 minutes.
```enforce
UF().Cron().runEndless(900, this, "SendAutoMessage");
```

### Delayed Teleport
Teleport a player 10 seconds after they use an item.
```enforce
UF().Cron().runOnce(UUtil.GetUnixInt() + 10, this, "TeleportPlayer", new Param1<PlayerBase>(player));
```

### Event Countdown
Start a server event at a specific real-world time (e.g., Friday 20:00 UTC).
1. Calculate unix timestamp of Friday 20:00.
2. `UF().Cron().runOnce(eventUnix, ...)`

## Tags
`cron`, `scheduling`, `timers`, `automation`, `events`, `periodic-tasks`, `UCronManager`, `how-to`, `reference`, `doc-usage`, `modder`

