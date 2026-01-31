# Universal Framework - Windows Quick Reference

This is a quick reference for running UF Server Service on Windows. For detailed instructions, see [Installation Guide](01_Installation.md).

---

## Installation

Windows uses the Electron application, which provides a graphical system tray interface for easy management.

**Download:** `UniversalFrameworkService-Setup-X.X.X.exe`

**Features:**
- System tray icon with menu
- One-click access to logs, config, and templates
- Built-in configuration editor
- MongoDB installation prompt if not detected
- FFmpeg bundled for TTS features

**Installation:**
1. Download the installer from [GitHub Releases](https://github.com/daemonforge/DayZ-UniveralApi/releases)
2. Run the installer
3. Follow prompts (install MongoDB if asked)
4. Service starts automatically

---

## File Locations

| File/Folder | Location |
|-------------|----------|
| Config file | `%APPDATA%\ufserverservice\config.json` |
| Log files | `%APPDATA%\ufserverservice\logs\` |
| Templates | `%APPDATA%\ufserverservice\templates\` |
| SSL certs (Greenlock) | `%APPDATA%\ufserverservice\greenlock\` |
| Audio cache (TTS) | `%APPDATA%\ufserverservice\audioCache\` |

**Quick Access:**
- Right-click tray icon -> "Open Data Directory"
- Right-click tray icon -> "View Logs"
- Right-click tray icon -> "[DIR] Discord Templates"

---

## Common Operations

### Start/Stop Service

- **Start**: Launch from Start Menu or shortcut
- **Stop**: Right-click tray icon -> "Exit"
- **Restart**: Right-click tray icon -> "Restart Service"

### View Logs

**Tray Menu:**
1. Right-click tray icon
2. Click "View Logs"

**PowerShell:**
```powershell
# Find log files
Get-ChildItem "$env:APPDATA\ufserverservice\logs\*.log"

# View latest log
Get-Content "$env:APPDATA\ufserverservice\logs\UF-*.log" -Tail 100

# Watch logs in real-time
Get-Content "$env:APPDATA\ufserverservice\logs\UF-*.log" -Wait -Tail 50
```

### Edit Configuration

**Tray Menu:**
1. Right-click tray icon
2. Click "Edit Config" (opens in default editor)
3. Save changes
4. Right-click tray icon -> "Restart Service"

**PowerShell:**
```powershell
notepad "$env:APPDATA\ufserverservice\config.json"
```

### Access Templates

**Tray Menu:**
1. Right-click tray icon
2. Click "[DIR] Discord Templates"

**PowerShell:**
```powershell
explorer "$env:APPDATA\ufserverservice\templates"
```

---

## Firewall Configuration

**Allow UF Service through Windows Firewall:**

```powershell
# Create firewall rule
New-NetFirewallRule -DisplayName "UF Service HTTPS" -Direction Inbound -LocalPort 443 -Protocol TCP -Action Allow

# Verify rule
Get-NetFirewallRule -DisplayName "UF Service*"
```

**Or via GUI:**
1. Open Windows Defender Firewall
2. Advanced Settings â†’ Inbound Rules â†’ New Rule
3. Port â†’ TCP â†’ 443 â†’ Allow â†’ Name: "UF Service HTTPS"

---

## Health Check

**Test service is running:**
```powershell
# Check if process is running
Get-Process -Name "ufserverservice*" -ErrorAction SilentlyContinue

# Check if port is listening
Test-NetConnection -ComputerName localhost -Port 443

# Test endpoint
Invoke-RestMethod -Uri "https://localhost:443/Status" -SkipCertificateCheck
```

---

## Troubleshooting

### Port Already in Use

```powershell
# Find what's using port 443
Get-NetTCPConnection -LocalPort 443 | Select-Object OwningProcess | ForEach-Object { Get-Process -Id $_.OwningProcess }
```

### Service Won't Start

1. Check if MongoDB is running:
   ```powershell
   Get-Service -Name MongoDB
   ```

2. Check for config errors - run from command line to see output:
   ```powershell
   & "$env:LOCALAPPDATA\Programs\ufserverservice\ufserverservice.exe"
   ```

3. Delete config and regenerate:
   ```powershell
   Remove-Item "$env:APPDATA\ufserverservice\config.json"
   # Restart service to generate new config
   ```

### SSL Certificate Issues

The service uses a bundled self-signed certificate by default. For custom certificates:
```json
{
    "Certificate": "C:\\path\\to\\certificate.crt",
    "CertificateKey": "C:\\path\\to\\private.key"
}
```

---

## Prerequisites Checklist

| Component | Required | Installation |
|-----------|----------|--------------|
| MongoDB 4.4+ | [YES] Yes | `winget install MongoDB.Server` |
| .NET Runtime | âŒ No | - |
| Visual C++ Redist | âš ï¸ Maybe | [Download](https://aka.ms/vs/17/release/vc_redist.x64.exe) |
| FFmpeg | âš ï¸ For TTS | Bundled with Electron app |
| ImageMagick | âš ï¸ For DDS | [Download](https://imagemagick.org/script/download.php) |

---

## Quick Config Example

**Minimal `config.json` for Windows:**
```json
{
    "DBServer": "mongodb://localhost:27017",
    "DB": "DayZ",
    "Port": 443,
    "ServerAuth": ["your-auth-token-here"]
}
```

**With Discord:**
```json
{
    "DBServer": "mongodb://localhost:27017",
    "DB": "DayZ",
    "Port": 443,
    "ServerAuth": ["your-auth-token-here"],
    "Discord": {
        "Client_Id": "your-client-id",
        "Client_Secret": "your-client-secret",
        "Bot_Token": "your-bot-token",
        "Guild_Id": "your-guild-id"
    }
}
```

    ## Tags
    `operators`, `windows`, `quickref`, `service`, `paths`, `how-to`, `doc-usage`
