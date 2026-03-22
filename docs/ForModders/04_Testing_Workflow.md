# Universal Framework - Testing Workflow

## Overview
Effective testing of UFramework mods requires understanding how to simulate the Client/Server architecture locally.

---

## 1. Local Testing Setup (Single Machine)

You do **not** need a rented dedicated server to test UFramework. You can run everything on your gaming PC.

### Requirements
1.  **DayZ Tools** (from Steam) - For `DayZDiag_x64.exe`.
2.  **UFService** (Windows version) running locally.
3.  **MongoDB** running locally.

### Configuration
1.  **Service**: Ensure `config.json` listener is `http://127.0.0.1:3000`.
2.  **Mod Config**: In your DayZ Server profile, set URL to `http://127.0.0.1:3000`.

---

## 2. Testing Flow

### Step 1: Start the Backend
Start the `UFServerService` via the tray app. Ensure the icon is **Green**.

### Step 2: Start Diag Server
Launch DayZ Diag in Server mode:
`DayZDiag_x64.exe -server -config=serverDZ.cfg -profiles=ServerProfile -mod=@UniversalApi;@YourMod`

### Step 3: Start Diag Client
Launch DayZ Diag in Client mode and connect to 127.0.0.1:
`DayZDiag_x64.exe -connect=127.0.0.1 -port=2302 -mod=@UniversalApi;@YourMod`

> **Note**: You may need to bypass SSL usage in your local configuration or ignore certificate errors if not using a valid cert locally.

---

## 3. Mocking Data (Forcing States)

Sometimes you want to test "What happens if the specific player data exists?" without grinding gameplay to create it.

### Tool: Database Direct Edit
Since UF uses standard MongoDB, you can use **MongoDB Compass** (GUI) to edit data live.

1.  Open MongoDB Compass.
2.  Connect to `mongodb://localhost:27017`.
3.  Navigate to `UniversalFramework` -> `Players` (or `Objects`).
4.  Find your record (look for your GUID).
5.  Edit the `data` field JSON directly.
6.  Restart the mission (or trigger a Load call in-game).

**Example Scenario:**
*   You are writing a "Bank Mod".
*   You want to test `Withdraw(1000)` but have 0 coins.
*   **Don't** farm coins in-game.
*   **Do** go to Mongo, set `"coins": 50000`, save, then test.

---

## Tags
`modder`, `testing`, `local-dev`, `mongo`, `workflow`, `how-to`, `doc-usage`

## 4. Testing Callbacks Without a Server

If you are prototyping logic and don't want to run the full stack, you can create a "Mock" mode in your code that simulates a generic response.

```enforce
// Temporary testing code
void LoadData(string id)
{
    #ifdef DIAG_DEVELOPER
    // Simulated fake response for quick iteration
    OnLoaded(0, UF_SUCCESS, id, "{\"coins\":100}");
    return;
    #endif

    // Real call
    UF().db().Load("MyMod", id, this, "OnLoaded");
}
```

This allows you to test your UI or logic flow instantly without waiting for network calls.
