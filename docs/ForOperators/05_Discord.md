# Universal Framework - Discord Setup Guide

This document covers the complete setup of Discord integration for Universal Framework.

## Table of Contents

1. [Overview](#overview)
2. [Discord Application Setup](#discord-application-setup)
3. [Bot Configuration](#bot-configuration)
4. [UF Service Configuration](#uf-service-configuration)
5. [Player Linking Process](#player-linking-process)
6. [Features and Capabilities](#features-and-capabilities)
7. [Troubleshooting](#troubleshooting)

---

## Overview

Universal Framework Discord integration enables:
- Linking player Steam accounts to Discord accounts
- Managing player Discord roles from the game server
- Sending direct messages to players via Discord
- Voice channel control (mute, kick, move)
- Channel message sending and reading

### Requirements

1. A Discord server (guild) you control
2. A Discord application with a bot
3. The UF Server Service must be publicly accessible (for OAuth2 callback)

---

## Discord Application Setup

### Step 1: Create a Discord Application

1. Go to the [Discord Developer Portal](https://discord.com/developers/applications)
2. Click **"New Application"**
3. Enter a name (e.g., "My DayZ Server")
4. Accept the Terms of Service
5. Click **"Create"**

### Step 2: Get Application Credentials

On the application page:

1. **General Information** tab:
   - Copy the **Application ID** (this is your `Client_Id`)
   
2. **OAuth2** tab:
   - Copy the **Client Secret** (this is your `Client_Secret`)
   - Click "Reset Secret" if you need to generate a new one

### Step 3: Configure OAuth2 Redirect

1. Go to **OAuth2 > General**
2. Under **Redirects**, click "Add Redirect"
3. Add your callback URL:
   ```
   https://your-uf-service-domain:port/discord/callback
   ```
   
**Examples:**
```
https://uf.example.com:443/discord/callback
https://192.168.1.100:8443/discord/callback
```

**Important:** The URL must:
- Use HTTPS
- Match your UF Service's public address exactly
- Include the `/discord/callback` path

---

## Bot Configuration

### Step 1: Create a Bot

1. Go to the **Bot** tab in your Discord application
2. Click **"Add Bot"**
3. Confirm by clicking "Yes, do it!"

### Step 2: Get Bot Token

1. Under **Token**, click "Reset Token"
2. Copy the token (this is your `Bot_Token`)
3. **Never share this token publicly**

### Step 3: Configure Bot Permissions

Under **Privileged Gateway Intents**, enable:
- **Server Members Intent** - Required for managing roles
- **Message Content Intent** - Required if reading message content

### Step 4: Set Bot Permissions

The bot needs these permissions:
- Manage Roles
- Send Messages
- Mute Members (for voice control)
- Move Members (for voice control)
- Read Messages/View Channels

### Step 5: Invite Bot to Your Server

1. Go to **OAuth2 > URL Generator**
2. Select scopes:
   - `bot`
   - `identify` (for OAuth2)
3. Select bot permissions:
   - Manage Roles
   - Send Messages
   - Mute Members
   - Move Members
   - View Channels
4. Copy the generated URL
5. Open the URL in a browser
6. Select your Discord server
7. Authorize the bot

### Step 6: Get Server (Guild) ID

1. Enable Developer Mode in Discord:
   - User Settings > App Settings > Advanced > Developer Mode
2. Right-click your server name
3. Click "Copy Server ID"
4. This is your `Guild_Id`

---

## UF Service Configuration

Add the Discord configuration to your `config.json`:

```json
{
    "Discord": {
        "Client_Id": "1234567890123456789",
        "Client_Secret": "your-client-secret-here",
        "Bot_Token": "your-bot-token-here",
        "Guild_Id": "1234567890123456789",
        "AllowToReRegister": false,
        "Restrict_Sign_Up": false,
        "Required_Role": "",
        "BlackList_Role": "",
        "Restrict_Sign_Up_Countries": []
    }
}
```

### Configuration Options Explained

#### Client_Id
The Application ID from Discord Developer Portal.

#### Client_Secret
The OAuth2 client secret from Discord Developer Portal.

#### Bot_Token
The bot token from the Bot section of Discord Developer Portal.

**Security:** Keep this secret. Anyone with this token can control your bot.

#### Guild_Id
Your Discord server's ID.

**Important:** The bot must be a member of this server. All role and voice operations are performed on this server.

#### AllowToReRegister

| Value | Behavior |
|-------|----------|
| `false` | Players cannot re-link if already linked |
| `true` | Players can link a new Discord account, overwriting the old link |

**Recommendation:** Keep `false` to prevent account sharing issues.

#### Restrict_Sign_Up

| Value | Behavior |
|-------|----------|
| `false` | All players can link Discord |
| `true` | Geographic restrictions are applied |

When enabled, uses `Restrict_Sign_Up_Countries` for filtering.

#### Required_Role

Discord Role ID that players must have to complete linking.

**Usage:** Leave empty to allow anyone. Set to a role ID to require membership verification.

**Example:** `"1234567890123456789"` - Only members with this role can link.

#### BlackList_Role

Discord Role ID that prevents players from linking.

**Usage:** Set to a role ID used for banned or restricted players.

#### Restrict_Sign_Up_Countries

Array of country codes for geographic filtering.

**Blacklist mode (block specific countries):**
```json
"Restrict_Sign_Up_Countries": ["blacklist", "CN", "RU"]
```
This blocks China and Russia.

**Whitelist mode (allow only specific countries):**
```json
"Restrict_Sign_Up_Countries": ["US", "CA", "GB", "DE"]
```
This only allows US, Canada, UK, and Germany.

**Note:** Geographic detection uses IP geolocation and may not be 100% accurate.

---

## Player Linking Process

### How Players Link Their Discord

1. Player connects to your DayZ server
2. Player receives a link URL (if `PromptDiscordOnConnect` is enabled, or via in-game mod feature)
3. Link format: `https://your-uf-service/Discord/login/STEAM_ID`
4. Player opens the link in their browser
5. Player is redirected to Discord for authorization
6. Player authorizes the application
7. Player is redirected back to UF Service
8. Steam account is linked to Discord account in database

### Login Page Customization

The Discord linking pages (login, success, error) can be fully customized using EJS templates. You can brand these pages to match your server's look and feel.

**Template Files:**
- `discordLogin.ejs` - Login/linking page
- `discordSuccess.ejs` - Success confirmation
- `discordError.ejs` - Error messages

**Template Locations:**
| Platform | Directory |
|----------|-----------|
| Windows | `%APPDATA%\ufserverservice\templates\` |
| Linux (systemd install) | `/var/lib/ufserverservice/templates/` |
| Linux (manual) | `./templates/` or set via `UF_SAVE_PATH` |

**Quick Access (Windows):** Right-click tray icon → "📁 Discord Templates"

For complete template customization documentation including variables, EJS syntax, and examples, see **[Discord Template Customization](21_Discord_Templates.md)**.

---

## Features and Capabilities

### Role Management

**Add Role:**
```
POST /Discord/AddRole/{GUID}
Body: { "Role": "ROLE_ID" }
```

**Remove Role:**
```
POST /Discord/RemoveRole/{GUID}
Body: { "Role": "ROLE_ID" }
```

### User Information

**Get User:**
```
POST /Discord/Get/{GUID}
```

Returns:
```json
{
    "Status": "Success",
    "Roles": ["role1", "role2"],
    "VoiceChannel": "channel_id",
    "id": "discord_user_id",
    "Username": "username",
    "GlobalName": "display_name",
    "Avatar": "avatar_url"
}
```

### Direct Messages

**Send DM:**
```
POST /Discord/Send/{GUID}
Body: { "Message": "Hello from the server!" }
```

### Voice Channel Control

**Mute User:**
```
POST /Discord/Mute/{GUID}
Body: { "State": 1 }  // 1 = mute, 0 = unmute
```

**Kick from Voice:**
```
POST /Discord/Kick/{GUID}
Body: { "Text": "Reason for kick" }
```

**Move to Channel:**
```
POST /Discord/Move/{GUID}/{ChannelId}
```

**Get Current Voice Channel:**
```
POST /Discord/GetChannel/{GUID}
```

### Channel Operations

**Send Message to Channel:**
```
POST /Discord/Channel/Send/{ChannelId}
Body: { "Message": "Server announcement!" }
```

**Get Messages from Channel:**
```
POST /Discord/Channel/Get/{ChannelId}
Body: { "Limit": 10 }
```

---

## Troubleshooting

### Discord Status Check

Check the service status endpoint:
```
GET /Status
```

The `Discord` field shows:
- `"Ready"` - Bot connected and working
- `"Disabled"` - Discord not configured
- `"Disconnected"` - Bot lost connection
- `"Pending"` - Bot is connecting
- `"Error"` - Configuration or connection error

### Common Errors

#### "Discord Disabled" or "NotSetup"

**Cause:** Discord configuration is incomplete.

**Solution:** Verify all required fields are set:
- `Client_Id`
- `Client_Secret`
- `Bot_Token`
- `Guild_Id`

#### "User not found in discord"

**Cause:** The Discord user is not a member of your Discord server.

**Solution:** The linked Discord account must be a member of the server specified in `Guild_Id`.

#### "Player Doesn't have discord set up"

**Cause:** The player has not linked their Discord account.

**Solution:** Player needs to complete the Discord linking process.

#### OAuth2 Callback Errors

**"Invalid redirect_uri"**

**Cause:** The callback URL doesn't match what's configured in Discord.

**Solution:**
1. Check the redirect URL in Discord Developer Portal > OAuth2
2. Ensure it exactly matches your UF Service URL
3. Include the full path: `https://domain:port/discord/callback`

**"Bad Request" on callback**

**Cause:** Invalid or expired OAuth2 code.

**Solution:** User needs to restart the linking process.

### Bot Connection Issues

**Bot shows offline in Discord**

1. Verify `Bot_Token` is correct
2. Check UF Service logs for connection errors
3. Ensure bot is invited to the server
4. Restart UF Service

**"Missing Permissions"**

1. Check bot's role position in server settings
2. Bot's role must be above roles it's trying to manage
3. Verify bot permissions in Discord server settings

### Role Management Issues

**"Cannot manage role"**

**Cause:** The bot's role is below the role it's trying to add/remove.

**Solution:** In Discord server settings, move the bot's role above the roles it needs to manage.

### Rate Limiting

Discord has rate limits on API calls. If you see 429 errors:
- Reduce frequency of Discord API calls
- Batch operations where possible
- Check Discord developer documentation for rate limits

---

## Security Considerations

1. **Never share Bot Token** - Anyone with this can control your bot
2. **Use HTTPS** - OAuth2 callbacks require HTTPS
3. **Limit bot permissions** - Only grant permissions the bot needs
4. **Monitor bot activity** - Check Discord server logs
5. **Regularly rotate secrets** - Update Client_Secret periodically

## Tags
`operators`, `discord`, `setup`, `oauth`, `bot`, `security`, `how-to`, `doc-usage`
