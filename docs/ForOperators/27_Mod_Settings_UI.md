# Mod Settings UI

The Mod Settings window provides a graphical interface for server operators to view and interact with custom settings pages registered by DayZ mods. Mods upload single-page HTML templates via the Enforce Script SDK, and operators can use these templates to configure mod-specific Globals through a visual editor.

---

## Accessing Mod Settings

1. Right-click the **UF Service** system tray icon
2. Select **Options → Mod Settings**

---

## Interface Overview

The Mod Settings window has a two-panel layout:

| Panel | Description |
|-------|-------------|
| **Left Sidebar** | Lists all registered mods with search filtering |
| **Main Pane** | Renders the selected mod's custom HTML settings page in a sandboxed iframe |

### Header Bar

When a mod is selected, the header shows:
- **Mod name** and status message
- **Author** tag (if provided)
- **Reload** button to re-render the template

---

## How It Works

Mods register custom HTML settings pages through the UFramework Enforce Script SDK. Each page is stored in the `ModSettings` MongoDB collection. When an operator selects a mod, its template is loaded and rendered inside a sandboxed iframe.

The template communicates with the database through a `window.UF` bridge that the service injects automatically. Templates can load and save data to the `Globals` collection, giving operators a visual way to configure mod settings.

```
Mod (DayZ Server) ──Register Template──► Service (MongoDB: ModSettings)
                                              │
Operator (Electron UI) ◄── Load Template ─────┘
         │
         └── iframe renders template
                  │
                  ├── UF.loadGlobal() ──► MongoDB: Globals
                  └── UF.saveGlobal() ──► MongoDB: Globals
```

---

## MongoDB Storage

### ModSettings Collection

Stores the registered templates and metadata. Indexed on `modId` (unique).

```json
{
    "modId": "my-mod",
    "modName": "My Awesome Mod",
    "template": "<!DOCTYPE html>...",
    "author": "AuthorName",
    "globals": ["MyMod_Config"],
    "createdAt": "2026-02-24T12:00:00.000Z",
    "updatedAt": "2026-02-24T12:00:00.000Z"
}
```

> Only `modId` and `template` are required. All other fields are optional metadata that the mod can provide during registration.

### Globals Collection

The actual configuration data is stored in the existing `Globals` collection. Mod Settings is a UI layer on top of Globals — it reads/writes the same documents that the Globals Editor and Enforce Script `UF().globals()` endpoint use.

---

## Save Confirmation

When a mod template attempts to save settings, the Mod Settings UI presents a **batch confirmation modal** before anything is written to the database. This acts as a safety layer — you always see exactly what a template wants to write before it happens.

### How It Works

1. The template calls `UF.saveGlobal()` (one or more times — e.g. a "Save All" button may save 6 globals at once)
2. The UI collects all save requests over a short window (~300ms)
3. A single modal appears showing the mod name and the number of globals being saved
4. Each global is listed as an expandable row — click to inspect the full JSON
5. Click **Save All** to write every global, or **Cancel** to reject the entire batch

### Modal Layout

```
┌─────────────────────────────────────────┐
│  ⚠️ Confirm Save                        │
├─────────────────────────────────────────┤
│  Heroes & Bandits wants to save 6       │
│  globals:                               │
│                                         │
│  ▸ HAB_GENERAL              2.1 KB      │
│  ▾ HAB_PANEL                3.4 KB      │
│  ┌─────────────────────────────────┐    │
│  │ {                               │    │
│  │   "AccentColorR": 0.85,         │    │
│  │   "AccentColorG": 0.65,         │    │
│  │   ...                           │    │
│  │ }                               │    │
│  └─────────────────────────────────┘    │
│  ▸ HAB_ACTIONS              1.8 KB      │
│  ▸ HAB_ACTIONS_HERO         0.4 KB      │
│  ▸ HAB_ACTIONS_BANDIT       0.3 KB      │
│  ▸ HAB_ACTIONS_BAMBI        0.1 KB      │
│                                         │
│                    [ Cancel ] [ Save All]│
└─────────────────────────────────────────┘
```

> **Note:** If a template saves only one global, the modal still appears but shows "wants to save 1 global".

> If the operator clicks **Cancel**, the template receives an error ("Save cancelled by operator") for all queued saves.

---

## Security

- All REST endpoints require **server auth** (the same auth-key used by the DayZ server)
- Templates run in a **sandboxed iframe** with `allow-scripts allow-forms` only — no access to the parent window DOM, no network requests, no navigation
- Templates communicate with the host exclusively through `postMessage`, which the renderer validates before forwarding to IPC
- Template size is capped at **2MB**

---

## Troubleshooting

| Problem | Solution |
|---------|----------|
| No mods appear in the sidebar | Verify the DayZ server is running and calling `UF().Settings().Register()` at startup. Check the service logs for registration errors. |
| Template shows blank | Open DevTools (Ctrl+Shift+I in the Mod Settings window) and check the console for errors. The template HTML may have syntax errors. |
| "Bridge unavailable" error | The preload script failed to load. Restart the Mod Settings window. |
| Settings don't save | Check that the Global name matches in both the template and the mod's Enforce Script. Verify MongoDB is running. |
| Save was cancelled but template shows error | This is expected. When you cancel the batch save, the template receives a rejection ("Save cancelled by operator") and should display a message. Well-written templates handle this gracefully. |
| Template doesn't update after mod update | `Register()` is an upsert — the mod should re-register its template on every server start (this is the recommended pattern). Restart the DayZ server so the mod pushes its latest template, then click **Reload** in the Mod Settings window. |
