# Universal Framework - Logger

## Overview

`UFLog` provides static logging methods with levels.

## Log Levels

```enforce
static const int LOG_ERROR = 0;
static const int LOG_VERBOSE = 1; //reserved for future use
static const int LOG_INFO = 2;
static const int LOG_DEBUG = 3;
```

## Static Methods

```enforce
UFLog.Info(string text);    // Normal operations
UFLog.Debug(string text);   // Debug info (when level allows)
UFLog.Err(string text);     // Errors (also calls Error2)
UFLog.Log(string text, int level);  // Log with custom level
UFLog.SetLogLevels(int level, int apiLevel = -99);
```

## Usage

```enforce
void Initialize() {
    UFLog.Info("MyMod initializing");
    
    if (LoadConfig()) {
        UFLog.Info("Config loaded");
    } else {
        UFLog.Err("Config load failed");
    }
}

void ProcessItem(ItemBase item) {
    if (!item) {
        UFLog.Err("ProcessItem: null item");
        return;
    }
    UFLog.Debug("Processing: " + item.GetType());
}
```

## Custom Logger

Extend `ULoggerBase` for category-specific logging:

```enforce
class MyModLog extends ULoggerBase {
    // Just override the id - that's all you need!
    	override static string getLogID(){return "MyMod";}
}

// Usage
MyModLog.Info("Started");
MyModLog.Debug("Processing item");
MyModLog.Err("Something failed");
```

## Log Levels

For controling log levels:

```enforce

class MyModLog extends ULoggerBase {
    // Just override the id - that's all you need!
    override static string getLogID(){return "MyMod";}
    //Set log levels
    override static void Init(){SetLogLevels(LOG_DEBUG, LOG_INFO); }
```

## Output

Log files saved to: `$profile:TYPE_YYYY-MM-DD_HH-MM-SS.log`

Format: `[LEVEL] HH:MM:SS | message`

```
[INFO] 14:30:00 | Started
[ERROR] 14:30:01 | Failed to connect
```
