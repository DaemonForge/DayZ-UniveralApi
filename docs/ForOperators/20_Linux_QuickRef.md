# Universal Framework - Linux Quick Reference

This is a quick reference for running UF Server Service on Linux. For detailed instructions, see [Installation Guide](01_Installation.md).

---

## Installation

### Automated Installation (Recommended)

```bash
# Download the binary and install script
wget https://github.com/daemonforge/DayZ-UniveralApi/releases/download/vX.X.X/ufserverservice-linux
wget https://github.com/daemonforge/DayZ-UniveralApi/releases/download/vX.X.X/install.sh
chmod +x install.sh ufserverservice-linux

# Run installer as root
sudo ./install.sh ./ufserverservice-linux
```

The installer will:
- Create service user (`ufservice`)
- Install binary to `/opt/ufserverservice/`
- Create data directories
- Generate default configuration
- Create and enable systemd service

---

## File Locations

| File/Folder | Location |
|-------------|----------|
| Binary | `/opt/ufserverservice/ufserverservice-linux` |
| Config file | `/etc/ufserverservice/config.json` **or** `/var/lib/ufserverservice/config.json` |
| Data directory | `/var/lib/ufserverservice/` |
| Log files | `/var/lib/ufserverservice/logs/` |
| Templates | `/var/lib/ufserverservice/templates/` |
| SSL certs (Greenlock) | `/var/lib/ufserverservice/greenlock/` |
| Audio cache (TTS) | `/var/lib/ufserverservice/audioCache/` |
| Systemd service | `/etc/systemd/system/ufserverservice.service` |

---

## Service Management

### Systemd Commands

```bash
# Start service
sudo systemctl start ufserverservice

# Stop service
sudo systemctl stop ufserverservice

# Restart service
sudo systemctl restart ufserverservice

# Check status
sudo systemctl status ufserverservice

# Enable auto-start on boot
sudo systemctl enable ufserverservice

# Disable auto-start
sudo systemctl disable ufserverservice

# Reload systemd after service file changes
sudo systemctl daemon-reload
```

---

## Viewing Logs

### Systemd Journal

```bash
# View all logs
sudo journalctl -u ufserverservice

# View logs since last boot
sudo journalctl -u ufserverservice -b

# View last 100 lines
sudo journalctl -u ufserverservice -n 100

# Follow logs in real-time
sudo journalctl -u ufserverservice -f

# Logs from last hour
sudo journalctl -u ufserverservice --since "1 hour ago"

# Logs from specific date
sudo journalctl -u ufserverservice --since "2024-01-01 00:00:00"
```

### Log Files

```bash
# List log files
ls -la /var/lib/ufserverservice/logs/

# View latest log
tail -100 /var/lib/ufserverservice/logs/UF-*.log

# Watch logs in real-time
tail -f /var/lib/ufserverservice/logs/UF-*.log

# Search logs for errors
grep -i error /var/lib/ufserverservice/logs/UF-*.log
```

---

## Configuration

### Edit Config

```bash
# Edit config file
sudo nano /var/lib/ufserverservice/config.json
# or
sudo vim /etc/ufserverservice/config.json

# Validate JSON syntax
cat /var/lib/ufserverservice/config.json | python3 -m json.tool

# Restart after changes
sudo systemctl restart ufserverservice
```

### View Current Config

```bash
cat /var/lib/ufserverservice/config.json | python3 -m json.tool
```

---

## Templates (Discord Linking Pages)

### Location

```bash
/var/lib/ufserverservice/templates/
```

### Template Files

| File | Purpose |
|------|---------|
| `discordLogin.ejs` | Login/linking page |
| `discordSuccess.ejs` | Success confirmation page |
| `discordError.ejs` | Error page |
| `favicon.ico` | Browser tab icon |
| `icon.png` | Page icon |
| `icon.svg` | Scalable icon |

### Edit Templates

```bash
# Navigate to templates
cd /var/lib/ufserverservice/templates/

# List templates
ls -la

# Edit a template
sudo nano discordLogin.ejs

# Templates reload automatically on next request
```

### Backup Templates

```bash
# Backup current templates
sudo cp -r /var/lib/ufserverservice/templates /var/lib/ufserverservice/templates.backup
```

---

## Firewall Configuration

### UFW (Ubuntu/Debian)

```bash
# Allow HTTPS
sudo ufw allow 443/tcp

# Check status
sudo ufw status verbose

# Reload
sudo ufw reload
```

### firewalld (RHEL/CentOS/Fedora)

```bash
# Allow HTTPS
sudo firewall-cmd --permanent --add-port=443/tcp

# Reload
sudo firewall-cmd --reload

# Verify
sudo firewall-cmd --list-ports
```

### iptables

```bash
# Allow HTTPS
sudo iptables -A INPUT -p tcp --dport 443 -j ACCEPT

# Save rules (Debian/Ubuntu)
sudo iptables-save > /etc/iptables/rules.v4

# Save rules (RHEL/CentOS)
sudo service iptables save
```

---

## Health Check

```bash
# Check if service is running
sudo systemctl is-active ufserverservice

# Check if port is listening
ss -tlnp | grep 443

# Test endpoint
curl -k https://localhost:443/Status

# Test with auth token
curl -k -H "Authorization: Bearer YOUR_TOKEN" https://localhost:443/Status
```

---

## Troubleshooting

### Service Won't Start

```bash
# Check detailed status
sudo systemctl status ufserverservice -l

# Check journal for errors
sudo journalctl -u ufserverservice -n 50 --no-pager

# Check file permissions
ls -la /opt/ufserverservice/
ls -la /var/lib/ufserverservice/

# Fix permissions
sudo chown -R ufservice:ufservice /opt/ufserverservice
sudo chown -R ufservice:ufservice /var/lib/ufserverservice
```

### Port Permission Issues

Ports below 1024 require special permissions:

```bash
# Option 1: Use port 8443 instead
sudo nano /var/lib/ufserverservice/config.json
# Change "Port": 443 to "Port": 8443

# Option 2: Grant capability to bind to low ports
sudo setcap 'cap_net_bind_service=+ep' /opt/ufserverservice/ufserverservice-linux
```

### MongoDB Connection Issues

```bash
# Check MongoDB is running
sudo systemctl status mongod

# Start MongoDB
sudo systemctl start mongod

# Test MongoDB connection
mongosh --eval "db.adminCommand('ping')"
```

### Config Syntax Errors

```bash
# Validate JSON
cat /var/lib/ufserverservice/config.json | python3 -m json.tool

# If errors, fix and restart
sudo systemctl restart ufserverservice
```

---

## Prerequisites

### MongoDB Installation

**Debian/Ubuntu:**
```bash
sudo apt update
sudo apt install -y mongodb
sudo systemctl enable mongod
sudo systemctl start mongod
```

**RHEL/CentOS/Fedora:**
```bash
sudo dnf install -y mongodb-org
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

**FFmpeg (for TTS):**
```bash
# Debian/Ubuntu
sudo apt install -y ffmpeg

# RHEL/CentOS/Fedora
sudo dnf install -y ffmpeg

# Arch
sudo pacman -S ffmpeg
```

**ImageMagick (for DDS conversion):**
```bash
# Debian/Ubuntu
sudo apt install -y imagemagick

# RHEL/CentOS/Fedora
sudo dnf install -y ImageMagick

# Arch
sudo pacman -S imagemagick
```

---

## Manual Installation

If you prefer not to use the install script:

```bash
# Create user
sudo useradd --system --no-create-home --shell /usr/sbin/nologin ufservice

# Create directories
sudo mkdir -p /opt/ufserverservice
sudo mkdir -p /var/lib/ufserverservice/{logs,templates,greenlock,audioCache}
sudo mkdir -p /etc/ufserverservice

# Copy binary
sudo cp ufserverservice-linux /opt/ufserverservice/
sudo chmod +x /opt/ufserverservice/ufserverservice-linux

# Set permissions
sudo chown -R ufservice:ufservice /opt/ufserverservice
sudo chown -R ufservice:ufservice /var/lib/ufserverservice
sudo chown -R ufservice:ufservice /etc/ufserverservice

# Create systemd service file
sudo nano /etc/systemd/system/ufserverservice.service
```

**Systemd Service File:**
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

```bash
# Enable and start
sudo systemctl daemon-reload
sudo systemctl enable ufserverservice
sudo systemctl start ufserverservice
```

---

## Quick Config Example

**Minimal `/var/lib/ufserverservice/config.json`:**
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

---

## Useful Aliases

Add to `~/.bashrc` for convenience:

```bash
# UF Service shortcuts
alias uf-status='sudo systemctl status ufserverservice'
alias uf-start='sudo systemctl start ufserverservice'
alias uf-stop='sudo systemctl stop ufserverservice'
alias uf-restart='sudo systemctl restart ufserverservice'
alias uf-logs='sudo journalctl -u ufserverservice -f'
alias uf-config='sudo nano /var/lib/ufserverservice/config.json'
alias uf-check='curl -k https://localhost:443/Status'
```

Then: `source ~/.bashrc`

## Tags
`operators`, `linux`, `quickref`, `systemd`, `service`, `how-to`, `doc-usage`
