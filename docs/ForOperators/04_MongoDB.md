# Universal Framework - MongoDB Guide

This document covers MongoDB installation, configuration, and maintenance for Universal Framework.

## Table of Contents

1. [Overview](#overview)
2. [Installation](#installation)
3. [Configuration](#configuration)
4. [Database Collections](#database-collections)
5. [Maintenance](#maintenance)
6. [Troubleshooting](#troubleshooting)
7. [Backup and Recovery](#backup-and-recovery)

---

## Overview

Universal Framework uses MongoDB as its primary data store. MongoDB is a document-oriented NoSQL database that stores data in flexible, JSON-like documents.

### Why MongoDB?

- **Flexible Schema**: Game data can vary between mods and items
- **JSON Native**: DayZ data is naturally JSON-formatted
- **Scalability**: Handles large amounts of player and object data
- **Performance**: Fast reads and writes for real-time game data

### Minimum Requirements

- MongoDB 4.4 or later (5.0+ recommended)
- At least 1GB RAM dedicated to MongoDB
- SSD storage recommended for better performance
- Storage space depends on player count and mod usage

---

## Installation

### Windows Installation

#### Using Windows Package Manager (Recommended)

```powershell
winget install MongoDB.Server
```

#### Manual Installation

1. Download MongoDB Community Server from https://www.mongodb.com/try/download/community
2. Run the installer
3. Select "Complete" installation
4. Check "Install MongoDB as a Service"
5. Keep the default data directory (`C:\Program Files\MongoDB\Server\X.X\data`)
6. Complete the installation

#### Verify Installation

```powershell
# Check service status
Get-Service -Name MongoDB

# Should output: Running
```

### Linux Installation

#### Debian/Ubuntu

```bash
# Import MongoDB public key
curl -fsSL https://pgp.mongodb.com/server-7.0.asc | \
   sudo gpg -o /usr/share/keyrings/mongodb-server-7.0.gpg --dearmor

# Add repository (Ubuntu 22.04 example)
echo "deb [ signed-by=/usr/share/keyrings/mongodb-server-7.0.gpg ] https://repo.mongodb.org/apt/ubuntu jammy/mongodb-org/7.0 multiverse" | \
   sudo tee /etc/apt/sources.list.d/mongodb-org-7.0.list

# Install
sudo apt update
sudo apt install -y mongodb-org

# Start and enable
sudo systemctl start mongod
sudo systemctl enable mongod
```

#### Fedora/RHEL/CentOS

```bash
# Create repo file
cat <<EOF | sudo tee /etc/yum.repos.d/mongodb-org-7.0.repo
[mongodb-org-7.0]
name=MongoDB Repository
baseurl=https://repo.mongodb.org/yum/redhat/\$releasever/mongodb-org/7.0/x86_64/
gpgcheck=1
enabled=1
gpgkey=https://pgp.mongodb.com/server-7.0.asc
EOF

# Install
sudo dnf install -y mongodb-org

# Start and enable
sudo systemctl start mongod
sudo systemctl enable mongod
```

#### Verify Installation

```bash
sudo systemctl status mongod
mongosh --eval "db.version()"
```

---

## Configuration

### Connection String

The UF Server Service connects to MongoDB using a connection string in `config.json`:

```json
{
    "DBServer": "mongodb://localhost:27017",
    "DB": "DayZ"
}
```

### Connection String Formats

**Local MongoDB (default):**
```
mongodb://localhost:27017
```

**Remote MongoDB:**
```
mongodb://192.168.1.100:27017
```

**With Authentication:**
```
mongodb://username:password@localhost:27017
```

**MongoDB Atlas (Cloud):**
```
mongodb+srv://username:password@cluster0.xxxxx.mongodb.net
```

### MongoDB Configuration (Optional)

The default MongoDB configuration works for most setups. For advanced configurations, edit the MongoDB config file:

**Windows:** `C:\Program Files\MongoDB\Server\X.X\bin\mongod.cfg`
**Linux:** `/etc/mongod.conf`

Example configuration:
```yaml
storage:
  dbPath: /var/lib/mongodb
  journal:
    enabled: true
  wiredTiger:
    engineConfig:
      cacheSizeGB: 1  # Adjust based on available RAM

net:
  port: 27017
  bindIp: 127.0.0.1  # Use 0.0.0.0 for remote access

security:
  authorization: disabled  # Enable for production with auth
```

### Enabling Authentication (Recommended for Production)

1. Connect to MongoDB:
   ```bash
   mongosh
   ```

2. Create admin user:
   ```javascript
   use admin
   db.createUser({
     user: "admin",
     pwd: "your-secure-password",
     roles: [ { role: "userAdminAnyDatabase", db: "admin" } ]
   })
   ```

3. Create database user for UF:
   ```javascript
   use DayZ
   db.createUser({
     user: "ufservice",
     pwd: "your-secure-password",
     roles: [ { role: "readWrite", db: "DayZ" } ]
   })
   ```

4. Enable authentication in MongoDB config:
   ```yaml
   security:
     authorization: enabled
   ```

5. Restart MongoDB and update UF config:
   ```json
   {
     "DBServer": "mongodb://ufservice:your-secure-password@localhost:27017/DayZ"
   }
   ```

---

## Database Collections

Universal Framework creates the following collections in the database:

### Objects

Stores mod-specific object data (bases, storage, custom items, etc.)

| Field | Description |
|-------|-------------|
| `_id` | MongoDB document ID |
| `ObjectId` | Custom object identifier |
| `Mod` | Mod name that owns this data |
| `data` | The actual object data (JSON) |

**Indexes:**
- `{ ObjectId: 1, Mod: 1 }` - Compound index for fast lookups

### Players

Stores player data and Discord linking information

| Field | Description |
|-------|-------------|
| `_id` | MongoDB document ID |
| `GUID` | Player's Steam GUID (normalized) |
| `AUTH` | Current authentication token |
| `Discord` | Discord linking information |
| `[ModName]` | Per-mod player data |

**Indexes:**
- `{ GUID: 1 }` - Fast player lookup

### Globals

Stores global server variables and mod configurations.

| Field | Description |
|-------|-------------|
| `_id` | MongoDB document ID |
| `Key` | Global variable name |
| `Mod` | Mod name that owns this variable |
| `Value` | The stored value |

### Messages

Stores in-game and Discord messages for the messaging system.

| Field | Description |
|-------|-------------|
| `_id` | MongoDB document ID |
| `channel` | Message channel/scope |
| `content` | Message content |
| `author` | Sender information |

### AI Collections

- **Chats**: Stores OpenAI chat session history
- **Assistants**: configuration for AI assistants
- **Threads**: OpenAI thread IDs and state

- `{ GUID: 1, AUTH: 1 }` - Auth token validation

### Globals

Stores global mod state (server-wide data)

| Field | Description |
|-------|-------------|
| `_id` | MongoDB document ID |
| `Mod` | Mod name |
| `Data` | Global data object |

**Indexes:**
- `{ Mod: 1 }` - Fast mod lookup

### Messages

Stores message queue data

| Field | Description |
|-------|-------------|
| `_id` | MongoDB document ID |
| `Mod` | Mod name |
| `Queue` | Queue name |
| `content` | Message content |
| `createdAt` | Timestamp |

**Indexes:**
- `{ Mod: 1, Queue: 1, createdAt: 1 }` - Efficient queue queries

### AIChats / AISessions

Stores AI chat sessions and message history (if AI features are used)

---

## Maintenance

### Index Creation

UF Service automatically creates indexes on first run when `CreateIndexes: true` in config. After successful creation, it sets this to `false`.

To manually trigger index creation:
1. Set `"CreateIndexes": true` in config.json
2. Restart UF Service
3. Check logs for "Successfully Created Indexes"

### Monitoring Database Size

**Using MongoDB Shell:**
```javascript
use DayZ
db.stats()
```

**Check collection sizes:**
```javascript
db.Objects.stats().size
db.Players.stats().size
db.Globals.stats().size
```

### Cleaning Old Data

**Remove old AI chat sessions (older than 30 days):**
```javascript
use DayZ
db.AIChats.deleteMany({
  createdAt: { $lt: new Date(Date.now() - 30*24*60*60*1000) }
})
```

**Remove orphaned messages:**
```javascript
db.Messages.deleteMany({
  createdAt: { $lt: new Date(Date.now() - 7*24*60*60*1000) }
})
```

### Compacting Database

After deleting significant data, compact the database to reclaim space:

```javascript
use DayZ
db.runCommand({ compact: "Objects" })
db.runCommand({ compact: "Players" })
```

---

## Troubleshooting

### MongoDB Won't Start

**Windows:**
```powershell
# Check service logs
Get-EventLog -LogName Application -Source MongoDB -Newest 10

# Try starting manually to see errors
& "C:\Program Files\MongoDB\Server\X.X\bin\mongod.exe" --dbpath "C:\data\db"
```

**Linux:**
```bash
# Check systemd logs
sudo journalctl -u mongod -n 50

# Check MongoDB log file
sudo tail -50 /var/log/mongodb/mongod.log
```

### Common Startup Errors

| Error | Solution |
|-------|----------|
| `dbpath does not exist` | Create the data directory manually |
| `Address already in use` | Another process is using port 27017 |
| `Insufficient permissions` | Check file ownership and permissions |
| `WiredTiger error` | Disk space issue or corrupted data |

### UF Service Can't Connect

**Check MongoDB is running:**
```bash
# Windows
Get-Service MongoDB

# Linux
sudo systemctl status mongod
```

**Test connection manually:**
```bash
mongosh mongodb://localhost:27017
```

**Common connection errors:**

| Error | Cause | Solution |
|-------|-------|----------|
| `Connection refused` | MongoDB not running | Start MongoDB service |
| `Authentication failed` | Wrong credentials | Verify username/password |
| `Network timeout` | Firewall or network issue | Check firewall rules |
| `SSL handshake failed` | SSL configuration mismatch | Verify SSL settings |

---

## Backup and Recovery

### Manual Backup

**Using mongodump:**
```bash
mongodump --db DayZ --out /path/to/backup/$(date +%Y%m%d)
```

**Windows:**
```powershell
& "C:\Program Files\MongoDB\Server\X.X\bin\mongodump.exe" --db DayZ --out "C:\backups\$(Get-Date -Format yyyyMMdd)"
```

### Automated Backups (Linux)

Create a backup script `/usr/local/bin/backup-mongodb.sh`:
```bash
#!/bin/bash
BACKUP_DIR="/var/backups/mongodb"
DATE=$(date +%Y%m%d_%H%M%S)
mongodump --db DayZ --out "$BACKUP_DIR/$DATE"
# Keep only last 7 days
find $BACKUP_DIR -type d -mtime +7 -exec rm -rf {} +
```

Add to crontab:
```bash
0 3 * * * /usr/local/bin/backup-mongodb.sh
```

### Restore from Backup

```bash
mongorestore --db DayZ /path/to/backup/DayZ
```

**Warning:** This will merge with existing data. To replace completely:
```bash
mongosh --eval "use DayZ; db.dropDatabase()"
mongorestore --db DayZ /path/to/backup/DayZ
```

### Point-in-Time Recovery

For production systems, consider enabling:
- Replica sets for high availability
- Oplog for point-in-time recovery
- MongoDB Atlas for managed backups

---

## Performance Tips

1. **SSD Storage**: Use SSD for the data directory
2. **RAM Allocation**: Ensure WiredTiger cache has adequate RAM
3. **Indexes**: Verify indexes are created (check logs)
4. **Connection Pooling**: UF Service uses connection pooling by default
5. **Monitor Slow Queries**: Enable profiling for troubleshooting
   ```javascript
   db.setProfilingLevel(1, { slowms: 100 })
   ```

---

## Security Recommendations

1. **Enable Authentication**: Use username/password for production
2. **Bind to localhost**: Only allow local connections if possible
3. **Firewall Rules**: Block port 27017 from external access
4. **Regular Updates**: Keep MongoDB updated for security patches
5. **Encrypted Connections**: Use TLS for remote connections

## Tags
`operators`, `mongodb`, `database`, `maintenance`, `security`, `performance`, `how-to`, `doc-usage`
