# UFServerService — Platform Feature Map: Windows (Electron) vs Linux (pkg)

> **Systematic cross-platform audit** of every file and subsystem in `UFServerServiceV2/`, identifying behavioral differences between the Windows (Electron) and Linux (pkg) packaging targets.  
> Last updated: February 2026

---

## Executive Summary

| Platform | Packaging | Entry Point | Runtime |
|----------|-----------|-------------|---------|
| **Windows** | Electron + electron-builder (NSIS installer) | `main.js` → `app.js` | Electron (Chromium + Node.js) |
| **Linux** | @yao-pkg/pkg (single binary) | `bin.js` → `app.js` | Embedded Node.js 22 snapshot |

**High-level delta count:**

| Category | Windows-Only Features | Linux-Only Features | Behavioral Differences |
|----------|----------------------|--------------------|-----------------------|
| GUI & Management | 7 windows + system tray | 0 (headless) | Complete GUI absent on Linux |
| MongoDB Setup | Auto-detect + auto-install | Manual prerequisite | Operator must install MongoDB themselves on Linux |
| Clustering | Disabled (single process) | Enabled when `cpuCount > 1` | Performance scaling available only on Linux |
| Tunnel Management | GUI start/stop/status in tray & settings | Config-driven auto-start + update checks | Auto-start parity; no GUI on Linux (expected) |
| Image Conversion | `texconv.exe` bundled | Bundled `magick` binary (ImageMagick), extracted at runtime | Different DDS backends, both bundled |
| Audio (TTS/FFmpeg) | `ffmpeg.exe` bundled in installer | Bundled `ffmpeg` binary, extracted at runtime | Both self-contained |
| Code Signing | Azure Trusted Signing | N/A | |
| Process Management | NSIS installer, tray icon | systemd service via `install-linux.sh` | Different lifecycle management |
| Save Path | `%APPDATA%/ufserverservice/` (Electron userData) | `./` or `UF_SAVE_PATH` env var | Different default data locations |
| **Proxy auto-renew** | Runs via Electron interval | Runs via `setInterval` in `bin.js` | Parity — both renew every 24h |
| Log Streaming to GUI | Winston stream → BrowserWindow IPC | No renderer target | Logs only go to console + file on Linux |

---

## 1. Package & Build Configuration — `package.json`

| Aspect | Windows (Electron) | Linux (pkg) | Impact |
|--------|-------------------|-------------|--------|
| Entry point | `main.js` (Electron main process) | `bin.js` (headless Node.js) | **Completely different startup paths** |
| Build command | `npm run build` (electron-builder, optionally Azure-signed) | `npm run pkg:linux` (pkg → single binary) | Different toolchains |
| Output artifact | NSIS installer `.exe` with ASAR archive | Single static binary `ufserverservice-linux` | Different distribution model |
| Post-build | N/A | `node build.js` (sets Windows exe metadata via rcedit — **only for Windows target**); copies `install-linux.sh` | `build.js` is Windows-only post-processing |
| ffmpeg bundling | `asarUnpack` + `extraFiles` → ffmpeg.exe beside executable | Listed in `pkg.assets` (`ffmpeg.exe` **and** `ffmpeg`) | Linux pkg bundles the binary but it's inside the snapshot — see TTS section for runtime behavior |
| **texconv.exe** | Bundled via `extraFiles` (`bin/texconv.exe → texconv.exe`) | N/A — Linux uses bundled ImageMagick (`bin/magick`) instead | Different conversion backends per platform |
| **magick (ImageMagick)** | N/A — Windows uses texconv | Bundled via `pkg:linux` build (`bin/magick → ../Build/Service/bin/magick`) | ImageMagick auto-detected and extracted at runtime |
| gamedig | Scripts + assets bundled in both | Same | Parity |
| Code signing | Azure Trusted Signing (`.env.trustedsigning`) | N/A | Windows-only |
| `node-windows` | Listed as dependency (unused in codebase — leftover) | Excluded by pkg ignore | Dead dependency, no impact |

---

## 2. Entry Points — `main.js` vs `bin.js`

### `main.js` — Windows/Electron (1,789 lines)

Responsibilities that **do not exist on Linux**:

| Feature | Description |
|---------|-------------|
| **System Tray icon** | Creates tray with context menu showing service status, Discord status, OpenAI status, tunnel status |
| **Console Window** | BrowserWindow showing live log stream via Winston stream transport → IPC |
| **Log Viewer Window** | Full MongoDB-backed log viewer with filtering, pagination, stats |
| **Settings Window** | Configuration editor (config.json) with save/restart, proxy domain registration, tunnel management |
| **Globals Editor Window** | CRUD editor for MongoDB `Globals` collection |
| **KB Manager Window** | Knowledge base + document management with embedding generation |
| **Data Manager Window** | Mod data viewer/deleter scanning all MongoDB collections |
| **Index Optimizer Window** | MongoDB index recommendations, current index listing, one-click creation |
| **MongoDB auto-install** | Checks port 27017 → Windows Service → winget; offers dialog to install MongoDB Server + Compass |
| **Proxy auto-renew** | 24-hour interval calling `ufapi.daemonforge.dev/keepalive` |
| **Proxy domain registration** | IPC handler to fetch available domains and register subdomains |
| **Tunnel GUI controls** | Start/stop/download/update cloudflared from settings + tray |
| **External link handler** | IPC bridge for opening URLs in system browser |
| **Log history streaming** | Winston stream transport → sends to all BrowserWindows |
| **Certificate error bypass** | Ignores self-signed cert errors for localhost status checks |

### `bin.js` — Linux/pkg (31 lines)

| Feature | Description |
|---------|-------------|
| **Global flags** | Sets `global.isElectron = false`, `SAVEPATH = process.env.UF_SAVE_PATH \|\| './'` |
| **Signal handling** | SIGINT/SIGTERM → graceful shutdown |
| **Requires `app.js`** | That's it — delegates everything to the shared Express server |

### Key Differences

| Aspect | Windows | Linux | Impact |
|--------|---------|-------|--------|
| `global.SAVEPATH` | `app.getPath('userData')` (e.g., `%APPDATA%/ufserverservice/`) | `UF_SAVE_PATH` env var or `./` | **Config, logs, certs, audio cache all resolve differently** |
| `global.isElectron` | `true` | `false` | Guards Electron-specific code paths |
| `global.APIVERSION` | `app.getVersion()` fallback | `require('./package.json').version` | Equivalent, but different source |
| `global.rootPath` | `__dirname` | `__dirname` | Same, but `__dirname` means different things inside pkg snapshot vs Electron asar |

---

## 3. Shared Core — `app.js` (470 lines)

The Express server is the shared backbone. Both platforms run this identically **except**:

| Aspect | Windows | Linux | Impact |
|--------|---------|-------|--------|
| **Clustering** | `!isElectron` → **disabled** (single process) | Enabled when `cpuCount > 1` via `os.cpus().length` or config | **Linux can scale across CPU cores; Windows cannot** |
| **Let's Encrypt** | Works, but localhost status checks conflict with Greenlock (suppressed) | Works cleanly | Minor — Greenlock suppresses localhost INVALID_HOSTNAME errors on Windows |
| **Version check** | `CheckRecentVersion()` in master or single-process mode | Same | Parity |
| **Index creation** | `CheckIndexes()` after 1s delay | Same | Parity |
| **KB embedding startup** | Runs `ensureAllEmbeddings` after 5s | Same | Parity |

### All REST API Routes (Shared — Platform-Agnostic)

| Route | Controller | Platform Notes |
|-------|-----------|---------------|
| `/Object` | `controllers/object.js` | Parity |
| `/Player` | `controllers/player.js` | Parity |
| `/Globals` | `controllers/global.js` | Parity |
| `/GetAuth` | `auth/controller.js` | Parity |
| `/Status` | `controllers/status.js` | Parity |
| `/Logger` | `controllers/logger.js` | Parity |
| `/Discord` | `discord/router.js` | Parity |
| `/ServerQuery` | `controllers/serverquery.js` | Parity (gamedig) |
| `/Random` | `controllers/trueRandom.js` | Clustering behavior differs (see Section 9) |
| `/Crypto` | `controllers/crypto.js` | Parity |
| `/Messages` | `controllers/messages.js` | Parity |
| `/AI/Chat` | `controllers/aiChat.js` | Parity |
| `/AI/Assistant` | `controllers/aiAssistant.js` | Parity |
| `/TTS` | `controllers/tts.js` | **FFmpeg differences** (see Section 7) |
| `/Images` | `controllers/images.js` | **DDS conversion differences** (see Section 8) |
| `/KB` | `controllers/kb.js` | Parity |

---

## 4. Configuration — `configLoader.js` (293 lines)

| Aspect | Windows | Linux | Impact |
|--------|---------|-------|--------|
| Config path | `%APPDATA%/ufserverservice/config.json` | `./config.json` or `$UF_SAVE_PATH/config.json` | **Different default locations** |
| Default port | 443 | 443 | Same — but Linux without root can't bind <1024; `install-linux.sh` defaults to **8443** |
| Auto-generated auth token | Yes, on first run | Yes, on first run | Parity |
| LetsEncrypt Greenlock config | Written to `SAVEPATH/greenlock/` | Same relative path | Parity |
| Tunnel config section | Present | Present | Parity — but no GUI to configure on Linux |
| Proxy config section | Present | Present | Parity — but auto-renew only runs in Electron |

---

## 5. Tunnel Manager — `tunnelManager.js` (720 lines)

| Aspect | Windows | Linux | Impact |
|--------|---------|-------|--------|
| Binary download URL | `cloudflared-windows-amd64.exe` | `cloudflared-linux-amd64` | Correct per-platform |
| Binary storage | `SAVEPATH/bin/cloudflared.exe` | `SAVEPATH/bin/cloudflared` | Correct |
| chmod after download | Skipped (win32) | `chmodSync(dest, 0o755)` | Correct |
| Process kill | `taskkill /pid /T /F` (kills process tree) | `SIGTERM` → 5s → `SIGKILL` | Different kill semantics — Windows kills tree, Linux single process |
| Auto-start | Via tray menu + config `Tunnel.autoStart` | Config-driven, started in `bin.js` 3s after boot | Parity — **RESOLVED** |
| Update checks | `startUpdateChecks()` called from `main.js` | `startUpdateChecks()` called from `bin.js` | Parity — **RESOLVED** |
| Status change listeners | Forwarded to BrowserWindows via IPC | No listeners registered | GUI feedback absent |
| macOS support | Not packaged | Binary URL defined in `PLATFORM_MAP` | darwin URL exists but no build target |

### Tunnel Auto-Start on Linux — RESOLVED

`bin.js` now calls `startTunnelIfConfigured()` and `tunnelManager.startUpdateChecks()` 3 seconds after `app.js` loads, matching the Electron behavior. The tunnel will auto-start when `Tunnel.enabled`, `Tunnel.token`, and `Tunnel.autoStart` are set in config.

---

## 6. Logging — `log.js` + `main.js` stream

| Aspect | Windows | Linux | Impact |
|--------|---------|-------|--------|
| Log level (dev) | `debug` (when running `electron .`) | `debug` (when running `node app.js`) | Parity |
| Log level (packaged) | `info` (or config `LogLevel`) | `info` (via `process.pkg` detection) | Parity |
| Console transport | Yes | Yes | Parity |
| Daily rotate file | `SAVEPATH/logs/UF-YYYY-MM-DD.log` | Same | Parity |
| In-memory history | `global.logHistory[]` (1000 entries) | Same (but never consumed) | Wasted memory on Linux — no renderer reads it |
| Stream → BrowserWindow | Winston stream transport added in `main.js.createLoggerStream()` | **Not added** | GUI log streaming is Windows-only |
| `process.pkg` detection | `Boolean(process.pkg \|\| ...)` | `Boolean(process.pkg)` → true | Correct |

---

## 7. TTS / Audio — `controllers/tts.js` (489 lines)

This is one of the most platform-divergent files.

### FFmpeg Path Resolution (`getFfmpegPath()`)

| Scenario | Windows (Electron) | Linux (pkg) | Impact |
|----------|-------------------|-------------|--------|
| Packaged Electron | `path.dirname(process.execPath)/ffmpeg.exe` (extracted by electron-builder) | N/A | Works out of the box |
| pkg binary | N/A | 1. `which ffmpeg` (system PATH) 2. `dirname(execPath)/bin/ffmpeg` (bundled) 3. Fallback: bare `'ffmpeg'` | Bundled binary ships with Linux build; auto-installed by `install-linux.sh` |
| Development | `require('ffmpeg-static')` | Same | Parity |

### FFmpeg Availability

| Platform | Default State | User Action Required |
|----------|---------------|---------------------|
| Windows | ✅ Bundled with installer | None |
| Linux | ✅ Bundled in `bin/ffmpeg`, extracted at runtime | None — auto-installed by `install-linux.sh` |

If the bundled binary is somehow missing and system FFmpeg is also unavailable, TTS endpoints will fail with an error log. The `install-linux.sh` script both auto-detects the bundled binary in `bin/` and offers to install the system package as a fallback.

---

## 8. Image DDS Conversion — `controllers/images.js` (325 lines)

### Conversion Tool Detection (`detectImageConversionTool()`)

| Platform | Primary Tool | Fallback | Impact |
|----------|-------------|----------|--------|
| **Windows** | `texconv.exe` (bundled with installer) | None | Works out of the box |
| **Linux (pkg)** | Bundled ImageMagick (`bin/magick`, extracted at runtime) | System ImageMagick (`magick`/`convert`) or `texconv` Linux binary | Works out of the box with bundled binary |

### Conversion Command Differences

| Tool | Command | DDS Quality Notes |
|------|---------|-------------------|
| `texconv.exe` (Windows) | `texconv.exe -f DXT5 -o <tmpDir> <input.png>` | DirectX-native, highest fidelity DXT5 |
| ImageMagick (Linux) | `magick <input.png> -define dds:compression=dxt5 <output.dds>` | Good but may differ in mipmap handling |
| texconv-linux (fallback) | Same as Windows texconv but Linux binary | Rare — requires manual placement |

### Endpoints Affected

| Endpoint | Function |
|----------|----------|
| `POST /Images/Generate` | Convert PNG URL → DDS base64 (async job) |
| `POST /Images/Download` | Convert PNG URL → DDS base64 (sync) |
| `POST /Images/Discord/:GUID` | Discord avatar → DDS base64 |

If **no conversion tool** is found on Linux (bundled binary missing AND no system ImageMagick), all three endpoints return `500` errors.

### Bundled ImageMagick Binary — RESOLVED

The `pkg:linux` build now copies `bin/magick` alongside the service binary. At runtime, `detectImageConversionTool()` checks for the bundled binary at `dirname(execPath)/bin/magick`, copies it to a temp directory with executable permissions, verifies DDS support, and uses it. The `install-linux.sh` script also auto-detects and installs the bundled binary to `/opt/ufserverservice/bin/magick`.

---

## 9. Clustering — `app.js` + `controllers/trueRandom.js`

| Aspect | Windows (Electron) | Linux (pkg/headless) | Impact |
|--------|-------------------|---------------------|--------|
| Cluster mode | **Disabled** (`!isElectron` guard) | **Enabled** when `cpuCount > 1` | Linux can fork worker processes for better throughput |
| Worker restart | N/A | Auto-restart on crash | Improved resilience on Linux |
| True Random pool | Single-process quantum pool | Master collects & distributes to workers via IPC | Different pool management architecture |

---

## 10. MongoDB Auto-Install — `main.js` only

| Step | Windows | Linux | Impact |
|------|---------|-------|--------|
| Port 27017 check | `net.Socket.connect()` | **Not performed** | |
| Windows Service check | `sc query "MongoDB"` | **Not performed** | |
| winget check + install dialog | `winget list MongoDB.Server` → `dialog.showMessageBox()` → auto-install | **Not performed** | |
| Compass install option | Yes — `winget install MongoDB.Compass.Community` | **Not available** | |
| Remote DB detection | Skips local check if `DBServer` is not localhost | **Not performed** (but also not needed — no auto-install) | |

On Linux, the `install-linux.sh` script checks for MongoDB and **warns** but doesn't auto-install.

---

## 11. GUI Management Windows (Electron-Only)

These 7 BrowserWindows exist **only** in `main.js` and are completely absent on Linux:

| Window | Preload | IPC Handlers | Function |
|--------|---------|-------------|----------|
| **Console** | `preload/console.js` | `onLogMessage` | Live log tail from Winston stream |
| **Log Viewer** | `preload/logs.js` | `logs:query`, `logs:getServers`, `logs:getTypes`, `logs:getStats`, `logs:delete` | Full MongoDB log browser with filters |
| **Settings** | `preload/settings.js` | `get-config`, `save-config`, `get-proxy-domains`, `register-proxy`, tunnel IPC, `open-external` | Configuration editor + proxy + tunnel |
| **Globals Editor** | `preload/globals.js` | `globals:list`, `globals:load`, `globals:save`, `globals:delete` | MongoDB globals CRUD |
| **KB Manager** | `preload/kb.js` | `kb:list`, `kb:get`, `kb:create`, `kb:update`, `kb:delete`, `kb:listDocuments`, etc. | Knowledge base + document + embedding management |
| **Data Manager** | `preload/modmanager.js` | `modmanager:list`, `modmanager:delete` | Mod data scanner/deleter |
| **Index Optimizer** | `preload/indexOptimizer.js` | `indexOptimizer:getRecommendations`, `getCurrentIndexes`, `createIndex` | MongoDB index analysis |

**All 7 management UIs are Windows-only.** Linux operators must manage everything via:
- Direct `config.json` editing
- MongoDB shell / Compass
- REST API calls
- `journalctl` / log files

---

## 12. Proxy Domain System

| Aspect | Windows | Linux | Impact |
|--------|---------|-------|--------|
| Register proxy subdomain | Settings window → IPC → `ufapi.daemonforge.dev/register` | **No UI; no code path** | Cannot register from Linux without custom scripting |
| Auto-renew keepalive | `startProxyAutoRenew()` in `main.js` — runs every 24h | `startProxyAutoRenew()` in `bin.js` — runs every 24h | Parity — **RESOLVED** |
| Rate limiting | 60s between IPC-based registrations | N/A | |

---

## 13. Process Lifecycle & Service Management

| Aspect | Windows | Linux | Impact |
|--------|---------|-------|--------|
| Installation | NSIS installer (per-machine, elevation, license) | `install-linux.sh` (systemd service, FHS paths) | Fundamentally different |
| Service management | Tray icon stop/restart; `app.relaunch()` / `app.exit()` | systemd `start`/`stop`/`restart`/`enable` | Different operational model |
| Auto-start on boot | Windows startup (NSIS option or manually) | `systemctl enable ufserverservice` | Different mechanisms |
| Graceful shutdown | `app.on('will-quit')` — stops tunnel, closes IndexManager | `process.on('SIGINT/SIGTERM')` → stops tunnel, clears intervals, closes IndexManager | Parity — **RESOLVED** |
| Data directories | `%APPDATA%/ufserverservice/` (auto-created by Electron) | `/var/lib/ufserverservice/` (created by install script) | Different FHS conventions |
| Configuration | `%APPDATA%/ufserverservice/config.json` | `/etc/ufserverservice/config.json` (symlinked) | Different paths |
| Log files | `%APPDATA%/ufserverservice/logs/` | `/var/log/ufserverservice/` + `/var/lib/ufserverservice/logs/` | Dual location on Linux |
| Security | Runs as current user | Dedicated `ufservice` user, `NoNewPrivileges`, `ProtectSystem=strict` | **Linux is more hardened** |

---

## 14. Default Port Mismatch

| Platform | Default Port | Reason |
|----------|-------------|--------|
| Windows (configLoader) | **443** | Electron runs with admin elevation via NSIS `requireAdministrator` |
| Linux (install-linux.sh) | **8443** | Non-root user can't bind <1024 |

The `configLoader.js` defaults port to **443**, but `install-linux.sh` creates a config with port **8443**. If a Linux user runs the binary without the install script, they'll get an `EACCES` error trying to bind port 443 unless running as root.

---

## 15. Build Post-Processing — `build.js`

`build.js` uses `rcedit.exe` to set Windows executable metadata:
- Icon
- File version
- Product version
- Requested execution level: `requireAdministrator`
- Company name, product name, file description

This runs **only** for the Windows pkg target (`npm run pkg` calls `node build.js`). The Linux build (`npm run pkg:linux`) does **not** call `build.js`. This is correct behavior.

---

## 16. `install-linux.sh` (559 lines) — Linux-Only

| Feature | Details |
|---------|---------|
| Package manager detection | apt, dnf, yum, pacman, zypper |
| Dependency installation | ffmpeg, ImageMagick (optional, prompted) |
| Dependency verification | MongoDB (critical), ffmpeg (optional), ImageMagick+DDS (optional) |
| Service user creation | `ufservice` (system user, no login shell) |
| Directory structure | FHS-compliant (`/opt`, `/var/lib`, `/var/log`, `/etc`) |
| Binary installation | Copies binary + sets executable permissions |
| Config creation | Default config with port 8443, symlinked from `/etc` |
| systemd unit | `ufserverservice.service` with security hardening |
| Uninstall | Full cleanup with interactive confirmation |

---

## 17. Miscellaneous Differences

### `node-windows` Dependency
Listed in `package.json` dependencies but **never imported or used** in any source file. Appears to be a leftover from a previous Windows Service implementation. It adds ~2MB to the npm install. Excluded from pkg builds via ignore rules.

### `undici` (in `trueRandom.js`)
Uses `undici.Agent` with `setGlobalDispatcher` to ignore SSL errors for ANU quantum random API. This is Node.js built-in in v22 — works on both platforms. No delta.

### `ensure-embeddings.js`
Standalone script (`node ensure-embeddings.js`) that generates KB embeddings. Sets `global.SAVEPATH = './'` — platform-agnostic. Can run on both platforms.

### Views/Renderers/Templates
All 7 view HTMLs and 7 renderer JS files are **Electron-only**. The `templates/` directory (Discord login/error EJS templates) is used by the Express server and works on both platforms.

---

## Complete Feature Parity Matrix

| Feature | Windows (Electron) | Linux (pkg) | Gap Severity |
|---------|-------------------|-------------|-------------|
| REST API (all 16 route groups) | ✅ | ✅ | None |
| MongoDB connection & CRUD | ✅ | ✅ | None |
| Discord bot | ✅ | ✅ | None |
| OpenAI Chat/Assistant | ✅ | ✅ | None |
| Knowledge Base + Embeddings | ✅ | ✅ (API-only) | **Low** — no management GUI |
| Server Query (gamedig) | ✅ | ✅ | None |
| Let's Encrypt SSL | ✅ | ✅ | None |
| Self-signed SSL | ✅ | ✅ | None |
| Rate limiting | ✅ | ✅ | None |
| Authentication (server + player) | ✅ | ✅ | None |
| Translation | ✅ | ✅ | None |
| Crypto conversion | ✅ | ✅ | None |
| True Random (quantum) | ✅ | ✅ | None |
| Message queues | ✅ | ✅ | None |
| Logging to file | ✅ | ✅ | None |
| Version check | ✅ | ✅ | None |
| DB index creation | ✅ | ✅ | None |
| **TTS (Text-to-Speech)** | ✅ Bundled ffmpeg | ✅ Bundled ffmpeg | None |
| **Image DDS conversion** | ✅ Bundled texconv | ✅ Bundled ImageMagick | None |
| **Clustering** | ❌ Disabled | ✅ Enabled | **low** — Linux has better scaling |
| **System Tray + GUI** | ✅ Full GUI | ❌ Headless only | **Medium** — 7 management windows absent |
| **MongoDB auto-install** | ✅ winget dialog | ❌ Must use install-linux.sh script  | **Medium** |
| **Tunnel auto-start** | ✅ Config + tray | ✅ Config-driven | None  |
| **Tunnel update checks** | ✅ Every 24h | ✅ Started with tunnel | None |
| **Proxy auto-renew** | ✅ Every 24h | ✅ Every 24h | None |
| **Graceful shutdown cleanup** | ✅ Tunnel stop + DB close | ✅ Tunnel stop + DB close | None |
| **Settings GUI** | ✅ | ❌ Edit config.json manually | **Low** — expected for server deployment |
| **Log viewer GUI** | ✅ | ❌ Use log files / journalctl | **Low** — expected for server deployment |
| **Index optimizer GUI** | ✅ | ❌ Use MongoDB shell | **Low** |
| **Globals editor GUI** | ✅ | ❌ Use MongoDB shell | **Low** |
| **KB manager GUI** | ✅ | ❌ Use API endpoints | **Low** |
| **Data manager GUI** | ✅ | ❌ Use MongoDB shell | **Low** |
| **Port 443 default** | ✅ Runs as admin | ⚠️ Needs root or port >1024 | **Low** |
| **In-memory log history** | ✅ Used by Console window | ⚠️ Allocated but never consumed | **Trivial** — wasted memory |

---

## Recommendations

### High Priority — RESOLVED

All three high-priority gaps have been closed in `bin.js`:
1. ~~**Tunnel auto-start**~~ — `bin.js` now calls `startTunnelIfConfigured()` + `startUpdateChecks()` after app loads.
2. ~~**Proxy auto-renew**~~ — `bin.js` now runs `startProxyAutoRenew()` with 24h keepalive interval.
3. ~~**Graceful shutdown**~~ — SIGINT/SIGTERM now stops the tunnel, clears the renew interval, and closes the IndexManager DB connection before exiting.

### Medium Priority (Operational Gaps)

4. ~~**Document system requirements for Linux**~~ — FFmpeg and ImageMagick are now bundled with the Linux build and auto-installed by `install-linux.sh`.
5. ~~**Consider bundling ffmpeg in the Linux pkg**~~ — Done. `bin/ffmpeg` is copied alongside the Linux binary during `pkg:linux` build and installed to `/opt/ufserverservice/bin/` by the install script.
6. ~~**Add `texconv` to pkg assets or commit to ImageMagick**~~ — Done. `bin/magick` (ImageMagick) is bundled, and `images.js` auto-detects and extracts it at runtime.

### Low Priority (Quality of Life)

7. **Remove `node-windows` from dependencies** — unused, adds weight.
8. **Skip `global.logHistory` allocation when `!isElectron`** — minor memory savings.
9. **Consider a minimal CLI management tool for Linux** — simple Node.js script to manage config, view status, trigger operations via the REST API.
