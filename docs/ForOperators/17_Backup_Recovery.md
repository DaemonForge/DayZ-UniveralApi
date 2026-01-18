# Backup & Recovery Guide

This document covers backup strategies, disaster recovery procedures, and data migration for UF Server Service.

## Overview

Critical data to back up:
1. **MongoDB Database** - All player, object, and global data
2. **Configuration Files** - Service and Let's Encrypt config
3. **SSL Certificates** - Custom or Let's Encrypt certificates
4. **Log Files** - For historical analysis

---

## MongoDB Backup

### Using mongodump

The primary tool for MongoDB backups is `mongodump`.

**Basic Backup**:
```bash
mongodump --uri="mongodb://localhost:27017" --db=DayZ --out=/backup/mongodb/$(date +%Y-%m-%d)
```

**With Authentication**:
```bash
mongodump --uri="mongodb://username:password@localhost:27017" --db=DayZ --out=/backup/mongodb/$(date +%Y-%m-%d)
```

**Compressed Backup**:
```bash
mongodump --uri="mongodb://localhost:27017" --db=DayZ --gzip --archive=/backup/DayZ-$(date +%Y-%m-%d).gz
```

### Automated Backup Script (Linux)

Create `/opt/scripts/backup-uf.sh`:
```bash
#!/bin/bash
BACKUP_DIR="/backup/ufservice"
DATE=$(date +%Y-%m-%d_%H-%M)
RETENTION_DAYS=30

# Create backup directory
mkdir -p "$BACKUP_DIR/$DATE"

# MongoDB backup
mongodump --uri="mongodb://localhost:27017" \
  --db=DayZ \
  --gzip \
  --archive="$BACKUP_DIR/$DATE/mongodb.gz"

# Config backup
cp /etc/ufserverservice/config.json "$BACKUP_DIR/$DATE/"

# Let's Encrypt backup
if [ -d "/var/lib/ufserverservice/greenlock" ]; then
  tar -czf "$BACKUP_DIR/$DATE/greenlock.tar.gz" /var/lib/ufserverservice/greenlock
fi

# Cleanup old backups
find "$BACKUP_DIR" -maxdepth 1 -type d -mtime +$RETENTION_DAYS -exec rm -rf {} \;

echo "Backup completed: $BACKUP_DIR/$DATE"
```

**Set up cron job**:
```bash
# Daily backup at 4 AM
0 4 * * * /opt/scripts/backup-uf.sh >> /var/log/uf-backup.log 2>&1
```

### Automated Backup Script (Windows PowerShell)

Create `C:\Scripts\backup-uf.ps1`:
```powershell
$BackupDir = "C:\Backup\UFService"
$Date = Get-Date -Format "yyyy-MM-dd_HH-mm"
$RetentionDays = 30
$MongoPath = "C:\Program Files\MongoDB\Server\7.0\bin"

# Create backup directory
New-Item -ItemType Directory -Force -Path "$BackupDir\$Date"

# MongoDB backup
& "$MongoPath\mongodump.exe" --uri="mongodb://localhost:27017" `
  --db=DayZ `
  --gzip `
  --archive="$BackupDir\$Date\mongodb.gz"

# Config backup
Copy-Item "$env:APPDATA\ufserverservice\config.json" "$BackupDir\$Date\"

# Let's Encrypt backup
$GreenlockPath = "$env:APPDATA\ufserverservice\greenlock"
if (Test-Path $GreenlockPath) {
  Compress-Archive -Path $GreenlockPath -DestinationPath "$BackupDir\$Date\greenlock.zip"
}

# Cleanup old backups
Get-ChildItem -Path $BackupDir -Directory | 
  Where-Object { $_.CreationTime -lt (Get-Date).AddDays(-$RetentionDays) } | 
  Remove-Item -Recurse -Force

Write-Host "Backup completed: $BackupDir\$Date"
```

**Set up Task Scheduler**:
1. Open Task Scheduler
2. Create Basic Task
3. Set daily trigger at desired time
4. Action: Start a program
5. Program: `powershell.exe`
6. Arguments: `-ExecutionPolicy Bypass -File C:\Scripts\backup-uf.ps1`

---

## Restore Procedures

### Restore MongoDB Database

**From mongodump directory**:
```bash
mongorestore --uri="mongodb://localhost:27017" --db=DayZ /backup/mongodb/2025-01-11/DayZ
```

**From compressed archive**:
```bash
mongorestore --uri="mongodb://localhost:27017" --gzip --archive=/backup/DayZ-2025-01-11.gz
```

**Drop existing database first** (full restore):
```bash
mongorestore --uri="mongodb://localhost:27017" --db=DayZ --drop /backup/mongodb/2025-01-11/DayZ
```

### Restore Configuration

1. Stop the UF Service
2. Copy the backed-up `config.json` to the config location
3. Verify the configuration
4. Start the UF Service

### Restore Let's Encrypt Certificates

1. Stop the UF Service
2. Extract the greenlock backup to the data directory
3. Start the UF Service

The service will use existing certificates if valid, or renew if needed.

---

## Backup Verification

### Test MongoDB Backup Integrity

```bash
# List contents of backup
mongorestore --uri="mongodb://localhost:27017" --gzip --archive=/backup/DayZ-2025-01-11.gz --dryRun

# Check specific collection
mongosh DayZ --eval "db.Players.countDocuments()"
```

### Verify Configuration Backup

```bash
# Check JSON validity
cat /backup/2025-01-11/config.json | jq .
```

---

## Disaster Recovery Scenarios

### Scenario 1: Database Corruption

**Symptoms**: Service errors, data inconsistencies, MongoDB crashes

**Recovery Steps**:
1. Stop the UF Service
2. Stop MongoDB
3. Backup current (corrupted) data directory just in case
4. Clear MongoDB data directory or use `--repair`
5. Start MongoDB
6. Restore from last known good backup
7. Start UF Service
8. Verify functionality

```bash
# Attempt repair first
mongod --repair --dbpath /var/lib/mongodb

# If repair fails, restore from backup
mongorestore --uri="mongodb://localhost:27017" --db=DayZ --drop /backup/last-good-backup/DayZ
```

### Scenario 2: Server Hardware Failure

**Recovery Steps**:
1. Provision new server
2. Install MongoDB
3. Install UF Server Service
4. Restore configuration from backup
5. Restore MongoDB database from backup
6. Update DNS/firewall as needed
7. Start services

### Scenario 3: Accidental Data Deletion

**Recovery Steps**:
1. Identify scope of deletion (collection, documents, etc.)
2. Find backup that contains the data
3. Restore specific data using a temporary database:

```bash
# Restore to temp database
mongorestore --uri="mongodb://localhost:27017" --db=DayZ_restore --gzip --archive=/backup/backup.gz

# Copy specific documents back
mongosh DayZ_restore --eval "
  db.Players.find({ GUID: '76561198012345678' }).forEach(function(doc) {
    db.getSiblingDB('DayZ').Players.insertOne(doc);
  });
"

# Drop temp database
mongosh --eval "db.getSiblingDB('DayZ_restore').dropDatabase()"
```

### Scenario 4: Configuration Lost

1. Stop UF Service
2. Restore `config.json` from backup
3. Or, let the service create a new default config and reconfigure
4. Regenerate auth tokens and update DayZ server configs
5. Start UF Service

---

## Data Migration

### Migrating to New Server

1. **On Old Server**:
   ```bash
   # Create full backup
   mongodump --uri="mongodb://localhost:27017" --db=DayZ --gzip --archive=./uf-migration.gz
   
   # Backup config
   cp /etc/ufserverservice/config.json ./config-backup.json
   ```

2. **Transfer files to new server** (scp, rsync, etc.)

3. **On New Server**:
   ```bash
   # Install MongoDB and UF Service
   
   # Restore database
   mongorestore --uri="mongodb://localhost:27017" --gzip --archive=./uf-migration.gz
   
   # Restore config
   cp ./config-backup.json /etc/ufserverservice/config.json
   
   # Update config for new environment (IP, domain, etc.)
   nano /etc/ufserverservice/config.json
   
   # Start service
   systemctl start ufserverservice
   ```

4. **Update DayZ servers** with new `ServerURL`

### Migrating Between Windows and Linux

The MongoDB data format is compatible, but paths in config differ:

1. Export MongoDB data with `mongodump`
2. Import on new system with `mongorestore`
3. Manually recreate or adapt the `config.json`
4. Regenerate Let's Encrypt certificates on new system

---

## Backup Best Practices

### Backup Schedule

| Data | Frequency | Retention |
|------|-----------|-----------|
| MongoDB | Daily | 30 days |
| Configuration | After changes | 90 days |
| Let's Encrypt | Weekly | 90 days |
| Logs | Optional | 30 days |

### Off-Site Backups

Store backups off-site for disaster recovery:
- Cloud storage (AWS S3, Google Cloud Storage, Azure Blob)
- Remote backup server
- Encrypted backup service

**Example: Sync to S3**:
```bash
aws s3 sync /backup/ufservice s3://your-bucket/ufservice-backups/
```

### Backup Testing

Regularly test backup restoration:
1. Set up a test environment
2. Restore from backup
3. Verify data integrity
4. Document any issues

### Encryption

Encrypt backups containing sensitive data:

```bash
# Encrypt with GPG
gpg --symmetric --cipher-algo AES256 backup.gz

# Decrypt
gpg --decrypt backup.gz.gpg > backup.gz
```

---

## Monitoring Backup Health

### Check Backup Size

```bash
# Should show reasonable growth over time
ls -lah /backup/ufservice/
```

### Verify Recent Backups

```bash
# Check if today's backup exists
ls -la /backup/ufservice/$(date +%Y-%m-%d)*
```

### Alert on Backup Failure

Add to backup script:
```bash
if [ $? -ne 0 ]; then
  echo "Backup failed!" | mail -s "UF Backup Alert" admin@example.com
fi
```

## Tags
`operators`, `backup`, `recovery`, `mongodb`, `disaster-recovery`, `how-to`, `doc-usage`
