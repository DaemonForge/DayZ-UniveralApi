# Universal Framework - Logger

## Overview

`UFLog` provides static logging methods with levels.

## Log Levels

```enforce
static const int LOG_ERROR = 0;
static const int LOG_VERBOSE = 1;
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
    protected static autoptr ULoggerBaseInstance m_Instance;
    
    override static void CreateInstance() {
        m_type = "MyMod";
        m_Instance = new ULoggerBaseInstance("MyMod");
    }
    
    override static ULoggerBaseInstance GetInstance() {
        if (!m_Instance) { CreateInstance(); }
        return m_Instance;
    }
}

// Usage
MyModLog.Info("Started");
MyModLog.Err("Failed");
```

## ULoggerBaseInstance

For direct instance control:

```enforce
autoptr ULoggerBaseInstance logger = new ULoggerBaseInstance("MyMod", 3);
logger.DoLog("Message", LOG_INFO);
logger.SetLogLevel(LOG_DEBUG);  // Set console level
logger.SetApiLogLevel(LOG_ERROR);  // Set API logging level
```

## Output

Log files saved to: `$profile:TYPE_YYYY-MM-DD_HH-MM-SS.log`

Format: `[LEVEL] HH:MM:SS | message`

```
[INFO] 14:30:00 | Started
[ERROR] 14:30:01 | Failed to connect
```
