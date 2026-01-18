# Universal Framework - Troubleshooting Guide

This document provides solutions for common issues with Universal Framework.

## Table of Contents

1. [Quick Diagnostics](#quick-diagnostics)
2. [Service Won't Start](#service-wont-start)
3. [MongoDB Connection Issues](#mongodb-connection-issues)
4. [DayZ Mod Connection Issues](#dayz-mod-connection-issues)
5. [Authentication Problems](#authentication-problems)
6. [Discord Integration Issues](#discord-integration-issues)
7. [AI/OpenAI Issues](#aiopenai-issues)
8. [Performance Issues](#performance-issues)
9. [Log Analysis](#log-analysis)
10. [Common Error Messages](#common-error-messages)

---

## Quick Diagnostics

### Check Service Status

**Status Endpoint:**
```
https://your-server:port/Status
```

**Expected Response:**
```json
{
    "Status": "Success",
    "Error": "NoAuth",
    "Version": "2.0.0",
    "Discord": "Online",
    "OpenAI": "Online"
}
```

**Status Field Meanings:**

| Field | Value | Meaning |
|-------|-------|---------|
| `Status` | `"Success"` | Service is running and database is writable |
| `Error` | `"NoAuth"` | Normal when accessing without auth token |
| `Error` | `"noerror"` | Authenticated request succeeded |
| `Discord` | `"Online"` | Discord bot connected |
| `Discord` | `"Disabled"` | Discord not configured |
| `Discord` | `"Error"` | Discord configuration error |
| `OpenAI` | `"Online"` | OpenAI configured and working |
| `OpenAI` | `"Pending"` | OpenAI not configured or initializing |

### Check Logs

**Windows:**
- Right-click tray icon > "View Logs"
- Or check: `%APPDATA%\ufserverservice\logs\`

**Linux:**
```bash
sudo journalctl -u ufserverservice -f
# or
sudo tail -f /var/log/ufserverservice/stdout.log
```

---

## Service Won't Start

### Windows: Application Crashes Immediately

**Symptoms:**
- Tray icon appears briefly then disappears
- No error message shown

**Possible Causes and Solutions:**

1. **Port already in use**
   ```powershell
   netstat -ano | findstr :443
   ```
   If a process is using the port, either stop it or change the port in config.

2. **Corrupted config.json**
   - Delete `%APPDATA%\ufserverservice\config.json`
   - Restart the application (new config will be generated)

3. **Missing Visual C++ Redistributable**
   - Install from Microsoft: https://aka.ms/vs/17/release/vc_redist.x64.exe

### Linux: Service Fails to Start

**Check systemd status:**
```bash
sudo systemctl status ufserverservice
```

**Common issues:**

1. **Permission denied**
   ```bash
   # Check file permissions
   ls -la /opt/ufserverservice/
   ls -la /var/lib/ufserverservice/
   
   # Fix permissions
   sudo chown -R ufservice:ufservice /opt/ufserverservice
   sudo chown -R ufservice:ufservice /var/lib/ufserverservice
   ```

2. **Port already in use**
   ```bash
   sudo lsof -i :443
   ```

3. **Config file issues**
   ```bash
   # Check config syntax
   cat /etc/ufserverservice/config.json | python3 -m json.tool
   ```

### Port Permission Issues (Linux)

Ports below 1024 require root privileges.

**Option 1: Use port 8443 instead**
```json
{ "Port": 8443 }
```

**Option 2: Grant capability to bind to low ports**
```bash
sudo setcap 'cap_net_bind_service=+ep' /opt/ufserverservice/ufserverservice-linux
```

---

## MongoDB Connection Issues

### "MongoServerSelectionError: connect ECONNREFUSED"

**Cause:** MongoDB is not running or not accessible.

**Windows:**
```powershell
# Check if MongoDB service is running
Get-Service -Name MongoDB

# Start if stopped
Start-Service MongoDB
```

**Linux:**
```bash
# Check MongoDB status
sudo systemctl status mongod

# Start if stopped
sudo systemctl start mongod
```

### "Authentication failed"

**Cause:** Wrong username/password in connection string.

**Solution:**
1. Verify credentials in `config.json`:
   ```json
   "DBServer": "mongodb://username:password@localhost:27017"
   ```
2. Test connection with mongosh:
   ```bash
   mongosh mongodb://username:password@localhost:27017
   ```

### "Database Write Error" in Status Check

**Cause:** MongoDB is running but cannot write.

**Check:**
1. Database exists and is accessible
2. Disk space available
3. User has write permissions

```bash
# Check disk space
df -h

# Test write in MongoDB
mongosh DayZ --eval "db.test.insertOne({test: 1})"
```

---

## DayZ Mod Connection Issues

### Config Loading Errors

**Symptom:** Server log shows `[UFConfig] Config is still null after Load()`

**Check:**
1. Configuration file exists at `$profile/UF/UFramework.json`
2. File is valid JSON (no syntax errors)
3. All required fields are present

### "UniversalRest.Post called with invalid token"

**Cause:** Authentication token is not being sent correctly.

**Check:**
1. `ServerAuth` in mod config matches a token in service config
2. `ServerURL` is correct and accessible
3. UF Service is running and reachable

**Test from DayZ server machine:**
```bash
curl -k https://your-service:port/Status
```

### Connection Refused/Timeout

**Possible Causes:**

1. **UF Service not running**
   - Verify service is running (check tray icon or systemd status)

2. **Firewall blocking connection**
   - Windows: Check Windows Firewall
   - Linux: Check iptables/firewalld
   ```bash
   sudo firewall-cmd --add-port=443/tcp --permanent
   sudo firewall-cmd --reload
   ```

3. **Wrong IP/Port in ServerURL**
   - Verify the URL is correct
   - Try using IP address instead of hostname
   - Verify port matches config

4. **Network routing issues**
   - If DayZ server and UF Service are on different networks, verify routing

### SSL/TLS Errors

DayZ's RestApi should accept self-signed certificates. If you see SSL errors:

1. Verify the service is running on HTTPS (not HTTP)
2. Check certificate files exist (if using custom certs)
3. Try accessing the service from a browser to diagnose cert issues

---

## Authentication Problems

### "NoAuth" Error on All Requests

**Cause:** `ServerAuth` token mismatch.

**Solution:**
1. Copy token from UF Service `config.json`:
   ```json
   "ServerAuth": ["abc123xyz..."]
   ```
2. Paste into DayZ mod `UFramework.json`:
   ```json
   "ServerAuth": "abc123xyz..."
   ```
3. Restart both services

### Player Authentication Fails

**Symptoms:**
- Players can't access their data
- "Unauthorized" errors in logs

**Check:**
1. Player has connected to server at least once
2. Player auth token hasn't expired
3. Database contains player record

```javascript
// In MongoDB
use DayZ
db.Players.findOne({ GUID: "player-guid-here" })
```

### Token Expiry Issues

Player auth tokens expire after 15 minutes of inactivity. The DayZ mod automatically renews tokens while players are connected.

If experiencing token issues:
1. Have the player reconnect to the server
2. Check if the DayZ server can reach UF Service

---

## Discord Integration Issues

### Bot Won't Connect

**Check:**
1. `Bot_Token` is correct (not Client_Secret)
2. Bot is invited to the guild specified in `Guild_Id`
3. Bot has required permissions

**Log indicators:**
```
Discord Pending... - Bot is trying to connect
Discord Ready - Bot connected successfully
Discord Error - Check bot token and permissions
```

### "User not found in discord"

**Meaning:** The Discord user is not in your Discord server.

**Solution:**
- The player's Discord account must be a member of the guild
- They may have left or been banned

### Role Operations Fail

**"Cannot manage role":**
1. Check bot's role hierarchy in Discord server settings
2. Bot's role must be ABOVE roles it tries to manage
3. Move the bot's role higher in the role list

### OAuth2 Callback Errors

**"Invalid redirect_uri":**
1. Go to Discord Developer Portal > OAuth2
2. Verify redirect URL matches exactly: `https://your-domain:port/discord/callback`
3. Include the port number if not using 443
4. Ensure HTTPS, not HTTP

---

## AI/OpenAI Issues

### "OpenAI: Pending" Status

**Meaning:** OpenAI API key not configured or invalid.

**Check:**
1. `OpenAIApi.ApiKey` is set in config
2. API key is valid (starts with `sk-`)
3. API key has available credits

### Rate Limits / Quota Exceeded

OpenAI has usage limits. Check your OpenAI dashboard for:
- Rate limits (requests per minute)
- Usage quotas (tokens per month)
- Billing status

### Prompt Protection Errors

If AI responses are being blocked:
```json
"OpenAIApi": {
    "enablePromptProtection": false
}
```

**Warning:** Disabling prompt protection allows prompt injection attacks.

---

## Performance Issues

### High CPU Usage

**Check:**
1. Rate limiting is working (check logs for "RateLimited" messages)
2. Database indexes exist
3. Connection pooling is working

**Force index creation:**
```json
{ "CreateIndexes": true }
```
Restart service, then set back to false.

### High Memory Usage

**Possible causes:**
1. Log history accumulation
2. Many concurrent connections
3. Large AI chat sessions

**Solutions:**
- Restart service periodically
- Clean old AI chat sessions from database
- Increase available RAM or reduce cpuCount

### Slow API Responses

**Check:**
1. MongoDB performance (slow queries)
2. Network latency between services
3. Discord API rate limits
4. OpenAI API response times

---

## Log Analysis

### Log Levels

| Level | Meaning |
|-------|---------|
| `error` | Critical issues requiring attention |
| `warn` | Potential problems to monitor |
| `info` | Normal operational messages |
| `debug` | Detailed information for troubleshooting |

### Common Log Patterns

**Successful startup:**
```
[App] Starting Universal Framework Service v2.0.0
[WebServer] Starting HTTPS server with bundled certificates
[App] API Webservice started
MongoDB connection pool established for Objects
MongoDB connection pool established for Players
Successfully Created Indexes
```

**Database issues:**
```
MongoDB connection lost, reconnecting...
MongoServerSelectionError: connect ECONNREFUSED
```

**Rate limiting (possible attack or need to increase limits):**
```
[WebServer] RateLimit reached - possible DDoS attack or need to increase request limit
```

**Discord connection:**
```
Discord Pending...
Discord Ready
```

### Enabling Debug Logging

For more detailed logs during troubleshooting, the log level is controlled by the packaged state:
- Development builds: debug level
- Production builds: info level

Check the logs directory for daily rotating log files:
- Windows: `%APPDATA%\ufserverservice\logs\`
- Linux: `/var/log/ufserverservice/` or `/var/lib/ufserverservice/logs/`

---

## Common Error Messages

> **For a complete list of error codes and log messages, see [Error Codes Reference](22_Error_Codes_Reference.md).**

### Service Errors

| Error | Cause | Solution |
|-------|-------|----------|
| `EADDRINUSE` | Port already in use | Change port or stop other service |
| `EACCES` | Permission denied | Run as admin or use port > 1024 |
| `ENOENT` | File not found | Check file paths in config |

### API Errors

| HTTP Status | Meaning | Solution |
|-------------|---------|----------|
| 204 | No Auth / Not Found | Check auth token |
| 400 | Bad Request | Check request body format |
| 429 | Rate Limited | Wait or whitelist IP |
| 500 | Internal Error | Check logs for details |
| 501 | Bad URL | Endpoint doesn't exist |

### Database Errors

| Error | Cause | Solution |
|-------|-------|----------|
| `ECONNREFUSED` | MongoDB not running | Start MongoDB |
| `auth failed` | Wrong credentials | Check connection string |
| `ns not found` | Collection missing | Will be auto-created |

### Discord Errors

| Error | Cause | Solution |
|-------|-------|----------|
| `TOKEN_INVALID` | Wrong bot token | Check Bot_Token |
| `MISSING_ACCESS` | Bot lacks permissions | Check bot permissions |
| `Unknown Guild` | Bot not in server | Invite bot to guild |

---

## Getting Help

If you can't resolve an issue:

1. **Check existing issues:** https://github.com/daemonforge/DayZ-UniveralApi/issues

2. **Gather information:**
   - UF Service version
   - Operating system
   - Relevant log entries
   - Steps to reproduce

3. **Open a new issue** with the gathered information

4. **Community support** may be available through Discord (check GitHub for links)

---

## Diagnostic Commands Reference

### Service Verification

**Windows PowerShell:**
```powershell
# Check if service is listening
Test-NetConnection -ComputerName localhost -Port 443

# Find process using port
Get-NetTCPConnection -LocalPort 443 | Select-Object OwningProcess | Get-Process

# Check service logs (last 100 lines)
Get-Content "$env:APPDATA\ufserverservice\logs\UF-*.log" -Tail 100

# Test endpoint (requires PowerShell 6+)
Invoke-RestMethod -Uri "https://localhost:443/Status" -SkipCertificateCheck
```

**Linux Bash:**
```bash
# Check if service is listening
ss -tlnp | grep 443

# Test endpoint
curl -k https://localhost:443/Status

# Authenticated test
curl -k -H "Authorization: Bearer YOUR_TOKEN" https://localhost:443/Status

# Check service logs
journalctl -u ufserverservice --since "1 hour ago"

# Watch logs in real-time
journalctl -u ufserverservice -f

# Check MongoDB connection
mongo --eval "db.adminCommand('ping')"

# MongoDB version
mongod --version

# Disk space
df -h

# Memory usage
free -h

# CPU usage
top -bn1 | head -20
```

### MongoDB Diagnostics

```bash
# Connect to MongoDB shell
mongosh DayZ

# List all collections
show collections

# Count documents per collection
db.Objects.countDocuments()
db.Players.countDocuments()
db.Globals.countDocuments()
db.Messages.countDocuments()
db.AIChats.countDocuments()
db.Logs.countDocuments()

# Check collection stats
db.Objects.stats()

# Find recent player connections
db.Players.find().sort({LastSeen: -1}).limit(5)

# Check indexes
db.Objects.getIndexes()

# Database size
db.stats()
```

### Network Diagnostics

**Windows:**
```powershell
# Trace route to UF Service (from DayZ server)
tracert your-uf-service.com

# DNS lookup
nslookup your-uf-service.com

# Check firewall rules
Get-NetFirewallRule -DisplayName "*UF*"

# List all listening ports
netstat -an | findstr "LISTENING"
```

**Linux:**
```bash
# Trace route
traceroute your-uf-service.com

# DNS lookup
dig your-uf-service.com

# Check firewall (UFW)
sudo ufw status verbose

# Check firewall (firewalld)
sudo firewall-cmd --list-all

# Check iptables
sudo iptables -L -n

# Check open connections
ss -s
```

---

## HTTP Status Code Quick Reference

| Code | Name | Framework Meaning |
|------|------|------------------|
| `200` | OK | Request successful, data returned |
| `201` | Created | New record created |
| `204` | No Content | No auth token OR object not found |
| `205` | Reset Content | Object not found in database |
| `206` | Partial Content | Duplicate key (record exists) |
| `207` | Multi-Status | Bad query format |
| `400` | Bad Request | Malformed request body |
| `404` | Not Found | Endpoint not found |
| `429` | Too Many Requests | Rate limit exceeded |
| `500` | Internal Server Error | Unhandled server error |
| `501` | Not Implemented | Bad URL path |

---

## Callback Status Code Reference

When mods receive callback responses, these status codes indicate outcomes:

### Database Operations

| Operation | Success | Not Found | Duplicate | Error |
|-----------|---------|-----------|-----------|-------|
| Load | 200 | 205 | - | 500 |
| Save | 200 | - | 206 | 500 |
| Update | 200 | 205 | - | 500 |
| Query | 200 | 200 (empty) | - | 207 |
| Transaction | 200 | 205 | - | 500 |

### Discord Operations

| Operation | Success | User Not Found | Permission Error | Bot Not Ready |
|-----------|---------|----------------|------------------|---------------|
| AddRole | 200 | 205 | 500 | 500 |
| RemoveRole | 200 | 205 | 500 | 500 |
| UserSend | 200 | 205 | 500 | 500 |
| ChannelSend | 200 | - | 500 | 500 |

### AI Operations

| Operation | Success | Session Not Found | API Error |
|-----------|---------|-------------------|-----------|
| Create | 200 | - | 500 |
| Send | 200 | 205 | 500 |
| Poll | 200 (with data) | 205 | 500 |

---

## Emergency Recovery Procedures

### Service Won't Start After Update

1. **Backup current config:**
   ```bash
   cp config.json config.json.backup
   ```

2. **Try with default config:**
   ```bash
   mv config.json config.json.old
   # Start service to generate new default config
   # Then merge settings from old config
   ```

3. **Rollback to previous version:**
   - Download previous release from GitHub
   - Replace executable only (keep config)

### Database Corruption

**Signs:**
- Queries return unexpected errors
- Service crashes on database operations
- MongoDB logs show corruption errors

**Recovery:**
```bash
# Stop service
sudo systemctl stop ufserverservice

# Repair MongoDB
mongod --repair

# Or restore from backup
mongorestore --db DayZ /path/to/backup/DayZ
```

### High Load / DDoS

**Immediate actions:**
1. Check `RateLimitWhiteList` - ensure your DayZ server IP is listed
2. Lower rate limits temporarily
3. Check logs for attacking IPs
4. Block offending IPs at firewall level

```bash
# Block IP with iptables
sudo iptables -A INPUT -s ATTACKING_IP -j DROP

# Block IP with UFW
sudo ufw deny from ATTACKING_IP
```

### Player Data Loss

**Before server wipe:**
```bash
# Full backup
mongodump --db DayZ --out /backup/$(date +%Y%m%d)
```

**If data was accidentally deleted:**
```bash
# Restore specific collection
mongorestore --db DayZ --collection Players /backup/date/DayZ/Players.bson
```

---

## Health Check Script

Save this as `health-check.sh` for automated monitoring:

```bash
#!/bin/bash
# UF Service Health Check

URL="https://localhost:443/Status"
EXPECTED="Success"

response=$(curl -sk "$URL" 2>&1)

if echo "$response" | grep -q "$EXPECTED"; then
    echo "OK: UF Service is healthy"
    exit 0
else
    echo "CRITICAL: UF Service check failed"
    echo "Response: $response"
    exit 2
fi
```

**Usage with cron for monitoring:**
```bash
*/5 * * * * /path/to/health-check.sh || echo "UF Service down" | mail -s "Alert" admin@example.com
```

## Tags
`operators`, `troubleshooting`, `health-check`, `monitoring`, `errors`, `how-to`, `doc-usage`
