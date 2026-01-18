# Universal Framework - Installation Guide

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Windows Installation](#windows-installation)
3. [Linux Installation](#linux-installation)
4. [DayZ Mod Installation](#dayz-mod-installation)
5. [First Run Configuration](#first-run-configuration)
6. [Verification Steps](#verification-steps)

---

## Prerequisites

### MongoDB (Required)

MongoDB is required for Universal Framework to function. The service will not start properly without a MongoDB connection.

#### Windows MongoDB Installation

**Option 1: Using Windows Package Manager (Recommended)**

```powershell
winget install MongoDB.Server
```

The Electron installer will prompt to install MongoDB if not detected.

**Option 2: Manual Installation**

1. Download MongoDB Community Server from https://www.mongodb.com/try/download/community
2. Run the installer, selecting "Complete" installation
3. Ensure "Install MongoDB as a Service" is checked
4. After installation, verify the service is running:
   ```powershell
   Get-Service -Name MongoDB
   ```

#### Linux MongoDB Installation

**Debian/Ubuntu:**
```bash
sudo apt update
sudo apt install mongodb
sudo systemctl enable mongod
sudo systemctl start mongod
```

**Fedora/RHEL/CentOS:**
```bash
sudo dnf install mongodb-org
sudo systemctl enable mongod
sudo systemctl start mongod
```

**Arch Linux:**
```bash
sudo pacman -S mongodb
sudo systemctl enable mongodb
sudo systemctl start mongodb
```

### Optional Dependencies

#### FFmpeg (Required for TTS/Audio features)

**Windows:**
- Bundled with the application (no installation required)

**Linux (Debian/Ubuntu):**
```bash
sudo apt install ffmpeg
```

**Linux (Fedora):**
```bash
sudo dnf install ffmpeg
```

#### ImageMagick / TexConv (Required for DDS image conversion)

**Windows:**
- Bundled with the application (uses `texconv.exe` automatically)

**Linux (Debian/Ubuntu):**
```bash
sudo apt install imagemagick
```

**Linux (Fedora):**
```bash
sudo dnf install ImageMagick
```

---

## Windows Installation

### Electron Application

The Electron application provides a GUI and system tray interface for easy management.

1. Download `UniversalFrameworkService-Setup-X.X.X.exe` from the [GitHub Releases](https://github.com/daemonforge/DayZ-UniveralApi/releases) page

2. Run the installer
   - If MongoDB is not detected, you will be prompted to install it
   - Choose "Install Server only" or "Install Server & Compass" (Compass is a GUI tool for MongoDB)

3. The application will install, launch, and create a system tray icon

4. **First Run:**
   - The service will generate a default configuration
   - A random `ServerAuth` token will be automatically generated

5. **Accessing Configuration:**
   - Right-click the system tray icon
   - Select **Options -> Settings** to configure the server port, database, and API keys via the UI
   - Select **Options -> KB Manager** to manage AI knowledge bases
   - Select **Options -> Globals Editor** to manage global variables

6. **Monitoring:**
   - Right-click the system tray icon -> **Console** to view the live server console
   - Right-click the system tray icon -> **Log Viewer** to view historical logs

---

## Linux Installation

### Using the Installation Script (Recommended)

1. Download the binary and installation script:
   ```bash
   wget https://github.com/daemonforge/DayZ-UniveralApi/releases/download/vX.X.X/ufserverservice-linux
   wget https://github.com/daemonforge/DayZ-UniveralApi/releases/download/vX.X.X/install.sh
   chmod +x install.sh ufserverservice-linux
   ```

2. Run the installer as root:
   ```bash
   sudo ./install.sh ./ufserverservice-linux
   ```

3. The installer will:
   - Create a service user (`ufservice`)
   - Install the binary to `/opt/ufserverservice/`
   - Create data directories at `/var/lib/ufserverservice/`
   - Generate a default configuration at `/etc/ufserverservice/config.json`
   - Create a systemd service

### Installation Paths (Linux)

| Path | Purpose |
|------|---------|
| `/opt/ufserverservice/` | Binary installation directory |
| `/etc/ufserverservice/config.json` | Configuration file |
| `/var/lib/ufserverservice/` | Data directory (greenlock, audioCache, etc.) |
| `/var/log/ufserverservice/` | Log files |

### Managing the Service

**Start the service:**
```bash
sudo systemctl start ufserverservice
```

**Stop the service:**
```bash
sudo systemctl stop ufserverservice
```

**Enable auto-start on boot:**
```bash
sudo systemctl enable ufserverservice
```

**Check service status:**
```bash
sudo systemctl status ufserverservice
```

**View logs:**
```bash
sudo journalctl -u ufserverservice -f
```

### Manual Installation (Advanced)

If you prefer not to use the installation script:

1. Create a user:
   ```bash
   sudo useradd --system --no-create-home --shell /usr/sbin/nologin ufservice
   ```

2. Create directories:
   ```bash
   sudo mkdir -p /opt/ufserverservice
   sudo mkdir -p /var/lib/ufserverservice/{logs,greenlock,audioCache}
   sudo mkdir -p /etc/ufserverservice
   ```

3. Copy the binary:
   ```bash
   sudo cp ufserverservice-linux /opt/ufserverservice/
   sudo chmod +x /opt/ufserverservice/ufserverservice-linux
   ```

4. Create a systemd service file at `/etc/systemd/system/ufserverservice.service`:
   ```ini
   [Unit]
   Description=Universal Framework Service for DayZ
   After=network.target mongod.service
   Wants=mongod.service

   [Service]
   Type=simple
   User=ufservice
   WorkingDirectory=/var/lib/ufserverservice
   Environment="UF_SAVE_PATH=/var/lib/ufserverservice/"
   ExecStart=/opt/ufserverservice/ufserverservice-linux
   Restart=always
   RestartSec=10

   [Install]
   WantedBy=multi-user.target
   ```

5. Set permissions:
   ```bash
   sudo chown -R ufservice:ufservice /opt/ufserverservice
   sudo chown -R ufservice:ufservice /var/lib/ufserverservice
   sudo chown -R ufservice:ufservice /etc/ufserverservice
   ```

6. Reload systemd and start:
   ```bash
   sudo systemctl daemon-reload
   sudo systemctl start ufserverservice
   ```

---

## DayZ Mod Installation

### Installing the Mod

1. Download `@UFramework` from the [GitHub Releases](https://github.com/daemonforge/DayZ-UniveralApi/releases) page

2. Extract the mod folder to your DayZ server's mod directory

3. Add `@UFramework` to your server's `-mod=` launch parameter:
   ```
   -mod=@UFramework;@YourOtherMods
   ```

4. **Important:** The `@UFramework` mod should be loaded before any mods that depend on it

### Mod File Structure

```
@UFramework/
├── Addons/
│   └── UFramework.pbo
└── Keys/
    └── UFramework.bikey
```

### Key Installation

Copy the `.bikey` file from `@UFramework/Keys/` to your server's `keys/` directory if you require client verification.

---

## First Run Configuration

### UF Server Service Configuration

On first run, the service creates a `config.json` file with default values. You must configure:

1. **ServerAuth**: An authentication token is auto-generated, but you may want to generate your own or add multiple tokens for different DayZ servers

2. **Port**: Default is 443 (HTTPS). Change if needed.

3. **MongoDB Connection**: Default is `mongodb://localhost:27017`. Update if your MongoDB is on a different host.

See [Service Configuration](02_ServiceConfiguration.md) for all options.

### DayZ Mod Configuration

On the DayZ server, create/edit the configuration file at `$profile/UF/UFramework.json`:

```json
{
    "ConfigVersion": "1",
    "ServerURL": "https://your-uf-service-address:443/",
    "ServerID": "myserver01",
    "ServerAuth": "your-serverauth-token-from-service-config",
    "EnableBuiltinLogging": 0,
    "PromptDiscordOnConnect": 0
}
```

**Important:** 
- The `ServerAuth` value must match one of the tokens in the UF Server Service's `config.json` `ServerAuth` array
- The `ServerURL` must include the trailing slash and use HTTPS

See [Mod Configuration](03_ModConfiguration.md) for all options.

---

## Verification Steps

### 1. Verify MongoDB is Running

**Windows:**
```powershell
Get-Service -Name MongoDB
```
Status should be "Running"

**Linux:**
```bash
sudo systemctl status mongod
```

### 2. Verify UF Server Service is Running

**Check service status endpoint:**

Using a web browser or curl, navigate to:
```
https://your-server-address:443/Status
```

Expected response:
```json
{
    "Status": "Success",
    "Error": "NoAuth",
    "Version": "2.0.0",
    "Discord": "Disabled",
    "OpenAI": "Pending"
}
```

Notes:
- "NoAuth" in Error field is normal when checking without authentication
- "Discord": "Disabled" is normal if Discord is not configured
- "OpenAI": "Pending" is normal if OpenAI is not configured

### 3. Verify DayZ Server Connection

1. Start your DayZ server with `@UFramework` loaded

2. Check the server logs for:
   ```
   [UFConfig] Config loaded successfully. BaseURL: https://your-server:443/
   ```

3. If you see errors about connection or authentication, verify:
   - The `ServerURL` is correct and accessible from the DayZ server
   - The `ServerAuth` matches between mod config and service config
   - Firewall rules allow the connection

### 4. Common First-Run Issues

| Issue | Solution |
|-------|----------|
| "Connection refused" | Check if UF Service is running and port is accessible |
| "SSL/TLS error" | Service uses self-signed cert by default; this is OK for testing |
| "Authentication failed" | Verify ServerAuth token matches in both configs |
| "MongoDB connection error" | Ensure MongoDB is running and accessible |

See [Troubleshooting](07_Troubleshooting.md) for more detailed solutions.

---

## Firewall Configuration

### Windows Firewall

**Allow inbound connections to UF Service:**

```powershell
New-NetFirewallRule -DisplayName "UF Service HTTPS" -Direction Inbound -LocalPort 443 -Protocol TCP -Action Allow
```

Or via GUI:
1. Open Windows Defender Firewall
2. Click "Advanced settings"
3. Click "Inbound Rules" → "New Rule"
4. Select "Port" → TCP → 443
5. Allow the connection
6. Name the rule "UF Service HTTPS"

### Linux (UFW)

```bash
sudo ufw allow 443/tcp
sudo ufw reload
```

### Linux (firewalld)

```bash
sudo firewall-cmd --permanent --add-port=443/tcp
sudo firewall-cmd --reload
```

### Linux (iptables)

```bash
sudo iptables -A INPUT -p tcp --dport 443 -j ACCEPT
sudo iptables-save > /etc/iptables/rules.v4
```

---

## Port Forwarding

If your UF Service is behind a NAT router, you need to forward the port:

1. **Identify your server's local IP** (e.g., `192.168.1.100`)
2. **Log in to your router's admin panel**
3. **Add a port forwarding rule:**
   - External port: 443
   - Internal IP: Your server's local IP
   - Internal port: 443
   - Protocol: TCP

**Alternative ports:** If port 443 is blocked or in use, you can use any port (e.g., 8443, 3000). Update both the service config and mod config accordingly.

---

## Cloud Provider Notes

### Amazon Web Services (AWS)

**Security Group Configuration:**
1. Navigate to EC2 → Security Groups
2. Select your instance's security group
3. Add Inbound Rule:
   - Type: HTTPS (or Custom TCP)
   - Port: 443
   - Source: 0.0.0.0/0 (or restrict to DayZ server IPs)

### Google Cloud Platform (GCP)

**Firewall Rule:**
```bash
gcloud compute firewall-rules create uf-service-https \
    --allow tcp:443 \
    --target-tags uf-service \
    --description "Allow UF Service HTTPS"
```

### Microsoft Azure

1. Navigate to your VM's Network Security Group
2. Add Inbound security rule:
   - Destination port ranges: 443
   - Protocol: TCP
   - Action: Allow

### DigitalOcean

1. Navigate to Networking → Firewalls
2. Create or edit firewall
3. Add Inbound Rule:
   - Type: HTTPS
   - Port: 443

---

## Health Check Endpoint

The `/Status` endpoint provides a quick health check:

```bash
curl -k https://your-server:443/Status
```

**Response fields:**

| Field | Description |
|-------|-------------|
| `Status` | "Success" when service is running |
| `Error` | "NoAuth" when called without token (normal) |
| `Version` | Service version (e.g., "2.0.0") |
| `Discord` | "Disabled", "Online", or "Offline" |
| `OpenAI` | "Disabled", "Pending", or "Ready" |

**Authenticated health check:**

```bash
curl -k -H "Authorization: Bearer YOUR_SERVER_AUTH_TOKEN" https://your-server:443/Status
```

This returns `"Error": "Ok"` instead of `"NoAuth"` when authentication succeeds.

---

## Upgrading

### Windows

1. Download the new installer
2. Close the running service (right-click tray icon → Exit)
3. Run the new installer (it will update in place)
4. Existing configuration is preserved

### Linux

1. Stop the service:
   ```bash
   sudo systemctl stop ufserverservice
   ```

2. Replace the binary:
   ```bash
   sudo cp ufserverservice-linux /opt/ufserverservice/
   sudo chmod +x /opt/ufserverservice/ufserverservice-linux
   ```

3. Start the service:
   ```bash
   sudo systemctl start ufserverservice
   ```

**Note:** Configuration files are preserved during upgrades. Check the release notes for any configuration changes required.

---

## Uninstallation

### Windows

1. Exit the service (right-click tray icon → Exit)
2. Run the uninstaller from Programs and Features or:
   ```powershell
   & "$env:LOCALAPPDATA\Programs\ufserverservice\Uninstall ufserverservice.exe"
   ```

**Note:** Configuration and data in `%APPDATA%\ufserverservice\` are NOT removed automatically. Delete manually if no longer needed.

### Linux

1. Stop and disable the service:
   ```bash
   sudo systemctl stop ufserverservice
   sudo systemctl disable ufserverservice
   ```

2. Remove files:
   ```bash
   sudo rm -rf /opt/ufserverservice
   sudo rm -rf /var/lib/ufserverservice
   sudo rm -rf /etc/ufserverservice
   sudo rm /etc/systemd/system/ufserverservice.service
   sudo systemctl daemon-reload
   ```

3. Remove the service user (optional):
   ```bash
   sudo userdel ufservice
   ```

## Tags
`operators`, `installation`, `windows`, `linux`, `setup`, `how-to`, `doc-usage`
