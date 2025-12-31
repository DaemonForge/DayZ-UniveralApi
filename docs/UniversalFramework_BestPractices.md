# Universal Framework - Best Practices

This guide covers recommended patterns, initialization sequences, and performance optimization strategies for Universal Framework.

---

## Initialization & UFrameworkReady

The framework provides a `UFrameworkReady()` callback that fires when authentication is complete and the API is ready to use.

### Server-Side Initialization

```enforce
modded class MissionServer
{
    override void UFrameworkReady(){
        super.UFrameworkReady(); // Always call super first!
        
        // Safe to make API calls here - server auth token is ready
        Print("[MyMod] Server UFrameworkReady - Starting data sync");
        
        // Example: Load global config
        U().globals().Load("MyMod", "serverConfig", this, "OnConfigLoaded");
        
        // Example: Start scheduled tasks
        U().Cron().runEndless(300, this, "PeriodicSync", NULL); // Every 5 minutes
    }
}
```

### Client-Side Initialization

```enforce
modded class MissionGameplay
{
    override void UFrameworkReady(){
        super.UFrameworkReady(); // Always call super first!
        
        // Safe to make API calls here - player auth token is ready
        Print("[MyMod] Client UFrameworkReady - Player authenticated");
        
        // Example: Load player-specific data
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (player && player.GetIdentity()){
            string guid = player.GetIdentity().GetId();
            U().db(PLAYER_DB).Load("MyMod", guid, this, "OnPlayerDataLoaded");
        }
    }
}
```

### Shared Initialization (Both Client & Server)

```enforce
modded class MissionBase
{
    override void UFrameworkReady(){
        super.UFrameworkReady(); // Always call super first!
        
        // This runs on BOTH client and server
        // Use context checks for side-specific logic
        
        if (GetGame().IsServer()){
            // Server-only initialization
        } else {
            // Client-only initialization  
        }
    }
}
```


### Key Rules

1. **Always call `super.UFrameworkReady()`** - Ensures proper initialization chain
2. **Don't make API calls before `UFrameworkReady()`** - Auth tokens won't be ready
3. **Check `GetGame().IsServer()` / `GetGame().IsClient()`** - Code runs in both contexts

---

## Offloading Work to Clients

One of the most powerful optimization strategies is **letting clients make their own database calls** instead of routing everything through the server. This dramatically reduces server load.

### The Problem: Server as Bottleneck

```enforce
// ❌ BAD: Server fetches data for every player request
// Server handles 100 players × 10 requests each = 1000 requests through server

modded class MissionServer 
{
    void OnPlayerRequestData(PlayerIdentity identity, string dataId){
        // Server makes the call, then has to RPC result back
        U().db().Load("MyMod", dataId, this, "OnDataLoaded");
        // ... then send to player via RPC
    }
}
```

### The Solution: Direct Client Database Access

```enforce
// ✅ GOOD: Each client fetches their own data directly
// Server handles 0 requests - clients talk directly to API

modded class MissionGameplay
{
    override void UFrameworkReady(){
        super.UFrameworkReady();
        
        // Client makes direct API call - no server involvement
        U().db().Load("MyMod", "publicData", this, "OnDataLoaded");
    }
    
    void OnDataLoaded(int cid, int status, string oid, string data){
        if (status == UF_SUCCESS){
            // Client processes data locally
        }
    }
}
```

### Permission Model for Client Access

| Database | Client Can Read | Client Can Write | Notes |
|----------|-----------------|------------------|-------|
| `OBJECT_DB` | ✅ All data | ❌ No | Public data store |
| `PLAYER_DB` | ✅ Own data only | ❌ No | Player-specific data |
| `globals()` | ✅ All data | ❌ No | Server config/state |

### When to Use Client-Side Calls

**Good candidates for client-side:**
- Reading public configuration (shop prices, game rules)
- Reading own player stats/profile
- Loading cosmetic data (skins, textures)
- Fetching leaderboards or public data
- AI chat conversations (player's own session)

**Keep on server-side:**
- Any write operations (Save, Transaction, Update)
- Cross-player data access
- Sensitive game logic
- Admin/privileged operations

### Example: Shop System Optimization

```enforce
// Client loads shop prices directly - server never touched
modded class ShopMenu 
{
    void LoadShopPrices(){
        // Client makes direct call to API
        U().db().Load("ShopMod", "prices", this, "OnPricesLoaded");
    }
    
    void OnPricesLoaded(int cid, int status, string oid, string data){
        if (status == UF_SUCCESS){
            // Parse and display prices locally
            ShopPrices prices = UJSONHandler<ShopPrices>.FromString(data);
            DisplayPrices(prices);
        }
    }
}

// When player buys - THAT goes through server for validation
// Server validates, deducts money, gives item
```

---

## Database Collection Usage

### OBJECT_DB vs PLAYER_DB

```enforce
// OBJECT_DB - General data accessible by all clients
// Good for: shared configs, public data, item definitions
U().db(OBJECT_DB).Load("MyMod", "globalConfig", this, "OnLoaded");
U().db().Load("MyMod", "globalConfig", this, "OnLoaded"); // Same as above (default)

// PLAYER_DB - Player-specific data, client can only read their own
// Good for: player profiles, stats, inventories (read-only from client)
U().db(PLAYER_DB).Load("MyMod", playerGuid, this, "OnLoaded");
```

### Globals Endpoint

```enforce
// globals() - Server-wide key-value store
// Clients can read, only server can write
U().globals().Load("MyMod", "serverStatus", this, "OnLoaded");

// Server-only write:
if (GetGame().IsServer()){
    U().globals().Save("MyMod", "serverStatus", "{\"online\":true}");
}
```

---

## Callback Patterns

### Basic Callback

```enforce
void LoadData(){
    U().db().Load("MyMod", "data123", this, "OnDataLoaded");
}

void OnDataLoaded(int cid, int status, string oid, string data){
    if (status == UF_SUCCESS){
        MyData obj = UJSONHandler<MyData>.FromString(data);
        ProcessData(obj);
    } else if (status == UF_EMPTY){
        // Record doesn't exist - create defaults
        CreateDefaults();
    } else {
        Print("[MyMod] Failed to load: " + UFStatusMessage(status));
    }
}
```

### Typed Handler Pattern

```enforce
// Create a static handler instance for your mod's data type
static autoptr UDBHandler<MyDataClass> g_MyDataHandler = new UDBHandler<MyDataClass>("MyMod", PLAYER_DB);

class MyModComponent
{
    void LoadPlayerData(string guid){
        g_MyDataHandler.Load(guid, this, "OnPlayerDataLoaded");
    }
    
    // Callback receives the typed data directly
    protected void OnPlayerDataLoaded(int cid, int status, string guid, MyDataClass data){
        if (status == UF_SUCCESS){
            Print("Loaded: " + data.someField);
            ProcessData(data);
        } else if (status == UF_EMPTY){
            // Record doesn't exist - create defaults
            data = new MyDataClass();
            data.someField = "default";
            g_MyDataHandler.Save(guid, data, this, "OnDataSaved");
        }
    }
    
    protected void OnDataSaved(int cid, int status, string guid, MyDataClass data){
        if (status == UF_SUCCESS){
            Print("Data saved successfully");
        }
    }
}
```

> **Note:** `UDBHandler<T>` uses callback functions (class instance + method name) for async 
> database operations. The callback receives the typed object directly.

---

## Scheduled Tasks with CronManager

### Method Signatures

```enforce
// All CronManager methods - note the EXACT parameter types:

// Run forever at interval (seconds)
void runEndless(int freqSeconds, Class obj, string fnName, Param params = NULL);

// Run at interval until Unix timestamp is reached
void runEndTime(int freqSeconds, int endCallUnix, Class obj, string fnName, Param params = NULL);

// Run at interval for exactly N executions
void runEndCount(int freqSeconds, int maxCount, Class obj, string fnName, Param params = NULL);

// Run once at specific Unix timestamp
void runOnce(int nextRunUnix, Class obj, string fnName, Param params = NULL);

// Remove a scheduled task
void Remove(Class obj, string fnName);
```

> **CRITICAL:** `runEndTime` and `runOnce` use **absolute Unix timestamps**, not relative seconds!
> Use `UUtil.GetUnixInt()` to get current time, then add seconds for future times.

### Usage Examples

```enforce
modded class MissionServer
{
    override void UFrameworkReady(){
        super.UFrameworkReady();
        
        // runEndless: Run every 60 seconds, forever
        // Params: (int freqSeconds, Class obj, string fnName, Param params)
        U().Cron().runEndless(60, this, "PeriodicTask", NULL);
        
        // runEndTime: Run every 30 seconds until Unix time is reached
        // Params: (int freqSeconds, int endCallUnix, Class obj, string fnName, Param params)
        int oneHourFromNow = UUtil.GetUnixInt() + 3600;  // Current time + 3600 seconds
        U().Cron().runEndTime(30, oneHourFromNow, this, "HourlyTask", NULL);
        
        // runEndCount: Run every 10 seconds, exactly 5 times total
        // Params: (int freqSeconds, int maxCount, Class obj, string fnName, Param params)
        U().Cron().runEndCount(10, 5, this, "LimitedTask", NULL);
        
        // runOnce: Run once at specific Unix timestamp
        // Params: (int nextRunUnix, Class obj, string fnName, Param params)
        int twoMinutesFromNow = UUtil.GetUnixInt() + 120;
        U().Cron().runOnce(twoMinutesFromNow, this, "DelayedStartup", NULL);
    }
    
    void PeriodicTask(){
        // Sync data, cleanup, etc.
    }
    
    void HourlyTask(){
        // Runs every 30 sec for 1 hour
    }
    
    void LimitedTask(){
        // Runs 5 times total
    }
    
    void DelayedStartup(){
        // Runs once after ~2 minutes
    }
}
```

---

## Server vs Client Context Checks

```enforce
// Check execution context before making calls
if (GetGame().IsServer()){
    // Server-only code
    U().db().Save("MyMod", "data", jsonString); // Writes require server auth
    U().ds().AddRole(playerGuid, "roleId");     // Discord operations
}

if (GetGame().IsClient()){
    // Client-only code  
    U().db().Load("MyMod", "publicData", this, "OnLoaded"); // Reads work
    // UI updates, local effects
}

// Alternative syntax
if (g_Game.IsServer()){
    // Server code
}
```

---

## Memory Management

### Use autoptr for Handlers

```enforce
// ✅ GOOD: autoptr automatically cleans up
autoptr MyHandler handler = new MyHandler();
handler.Load();

// ❌ BAD: Memory leak - no cleanup
MyHandler handler = new MyHandler();
handler.Load();
```

### Clean Up Cron Tasks

```enforce
override void OnMissionFinish(){
    super.OnMissionFinish();
    
    // Remove scheduled tasks to prevent memory leaks
    U().Cron().Remove(this, "PeriodicTask");
    U().Cron().Remove(this, "OtherTask");
}
```

---

## Callback ID Tracking & Cancellation

API calls return a **callback ID (cid)** that you can use to track and cancel pending requests. This is critical when the object making the request might be deleted before the callback returns.

### Why Track Callback IDs?

If an object is deleted while an API call is pending, the callback will try to invoke a method on a deleted object → **crash**.

```enforce
// ❌ DANGEROUS: Object deleted before callback returns
class MyTemporaryUI {
    void LoadData(){
        U().db().Load("MyMod", "data", this, "OnLoaded"); // Pending...
    }
    // If UI is closed/deleted before OnLoaded fires → CRASH
}
```

### Tracking Pending Calls

```enforce
class MySafeHandler {
    protected ref array<int> m_PendingCalls = new array<int>;
    
    void LoadData(){
        int cid = U().db().Load("MyMod", "data", this, "OnLoaded");
        if (cid != -1){
            m_PendingCalls.Insert(cid);
        }
    }
    
    void LoadMoreData(){
        int cid = U().db().Load("MyMod", "moreData", this, "OnMoreLoaded");
        if (cid != -1){
            m_PendingCalls.Insert(cid);
        }
    }
    
    void OnLoaded(int cid, int status, string oid, string data){
        // Remove from pending list
        int idx = m_PendingCalls.Find(cid);
        if (idx != -1) m_PendingCalls.Remove(idx);
        
        // Process data...
    }
    
    void OnMoreLoaded(int cid, int status, string oid, string data){
        int idx = m_PendingCalls.Find(cid);
        if (idx != -1) m_PendingCalls.Remove(idx);
        
        // Process data...
    }
    
    void ~MySafeHandler(){
        // Cancel all pending calls on destruction
        CancelPendingCalls();
    }
    
    void CancelPendingCalls(){
        foreach (int cid : m_PendingCalls){
            U().RequestCallCancel(cid);
        }
        m_PendingCalls.Clear();
    }
}
```

### Entity with Pending Calls

```enforce
modded class ItemBase {
    protected ref array<int> m_UFPendingCalls = new array<int>;
    
    void LoadItemData(){
        int cid = U().db().Load("MyMod", GetUFOID(), this, "OnItemDataLoaded");
        if (cid != -1){
            m_UFPendingCalls.Insert(cid);
        }
    }
    
    void OnItemDataLoaded(int cid, int status, string oid, string data){
        int idx = m_UFPendingCalls.Find(cid);
        if (idx != -1) m_UFPendingCalls.Remove(idx);
        
        if (status == UF_SUCCESS){
            // Apply data to item
        }
    }
    
    override void EEDelete(EntityAI parent){
        // Cancel pending API calls before deletion
        foreach (int cid : m_UFPendingCalls){
            U().RequestCallCancel(cid);
        }
        m_UFPendingCalls.Clear();
        
        super.EEDelete(parent);
    }
}
```

---

## Cron Cleanup for Entities & Objects

When an entity or object is deleted, you **must** remove any cron jobs registered to it. Otherwise the CronManager will try to call methods on deleted objects → **crash**.

### Entity Cron Cleanup Pattern

```enforce
modded class ItemBase {
    protected bool m_HasCronJobs = false;
    
    void StartPeriodicSync(){
        U().Cron().runEndless(30, this, "SyncItemData", NULL);
        m_HasCronJobs = true;
    }
    
    void SyncItemData(){
        // Periodic sync logic
    }
    
    override void EEDelete(EntityAI parent){
        // CRITICAL: Remove cron jobs before deletion
        if (m_HasCronJobs){
            U().Cron().Remove(this, "SyncItemData");
            m_HasCronJobs = false;
        }
        
        super.EEDelete(parent);
    }
}
```

### Player Cron Cleanup Pattern

```enforce
modded class PlayerBase {
    protected bool m_UFCronActive = false;
    
    void StartPlayerSync(){
        if (!m_UFCronActive){
            U().Cron().runEndless(60, this, "PeriodicPlayerSync", NULL);
            m_UFCronActive = true;
        }
    }
    
    void PeriodicPlayerSync(){
        // Sync player data
    }
    
    override void EEKilled(Object killer){
        CleanupCronJobs();
        super.EEKilled(killer);
    }
    
    // Note: OnDisconnect() is NOT an override - it's called via InvokeOnDisconnect()
    // from MissionServer.PlayerDisconnected(). To hook into disconnect, you can
    // either override this method (which shadows the parent) or use EEDelete.
    void OnDisconnect(){
        CleanupCronJobs();
        super.OnDisconnect();
    }
    
    void CleanupCronJobs(){
        if (m_UFCronActive){
            U().Cron().Remove(this, "PeriodicPlayerSync");
            m_UFCronActive = false;
        }
    }
}
```

### UI/Menu Cron Cleanup

```enforce
class MyCustomMenu extends UIScriptedMenu {
    protected bool m_RefreshActive = false;
    
    override void OnShow(){
        super.OnShow();
        // Start periodic refresh while menu is open
        U().Cron().runEndless(5, this, "RefreshData", NULL);
        m_RefreshActive = true;
    }
    
    void RefreshData(){
        U().db().Load("MyMod", "liveData", this, "OnRefreshLoaded");
    }
    
    override void OnHide(){
        // CRITICAL: Stop cron when menu closes
        if (m_RefreshActive){
            U().Cron().Remove(this, "RefreshData");
            m_RefreshActive = false;
        }
        super.OnHide();
    }
    
    void ~MyCustomMenu(){
        // Safety net - also clean up in destructor
        if (m_RefreshActive){
            U().Cron().Remove(this, "RefreshData");
        }
    }
}
```

### Mission Cleanup Pattern

```enforce
modded class MissionServer {
    override void UFrameworkReady(){
        super.UFrameworkReady();
        
        U().Cron().runEndless(60, this, "SyncGlobalData", NULL);
        U().Cron().runEndless(300, this, "CleanupOldRecords", NULL);
    }
    
    override void OnMissionFinish(){
        // Clean up ALL cron jobs registered to this mission
        U().Cron().Remove(this, "SyncGlobalData");
        U().Cron().Remove(this, "CleanupOldRecords");
        
        super.OnMissionFinish();
    }
}
```

---

## Complete Cleanup Example

Here's a complete pattern showing proper cleanup of both API calls and cron jobs:

```enforce
class MyManagedObject {
    protected ref array<int> m_PendingCalls = new array<int>;
    protected ref array<string> m_CronMethods = new array<string>;
    
    void Initialize(){
        // Register cron job
        U().Cron().runEndless(30, this, "PeriodicUpdate", NULL);
        m_CronMethods.Insert("PeriodicUpdate");
        
        // Make initial API call
        int cid = U().db().Load("MyMod", "config", this, "OnConfigLoaded");
        if (cid != -1) m_PendingCalls.Insert(cid);
    }
    
    void PeriodicUpdate(){
        int cid = U().db().Load("MyMod", "liveData", this, "OnLiveDataLoaded");
        if (cid != -1) m_PendingCalls.Insert(cid);
    }
    
    void OnConfigLoaded(int cid, int status, string oid, string data){
        RemovePendingCall(cid);
        // Process...
    }
    
    void OnLiveDataLoaded(int cid, int status, string oid, string data){
        RemovePendingCall(cid);
        // Process...
    }
    
    protected void RemovePendingCall(int cid){
        int idx = m_PendingCalls.Find(cid);
        if (idx != -1) m_PendingCalls.Remove(idx);
    }
    
    void Cleanup(){
        // Cancel all pending API calls
        foreach (int cid : m_PendingCalls){
            U().RequestCallCancel(cid);
        }
        m_PendingCalls.Clear();
        
        // Remove all cron jobs
        foreach (string method : m_CronMethods){
            U().Cron().Remove(this, method);
        }
        m_CronMethods.Clear();
    }
    
    void ~MyManagedObject(){
        Cleanup();
    }
}
```

---

## JSON Serialization Notes

### Boolean Values

Booleans are serialized as integers (`0`/`1`) in JSON:

```enforce
class MyData {
    bool isActive;  // Saved as "isActive": 1 or "isActive": 0
}
```

When manually parsing JSON, check for both:
```enforce
// Handle bool as int from JSON
bool isActive = (jsonObj.GetInt("isActive") == 1);
```

### Use UJSONHandler for Type Safety

```enforce
// Serialize
string json = UJSONHandler<MyClass>.ToString(myObject);

// Deserialize
MyClass obj = UJSONHandler<MyClass>.FromString(json);
```

---

## Error Handling

### Always Check Status Codes

```enforce
void OnCallback(int cid, int status, string oid, string data){
    switch(status){
        case UF_SUCCESS:
            // Data loaded successfully
            ProcessData(data);
            break;
        case UF_EMPTY:
            // Record not found - may need to create
            CreateDefaultRecord();
            break;
        case UF_UNAUTHORIZED:
            // Auth failed - token may have expired
            Print("[MyMod] Auth error - requesting new token");
            break;
        case UF_SERVER_ERROR:
            // Backend issue - maybe retry later
            ScheduleRetry();
            break;
        default:
            Print("[MyMod] Unexpected status: " + status);
    }
}
```

### Use UFStatusMessage for Logging

```enforce
if (status != UF_SUCCESS){
    Print("[MyMod] Operation failed: " + UFStatusMessage(status));
}
```

---

## Performance Tips

1. **Batch related data** - Store related fields in one document instead of many small ones
2. **Cache frequently-read data** - Don't re-fetch data that rarely changes
3. **Use Transactions for counters** - Atomic increments prevent race conditions
4. **Offload to clients** - Let clients read their own data directly
5. **Use Cron instead of OnUpdate** - Scheduled tasks are more efficient than per-frame checks
6. **Clean up handlers** - Use autoptr or manually delete to prevent memory leaks

---

## Common Mistakes to Avoid

```enforce
// ❌ Making API calls before UFrameworkReady
void MissionBase(){
    U().db().Load(...); // Auth not ready yet!
}

// ❌ Forgetting super call
override void UFrameworkReady(){
    // Missing super.UFrameworkReady()!
    DoStuff();
}

// ❌ Client trying to write
if (GetGame().IsClient()){
    U().db().Save(...); // Will fail - clients can't write
}

// ❌ Not checking status codes
void OnLoaded(int cid, int status, string oid, string data){
    MyData d = UJSONHandler<MyData>.FromString(data); // Crashes if status != SUCCESS
}

// ❌ Memory leaks with handlers
MyHandler h = new MyHandler(); // No autoptr = leak

// ❌ Deleting object with pending API calls - CRASH!
class BadExample {
    void LoadData(){
        U().db().Load("Mod", "id", this, "OnLoaded"); // Pending...
    }
    // Object deleted before callback → crash when callback tries to fire
}

// ❌ Deleting object with active cron jobs - CRASH!
class AnotherBadExample {
    void Start(){
        U().Cron().runEndless(10, this, "Update", NULL);
    }
    // Object deleted but cron still tries to call Update() → crash
}

// ❌ Not removing cron in EEDelete
modded class ItemBase {
    void StartSync(){
        U().Cron().runEndless(30, this, "Sync", NULL);
    }
    // Missing cleanup in EEDelete → crash when item is deleted
}
```

### The Fix for Cleanup Issues

```enforce
// ✅ CORRECT: Track and cancel pending calls
class GoodExample {
    protected ref array<int> m_Pending = new array<int>;
    protected bool m_HasCron = false;
    
    void LoadData(){
        int cid = U().db().Load("Mod", "id", this, "OnLoaded");
        if (cid != -1) m_Pending.Insert(cid);
    }
    
    void StartCron(){
        U().Cron().runEndless(10, this, "Update", NULL);
        m_HasCron = true;
    }
    
    void ~GoodExample(){
        // Cancel pending API calls
        foreach (int cid : m_Pending){
            U().RequestCallCancel(cid);
        }
        // Remove cron jobs
        if (m_HasCron){
            U().Cron().Remove(this, "Update");
        }
    }
}

// ✅ CORRECT: Entity cleanup in EEDelete
modded class ItemBase {
    protected bool m_HasCron = false;
    
    void StartSync(){
        U().Cron().runEndless(30, this, "Sync", NULL);
        m_HasCron = true;
    }
    
    override void EEDelete(EntityAI parent){
        if (m_HasCron){
            U().Cron().Remove(this, "Sync");
            m_HasCron = false;
        }
        super.EEDelete(parent);
    }
}
```
