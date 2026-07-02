# Configuration Editor (Settings UI)

The Windows Electron application includes a graphical configuration editor that provides an intuitive interface for managing all service settings without manually editing JSON files.

---

## Accessing the Settings UI

1. Right-click the **UF Service** system tray icon
2. Select **Options â†’ Settings**

The Settings window will open with collapsible sections for each configuration category.

---

## Configuration Sections

### Authentication Keys

Manage server authentication tokens that DayZ servers use to authenticate with the service.

| Field | Description |
|-------|-------------|
| **Auth Token** | Read-only field displaying the token value |
| **Label** | Optional description for the token (e.g., "Server 1", "Dev Server") |

**Actions:**
- **Generate New Auth**: Creates a new cryptographically secure authentication token
- **Copy**: Copies the token to clipboard
- **Delete**: Removes the token

> **Tip**: Use labels to identify which server each token belongs to. This helps manage multiple DayZ servers.

---

### Database

Configure the MongoDB connection settings.

| Field | Description |
|-------|-------------|
| **DB Server** | MongoDB connection string (e.g., `mongodb://localhost:27017`) |
| **Database Name** | Name of the database to use (e.g., `UniversalApi`) |

**Examples:**
```
# Local MongoDB
mongodb://localhost:27017

# MongoDB with authentication
mongodb://user:password@localhost:27017

# MongoDB Atlas
mongodb+srv://user:password@cluster.mongodb.net
```

---

### Web Server

Configure the HTTP/HTTPS server settings.

| Field | Description |
|-------|-------------|
| **IP Address** | IP to bind to (`0.0.0.0` for all interfaces) |
| **Port** | Port number (default: `3000`) |
| **Rate Limit WhiteList** | IP addresses exempt from rate limiting |

#### Certificate Types

| Type | Description |
|------|-------------|
| **Self Signed** | Generates a self-signed certificate (development only) |
| **Let's Encrypt** | Automatic free SSL certificates (requires domain) |
| **Own Certificates** | Use your own certificate files |
| **Proxy** | Use when behind a reverse proxy (Cloudflare, nginx) |

##### Let's Encrypt Configuration
When selecting **Let's Encrypt**:

| Field | Description |
|-------|-------------|
| **Domain** | Your domain name (e.g., `api.myserver.com`) |
| **Email** | Email for Let's Encrypt notifications |
| **Alt Names** | Comma-separated alternative domain names |

##### Own Certificates Configuration
When selecting **Own Certificates**:

| Field | Description |
|-------|-------------|
| **Certificate** | Paste your SSL certificate (PEM format) |
| **Certificate Key** | Paste your private key (PEM format) |

##### Proxy Configuration
When selecting **Proxy**:
- Register for a free DaemonForge proxy subdomain
- The service handles SSL termination at the proxy level
- Your server runs HTTP locally while clients connect via HTTPS

---

### Discord

Configure Discord bot integration for player authentication and messaging.

| Field | Description |
|-------|-------------|
| **Client ID** | Discord application Client ID |
| **Client Secret** | Discord application Client Secret |
| **Bot Token** | Discord bot token |
| **Guild ID** | Discord server (guild) ID |
| **Allow To ReRegister** | Allow players to re-link their Discord account |

> **See Also**: [Discord Configuration Guide](05_Discord.md) for detailed setup instructions.

---

### OpenAI

Configure OpenAI API for AI-powered features.

| Field | Description |
|-------|-------------|
| **API Key** | Your OpenAI API key (starts with `sk-`) |
| **Enable Prompt Protection** | Adds protection against prompt injection attacks in system prompts |

> **See Also**: [AI Features](15_AI_Features.md) for complete AI documentation.

---

### Advanced

Advanced settings for fine-tuning the service.

#### Discord Restrictions

| Field | Description |
|-------|-------------|
| **Required Role** | Role ID required for Discord linking |
| **Blacklist Role** | Role ID that prevents Discord linking |
| **Restrict Discord Sign Up** | Enable/disable the role restrictions |

---

## Saving Configuration

After making changes:

1. Click **Save** at the bottom of the form (or use the floating save button)
2. Confirm the save when prompted
3. The service will automatically restart with the new configuration

**Unsaved Changes:**
- The Save and Cancel buttons appear when you have unsaved changes
- A floating Save button appears in the corner for quick access
- You'll be prompted if you try to close with unsaved changes

---

## Cancel/Revert

To discard changes:
1. Click **Cancel** or close the window
2. Confirm you want to discard changes

This reloads the configuration from disk, reverting any unsaved modifications.

---

## Tips & Best Practices

1. **Backup Before Major Changes**: The configuration is stored in `%APPDATA%\ufserverservice\config.json`

2. **Test Auth Keys**: After generating a new auth key, test it with a development server before deploying

3. **Rate Limiting**: Add your DayZ server IP to the Rate Limit WhiteList to prevent rate limit issues

4. **Discord Token Security**: The bot token becomes read-only after first save for security

5. **Let's Encrypt**: Ensure port 80 is accessible for domain validation

---

## Troubleshooting

### Settings Won't Save
- Check file permissions on `%APPDATA%\ufserverservice\config.json`
- Ensure the service has write access to the app data folder

### Service Doesn't Restart
- Check the Console (right-click tray â†’ Console) for error messages
- Review logs in View Logs for detailed errors

### Database Connection Failed
- Verify MongoDB is running: `net start MongoDB` in PowerShell
- Check the connection string format
- Ensure the database port is not blocked by firewall

---

## Related Documentation

- [Installation Guide](01_Installation.md)
- [Service Configuration](02_ServiceConfiguration.md) - Raw JSON configuration reference
- [Windows Quick Reference](19_Windows_QuickRef.md)

## Tags
`operators`, `ui`, `config-editor`, `windows`, `settings`, `how-to`, `doc-usage`
