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

To create a category-specific logger, you must extend `ULoggerBase` and implement the static singleton pattern. 

**Note:** You cannot just override the ID because static methods in Enforce Script don't inherit the way instance methods do. You must implement the wrappers.

```enforce
class MyModLog extends ULoggerBase {
    // 1. Define the ID
    override static string getLogID(){ return "MyMod"; }
    
    // 2. Define the static instance variable
    static protected ref ULoggerBaseInstance m_MyModLoggerInstance;

    // 3. Implement Singleton retrieval
    override static ULoggerBaseInstance GetInstance(){
        if (!m_MyModLoggerInstance){
            CreateInstance();
            Init();
        }
        return m_MyModLoggerInstance;
    }
    
    // 4. Implement Instance creation
    override static void CreateInstance(){
        m_MyModLoggerInstance = new ULoggerBaseInstance(getLogID());
    }

    // 5. Implement Wrappers
    override static void Log(string text, int level = 1) {
        GetInstance().DoLog(text,level);
    }
    
    override static void Info(string text){
        GetInstance().DoLog(text, LOG_INFO);
    }
    
    override static void Debug(string text){
        GetInstance().DoLog(text, LOG_DEBUG);
    }

    override static void Err(string text){
        Error2("[" + getLogID() + "] Error", text);
        GetInstance().DoLog(text, LOG_ERROR);
    }

    override static void SetLogLevels(int level, int apiLevel = -99){
        if (apiLevel == -99){
            apiLevel = level;
        }
        GetInstance().SetLogLevel(level);
        GetInstance().SetApiLogLevel(apiLevel);
    }
}
```

// Usage
MyModLog.Info("Started");
MyModLog.Debug("Processing item");


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

## Best Practices

### Use Consistent Prefixes
Prefix messages with a system name or context. Example: `Inventory/Save`, `Clan/Invite`.

### Avoid Spamming
Donâ€™t log inside tight loops or per-frame callbacks unless you gate it behind `LOG_DEBUG`.

## Common Use Cases

### Startup Diagnostics
Log config load results and endpoint connectivity checks during `UFrameworkReady()`.

### Error Auditing
Log failed database transactions or Discord operations to trace outages.

## Tags
`logging`, `debug`, `diagnostics`, `server`, `client`, `troubleshooting`, `reference`, `doc-usage`, `modder`
