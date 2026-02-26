# Universal Framework - Mod Settings

## Overview

The Mod Settings endpoint (`UFModSettingsEndpoint`) lets mods register custom HTML settings pages with the UF Service. Server operators can then view and interact with these editors through the Electron UI under **Options → Mod Settings**.

This is ideal for mods that store configuration in Globals and want to give operators a friendly UI instead of hand-editing JSON.

> **Idempotent:** `Register()` is an upsert. Calling it with the same `modId` overwrites the previous template and metadata. Mods **should** call `Register()` on every server start to ensure the latest template version is deployed. This is safe and expected — DayZ mods don't persist registration state across restarts.

## Use Cases

- Visual configuration editors for mod settings
- Item/faction/spawn management UIs
- Economy tuning dashboards
- Any mod that stores config in Globals

## Accessing the Endpoint

```enforce
UFModSettingsEndpoint modSettings = U().Settings();
```

## Permissions

| Operation | Server | Player (Client) |
|-----------|--------|-----------------|
| Register  | [YES]  | ❌               |

> **Note:** Registration is server-only. Templates are rendered in the operator's Electron UI.
>
> **Idempotent:** `Register()` performs an upsert — calling it with the same `modId` replaces the previous template and metadata. Call it on every server start.

---

## Register Operations

### Minimal — Fire-and-Forget

Only `modId` and the HTML template string are required:

```enforce
string html = "<html><body><h1>My Settings</h1>...</body></html>";
U().Settings().Register("my-mod", html);
```

### With Display Name

```enforce
U().Settings().Register("my-mod", "My Awesome Mod", html);
```

### With Instance Callback

```enforce
U().Settings().Register("my-mod", "My Awesome Mod", html, this, "OnRegistered");

void OnRegistered(int cid, int status, string oid, string data) {
    if (status == UF_SUCCESS) {
        Print("[MyMod] Settings page registered!");
    }
}
```

### With UFCallbackBase Callback

```enforce
U().Settings().Register("my-mod", "My Awesome Mod", html, new MyCallback());
```

### Full Metadata — With Author and Globals

```enforce
autoptr TStringArray globals = new TStringArray;
globals.Insert("MyMod_Config");
globals.Insert("MyMod_Factions");

U().Settings().Register("my-mod", "My Awesome Mod", "AuthorName", html, globals);
```

### Full Metadata + Instance Callback

```enforce
U().Settings().Register("my-mod", "My Awesome Mod", "AuthorName", html, globals, this, "OnRegistered");

void OnRegistered(int cid, int status, string oid, string data) {
    if (status == UF_SUCCESS) {
        Print("[MyMod] Settings page registered!");
    }
}
```

### Full Metadata + UFCallbackBase Callback

```enforce
U().Settings().Register("my-mod", "My Awesome Mod", "AuthorName", html, globals, new MyCallback());
```

---

## Method Signatures

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

---

## Parameters

### Required

| Parameter | Type | Description |
|-----------|------|-------------|
| `modId` | `string` | Unique identifier (alphanumeric, hyphens, underscores, 1-64 chars) |
| `tmpl` | `string` | Full HTML template string (single page with inline CSS & JS) |

### Optional

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `modName` | `string` | `modId` | Human-readable display name shown in the sidebar |
| `author` | `string` | `""` | Author name displayed as a tag in the UI header |
| `globals` | `TStringArray` | `NULL` | Informational list of Global names (populates `UF.globals` in the template) |
| `cbInstance` | `Class` | — | Object to receive the callback |
| `cbFunction` | `string` | — | Method name on cbInstance to call |
| `cb` | `UFCallbackBase` | — | Callback object |

> **Note:** Templates can load/save **any** global via `UF.loadGlobal()` / `UF.saveGlobal()` regardless of what's listed in `globals`. The `globals` array is purely informational — it populates `UF.globals` inside the template for convenience.

### Return Value

All overloads return an `int` call ID, or `-1` on error (missing modId or template).

### Callback Signature

```enforce
void OnRegistered(int cid, int status, string oid, string data)
```

| Parameter | Type | Description |
|-----------|------|-------------|
| `cid` | `int` | Call ID matching the return value |
| `status` | `int` | HTTP status code (`200` on success) |
| `oid` | `string` | The `modId` that was registered |
| `data` | `string` | Raw response body |

---

## Payload Object

`UFModSettingsPayload` is the data transfer object sent to the service:

```enforce
class UFModSettingsPayload extends UFObject_Base {
    string modName;
    string author;
    string template;
    autoptr array<string> globals;
    
    override string ToJson() {
        return UJSONHandler<UFModSettingsPayload>.ToString(this);
    }
}
```

You don't need to create this directly — the `Register()` overloads build it internally.

---

## Template API (`window.UF`)

When the template is rendered in the Electron UI, the host injects a `window.UF` bridge with these APIs:

### Properties

| Property | Type | Description |
|----------|------|-------------|
| `UF.modId` | `string` | Your mod's unique identifier |
| `UF.globals` | `string[]` | Array of global names declared during registration (may be empty) |

### Methods

| Method | Returns | Description |
|--------|---------|-------------|
| `UF.loadGlobal(name)` | `Promise<object\|null>` | Load a global's data from MongoDB |
| `UF.saveGlobal(name, data)` | `Promise<boolean>` | Save data to a global (operator confirmation required) |
| `UF.ready()` | `void` | Notify host that template finished loading (called automatically on DOM load) |

### Example Template Usage

```javascript
// Load settings
var config = await UF.loadGlobal('MyMod_Config');
if (config) {
    document.getElementById('difficulty').value = config.difficulty || 'normal';
}

// Save settings (operator will see a confirmation prompt)
await UF.saveGlobal('MyMod_Config', {
    difficulty: document.getElementById('difficulty').value,
    maxPlayers: 40
});
```

> **Save Confirmation:** Every `saveGlobal()` call shows a confirmation modal to the operator before writing. If multiple saves fire at once (e.g. a "Save All" button), they are batched into a single prompt. If cancelled, the Promise rejects with `"Save cancelled by operator"`.

### `window.onUFReady(initData)` (Optional)

Define this function to receive initialization data after the template reports ready:

```javascript
window.onUFReady = function(data) {
    console.log('Mod ID:', data.modId);
    console.log('Globals:', data.globals);
};
```

---

## Template Guidelines

- Keep all CSS and JS **inline** in the HTML file
- Use a dark theme (background `#1e1e1e`, text `#f0f0f0`) to match the host UI
- Show loading states while waiting for async operations
- Handle errors gracefully and display user-friendly messages
- Validate user input before saving
- Stay under the **2 MB** template size limit
- Don't use external CDN links (iframe is sandboxed, no network access)
- Don't try to access `parent.document` (cross-origin restriction)
- Don't use `window.open()` or navigation (sandbox blocks it)

---

## Complete Example

### Enforce Script (Server Init)

```enforce
modded class MissionServer {
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
                autoptr TStringArray globals = new TStringArray;
                globals.Insert("MyMod_Config");
                
                U().Settings().Register(
                    "my-mod",
                    "My Awesome Mod",
                    "AuthorName",
                    html,
                    globals,
                    this,
                    "OnSettingsRegistered"
                );
            }
        }
    }
    
    void OnSettingsRegistered(int cid, int status, string oid, string data) {
        if (status == UF_SUCCESS) {
            Print("[MyMod] Settings page registered!");
        } else {
            Print("[MyMod] Failed to register settings. Status: " + status);
        }
    }
}
```

### HTML Template (settings.html)

```html
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>My Mod Settings</title>
    <style>
        body { background: #1e1e1e; color: #f0f0f0; font-family: 'Segoe UI', sans-serif; padding: 20px; }
        button { padding: 8px 16px; background: #0e639c; color: #fff; border: none; border-radius: 4px; cursor: pointer; }
        input, select { padding: 8px; background: #2a2a2a; color: #f0f0f0; border: 1px solid #3a3a3a; border-radius: 4px; }
    </style>
</head>
<body>
    <h1>My Mod Settings</h1>
    <div id="loading">Loading...</div>
    <div id="settings" style="display:none;">
        <label>Difficulty:</label>
        <select id="difficulty">
            <option value="easy">Easy</option>
            <option value="normal">Normal</option>
            <option value="hard">Hard</option>
        </select>
        <br><br>
        <button id="saveBtn">Save</button>
        <p id="status"></p>
    </div>

    <script>
        var GLOBAL_NAME = 'MyMod_Config';

        async function load() {
            try {
                var config = await UF.loadGlobal(GLOBAL_NAME);
                if (config) {
                    document.getElementById('difficulty').value = config.difficulty || 'normal';
                }
            } catch (e) {
                document.getElementById('status').textContent = 'Error: ' + e.message;
            }
            document.getElementById('loading').style.display = 'none';
            document.getElementById('settings').style.display = 'block';
        }

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

---

## Related

- [Global Handler](UniversalFramework_GlobalHandler.md) - Loading and saving globals from Enforce Script
- [Callbacks](UniversalFramework_Callbacks.md) - Callback patterns and best practices
- [Best Practices](UniversalFramework_BestPractices.md) - Initialization, `UFrameworkReady`, patterns
