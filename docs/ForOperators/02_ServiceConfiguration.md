# Universal Framework - Service Configuration

This document details all configuration options in the UF Server Service `config.json` file.

> **[!] Windows Users:**
> Do NOT edit `config.json` manually unless necessary.
> Use the **Settings** menu in the System Tray application:
> **Right-click Tray Icon -> Options -> Settings**
> This ensures syntax validity and applies changes correctly.

## Configuration File Locations

| Platform | Location |
|----------|----------|
| Windows | `%APPDATA%\ufserverservice\config.json` |
| Linux | `/etc/ufserverservice/config.json` or `/var/lib/ufserverservice/config.json` |

## Complete Configuration Reference

### Database Settings

```json
{
    "DBServer": "mongodb://localhost:27017",
    "DB": "DayZ",
    "CreateIndexes": true
}
```

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `DBServer` | string | `"mongodb://localhost:27017"` | MongoDB connection string. Format: `mongodb://[username:password@]host[:port]` |
| `DB` | string | `"DayZ"` | Name of the MongoDB database to use |
| `CreateIndexes` | boolean | `true` | Whether to create database indexes on startup. Automatically set to `false` after first successful creation |

**Notes:**
- If MongoDB requires authentication: `"mongodb://username:password@localhost:27017"`
- For MongoDB Atlas (cloud): `"mongodb+srv://username:password@cluster.mongodb.net"`
- Indexes improve query performance significantly. Leave `CreateIndexes` as true for first run.

### Server Settings

```json
{
    "IP": "0.0.0.0",
    "Port": 443
}
```

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `IP` | string | `"0.0.0.0"` | IP address to bind to. `0.0.0.0` listens on all interfaces |
| `Port` | number | `443` | Port number for HTTPS server |

**Port Considerations:**
- Port 443 is the standard HTTPS port (requires admin/root privileges)
- Use port 8443 if running as a non-privileged user
- Ensure your firewall allows inbound connections on this port
- DayZ mod config must specify the correct port in the ServerURL

### Authentication

```json
{
    "ServerAuth": [
        "your-randomly-generated-token-here"
    ],
    "ServerAuthLabels": [
        "MainServer"
    ]
}
```

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `ServerAuth` | string[] | `[]` (auto-generated) | Array of valid authentication tokens. Each DayZ server needs one of these tokens |
| `ServerAuthLabels` | string[] | `[]` | Optional labels for each auth token (for your reference only) |

**Token Generation:**
- On first run, a random 48-character token is automatically generated
- Tokens can contain: `A-Z`, `a-z`, `0-9`, and special characters: `-.!~`
- Each DayZ server using this service should have its own token for identification

**Multiple Servers Example:**
```json
{
    "ServerAuth": [
        "abc123...",
        "xyz789..."
    ],
    "ServerAuthLabels": [
        "US-East-Server",
        "EU-West-Server"
    ]
}
```

### Rate Limiting

```json
{
    "RequestLimit": 500,
    "RequestLimitQuery": 400,
    "RequestLimitStatus": 100,
    "RequestLimitServerQuery": 200,
    "RequestLimitTranslate": 200,
    "RequestLimitLogger": 500,
    "RequestLimitCrypto": 150,
    "RateLimitWhiteList": ["127.0.0.1"]
}
```

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `RequestLimit` | number | `500` | Default max requests per 10-second window |
| `RequestLimitQuery` | number | `400` | Max database query requests per 10-second window |
| `RequestLimitStatus` | number | `100` | Max status check requests per 10-second window |
| `RequestLimitServerQuery` | number | `200` | Max server query requests per 10-second window |
| `RequestLimitTranslate` | number | `200` | Max translation requests per 10-second window |
| `RequestLimitLogger` | number | `500` | Max logging requests per 10-second window |
| `RequestLimitCrypto` | number | `150` | Max crypto requests per 10-second window |
| `RateLimitWhiteList` | string[] | `["127.0.0.1"]` | IP addresses exempt from rate limiting |

**When to Adjust:**
- Increase limits for high-population servers
- Add your DayZ server's IP to `RateLimitWhiteList` if hitting rate limits
- The service logs a warning when rate limits are reached

### SSL/TLS Certificates

```json
{
    "Certificate": "",
    "CertificateKey": ""
}
```

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `Certificate` | string | `""` | Path to SSL certificate file (.crt or .pem) |
| `CertificateKey` | string | `""` | Path to SSL private key file (.key) |

**Notes:**
- If both are empty, the service uses a bundled self-signed certificate
- Self-signed certificates work fine for DayZ server communication
- For custom certificates, provide absolute paths to both files
- See [SSL/HTTPS Configuration](06_SSL_HTTPS.md) for Let's Encrypt setup

### Let's Encrypt (Automatic SSL)

```json
{
    "LetsEncypt": {
        "Enabled": false,
        "Domain": "yourdomain.com",
        "Email": "admin@yourdomain.com",
        "AltNames": []
    }
}
```

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `Enabled` | boolean | `false` | Enable Let's Encrypt automatic certificate management |
| `Domain` | string | `""` | Primary domain name (must be publicly accessible) |
| `Email` | string | `""` | Email for Let's Encrypt notifications |
| `AltNames` | string[] | `[]` | Additional domain names (Subject Alternative Names) |

**Requirements for Let's Encrypt:**
- Domain must point to this server's public IP
- Port 80 must be accessible for ACME challenges
- Port 443 for HTTPS traffic
- Cannot use localhost or internal IP addresses

### Discord Integration

```json
{
    "Discord": {
        "Client_Id": "",
        "Client_Secret": "",
        "Bot_Token": "",
        "Guild_Id": "",
        "AllowToReRegister": false,
        "Restrict_Sign_Up": false,
        "Required_Role": "",
        "BlackList_Role": "",
        "Restrict_Sign_Up_Countries": []
    }
}
```

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `Client_Id` | string | `""` | Discord OAuth2 Application Client ID |
| `Client_Secret` | string | `""` | Discord OAuth2 Application Client Secret |
| `Bot_Token` | string | `""` | Discord Bot Token |
| `Guild_Id` | string | `""` | Discord Server (Guild) ID for operations |
| `AllowToReRegister` | boolean | `false` | Allow players to re-link their Discord accounts |
| `Restrict_Sign_Up` | boolean | `false` | Enable geographic restrictions on Discord sign-up |
| `Required_Role` | string | `""` | Role ID required to complete Discord linking |
| `BlackList_Role` | string | `""` | Role ID that prevents Discord linking |
| `Restrict_Sign_Up_Countries` | string[] | `[]` | Country codes for geographic restrictions |

**Geographic Restrictions:**
- First element can be `"blacklist"` to invert the list (block listed countries)
- Without `"blacklist"`, only listed countries are allowed
- Example: `["blacklist", "CN"]` blocks China
- Example: `["US", "CA", "GB"]` only allows US, Canada, UK

See [Discord Setup](05_Discord.md) for complete setup instructions.

### OpenAI Integration

```json
{
    "OpenAIApi": {
        "ApiKey": "",
        "BaseURL": "",
        "DefaultModel": "",
        "EmbeddingModel": "",
        "enablePromptProtection": true
    }
}
```

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `ApiKey` | string | `""` | API key for OpenAI (or the compatible provider when `BaseURL` is set) |
| `BaseURL` | string | `""` | Optional OpenAI-compatible API base URL. Empty = OpenAI |
| `DefaultModel` | string | `""` | Default chat model. Empty = `gpt-4o-mini`. Set this when using `BaseURL` |
| `EmbeddingModel` | string | `""` | Embedding model for Knowledge Bases. Empty = `text-embedding-3-large` |
| `enablePromptProtection` | boolean | `true` | Enable safeguards against prompt injection |

**Notes:**
- Get an API key from https://platform.openai.com/api-keys
- API usage incurs costs based on token usage
- Leave empty to disable AI features

#### OpenAI-Compatible Providers (Open-Source Models)

Set `BaseURL` to use any OpenAI-compatible provider (Vultr Serverless Inference, Cloudflare Workers AI, Ollama, vLLM, etc.). When `BaseURL` is set, the service talks to the provider via the standard Chat Completions API instead of OpenAI's Responses API.

Examples:

| Provider | BaseURL | Example DefaultModel |
|----------|---------|----------------------|
| Vultr | `https://api.vultrinference.com/v1` | `llama-3.3-70b-instruct-fp8` |
| Cloudflare Workers AI | `https://api.cloudflare.com/client/v4/accounts/<ACCOUNT_ID>/ai/v1` | `@cf/meta/llama-3.3-70b-instruct-fp8-fast` |
| Ollama (local) | `http://localhost:11434/v1` | `llama3.1` (ApiKey can be any placeholder) |

Compatible-provider limitations:
- `DefaultModel` must be set to a model your provider serves (OpenAI model names won't exist there). Chats can still override the model per-chat.
- Knowledge Bases require the provider to offer an `/embeddings` endpoint; set `EmbeddingModel` accordingly. Changing the embedding model makes existing KB embeddings unusable until documents are re-embedded (embedding dimensions must match).
- JSON response format is enforced via prompting and validation retries rather than provider-side structured output.
- AI Assistants and OpenAI TTS voices require real OpenAI — they use proprietary APIs that compatible providers do not implement.
- Tool/function calling requires a model that supports it.

### ElevenLabs Integration (TTS)

```json
{
    "ElevenLabs": {
        "ApiKey": "",
        "enablePromptProtection": true
    }
}
```

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `ApiKey` | string | `""` | ElevenLabs API key for Text-to-Speech |
| `enablePromptProtection` | boolean | `true` | Enable safeguards against prompt injection |

**Notes:**
- Get an API key from https://elevenlabs.io
- Requires FFmpeg for audio processing
- Leave empty to disable TTS features

### Proxy/Tunnel Settings

```json
{
    "Proxy": {
        "primaryDomain": "",
        "subdomain": "",
        "token": "",
        "lastRenew": "",
        "autoRenew": false
    }
}
```

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `primaryDomain` | string | `""` | Primary domain for proxy service |
| `subdomain` | string | `""` | Subdomain for proxy service |
| `token` | string | `""` | Authentication token for proxy service |
| `lastRenew` | string | `""` | Timestamp of last token renewal |
| `autoRenew` | boolean | `false` | Automatically renew proxy token |

**Note:** This is for advanced tunnel configurations (e.g., Cloudflare Tunnel).

### Miscellaneous Settings

```json
{
    "AllowClientWrite": false,
    "LogToFile": true,
    "CheckForNewVersion": true,
    "cpuCount": 1,
    "Functions": {}
}
```

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `AllowClientWrite` | boolean | `false` | **Security setting.** Allow player clients to write data directly (not recommended) |
| `LogToFile` | boolean | `true` | Enable logging to daily rotating log files |
| `CheckForNewVersion` | boolean | `true` | Check GitHub for new versions on startup |
| `cpuCount` | number | `1` | Number of worker processes for clustering (Linux only) |
| `Functions` | object | `{}` | Reserved for custom function configurations |

**Security Warning:**
- `AllowClientWrite: true` allows player clients to directly modify database records
- This is a significant security risk and should remain `false` in production
- All writes should go through the DayZ server, not directly from clients

---

## Example Complete Configuration

```json
{
    "DBServer": "mongodb://localhost:27017",
    "DB": "DayZ",
    "AllowClientWrite": false,
    "IP": "0.0.0.0",
    "Port": 443,
    "CreateIndexes": false,
    "LogToFile": true,
    "CheckForNewVersion": true,
    "RequestLimit": 500,
    "RequestLimitQuery": 400,
    "RequestLimitStatus": 100,
    "RequestLimitServerQuery": 200,
    "RequestLimitTranslate": 200,
    "RequestLimitLogger": 500,
    "RequestLimitCrypto": 150,
    "RateLimitWhiteList": [
        "127.0.0.1",
        "your-dayz-server-ip"
    ],
    "ServerAuth": [
        "your-server-auth-token-here"
    ],
    "ServerAuthLabels": [
        "MainServer"
    ],
    "Certificate": "",
    "CertificateKey": "",
    "Discord": {
        "Client_Id": "",
        "Client_Secret": "",
        "Bot_Token": "",
        "Guild_Id": "",
        "AllowToReRegister": false,
        "Restrict_Sign_Up": false,
        "Required_Role": "",
        "BlackList_Role": "",
        "Restrict_Sign_Up_Countries": []
    },
    "OpenAIApi": {
        "ApiKey": "",
        "enablePromptProtection": true
    },
    "Functions": {},
    "LetsEncypt": {
        "Enabled": false,
        "Domain": "",
        "Email": "",
        "AltNames": []
    },
    "Proxy": {
        "primaryDomain": "",
        "subdomain": "",
        "token": "",
        "lastRenew": "",
        "autoRenew": false
    }
}
```

---

## Configuration Changes

After modifying `config.json`:

**Windows:**
- Right-click the system tray icon and select "Restart Service"
- Or close and reopen the application

**Linux:**
```bash
sudo systemctl restart ufserverservice
```

---

## Configuration Validation

The service validates configuration on startup and will:
- Normalize missing values to defaults
- Auto-generate `ServerAuth` if empty
- Log warnings for invalid values
- Disable Let's Encrypt if domain validation fails

Check the logs for any configuration warnings after startup.

---

## Rate Limiting Deep Dive

### Understanding Rate Limits

Rate limits are enforced per IP address, measured in requests per 10-second window. When a client exceeds the limit, they receive HTTP 429 (Too Many Requests) responses.

### Rate Limit Endpoints

| Limit Setting | Applies To |
|---------------|------------|
| `RequestLimit` | `/Object/*`, `/Player/*`, `/Globals/*`, `/Messages/*`, `/AI/*`, `/TTS/*`, `/Images/*` |
| `RequestLimitQuery` | `/Object/Query`, `/Player/Query`, `/Object/Query/Update` |
| `RequestLimitStatus` | `/Status` |
| `RequestLimitServerQuery` | `/ServerQuery/*` |
| `RequestLimitTranslate` | Translation endpoints |
| `RequestLimitLogger` | `/Logger/*` |
| `RequestLimitCrypto` | `/Crypto/*` |

### Sizing Recommendations

**Small Server (< 30 players):**
```json
{
    "RequestLimit": 500,
    "RequestLimitQuery": 400
}
```

**Medium Server (30-80 players):**
```json
{
    "RequestLimit": 1000,
    "RequestLimitQuery": 800
}
```

**Large Server (80+ players):**
```json
{
    "RequestLimit": 2000,
    "RequestLimitQuery": 1500
}
```

**High-Frequency Mods (e.g., real-time sync):**
```json
{
    "RequestLimit": 5000,
    "RequestLimitQuery": 3000,
    "RateLimitWhiteList": ["your-dayz-server-ip"]
}
```

### Monitoring Rate Limits

Watch the logs for rate limit warnings:
```
[WARN] Rate limit exceeded for IP xxx.xxx.xxx.xxx
```

If you see these frequently, either:
1. Increase the relevant limit
2. Add the IP to `RateLimitWhiteList`
3. Investigate if the traffic is legitimate

---

## Clustering (Multi-Core)

The `cpuCount` option enables worker process clustering on Linux for better performance on multi-core systems.

### Configuration

```json
{
    "cpuCount": 4
}
```

| Value | Behavior |
|-------|----------|
| `1` | Single process (default) |
| `2-N` | Spawn N worker processes |
| `0` or negative | Use all available CPU cores |

### When to Use

- **Single core/VM**: Leave at `1`
- **4+ core server**: Set to `2` or `4` for improved throughput
- **High load**: Set to number of cores minus 1 (leave one for OS)

### Notes

- Each worker has its own memory space
- State is shared via MongoDB (not in-memory)
- Rate limits apply per-worker
- Only effective on Linux (Windows always runs single process)

---

## Configuration Scenarios

### Scenario 1: Simple Single Server

Basic setup with one DayZ server, no Discord, no AI:

```json
{
    "DBServer": "mongodb://localhost:27017",
    "DB": "DayZ",
    "Port": 443,
    "ServerAuth": ["generated-token"],
    "ServerAuthLabels": ["MyServer"]
}
```

### Scenario 2: Multi-Server with Discord

Multiple DayZ servers with Discord integration:

```json
{
    "DBServer": "mongodb://localhost:27017",
    "DB": "DayZ",
    "Port": 443,
    "ServerAuth": [
        "token-for-us-east",
        "token-for-eu-west"
    ],
    "ServerAuthLabels": [
        "US-East",
        "EU-West"
    ],
    "Discord": {
        "Client_Id": "123456789012345678",
        "Client_Secret": "your-client-secret",
        "Bot_Token": "your-bot-token",
        "Guild_Id": "123456789012345678"
    }
}
```

### Scenario 3: Full Feature Set

All features enabled with Let's Encrypt:

```json
{
    "DBServer": "mongodb://localhost:27017",
    "DB": "DayZ",
    "Port": 443,
    "cpuCount": 2,
    "ServerAuth": ["your-secure-token"],
    "LetsEncypt": {
        "Enabled": true,
        "Domain": "uf.yourserver.com",
        "Email": "admin@yourserver.com"
    },
    "Discord": {
        "Client_Id": "...",
        "Client_Secret": "...",
        "Bot_Token": "...",
        "Guild_Id": "..."
    },
    "OpenAIApi": {
        "ApiKey": "sk-..."
    },
    "ElevenLabs": {
        "ApiKey": "..."
    },
    "RequestLimit": 1000,
    "RateLimitWhiteList": ["your-dayz-server-ip"]
}
```

### Scenario 4: Behind Reverse Proxy

Running behind nginx or similar (SSL handled by proxy):

```json
{
    "DBServer": "mongodb://localhost:27017",
    "DB": "DayZ",
    "IP": "127.0.0.1",
    "Port": 3000,
    "ServerAuth": ["your-token"]
}
```

Note: The proxy would handle SSL and forward to localhost:3000.

---

## Environment Variables

Some settings can be overridden via environment variables:

| Variable | Purpose |
|----------|---------|
| `UF_SAVE_PATH` | Override data directory location |
| `UF_CONFIG_PATH` | Override config.json location |

**Linux Example:**
```bash
export UF_SAVE_PATH=/var/lib/ufserverservice
export UF_CONFIG_PATH=/etc/ufserverservice/config.json
```

---

## Troubleshooting Configuration

### Configuration Not Loading

1. Verify file is valid JSON:
   ```bash
   cat config.json | python -m json.tool
   ```

2. Check file permissions (Linux):
   ```bash
   ls -la /etc/ufserverservice/config.json
   ```

3. Look for errors in service logs

### Values Not Taking Effect

1. Confirm you restarted the service after changes
2. Check if value is being normalized (compare to defaults)
3. Some values require index recreation (`CreateIndexes: true`)

### Rate Limit Issues

1. Check if your IP is in `RateLimitWhiteList`
2. Verify which limit applies to your endpoint
3. Consider if clustering affects the effective limit

## Tags
`operators`, `configuration`, `service`, `config.json`, `settings`, `reference`, `doc-usage`
