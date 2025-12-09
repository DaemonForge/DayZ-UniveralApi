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
UCronManager cron = U().Cron();
```

## Scheduling Methods

### runEndless

Execute a function repeatedly at a fixed interval forever.

```enforce
// Run every 60 seconds
U().Cron().runEndless(60, this, "OnMinuteTick");

// With parameters
Param1<string> params = new Param1<string>("hello");
U().Cron().runEndless(30, this, "OnHalfMinute", params);

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
// Run every 5 seconds until midnight UTC
int midnight = GetMidnightUnix();
U().Cron().runEndTime(5, midnight, this, "OnPoll");

// Run for the next hour
int oneHourFromNow = UUtil.GetUnixInt() + 3600;
U().Cron().runEndTime(10, oneHourFromNow, this, "HourlyTask");

void OnPoll() {
    Print("Polling...");
}
```

### runEndCount

Execute a specific number of times then stop.

```enforce
// Run 5 times, every 10 seconds
U().Cron().runEndCount(10, 5, this, "OnCountedRun");

// Run 3 times with params
Param2<int, string> params = new Param2<int, string>(100, "bonus");
U().Cron().runEndCount(60, 3, this, "GiveBonus", params);

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
// Run 5 minutes from now
int fiveMinutes = UUtil.GetUnixInt() + 300;
U().Cron().runOnce(fiveMinutes, this, "DelayedAction");

// Schedule for specific time
int targetTime = CalculateNextEventTime();
U().Cron().runOnce(targetTime, this, "ScheduledEvent");

void DelayedAction() {
    Print("Delayed action executed");
}
```

## Removing Scheduled Tasks

```enforce
// Remove a specific scheduled function
U().Cron().Remove(this, "OnMinuteTick");

// Always clean up in destructor
void ~MyClass() {
    U().Cron().Remove(this, "DoWork");
}
```

## Practical Examples

### Auto-Save System

```enforce
class AutoSaveManager {
    protected autoptr UDBHandler<PlayerData> m_DB;
    
    void Init() {
        // Save all players every 5 minutes
        U().Cron().runEndless(300, this, "SaveAllPlayers");
    }
    
    void ~AutoSaveManager() {
        U().Cron().Remove(this, "SaveAllPlayers");
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
        U().Cron().runOnce(executeAt, this, "SpawnAirdrop", params);
        
        // Announce 1 minute before
        int announceAt = executeAt - 60;
        U().Cron().runOnce(announceAt, this, "AnnounceAirdrop", params);
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

1. **Always clean up** cron entries in destructors
2. **Use appropriate frequencies** - don't schedule too frequently
3. **Handle null objects** - the cron manager runs cleanup every 15 minutes
4. **Use Unix timestamps** for precise timing calculations
5. **Pass parameters** via Param classes instead of storing state
6. **Consider server performance** when scheduling many tasks
