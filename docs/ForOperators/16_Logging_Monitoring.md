# Logging & Monitoring Guide

This document covers logging configuration, log locations, log analysis, and monitoring recommendations.

## Overview

UF Server Service uses Winston for logging with:
- Console output (colorized)
- Daily rotating log files
- In-memory log history (for Electron GUI)
- Configurable log levels

---

## Log Locations

### Windows

The service logs to the AppData folder:
```
%APPDATA%\ufserverservice\logs\
```
*Usually: `C:\Users\<username>\AppData\Roaming\ufserverservice\logs\`*

### Linux (Systemd Service)

**Application Logs (Winston):**
```
/var/lib/ufserverservice/logs/
```

**Service Startup Logs (stdout/stderr):**
```
/var/log/ufserverservice/stdout.log
/var/log/ufserverservice/stderr.log
```
*Check these if the service fails to start.*

---

## Log File Naming

Log files use the pattern: `UF-YYYY-MM-DD.log`

Example:
- `UF-2025-01-11.log`
- `UF-2025-01-10.log`

They are rotated daily and kept for 30 days.

---

## Log Configuration

### Log Level

You can set the log verbosity in `config.json` (Optional):

```json
{
  "LogLevel": "info"
}
```

**Available Levels**:
1. `error` (Least verbose)
2. `warn`
3. `info` (Default)
4. `debug` (Most verbose)

**Default Behavior**:
- **Packaged Builds (Production)**: Defaults to `debug` to assist with troubleshooting issues in the field.
- **Source/Dev Installs**: Defaults to `info` (or `LogLevel` from config).

To reduce log size in production, set `"LogLevel": "info"` or `"error"`.

### Log Rotation Settings
- **Rotation**: Daily
- **Max Size**: 128MB per file
- **Max Files**: 30 Days retention
- **Zipped**: No

---

## Log Format

### File Output (JSON)
Logs are stored in JSON format for easy parsing:

```json
{"level":"info","message":"[WebServer] Server started","timestamp":"1/11/2025, 2:30:45 PM"}
{"level":"info","message":"New Log Registered","clientType":"Server","clientId":"a1b2...","timestamp":"..."}
```

### Console Output
Colorized text for readability:
```
info: [WebServer] Server started
warn: [DB] Connection slow
```

---

## Accessing Logs via GUI (Windows)

1. Open the UF Server Service app.
2. Click **Logs** in the sidebar.
3. This view shows the *current session* logs.
4. Use "Open Log Folder" to see historical files.

---

## Monitoring Recommendations

1. **Process Monitoring**:
   - Monitor the PID of `ufserverservice.exe` (Windows) or `ufserverservice-linux` (Linux).
   - Ensure MongoDB process is running.

2. **Log Monitoring**:
   - Watch for `error` level logs.
   - Common errors: `Discord Connection Error`, `MongoDB Connection Error`.

3. **API Health Check**:
   - Poll `GET /Status` every few minutes.
   - Alert if `Status` is not `Online` or `Discord` is `Disabled` (if expected to be enabled).

4. **Disk Space**:
   - Monitor the logs folder size.
   - Monitor MongoDB database size.

## Common Log Messages

| Message | Meaning | Action |
|---------|---------|--------|
| `Server started on Port...` | Service is running | None |
| `MongoDB Connected` | DB connection successful | None |
| `Discord Bot Ready` | Discord bot is online | None |
| `RateLimit reached` | Client hitting API too fast | Check client/mod loop frequency |
| `Greenlock ...` | SSL Certificate activity | informational |

## Tags
`operators`, `logging`, `monitoring`, `health-check`, `alerts`, `reference`, `doc-usage`
