# Universal Framework - Mod Settings HTML Templates

## Overview

Mod Settings templates are single-page HTML files that run inside a sandboxed iframe in the UFramework Electron UI. The host injects a `window.UF` bridge script that lets your template read and write Globals stored in MongoDB.

This document covers how to build templates, the full `window.UF` API, styling conventions, security constraints, and common patterns.

---

## How Templates Are Rendered

```
┌─────────────────────────────────────────────────────────┐
│  Electron BrowserWindow  (modSettings.html + renderer)  │
│                                                         │
│  ┌─────────────────────────────────────────────────┐    │
│  │  iframe  (sandbox="allow-scripts allow-forms")  │    │
│  │                                                 │    │
│  │  Your HTML template runs here.                  │    │
│  │  window.UF is injected before </head>.          │    │
│  │                                                 │    │
│  │  UF.loadGlobal() ──► postMessage ──► IPC        │    │
│  │  UF.saveGlobal() ──► postMessage ──► IPC        │    │
│  └─────────────────────────────────────────────────┘    │
│                         ▲ postMessage ▼                 │
│  renderer.js ◄──── message handler ────► Electron IPC   │
│                                          ▼              │
│                                     MongoDB (Globals)   │
└─────────────────────────────────────────────────────────┘
```

1. The renderer fetches your template HTML from the `ModSettings` collection
2. It injects the UF bridge script before `</head>` (or `</body>`, or at the end)
3. It creates a Blob URL and loads it in a sandboxed iframe
4. Your template uses `window.UF` methods, which send `postMessage` to the parent
5. The renderer relays those messages over Electron IPC to MongoDB

---

## Template Rules

| Rule | Detail |
|------|--------|
| **Self-contained** | All CSS and JS must be inline. No external `<link>` or `<script src="...">` tags. |
| **Single file** | One `.html` file. No multi-file bundles. |
| **Max size** | 2 MB |
| **Sandboxed** | `sandbox="allow-scripts allow-forms"` — no navigation, no popups, no top-level access |
| **No network** | The iframe cannot fetch external URLs. CDN links won't work. |
| **No parent access** | `parent.document` is cross-origin and will throw |
| **Idempotent registration** | The mod should call `Register()` on every server start. The service upserts — the latest template always wins. No data is lost because settings live in the `Globals` collection, not in the template. |

---

## `window.UF` API Reference

The bridge script is injected automatically. It provides the `window.UF` namespace.

### Properties

| Property | Type | Description |
|----------|------|-------------|
| `UF.modId` | `string` | Your mod's unique identifier (set during registration) |
| `UF.globals` | `string[]` | Global names declared during registration (informational, may be empty) |

### `UF.loadGlobal(globalName)` → `Promise<object|null>`

Loads a Global's data from MongoDB.

| Parameter | Type | Description |
|-----------|------|-------------|
| `globalName` | `string` | The Global name to load (e.g. `"MyMod_Config"`) |

**Returns:** The saved data object directly, or `null` if no document exists.

```javascript
var config = await UF.loadGlobal('MyMod_Config');
if (config) {
    // config is your saved object, e.g. { difficulty: "hard", maxPlayers: 40 }
} else {
    // No saved data — use defaults
}
```

> **Note:** `loadGlobal` unwraps the MongoDB document automatically. You receive just the `data` field — not the full `{ id, mod, data }` wrapper.

### `UF.saveGlobal(globalName, data)` → `Promise<boolean>`

Saves a JSON object as the Global's `data` field. Creates the Global if it doesn't exist.

| Parameter | Type | Description |
|-----------|------|-------------|
| `globalName` | `string` | The Global name |
| `data` | `object` | The data to save (replaces the entire `data` field) |

**Returns:** `true` on success.

```javascript
await UF.saveGlobal('MyMod_Config', {
    difficulty: 'hard',
    maxPlayers: 40,
    factions: ['Survivors', 'Bandits']
});
```

### `UF.ready()`

Notifies the host that your template has finished loading. This is called automatically on `DOMContentLoaded`, so you typically don't need to call it manually.

Only call this yourself if you disable the auto-fire or need to delay readiness.

### `window.onUFReady(initData)` (Optional Callback)

If you define this function, the host calls it after it receives the `ready` signal and sends initialization data back.

```javascript
window.onUFReady = function(data) {
    console.log('Mod ID:', data.modId);       // string
    console.log('Globals:', data.globals);     // string[]
};
```

---

## Lifecycle

```
1. iframe loads your HTML
2. Bridge script auto-calls UF.ready() on DOMContentLoaded
3. Host sends { type: "modSettings:init", modId, globals } → triggers window.onUFReady()
4. Your code calls UF.loadGlobal() to fetch current settings
5. User edits form → your code calls UF.saveGlobal() to persist
```

All `loadGlobal` and `saveGlobal` calls have a **30-second timeout**. If no response is received, the Promise rejects with a timeout error.

> **Save Confirmation:** When your template calls `UF.saveGlobal()`, the operator is shown a confirmation modal before data is written. If the template fires multiple `saveGlobal()` calls in quick succession (e.g. from a "Save All" button), they are automatically **batched** into a single modal — the operator sees all globals listed with expandable JSON previews. They can inspect each one, then approve or cancel the entire batch. If cancelled, every `saveGlobal` Promise rejects with `"Save cancelled by operator"`.

---

## Styling Guide

Templates are displayed in a dark-themed Electron window. Match these conventions for a seamless look.

### Base Colors

| Element | Color | Hex |
|---------|-------|-----|
| Background | Dark grey | `#1e1e1e` |
| Surface / Cards | Slightly lighter | `#252526` |
| Input fields | Dark fill | `#2a2a2a` |
| Borders | Subtle grey | `#3a3a3a` |
| Primary text | Off-white | `#f0f0f0` |
| Secondary text | Muted | `#888888` |
| Labels | Light grey | `#aaaaaa` |
| Accent / Primary buttons | Blue | `#0e639c` |
| Accent hover | Lighter blue | `#1177bb` |
| Success | Green | `#4caf50` |
| Error | Red | `#f44336` |
| Warning | Orange | `#ff9800` |
| Info | Light blue | `#64b5f6` |

### Recommended CSS Reset

```css
*, *::before, *::after { box-sizing: border-box; }

body {
    margin: 0;
    padding: 20px;
    font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
    background-color: #1e1e1e;
    color: #f0f0f0;
    line-height: 1.5;
}
```

### Form Controls

```css
input[type="text"],
input[type="number"],
textarea,
select {
    width: 100%;
    padding: 8px 12px;
    border: 1px solid #3a3a3a;
    background-color: #2a2a2a;
    color: #f0f0f0;
    border-radius: 4px;
    font-size: 14px;
    margin-bottom: 12px;
}

input:focus, textarea:focus, select:focus {
    outline: none;
    border-color: #0e639c;
}
```

### Buttons

```css
button {
    padding: 8px 20px;
    border: none;
    border-radius: 4px;
    cursor: pointer;
    font-size: 14px;
    transition: background 0.15s;
}

/* Primary */
.btn-primary { background: #0e639c; color: #fff; }
.btn-primary:hover { background: #1177bb; }

/* Secondary */
.btn-secondary { background: #3a3d41; color: #fff; }
.btn-secondary:hover { background: #4a4d51; }

/* Danger */
.btn-danger { background: #a1260d; color: #fff; }
.btn-danger:hover { background: #c02f11; }

button:disabled { opacity: 0.5; cursor: default; }
```

### Status Bar

A reusable status bar pattern for showing success/error/info messages:

```css
.status-bar {
    padding: 8px 12px;
    border-radius: 4px;
    margin-bottom: 16px;
    font-size: 13px;
    display: none;
}
.status-bar.success { display: block; background: #1b3a1b; color: #4caf50; border: 1px solid #2e5c2e; }
.status-bar.error   { display: block; background: #3a1b1b; color: #f44336; border: 1px solid #5c2e2e; }
.status-bar.info    { display: block; background: #1b2a3a; color: #64b5f6; border: 1px solid #2e4a5c; }
```

```javascript
function showStatus(type, message) {
    var el = document.getElementById('statusBar');
    el.className = 'status-bar ' + type;
    el.textContent = message;
    if (type === 'success') {
        setTimeout(function() { el.className = 'status-bar'; }, 3000);
    }
}
```

### Labels

```css
label {
    display: block;
    font-size: 13px;
    color: #aaa;
    margin-bottom: 4px;
}
```

---

## Common Patterns

### Loading State

Show a spinner while data loads, then reveal the settings panel:

```html
<div id="loading" class="loading">Loading settings</div>
<div id="settings" style="display: none;">
    <!-- Your form -->
</div>
```

```css
.loading {
    text-align: center;
    padding: 40px;
    color: #888;
}
.loading::after {
    content: '';
    display: inline-block;
    width: 20px;
    height: 20px;
    border: 2px solid #555;
    border-top-color: #0e639c;
    border-radius: 50%;
    animation: spin 0.8s linear infinite;
    margin-left: 8px;
    vertical-align: middle;
}
@keyframes spin { to { transform: rotate(360deg); } }
```

```javascript
async function loadSettings() {
    var config = await UF.loadGlobal('MyMod_Config');
    if (config) {
        populateForm(config);
    } else {
        useDefaults();
    }
    document.getElementById('loading').style.display = 'none';
    document.getElementById('settings').style.display = 'block';
}
loadSettings();
```

### Default Config with Fallback

```javascript
var DEFAULT_CONFIG = {
    maxPlayers: 40,
    difficulty: 'normal',
    friendlyFire: false
};

async function loadSettings() {
    var config = await UF.loadGlobal('MyMod_Config');
    if (!config || typeof config !== 'object') {
        config = JSON.parse(JSON.stringify(DEFAULT_CONFIG));
        showStatus('info', 'No saved settings found. Using defaults.');
    }
    populateForm(config);
}
```

### Save with Disable + Status Feedback

```javascript
document.getElementById('saveBtn').addEventListener('click', async function() {
    var btn = document.getElementById('saveBtn');
    btn.disabled = true;
    try {
        var config = readForm();
        await UF.saveGlobal('MyMod_Config', config);
        showStatus('success', 'Settings saved!');
    } catch (err) {
        if (err.message === 'Save cancelled by operator') {
            showStatus('info', 'Save cancelled by the server operator.');
        } else {
            showStatus('error', 'Save failed: ' + err.message);
        }
    } finally {
        btn.disabled = false;
    }
});
```

### Multiple Globals

Templates can access **any** global, not just the ones declared at registration:

```javascript
// Load multiple configs
var [generalConfig, factionConfig] = await Promise.all([
    UF.loadGlobal('MyMod_General'),
    UF.loadGlobal('MyMod_Factions')
]);

// Save independently
await UF.saveGlobal('MyMod_General', generalConfig);
await UF.saveGlobal('MyMod_Factions', factionConfig);
```

### Batch Saving Multiple Globals

When a template saves multiple globals from a single user action (e.g. a "Save All" button), the UI **batches** all `saveGlobal()` calls into a single operator confirmation prompt. The operator sees every global listed with its JSON payload and can approve or cancel them all at once.

Fire all saves without awaiting each one individually — they'll be collected and shown together:

```javascript
async function saveAll() {
    try {
        await Promise.all([
            UF.saveGlobal('MyMod_General', readGeneralForm()),
            UF.saveGlobal('MyMod_Factions', readFactionsForm()),
            UF.saveGlobal('MyMod_Spawns', readSpawnsForm())
        ]);
        showStatus('success', 'All settings saved!');
    } catch (err) {
        if (err.message === 'Save cancelled by operator') {
            showStatus('info', 'Save cancelled by the server operator.');
        } else {
            showStatus('error', 'Save failed: ' + err.message);
        }
    }
}
```

> **Tip:** Using `Promise.all()` ensures all saves arrive within the batching window and appear in a single confirmation dialog. If you `await` each save sequentially, the operator may see separate prompts for each one.

### CRUD List (Add / Edit / Remove Items)

A common pattern for managing arrays of objects (factions, items, spawns, etc.):

```javascript
var config = { factions: [] };

function renderList() {
    var list = document.getElementById('factionList');
    list.innerHTML = '';
    config.factions.forEach(function(faction, index) {
        var row = document.createElement('div');
        row.className = 'list-row';
        row.innerHTML =
            '<span>' + escapeHtml(faction.name) + '</span>' +
            '<button data-index="' + index + '" class="edit-btn">Edit</button>' +
            '<button data-index="' + index + '" class="remove-btn">Remove</button>';
        list.appendChild(row);
    });

    list.querySelectorAll('.edit-btn').forEach(function(btn) {
        btn.addEventListener('click', function() {
            editItem(parseInt(btn.dataset.index));
        });
    });
    list.querySelectorAll('.remove-btn').forEach(function(btn) {
        btn.addEventListener('click', function() {
            removeItem(parseInt(btn.dataset.index));
        });
    });
}

function addItem() {
    var name = prompt('Name:');
    if (!name) return;
    config.factions.push({ name: name });
    renderList();
}

function editItem(index) {
    var faction = config.factions[index];
    var name = prompt('Name:', faction.name);
    if (name === null) return;
    faction.name = name || faction.name;
    renderList();
}

function removeItem(index) {
    if (!confirm('Remove "' + config.factions[index].name + '"?')) return;
    config.factions.splice(index, 1);
    renderList();
}
```

### HTML Escaping

Always escape user data before inserting into the DOM to prevent XSS:

```javascript
function escapeHtml(str) {
    var div = document.createElement('div');
    div.textContent = str || '';
    return div.innerHTML;
}
```

### Two-Column Layout

```css
.columns { display: flex; gap: 20px; }
.column  { flex: 1; min-width: 0; }
```

```html
<div class="columns">
    <div class="column">
        <label for="maxPlayers">Max Players</label>
        <input type="number" id="maxPlayers" min="1" max="100" value="40">
    </div>
    <div class="column">
        <label for="difficulty">Difficulty</label>
        <select id="difficulty">
            <option value="easy">Easy</option>
            <option value="normal">Normal</option>
            <option value="hard">Hard</option>
        </select>
    </div>
</div>
```

---

## Error Handling

Always wrap `loadGlobal` and `saveGlobal` in try/catch:

```javascript
try {
    var config = await UF.loadGlobal('MyMod_Config');
    // Use config...
} catch (err) {
    showStatus('error', 'Failed to load: ' + err.message);
}
```

Common error scenarios:
- **Timeout** (30s) — the IPC bridge didn't respond in time
- **Bridge unavailable** — the Electron preload didn't initialize properly
- **MongoDB error** — connection issue on the service side

---

## Do's and Don'ts

### Do
- Keep **all** CSS and JS inline in one HTML file
- Match the dark theme (`#1e1e1e` background, `#f0f0f0` text)
- Show a loading indicator while awaiting async calls
- Validate user input before saving
- Use `escapeHtml()` when inserting user data into the DOM
- Provide defaults when no saved data exists
- Handle errors gracefully with visible status messages
- Disable save buttons during save operations

### Don't
- Don't reference external CDNs, stylesheets, or scripts
- Don't try to access `parent.document` or `window.top`
- Don't use `window.open()` or link navigation
- Don't store sensitive data (templates are visible to anyone with server auth)
- Don't exceed the 2 MB size limit
- Don't assume globals exist — always handle `null` from `loadGlobal`

---

## Minimal Template

The absolute simplest working template:

```html
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <title>My Settings</title>
  <style>
    body { background: #1e1e1e; color: #f0f0f0; font-family: sans-serif; padding: 20px; }
    button { padding: 8px 16px; background: #0e639c; color: #fff; border: none; border-radius: 4px; cursor: pointer; }
    input, select { padding: 8px; background: #2a2a2a; color: #f0f0f0; border: 1px solid #3a3a3a; border-radius: 4px; }
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
    var GLOBAL = 'MyMod_Config';

    async function load() {
      try {
        var config = await UF.loadGlobal(GLOBAL);
        if (config) {
          document.getElementById('difficulty').value = config.difficulty || 'normal';
        }
      } catch (e) {
        document.getElementById('status').textContent = 'Error: ' + e.message;
      }
    }

    document.getElementById('saveBtn').addEventListener('click', async function() {
      try {
        await UF.saveGlobal(GLOBAL, {
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

---

## Full Example

A complete "Factions Mod" example is available at:
`UFServerServiceV2/data/example-mod-settings-template.html`

It demonstrates:
- Loading and saving a complex config with nested arrays
- CRUD operations on a list of factions (add / edit / remove)
- Two-column layouts with diverse form controls (text, number, select, checkbox)
- Status bar with auto-dismiss success messages
- Loading spinner
- Default config fallback
- HTML escaping for user data

---

## Related

- [Mod Settings SDK](UniversalFramework_ModSettings.md) — Enforce Script `Register()` API
- [Global Handler](UniversalFramework_GlobalHandler.md) — Loading and saving globals from Enforce Script
- [Callbacks](UniversalFramework_Callbacks.md) — Callback patterns and best practices
