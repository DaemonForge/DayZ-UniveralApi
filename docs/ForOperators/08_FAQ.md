# Universal Framework - Frequently Asked Questions

A comprehensive FAQ for server operators using Universal Framework.

## Table of Contents

1. [General Questions](#general-questions)
2. [Installation Questions](#installation-questions)
3. [Configuration Questions](#configuration-questions)
4. [Database Questions](#database-questions)
5. [Discord Questions](#discord-questions)
6. [Security Questions](#security-questions)
7. [Performance Questions](#performance-questions)
8. [Mod Development Questions](#mod-development-questions)

---

## General Questions

### What is Universal Framework?

Universal Framework (UF) is a backend service system for DayZ servers that provides:
- Database storage (MongoDB) for mods to persist data
- Discord integration (role management, messaging, voice control)
- AI/OpenAI integration for in-game AI features
- Cross-server data sharing capabilities
- Authentication system for secure data access

It consists of two parts:
1. A DayZ mod (`@UFramework`) that runs on your server
2. A backend service (UF Server Service) that handles data storage and external integrations

### Do I need to wipe my server to install Universal Framework?

**Yes, a server wipe is recommended and often required.** 

When you install UF, mods using it will expect player data to exist in the MongoDB database. Players who connected before UF was installed won't have database records, causing issues with UF-dependent mods.

Plan your installation during scheduled downtime and communicate the wipe to your players.

### Can I run UF Server Service on a different machine than my DayZ server?

**Yes.** The UF Server Service can run anywhere as long as:
- The DayZ server can reach it over HTTPS
- The correct `ServerURL` is configured in the mod
- Firewall allows the connection

Common setups:
- Same machine as DayZ server
- Separate dedicated server
- Cloud VPS (AWS, DigitalOcean, etc.)
- Home server (with port forwarding)

### What are the system requirements for UF Server Service?

**Minimum:**
- 1 CPU core
- 1 GB RAM (2 GB recommended with MongoDB on same machine)
- 1 GB storage (more for logs and AI cache)
- MongoDB 4.4+
- Network access from DayZ server

**Recommended for high-population servers:**
- 2+ CPU cores
- 4 GB RAM
- SSD storage
- Dedicated MongoDB instance

### Is Universal Framework free?

**Yes.** Universal Framework is open source under the AGPL-3.0 license.

Note: Some integrated services have their own costs:
- OpenAI API: Pay per token usage
- ElevenLabs TTS: Pay per character
- MongoDB Atlas (cloud): Free tier available, paid for more resources

---

## Installation Questions

### Where do I download Universal Framework?

Official releases are on GitHub:
https://github.com/daemonforge/DayZ-UniveralApi/releases

Download:
- `UniversalFrameworkService-Setup-X.X.X.exe` for Windows (Electron app)
- `ufserverservice-linux` + `install.sh` for Linux
- `@UFramework.zip` for the DayZ mod

### Can I run multiple DayZ servers with one UF Server Service?

**Yes.** A single UF Server Service can handle multiple DayZ servers.

**Setup:**
1. Add a separate auth token for each DayZ server in the service config:
   ```json
   "ServerAuth": ["token-for-server1", "token-for-server2"]
   "ServerAuthLabels": ["US-East", "EU-West"]
   ```
2. Configure each DayZ server's mod with its respective token

Data can be:
- Shared between servers (same mod name)
- Separated per server (different mod names)

### Does UF Service require a public IP?

**It depends on your setup:**

| Scenario | Public IP Required? |
|----------|---------------------|
| DayZ server on same machine | No |
| DayZ server on same local network | No |
| DayZ server on different network | Yes (or VPN) |
| Discord OAuth2 linking | Yes (for callback URL) |
| Let's Encrypt SSL | Yes |

### Why does the Windows installer ask about MongoDB?

MongoDB is required for UF to function. The Electron installer checks if MongoDB is installed and offers to install it if missing.

Options:
- **Install Server only**: Basic MongoDB server
- **Install Server & Compass**: MongoDB server + GUI management tool
- **Cancel**: Skip if you have MongoDB installed elsewhere

### How do I update Universal Framework?

**UF Server Service:**
1. Stop the service
2. Download the new version
3. Replace the executable or run the new installer
4. Start the service

**DayZ Mod:**
1. Stop your DayZ server
2. Replace the `@UFramework` folder with the new version
3. Start your DayZ server

**Note:** Check release notes for any configuration changes required.

---

## Configuration Questions

### Where is the configuration file located?

**UF Server Service:**
| Platform | Location |
|----------|----------|
| Windows | `%APPDATA%\ufserverservice\config.json` |
| Linux | `/etc/ufserverservice/config.json` |

**DayZ Mod:**
```
$profile/UF/UFramework.json
```
Where `$profile` is your DayZ server's profile directory.

### How do I generate a new ServerAuth token?

You can generate a new token manually or let the service do it:

**Automatic (on first run):**
Delete the existing token from `ServerAuth` array and restart:
```json
"ServerAuth": []
```
A new token will be generated.

**Manual:**
Create a random 48-character string using these characters:
`A-Z`, `a-z`, `0-9`, `-`, `.`, `!`, `~`

### What port should I use?

**Recommended ports:**
| Port | Notes |
|------|-------|
| 443 | Standard HTTPS, requires admin/root |
| 8443 | Common alternative, no special privileges |
| Other | Any unused port works |

**Considerations:**
- Port 443 is the default
- Ports below 1024 require admin/root on most systems
- Update firewall rules for your chosen port
- Include port in mod's `ServerURL` if not 443

### Can I change configuration while the service is running?

**No.** Configuration is loaded at startup. You must restart the service for changes to take effect.

### What does the trailing slash in ServerURL matter?

The trailing slash is required in the mod's `ServerURL`:
```json
"ServerURL": "https://example.com:443/"
```

If missing, the mod will add it automatically and save the config, but it's best practice to include it.

---

## Database Questions

### What data is stored in MongoDB?

| Collection | Data Stored |
|------------|-------------|
| Objects | Mod-created objects (bases, storage, custom items) |
| Players | Player records, Discord links, per-mod player data |
| Globals | Server-wide mod data |
| Messages | Message queue data |
| AIChats | AI conversation sessions |
| Various KB collections | Knowledge base data |

### How often should I backup the database?

**Recommended backup schedule:**
- Daily automated backups
- Keep 7-30 days of backups
- Backup before updates or major changes

See [MongoDB Guide](04_MongoDB.md) for backup instructions.

### Can I view/edit data in MongoDB?

**Yes.** You can use:
- **MongoDB Compass**: GUI tool for browsing/editing data
- **mongosh**: Command-line MongoDB shell
- **VS Code MongoDB extension**: View data in your editor

**Example using mongosh:**
```javascript
use DayZ
db.Players.findOne({ GUID: "12345678901234567" })
```

### How do I migrate data from another database system?

UF specifically uses MongoDB. If migrating from another system:
1. Export data from the old system as JSON
2. Format to match UF's collection structures
3. Import using `mongoimport` or custom scripts

There is no built-in migration tool from other database systems.

### What happens if MongoDB runs out of disk space?

MongoDB will become read-only and eventually stop accepting writes. The UF Service will log database write errors.

**Prevention:**
- Monitor disk usage
- Set up alerts for low disk space
- Enable log rotation to prevent log accumulation
- Periodically clean old data (AI sessions, old messages)

---

## Discord Questions

### Do players need to be in my Discord server to link their accounts?

**Yes.** Players must be a member of the Discord server specified in `Guild_Id` for most features to work.

Role management, voice control, and nickname setting all require guild membership.

### Can I use Discord features without a public URL?

**Partially:**
- Role management, DMs, voice control: Work without public URL
- OAuth2 player linking: Requires public URL for callback

For linking without a public URL, you would need a workaround like:
- Cloudflare Tunnel
- ngrok
- Manual database linking (not recommended)

### Why can't the bot manage certain roles?

Discord's role hierarchy means the bot can only manage roles **below** its own role.

**Fix:**
1. Open Discord Server Settings > Roles
2. Find the bot's role
3. Drag it above all roles it needs to manage
4. Save changes

### How do I get Role IDs?

1. Enable Developer Mode in Discord:
   - User Settings > App Settings > Advanced > Developer Mode
2. Right-click on a role in Server Settings > Roles
3. Click "Copy Role ID"

### Can players link multiple Steam accounts to one Discord?

By default, **no**. Each Discord account can only be linked to one Steam GUID.

With `AllowToReRegister: true`, players can re-link, but this replaces the old link rather than adding a new one.

---

## Security Questions

### Is my data encrypted?

- **In Transit**: Yes, all connections use HTTPS
- **At Rest**: MongoDB default is unencrypted (enable MongoDB encryption for sensitive data)
- **Auth Tokens**: Stored as plaintext in config files (protect these files)

### What does AllowClientWrite do and should I enable it?

**`AllowClientWrite`** allows player game clients to directly write data to the database.

**Default: `false` (recommended)**

When disabled, only authenticated server requests can write data. When enabled, any client with an auth token could potentially modify data.

**Only enable if:**
- You have a specific mod requiring client writes
- You understand the security implications
- You trust all players with write access

### Should I change the default port?

Using non-standard ports provides minimal security benefit ("security through obscurity"). Focus on:
- Keeping auth tokens secure
- Using firewalls to restrict access
- Keeping software updated

That said, using a non-standard port won't hurt and may reduce automated scanning.

### How do I secure MongoDB?

See the MongoDB Guide for details. Key steps:
1. Enable authentication
2. Create a dedicated user for UF
3. Bind to localhost if possible
4. Use firewall rules to restrict access
5. Keep MongoDB updated

---

## Performance Questions

### How many players can UF handle?

This depends on many factors:
- Server hardware
- MongoDB performance
- How mods use the API
- Network latency

A typical dedicated server can handle hundreds of concurrent players without issues. Rate limiting protects against overload.

### Why am I hitting rate limits?

Rate limits are per-IP. If your DayZ server is hitting limits:

1. Add your DayZ server's IP to the whitelist:
   ```json
   "RateLimitWhiteList": ["127.0.0.1", "your-server-ip"]
   ```

2. Or increase the limits:
   ```json
   "RequestLimit": 1000,
   "RequestLimitQuery": 800
   ```

### Can I run UF on the same machine as my DayZ server?

**Yes.** This is a common setup. MongoDB, UF Service, and DayZ server can all run on the same machine.

**Resource considerations:**
- Ensure enough RAM for all services
- SSD recommended for database performance
- Monitor resource usage during peak times

---

## Mod Development Questions

### Where is the modding documentation?

Full modding documentation is in the `docs/moddingRef/` directory:
- Database operations
- Discord integration
- AI Chat systems
- Callbacks and async patterns
- Best practices

Also see: https://github.com/daemonforge/DayZ-UniveralApi/wiki/Developer-Reference

### Can I test without a real DayZ server?

**Partially.** The UF Server Service works independently:

- Test endpoints with curl or Postman
- Test database operations directly
- Test Discord bot functionality

But full mod testing requires a DayZ server or client.

### How do I know which endpoints are available?

See the controller files in `UFServerServiceV2/controllers/`:
- `object.js` - Object database operations
- `player.js` - Player database operations
- `global.js` - Global state
- Discord operations in `discord/router.js`
- AI operations in `aiChat.js`

### Can mods share data through UF?

**Yes.** Multiple mods can read/write to shared mod names. This enables:
- Cross-mod data access
- Economy systems spanning multiple mods
- Shared player progression

Coordinate with other mod developers for shared data structures.

---

## Additional Questions

### I have a question not covered here. Where can I get help?

1. Check the [GitHub Issues](https://github.com/daemonforge/DayZ-UniveralApi/issues) for existing discussions
2. Search the [Wiki](https://github.com/daemonforge/DayZ-UniveralApi/wiki)
3. Open a new issue with details about your question

### How do I report a bug?

Open an issue on GitHub with:
- UF version number
- Operating system
- Steps to reproduce
- Expected vs actual behavior
- Relevant log entries
- Configuration (with sensitive values removed)

### How can I contribute to Universal Framework?

Contributions are welcome! See the GitHub repository for:
- Contributing guidelines
- Open issues labeled "good first issue"
- Feature requests you could implement

Pull requests are accepted for bug fixes, features, and documentation improvements.

## Tags
`operators`, `faq`, `support`, `troubleshooting`, `how-to`, `doc-usage`
