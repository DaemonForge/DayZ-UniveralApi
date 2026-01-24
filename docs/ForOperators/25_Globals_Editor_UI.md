# Globals Editor UI

The Globals Editor provides a graphical interface for viewing and editing global state variables stored in the database. Globals are key-value stores that persist data shared across server sessions.

---

## Accessing Globals Editor

1. Right-click the **UF Service** system tray icon
2. Select **Options â†’ Globals Editor**

---

## Interface Overview

The Globals Editor has a two-panel layout:

| Panel | Description |
|-------|-------------|
| **Left Sidebar** | Lists all modules (mods) with global data |
| **Main Editor** | JSON editor for the selected module's data |

---

## Understanding Globals

Globals are organized by **module** (typically your mod name). Each module has a single JSON document containing all its global state.

**Example Structure:**
```json
{
  "serverRestartTime": "2026-01-12T06:00:00Z",
  "eventActive": true,
  "leaderboard": [
    { "name": "Player1", "score": 1500 },
    { "name": "Player2", "score": 1200 }
  ],
  "economySettings": {
    "traderMultiplier": 1.5,
    "taxRate": 0.1
  }
}
```

---

## Viewing Globals

1. Select a **module** from the left sidebar
2. The module's JSON data appears in the editor
3. Use the search box to filter modules by name

**Hidden Modules:**
Some system modules (like `universalapistatus`) are hidden by default as they're for internal use.

---

## Editing Globals

1. Select the module to edit
2. Modify the JSON in the editor
   - The editor provides syntax highlighting
   - Invalid JSON is highlighted with errors
3. Click **Save** when done

**Editor Features:**
- Syntax highlighting for JSON
- Auto-indentation
- Error highlighting for invalid JSON
- Line numbers

---

## Status Indicators

| Status | Meaning |
|--------|---------|
| **Select a module** | No module selected yet |
| **Ready** | Module loaded, no changes |
| **Unsaved changes** | Changes made, not yet saved |
| **Error** | Problem loading or saving |

---

## Actions

| Button | Description |
|--------|-------------|
| **ðŸ”„ Refresh** | Reload the module list |
| **Reload** | Discard changes, reload from database |
| **Delete** | Delete the entire module's global data |
| **Save** | Save changes to the database |

---

## Deleting Globals

To delete a module's global data:

1. Select the module
2. Click **Delete**
3. Confirm deletion

> **Warning**: This permanently deletes all global data for that module. The mod may recreate default values on next use.

---

## Common Use Cases

### Server Event Management
```json
{
  "currentEvent": "double_xp",
  "eventStart": "2026-01-12T00:00:00Z",
  "eventEnd": "2026-01-13T00:00:00Z",
  "eventSettings": {
    "xpMultiplier": 2.0,
    "lootMultiplier": 1.5
  }
}
```

### Economy Configuration
```json
{
  "currencyName": "Credits",
  "startingBalance": 1000,
  "maxBalance": 1000000,
  "taxEnabled": true,
  "taxRate": 0.05
}
```

### Leaderboards
```json
{
  "topPlayers": [
    { "guid": "abc123", "name": "TopPlayer", "kills": 150 },
    { "guid": "def456", "name": "Pro", "kills": 125 }
  ],
  "lastUpdated": "2026-01-12T12:00:00Z"
}
```

---

## Using Globals in Your Mod

### From Enforce Script

```cpp
// Save global data
string json = "{\"serverMessage\":\"Welcome!\"}";
U().globals().Save("MyMod", json, this, "OnGlobalsSaved");

// Load global data
U().globals().Load("MyMod", this, "OnGlobalsLoaded");

void OnGlobalsLoaded(int cid, int status, string oid, string data) {
    if (status == 200) {
        // Parse and use the JSON data
        JsonSerializer js = new JsonSerializer();
        // ... parse data
    }
}
```

### Via REST API

```bash
# Get globals for a module
curl -X GET https://your-server:3000/Globals/MyMod \
  -H "Authorization: Bearer YOUR_AUTH_TOKEN"

# Set globals for a module
curl -X POST https://your-server:3000/Globals/MyMod \
  -H "Authorization: Bearer YOUR_AUTH_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"serverMessage": "Welcome!", "eventActive": true}'
```

---

## Best Practices

1. **Use Descriptive Keys**: `playerKillCount` not `pkc`

2. **Structure Data Logically**: Group related settings together

3. **Document Your Schema**: Keep track of what each field means

4. **Validate Before Saving**: Ensure JSON is valid and values are correct

5. **Backup Important Data**: Export critical globals before making changes

6. **Avoid Storing Large Data**: Globals are for configuration, not bulk data

---

## Troubleshooting

### Module Not Appearing
- Click **ðŸ”„ Refresh** to reload the list
- Ensure the mod has saved globals at least once
- Check that MongoDB is running

### Save Fails
- Verify JSON is valid (no syntax errors)
- Check MongoDB connection
- Review Console for detailed errors

### Data Looks Wrong
- Click **Reload** to refresh from database
- Another process may have modified the data

### Editor Not Loading
- Refresh the window
- Check Console for JavaScript errors
- Restart the UF Service

---

## Related Documentation

- [API Endpoints - Globals](11_API_Endpoints_Globals_Status.md)
- [Configuration Editor](23_Config_Editor_UI.md)
- [KB Manager](24_KB_Manager_UI.md)

## Tags
`operators`, `globals`, `ui`, `editor`, `database`, `how-to`, `doc-usage`
