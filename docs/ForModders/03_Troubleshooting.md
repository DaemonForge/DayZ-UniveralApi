# Universal Framework - Troubleshooting & Debugging

## Overview
This guide focuses on troubleshooting issues from the **DayZ Mod implementation** perspective. If you are a server admin looking for service errors (e.g., MongoDB crash), see the Operator Troubleshooting guide.

---

## 1. Quick Health Check

If your mod functionality isn't working, first verify the framework itself is healthy.
Add this print to your `UFrameworkReady()` override:

```enforce
override void UFrameworkReady()
{
    super.UFrameworkReady();
    
    Print("[MyMod] Checking Status:");
    Print("  IsOnline: " + UF().IsOnline());
    Print("  HasAuth: " + UF().HasValidAuth());
    Print("  ServerID: " + UF().GetServerID());
}
```

### Interpretation
*   **IsOnline = True**: The script has successfully handshaked with the `UFServerService`.
*   **IsOnline = False**: The RestAPI connection failed.
    *   *Cause*: Service is not running.
    *   *Cause*: `UFramework.json` URL is incorrect.
    *   *Cause*: Firewall is blocking port (usually 80, 443, or 3000).
*   **HasAuth = False**: Connection exists, but authentication failed.
    *   *Server*: `ServerAuth` key in `.json` doesn't match `config.json` in service.
    *   *Client*: Server failed to send token to client (RPC issue).

---

## 2. Common Error Scenarios

### "My Callback Never Fires"
You call `UF().db().Load(...)`, but your function `OnLoaded` is never executed.

**Likely Causes:**
1.  **Static Function Used**: Callbacks CANNOT be static methods.
    *   **WRONG:** `static void OnLoaded(...) { }`
    *   **CORRECT:** `void OnLoaded(...) { }` (instance method)
    *   *Fix*: Remove `static` keyword and pass `this` as the callback instance.
2.  **Garbage Collections**: Did you use a class instance that was deleted?
    *   *Fix*: Ensure the class passing `this` stays alive (e.g., `MissionServer` is safe, a temporary funtion variable is not).
3.  **Function Name Typo**: The string name must match EXACTLY.
    *   `UF().db().Load(..., this, "OnLoaded")` vs `void OnLoad(...)`.
4.  **Signature Mismatch**: The callback MUST have `int cid, int status, string oid, T data`.
5.  **Runtime Error in Callback**: If your callback crashes DayZ (null pointer), the log usually truncates before printing the error.
    *   *Fix*: Add `Print("Callback started");` at the very top of your callback.

### "I get UF_UNAUTHORIZED (401)"
*   **On Server**: Your API Key is wrong. Check `$profile:UF/UFramework.json`.
*   **On Client**: The player's session token expired or wasn't received.
    *   *Fix*: Clients usually auto-request tokens. If persistent, restart the local game client.

### "I get UF_EMPTY (204) when I expect data"
This is **not an error**. It means "Success, but no data found".
*   If you tried to LOAD a player who hasn't played before, this is normal.
*   Your code should handle this: `if (status == UF_EMPTY) { CreateNewData(); }`.

## Tags
`modder`, `troubleshooting`, `debugging`, `status-codes`, `auth`, `connectivity`, `how-to`, `doc-usage`

### "I get UF_ERROR (418 or 500)"
*   Check the **Service Logs** (`%APPDATA%\ufserverservice\logs` or `/var/lib/ufserverservice/logs`).
*   The script received a generic error because the backend crashed or threw an exception (e.g., MongoDB connection lost).

---

## 3. Debugging Tools

### 1. Enable Verbose Logging
In your `Init.c` or strictly for testing:

```enforce
UFLog.SetLogLevels(UFLog.LOG_DEBUG);
```

This floods your `script.log` or RPT with `[UF] [Debug]` messages, showing every HTTP request and response.

**Look for:**
*   `[UF] [Api] POST http://...` (The outgoing request)
*   `[UF] [Api] Response: ...` (The incoming JSON)

### 2. Manual HTTP Test
If you suspect the backend is broken, use a browser or tool like Postman to hit the URL found in your config.
*   `GET https://your-server-url/Public/Ping`
*   Should reply: `{"status":"online"}`.

### 3. Print-Debugging Callbacks
Always safeguard your data parsing:

```enforce
void OnDataReceived(int cid, int status, string oid, string data)
{
    Print("DEBUG: Status=" + status + " Data=" + data); // <--- Add this!
    
    // Validate before casting
    if (!data || data == "") return;
    
    // ... logic ...
}
```

---

## 4. Known Enforce Script Quirks with UF
*   **Ref vs Autoptr**: UFramework uses `autoptr` heavily. If you pass `ref` objects into a system expecting `autoptr`, they might get double-deleted.
*   **Typename Mismatch**: If you use `UDBHandler<MyType>`, ensure `MyType` has a default constructor `void MyType() {}`.
