# Universal Framework - DayZ Mod Configuration

This document covers the configuration of the Universal Framework mod (`@UFramework`) on the DayZ server.

## Configuration File

The mod configuration is stored at:
```
$profile/UF/UFramework.json
```

Where `$profile` is your DayZ server's profile directory (typically specified with `-profiles=` launch parameter).

**Example path:** `C:\DayZServer\Profiles\UF\UFramework.json`

---

## Configuration Options

### Complete Configuration Example

```json
{
    "ConfigVersion": "1",
    "ServerURL": "https://192.168.1.100:443/",
    "ServerID": "myserver01",
    "ServerAuth": "your-auth-token-from-service-config",
    "EnableBuiltinLogging": 0,
    "PromptDiscordOnConnect": 0
}
```

### Option Details

#### ConfigVersion

| | |
|---|---|
| **Type** | string |
| **Default** | `"1"` |
| **Description** | Internal version number for configuration migration |

**Note:** Do not modify this value. It is managed automatically by the framework.

---

#### ServerURL

| | |
|---|---|
| **Type** | string |
| **Required** | Yes |
| **Example** | `"https://your-service-address:443/"` |
| **Description** | Full URL to your UF Server Service endpoint |

**Format Requirements:**
- Must start with `https://`
- Must include the port number
- Must end with a trailing slash `/`

**Valid Examples:**
```
"https://192.168.1.100:443/"
"https://uf.example.com:443/"
"https://localhost:8443/"
```

**Invalid Examples:**
```
"http://192.168.1.100:443/"    âŒ Must use https
"https://192.168.1.100:443"    âŒ Missing trailing slash
"192.168.1.100:443/"           âŒ Missing https://
```

**Important:**
- The mod will automatically add the trailing slash if missing, but include it for clarity
- The DayZ server must be able to reach this URL over the network
- If using a self-signed certificate (default), the connection will still work

---

#### ServerID

| | |
|---|---|
| **Type** | string |
| **Required** | Yes |
| **Example** | `"myserver01"` |
| **Description** | Unique identifier for this DayZ server |

**Purpose:**
- Used for logging and identification in the UF Server Service
- Helps distinguish between multiple DayZ servers using the same UF Service
- Appears in logs and can be used for server-specific configurations

**Recommendations:**
- Use a descriptive, unique name
- No spaces (use underscores or dashes)
- Examples: `"US_East_PvP"`, `"EU_Namalsk_01"`, `"Server1"`

---

#### ServerAuth

| | |
|---|---|
| **Type** | string |
| **Required** | Yes |
| **Example** | `"avbrwklzaP~UcRPIpfB3~H2Ev9DFGAR4uqYu7IoNa7ScyBmd"` |
| **Description** | Authentication token for API access |

**Critical:** This value must exactly match one of the tokens in the UF Server Service's `config.json` `ServerAuth` array.

**Example Match:**

UF Service `config.json`:
```json
{
    "ServerAuth": [
        "avbrwklzaP~UcRPIpfB3~H2Ev9DFGAR4uqYu7IoNa7ScyBmd"
    ]
}
```

DayZ Mod `UFramework.json`:
```json
{
    "ServerAuth": "avbrwklzaP~UcRPIpfB3~H2Ev9DFGAR4uqYu7IoNa7ScyBmd"
}
```

**Security Notes:**
- Treat this token like a password
- Do not share in public repositories or Discord
- Each DayZ server can have its own unique token for tracking purposes

---

#### EnableBuiltinLogging

| | |
|---|---|
| **Type** | integer |
| **Default** | `0` |
| **Values** | `0`, `1`, `2` |
| **Description** | Controls the framework's logging behavior |

**Value Explanations:**
- `0` (Disabled): Standard vanilla behavior. No extra logs.
- `1` (Enabled): Logs both Vanilla Admin Logs AND Universal Framework Logs.
- `2` (Exclusive): Logs Universal Framework Logs ONLY (suppresses Vanilla Admin Logs).

**When to Enable:**
- Debugging connection issues
- Troubleshooting mod problems
- Development and testing

**Note:** Enabling this will increase log output. Disable in production for cleaner logs.

---

#### PromptDiscordOnConnect

| | |
|---|---|
| **Type** | integer |
| **Default** | `0` |
| **Values** | `0` = Disabled, `1` = Enabled |
| **Description** | Automatically prompt players to link Discord when they connect |

**When to Enable:**
- If you want to encourage Discord linking
- If your server requires Discord integration for certain features

**Behavior:**
- When enabled and a player connects without a linked Discord account
- The player receives a UI prompt to link their Discord account

---

## Configuration File Auto-Creation

If the configuration file does not exist when the server starts:
1. The framework creates the `UF` directory in your profile folder
2. A default `UFramework.json` is created with empty values
3. The server will log errors about missing configuration

**You must fill in `ServerURL`, `ServerID`, and `ServerAuth` for the framework to function.**

---

## Configuration Examples

### Basic Local Setup

For testing with UF Service on the same machine:

```json
{
    "ConfigVersion": "1",
    "ServerURL": "https://localhost:443/",
    "ServerID": "localdev",
    "ServerAuth": "your-token-here",
    "EnableBuiltinLogging": 1,
    "PromptDiscordOnConnect": 0
}
```

### Production Server

For a production DayZ server:

```json
{
    "ConfigVersion": "1",
    "ServerURL": "https://uf.yourdomain.com:443/",
    "ServerID": "US_East_01",
    "ServerAuth": "production-token-here",
    "EnableBuiltinLogging": 0,
    "PromptDiscordOnConnect": 0
}
```

### Remote UF Service

When UF Service is on a different machine:

```json
{
    "ConfigVersion": "1",
    "ServerURL": "https://192.168.1.50:8443/",
    "ServerID": "DayZ_Main",
    "ServerAuth": "your-token-here",
    "EnableBuiltinLogging": 0,
    "PromptDiscordOnConnect": 1
}
```

---

## Troubleshooting Configuration Issues

### Configuration File Not Found

**Symptom:** Logs show `[UFConfig] Creating new config and loading from file...` followed by errors

**Solution:**
1. Check that the profile path is correct
2. Create the `UF` directory manually if needed
3. Create `UFramework.json` with the correct values

### ServerURL Connection Failed

**Symptom:** `[UF] [Api] UniversalRest.Post called with invalid token`

**Possible Causes:**
1. `ServerURL` is incorrect or unreachable
2. UF Service is not running
3. Firewall blocking the connection
4. Wrong port number

**Verification:**
- From the DayZ server machine, try to access the Status endpoint:
  ```
  curl -k https://your-server-url:port/Status
  ```
- Should return JSON with `"Status": "Success"`

### Authentication Errors

**Symptom:** API calls return status 204 (NoAuth)

**Possible Causes:**
1. `ServerAuth` token does not match
2. Token has extra spaces or characters
3. Token was changed on one side but not the other

**Solution:**
- Copy the exact token from UF Service `config.json` to mod `UFramework.json`
- Ensure no leading/trailing whitespace
- Restart both services after changes

### Configuration Changes Not Applied

**Solution:**
- Restart the DayZ server completely
- The configuration is loaded once at server startup
- Changes require a full server restart

---

## Client vs Server Configuration

The `UFramework.json` configuration is **server-side only**. The mod:

1. Loads configuration on the server
2. Sends limited config info to clients via RPC
3. Clients receive authentication tokens from the server
4. Clients never directly access `ServerAuth`

**Player clients do not need any configuration files** - the server handles all configuration and authentication.

---

## Understanding Callbacks (For Troubleshooting)

When mods use Universal Framework, they make asynchronous REST API calls. Understanding this helps troubleshoot issues.

### How Callbacks Work

1. **Mod makes request** â†’ Framework sends HTTP request to UF Service
2. **UF Service processes** â†’ Database operation, Discord call, etc.
3. **Response received** â†’ Framework calls the mod's callback function
4. **Mod handles result** â†’ Updates game state, sends response to player

### Callback Status Codes

When troubleshooting mod issues, these status codes indicate what happened:

| Status | Meaning | Common Cause |
|--------|---------|--------------|
| `200` | Success | Operation completed successfully |
| `201` | Created | New record created |
| `204` | NoAuth | Authentication failed (check ServerAuth) |
| `205` | NotFound | Requested data doesn't exist |
| `206` | Duplicate | Record already exists |
| `207` | BadQuery | Invalid query format |
| `400` | BadRequest | Malformed request |
| `429` | RateLimited | Too many requests (increase rate limits) |
| `500` | ServerError | UF Service internal error |
| `0` | NoResponse | Connection failed (network/firewall) |

### Debugging Connection Issues

If mods report callback failures:

1. **Enable built-in logging:**
   ```json
   {
       "EnableBuiltinLogging": 1
   }
   ```

2. **Check DayZ server logs for:**
   - `[UF]` prefixed messages
   - HTTP status codes
   - Connection errors

3. **Common patterns:**
   - All callbacks return `0` â†’ Network connectivity issue
   - All callbacks return `204` â†’ Authentication mismatch
   - Occasional `429` â†’ Rate limiting (increase limits or whitelist IP)

---

## API Endpoints Used by Framework

The mod communicates with these UF Service endpoints:

### Database Endpoints

| Endpoint | Method | Purpose |
|----------|--------|---------|
| `/Object/Load` | POST | Load object data |
| `/Object/Save` | POST | Save object data |
| `/Object/Update` | POST | Partial update |
| `/Object/Transaction` | POST | Atomic increment/decrement |
| `/Object/Query` | POST | Search objects |
| `/Player/Load` | POST | Load player data |
| `/Player/Save` | POST | Save player data |
| `/Player/Update` | POST | Partial update |
| `/Globals/Load` | POST | Load global state |
| `/Globals/Save` | POST | Save global state |

### Discord Endpoints

| Endpoint | Method | Purpose |
|----------|--------|---------|
| `/Discord/AddRole` | POST | Add role to player |
| `/Discord/RemoveRole` | POST | Remove role from player |
| `/Discord/UserSend` | POST | Send DM to player |
| `/Discord/ChannelSend` | POST | Send to channel |
| `/Discord/GetUserInfo` | POST | Get Discord user details |

### AI Endpoints

| Endpoint | Method | Purpose |
|----------|--------|---------|
| `/AI/Chat/Create` | POST | Create chat session |
| `/AI/Chat/Send` | POST | Send message |
| `/AI/Chat/Poll` | POST | Check for response |
| `/TTS/Speak` | POST | Generate speech audio |
| `/Images/Generate` | POST | Generate AI image |

### Utility Endpoints

| Endpoint | Method | Purpose |
|----------|--------|---------|
| `/Status` | GET | Health check |
| `/Logger/Log` | POST | Send log entry |
| `/Random` | GET | Get random number |
| `/Messages/Read` | POST | Read from queue |
| `/Messages/Write` | POST | Write to queue |

---

## Data Flow Diagram

```
â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”
â”‚                        DayZ Server                               â”‚
â”‚  â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”     â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”     â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”     â”‚
â”‚  â”‚ Custom Mod â”‚ â”€â”€â–º â”‚ UFramework   â”‚ â”€â”€â–º â”‚ RestApi        â”‚     â”‚
â”‚  â”‚ (uses UF)  â”‚ â—„â”€â”€ â”‚ (callbacks)  â”‚ â—„â”€â”€ â”‚ (HTTP client)  â”‚     â”‚
â”‚  â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜     â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜     â””â”€â”€â”€â”€â”€â”€â”€â”¬â”€â”€â”€â”€â”€â”€â”€â”€â”˜     â”‚
â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”¼â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜
                                                   â”‚ HTTPS
                                                   â–¼
â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”
â”‚                      UF Server Service                           â”‚
â”‚  â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”     â”‚
â”‚  â”‚                    Express.js Routes                   â”‚     â”‚
â”‚  â”‚  /Object  /Player  /Discord  /AI/Chat  /Messages       â”‚     â”‚
â”‚  â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”¬â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜     â”‚
â”‚                           â”‚                                      â”‚
â”‚  â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”¼â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”     â”‚
â”‚  â”‚        Controllers / Business Logic                    â”‚     â”‚
â”‚  â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”¬â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜     â”‚
â”‚                           â”‚                                      â”‚
â”‚  â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”  â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”¼â”€â”€â”€â”€â”€â”€â”€â”€â”€â”  â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”      â”‚
â”‚  â”‚ MongoDB  â”‚  â”‚  Discord â”‚ OpenAI  â”‚  â”‚  Other Services â”‚      â”‚
â”‚  â”‚ (data)   â”‚  â”‚   API    â”‚  API    â”‚  â”‚  (FFmpeg, etc)  â”‚      â”‚
â”‚  â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜  â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”´â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜  â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜      â”‚
â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜
```

---

## Performance Considerations

### Request Timing

All framework API calls are asynchronous:
- DayZ server continues running while waiting
- Callbacks execute when responses arrive
- Typical latency: 10-100ms for local, 50-300ms for remote

### Best Practices for Server Operators

1. **Run UF Service close to DayZ server** - Lower latency = better experience
2. **Monitor rate limits** - Frequent 429 errors indicate need for adjustment
3. **Use separate database** - Don't share MongoDB with other applications
4. **Regular backups** - Player data is valuable

### Mod Impact

Mods using Universal Framework may:
- Make frequent API calls (check rate limits)
- Store large amounts of data (monitor MongoDB disk usage)
- Use AI features (check OpenAI billing)

If a specific mod causes issues:
1. Check the mod's documentation for UF requirements
2. Contact the mod developer with callback status codes
3. Increase rate limits if seeing 429 errors

---

## Security Considerations

### Token Protection

- **Never share** `ServerAuth` tokens publicly
- **Rotate tokens** periodically (update both configs)
- **Use unique tokens** per DayZ server for audit trails

### Network Security

- **Use HTTPS** exclusively (the framework enforces this)
- **Firewall rules** - Only allow DayZ server IPs if possible
- **Private network** - Ideal to run UF Service on same network as DayZ server

### Player Data

- All player data flows through UF Service to MongoDB
- Framework uses JWT tokens for player-specific operations
- Server auth tokens are never exposed to player clients

## Tags
`operators`, `mod-config`, `server`, `auth`, `security`, `how-to`, `doc-usage`
