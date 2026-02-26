# Mod Settings — Custom Settings Pages for DayZ Mods

## Overview

The **Mod Settings** system lets modders create custom, single-page HTML settings editors for their DayZ mods. Server operators can view and interact with these editors through the UFramework Service's Electron UI (under **Options → Mod Settings**).

This is ideal for mods that store configuration in UFramework Globals and want to give server operators a friendly UI instead of requiring them to hand-edit JSON.

### How It Works

```
┌──────────────────┐     REST POST      ┌──────────────────┐    MongoDB
│  DayZ Mod Server │ ──────────────────► │  UF Service API  │ ──► ModSettings
│  (Enforce Script)│   Register          │  (Node.js)       │    collection
└──────────────────┘                     └──────────────────┘
                                                │
                                         Electron UI reads
                                         templates & renders
                                         in sandboxed iframe
                                                │
                                                ▼
                                         ┌──────────────────┐
                                         │  Mod Settings UI  │
                                         │  (Electron Window) │
                                         │  iframe ↔ Globals │
                                         └──────────────────┘
```

1. **Your mod** registers an HTML template with the service at startup
2. **The service** stores it in the `ModSettings` MongoDB collection
3. **Server operators** open the Mod Settings window and see your editor
4. **Your template** uses `window.UF` to load/save Global data from MongoDB

---

## Quick Start

### 1. Create Your HTML Template

Create a single `.html` file with all CSS and JavaScript inline. The service will inject a `window.UF` bridge script that provides access to Globals.

```html
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <title>My Mod Settings</title>
  <style>
    body { background: #1e1e1e; color: #f0f0f0; font-family: sans-serif; padding: 20px; }
    button { padding: 8px 16px; background: #0e639c; color: #fff; border: none; border-radius: 4px; cursor: pointer; }
    input { padding: 8px; background: #2a2a2a; color: #f0f0f0; border: 1px solid #3a3a3a; border-radius: 4px; }
  </style>
</head>
<body>
  <h1>My Mod Settings</h1>
  <label>Difficulty:</label>
  <select id="difficulty">
    <option value="easy">Easy</option>
    <option value="normal">Normal</option>
    <option value="hard">Hard</option>
  </select>
  <br><br>
  <button id="saveBtn">Save</button>
  <p id="status"></p>

  <script>
    var GLOBAL_NAME = 'MyMod_Config';

    // Load settings when the page opens
    async function load() {
      try {
        var config = await UF.loadGlobal(GLOBAL_NAME);
        if (config) {
          document.getElementById('difficulty').value = config.difficulty || 'normal';
        }
      } catch (e) {
        document.getElementById('status').textContent = 'Error: ' + e.message;
      }
    }

    // Save settings when button clicked
    document.getElementById('saveBtn').addEventListener('click', async function() {
      try {
        await UF.saveGlobal(GLOBAL_NAME, {
          difficulty: document.getElementById('difficulty').value
        });
        document.getElementById('status').textContent = 'Saved!';
      } catch (e) {
        document.getElementById('status').textContent = 'Error: ' + e.message;
      }
    });

    load();
  </script>
</body>
</html>
```

### 2. Register the Template from Your Mod (Enforce Script)

In your mod's initialization code (e.g., in a `MissionServer` `OnInit` or `MissionGameplay`), register the template. Only `modId` and the HTML template are required — everything else is optional.

> **Idempotent:** `Register()` is an upsert — calling it with the same `modId` overwrites the previous template and metadata. You **should** call `Register()` on every server start so that the service always has the latest version of your settings page. This is safe and expected; DayZ mods don't persist registration across restarts.

```enforce
// Option A: Minimal — just modId + template
class MyModMission extends MissionServer {
    override void OnInit() {
        super.OnInit();
        
        if (GetGame().IsServer()) {
            string html = "<html><body><h1>My Settings</h1>...</body></html>";
            U().Settings().Register("my-mod", html);
        }
    }
}
```

```enforce
// Option B: With a display name
class MyModMission extends MissionServer {
    override void OnInit() {
        super.OnInit();
        
        if (GetGame().IsServer()) {
            string html = "<html><body><h1>My Settings</h1>...</body></html>";
            U().Settings().Register("my-mod", "My Awesome Mod", html);
        }
    }
}
```

```enforce
// Option C: Load template from a file + callback (recommended for larger templates)
class MyModMission extends MissionServer {
    override void OnInit() {
        super.OnInit();
        
        if (GetGame().IsServer()) {
            string html = "";
            FileHandle file = OpenFile("$profile:MyMod/settings.html", FileMode.READ);
            if (file) {
                string line;
                while (FGets(file, line) >= 0) {
                    html += line + "\n";
                }
                CloseFile(file);
            }
            
            if (html != "") {
                U().Settings().Register(
                    "my-mod",
                    "My Awesome Mod",
                    html,
                    this,                    // Callback instance
                    "OnSettingsRegistered"   // Callback function
                );
            }
        }
    }
    
    void OnSettingsRegistered(int cid, int status, string oid, string data) {
        if (status == 200) {
            Print("[MyMod] Settings page registered successfully!");
        } else {
            Print("[MyMod] Failed to register settings page. Status: " + status);
        }
    }
}
```

---

## SDK Reference

### `U().Settings().Register()`

Several overloads are available, from minimal to full metadata:

```enforce
// Minimal — fire-and-forget
int Register(string modId, string tmpl);

// With display name — fire-and-forget
int Register(string modId, string modName, string tmpl);

// With display name + instance callback
int Register(string modId, string modName, string tmpl, Class cbInstance, string cbFunction);

// With display name + UFCallbackBase callback
int Register(string modId, string modName, string tmpl, UFCallbackBase cb);

// Full metadata — fire-and-forget
int Register(string modId, string modName, string author, string tmpl, TStringArray globals);

// Full metadata + instance callback
int Register(string modId, string modName, string author, string tmpl, TStringArray globals, Class cbInstance, string cbFunction);

// Full metadata + UFCallbackBase callback
int Register(string modId, string modName, string author, string tmpl, TStringArray globals, UFCallbackBase cb);
```

**Required parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `modId` | `string` | Unique identifier (alphanumeric, hyphens, underscores, 1-64 chars) |
| `tmpl` | `string` | The full HTML template string |

**Optional parameters (defaults applied server-side if empty/omitted):**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `modName` | `string` | `modId` | Human-readable display name shown in the sidebar |
| `author` | `string` | `""` | Author name displayed as a tag |
| `globals` | `TStringArray` | `[]` | Informational list of Global names (passed to `UF.globals` in the template) |
| `cbInstance` | `Class` | — | Object to receive the callback |
| `cbFunction` | `string` | — | Method name on cbInstance to call |
| `cb` | `UFCallbackBase` | — | Callback object |

> **Note:** Templates can load/save **any** global via `UF.loadGlobal()` / `UF.saveGlobal()` regardless of what's listed in `globals`. The `globals` array is purely informational — it populates `UF.globals` inside the template for convenience.

**Returns:** Call ID (`int`), or `-1` on error.

> **Idempotent:** Calling `Register()` with the same `modId` performs an upsert — it creates the entry if new, or fully replaces the template and metadata if it already exists. Call it on every server start to keep the template up-to-date.

**Callback signature:**
```enforce
void OnRegistered(int cid, int status, string oid, string data)
```

---

## Template API Reference (`window.UF`)

When your HTML template is rendered in the Mod Settings UI, the service injects a bridge script that provides the `window.UF` object. This is your interface for interacting with the database.

### Properties

| Property | Type | Description |
|----------|------|-------------|
| `UF.modId` | `string` | Your mod's unique identifier |
| `UF.globals` | `string[]` | Array of global names declared during registration (informational, may be empty) |

### Methods

#### `UF.loadGlobal(globalName)` → `Promise<object|null>`

Loads the JSON data for a global from MongoDB.

**Parameters:**
- `globalName` (string) — The name of the global to load (e.g. `"MyMod_Config"`)

**Returns:** A Promise that resolves to your saved settings object, or `null` if no data has been saved yet.

**Example:**
```javascript
var config = await UF.loadGlobal('MyMod_Config');
if (config) {
    console.log('Difficulty:', config.difficulty);
} else {
    // No saved data — use defaults
}
```

---

#### `UF.saveGlobal(globalName, data)` → `Promise<boolean>`

Saves a JSON object as the global's `data` field. Creates the global if it doesn't exist.

**Parameters:**
- `globalName` (string) — The name of the global
- `data` (object) — The data to save (replaces the entire `data` field)

**Returns:** A Promise that resolves to `true` on success, or rejects if the operator cancels.

> **Operator Confirmation:** Every `saveGlobal()` call goes through a confirmation modal on the operator's side. If the template fires multiple saves (e.g. from a "Save All" button), they are batched into a single prompt — the operator sees all globals listed with expandable JSON previews and can approve or cancel the entire batch. If cancelled, the Promise rejects with `"Save cancelled by operator"`.

**Example:**
```javascript
await UF.saveGlobal('MyMod_Config', {
    difficulty: 'hard',
    maxPlayers: 40,
    factions: ['Survivors', 'Bandits']
});
```

---

#### `UF.ready()`

Notifies the host that the template has finished loading. This is called automatically when the DOM loads, but you can call it manually if needed.

---

#### `window.onUFReady(initData)` (optional callback)

If you define this function, it will be called when the host sends initialization data after the template reports ready.

```javascript
window.onUFReady = function(data) {
    console.log('Mod ID:', data.modId);
    console.log('Globals:', data.globals);
};
```

---

## Template Guidelines

### Do's
- Keep all CSS and JS **inline** in the HTML file
- Use a dark theme (background `#1e1e1e`, text `#f0f0f0`) to match the host UI
- Show loading states while waiting for `UF.loadGlobal()` / `UF.saveGlobal()`
- Handle errors gracefully and show user-friendly messages
- Validate user input before saving
- Stay under the **2MB** template size limit

### Don'ts
- Don't use external CDN links (the iframe is sandboxed, network access is restricted)
- Don't try to access `parent.document` (cross-origin, will throw errors)
- Don't use `window.open()` or navigation (iframe sandbox blocks it)
- Don't store sensitive data (templates are visible to anyone with server auth)

### Recommended Template Structure

```html
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Your Mod Name</title>
    <style>
        /* Dark theme styles matching the host UI */
        body { background: #1e1e1e; color: #f0f0f0; font-family: 'Segoe UI', sans-serif; padding: 20px; }
        /* ... your styles ... */
    </style>
</head>
<body>
    <!-- Your settings UI -->
    <div id="app">
        <div id="loading">Loading...</div>
        <div id="settings" style="display:none;">
            <!-- Form fields, tables, etc. -->
        </div>
    </div>

    <script>
        // 1. Define your global name(s)
        var CONFIG_GLOBAL = 'MyMod_Config';

        // 2. Load settings
        async function loadSettings() {
            var config = await UF.loadGlobal(CONFIG_GLOBAL);
            if (config) {
                // Populate your form from config
            }
            document.getElementById('loading').style.display = 'none';
            document.getElementById('settings').style.display = 'block';
        }

        // 3. Save settings
        async function saveSettings() {
            var config = { /* read from your form */ };
            await UF.saveGlobal(CONFIG_GLOBAL, config);
        }

        // 4. Init
        loadSettings();
    </script>
</body>
</html>
```

---

## Example Template

A full working example template is available at:
`UFServerServiceV2/data/example-mod-settings-template.html`

This demonstrates:
- Loading and saving a complex config with nested arrays
- CRUD operations on a list of factions (add/edit/remove)
- Dark theme styling matching the host UI
- Form controls (text, number, select, checkbox)
- Status messages and loading states
- Error handling
