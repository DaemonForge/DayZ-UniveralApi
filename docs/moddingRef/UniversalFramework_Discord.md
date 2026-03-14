# Universal Framework - Discord Integration

## Overview

The Discord endpoint (`UniversalDSEndpoint`) provides full Discord integration including user management, role assignments, direct messages, channel operations, and voice channel control.

## Prerequisites

- Discord bot configured in UFServerService
- Players must link their Discord account via the web interface
- Bot must have appropriate permissions in your Discord server

## Accessing the Discord Endpoint

```enforce
UniversalDSEndpoint discord = U().ds();
```

## Permissions

| Operation | Server | Player (Client) |
|-----------|--------|----------------|
| Get / GetChannel | [YES] | [YES] Own GUID only |
| AddRole / RemoveRole | [YES] | âŒ |
| Mute / Kick / Move | [YES] | âŒ |
| Send (DM) / SetNickname | [YES] | âŒ |
| Check / CheckRole | [YES] (no auth) | [YES] (no auth) |
| Channel Create/Delete/Edit | [YES] | âŒ |
| Channel Send/Messages | [YES] | [YES] |

> **Note:** Players can only query their own Discord info. Role modifications, DMs, and voice controls are server-only.

## Account Linking

### Get Link URL

```enforce
// Get link URL for current player (client-side)
string linkUrl = U().ds().Link();

// Get link URL for specific player (server-side)
// This link flow expects the player's plain Steam ID.
// Do not pass GetId() here.
string linkUrl = U().ds().Link(player.GetIdentity().GetPlainId());

// Display to player
Print("Link your Discord: " + linkUrl);
```

### Check Discord Status

`CheckDiscord()` and `CheckRoleDiscord()` are looked up against the player's GUID in the service. The backend normalizes either a plain Steam ID or an existing GUID, so `GetIdentity().GetId()` is preferred for consistency with the rest of the framework.

```enforce
// Check if player has Discord linked
U().ds().CheckDiscord(playerId, this, "OnDiscordCheck");

void OnDiscordCheck(int cid, int status, string oid, StatusObject result) {
    if (status == UF_SUCCESS) {
        Print("Discord linked");
    } else if (status == UF_NOTSETUP) {
        Print("Discord not linked");
    }
}
```

### Check Role Membership

```enforce
// Check if player has a specific role
U().ds().CheckRoleDiscord(playerId, "RoleId123", this, "OnRoleCheck");

void OnRoleCheck(int cid, int status, string oid, StatusObject result) {
    if (status == UF_SUCCESS) {
        Print("Player has the required role");
    } else {
        Print("Player does not have the role");
    }
}
```

## User Operations

### Get User Info

```enforce
U().ds().GetUser(playerId, this, "OnUserInfo");

void OnUserInfo(int cid, int status, string oid, UDiscordUser user) {
    if (status == UF_SUCCESS && user) {
        Print("Username: " + user.Username);
        Print("Discord ID: " + user.id);
        Print("Avatar: " + user.Avatar);
    }
}
```

### Send Direct Message

```enforce
// Send DM to player
U().ds().UserSend(playerId, "Welcome to the server!", this, "OnDMSent");

void OnDMSent(int cid, int status, string oid, UDiscordStatusObject result) {
    if (status == UF_SUCCESS) {
        Print("DM sent successfully");
    }
}
```

### Set Nickname

```enforce
U().ds().SetNickname(playerId, "NewNickname", this, "OnNicknameSet");
```

### Kick User

```enforce
U().ds().KickUser(playerId, "Reason for kick", this, "OnKicked");
```

### Mute/Unmute User

```enforce
// Mute
U().ds().MuteUser(playerId, true, this, "OnMuteChanged");

// Unmute
U().ds().MuteUser(playerId, false, this, "OnMuteChanged");
```

## Role Management

### Add Role

```enforce
U().ds().AddRole(playerId, "RoleId123", this, "OnRoleAdded");

void OnRoleAdded(int cid, int status, string oid, UDiscordUser user) {
    if (status == UF_SUCCESS) {
        Print("Role added successfully");
    }
}
```

### Remove Role

```enforce
U().ds().RemoveRole(playerId, "RoleId123", this, "OnRoleRemoved");
```

## Voice Channel Operations

### Get User's Current Channel

```enforce
U().ds().GetUsersChannel(playerId, this, "OnChannelInfo");

void OnChannelInfo(int cid, int status, string oid, UDiscordChannelInfo info) {
    if (status == UF_SUCCESS && info) {
        Print("User is in channel: " + info.ChannelName);
    }
}
```

### Move User to Channel

```enforce
U().ds().MoveTo(playerId, "VoiceChannelId123", this, "OnMoved");
```

## Channel Operations

### Create Channel

```enforce
autoptr UChannelOptions options = new UChannelOptions();
options.Type = 0;  // 0 = text, 2 = voice
options.Parent = "CategoryId123";
options.Topic = "Channel topic";
options.Position = 1;

U().ds().ChannelCreate("new-channel-name", options, this, "OnChannelCreated");

void OnChannelCreated(int cid, int status, string oid, UDiscordStatusObject result) {
    if (status == UF_SUCCESS) {
        Print("Channel created");
    }
}
```

### Delete Channel

```enforce
U().ds().ChannelDelete("ChannelId123", "Cleanup", this, "OnDeleted");
```

### Edit Channel

```enforce
autoptr UChannelUpdateOptions options = new UChannelUpdateOptions();
options.Name = "renamed-channel";
options.Topic = "Updated topic";

U().ds().ChannelEdit("ChannelId123", "Updating channel", options, this, "OnEdited");
```

### Send Channel Message

```enforce
// Simple text message
U().ds().ChannelSend("ChannelId123", "Hello from DayZ!");

// With callback
U().ds().ChannelSend("ChannelId123", "Server is online!", this, "OnMessageSent");
```

### Send Embed Message

```enforce
autoptr UDiscordEmbed embed = new UDiscordEmbed();
embed.Title = "Server Status";
embed.Description = "Current server information";
embed.Color = 0x00FF00;  // Green

// Add fields
embed.AddField("Players", "25/60", true);
embed.AddField("Uptime", "12h 30m", true);
embed.AddField("Map", "Chernarus", false);

// Add footer
embed.Footer = "Updated: " + UUtil.GetTimeStamp();

U().ds().ChannelSendEmbed("ChannelId123", embed, this, "OnEmbedSent");
```

### Get Channel Messages

```enforce
// Get recent messages
autoptr UDiscordChannelFilter filter = new UDiscordChannelFilter();
filter.Limit = 10;

U().ds().ChannelMessages("ChannelId123", this, "OnMessages", filter);

void OnMessages(int cid, int status, string oid, array<autoptr UDiscordMessage> messages) {
    if (status == UF_SUCCESS && messages) {
        foreach (UDiscordMessage msg : messages) {
            Print(msg.Author + ": " + msg.Content);
        }
    }
}
```

## Complete Examples

### VIP Role System

```enforce
class VIPManager {
    protected string m_VIPRoleId = "123456789";
    
    void CheckVIP(PlayerBase player) {
        string guid = player.GetIdentity().GetId();
        U().ds().CheckRoleDiscord(guid, m_VIPRoleId, this, "OnVIPCheck");
    }
    
    void OnVIPCheck(int cid, int status, string oid, StatusObject result) {
        PlayerBase player = PlayerBase.Cast(UUtil.FindPlayer(oid));
        if (!player) return;
        
        if (status == UF_SUCCESS) {
            // Player has VIP role
            GrantVIPPerks(player);
            UUtil.SendNotification("VIP", "Welcome VIP!", player.GetIdentity());
        } else if (status == UF_NOTSETUP) {
            // Discord not linked
            string link = U().ds().Link(oid);
            UUtil.SendNotification("Discord", "Link Discord for VIP: " + link, player.GetIdentity());
        }
    }
    
    void GrantVIPPerks(PlayerBase player) {
        // Grant VIP benefits
    }
}
```

### Kill Feed Discord Bot

```enforce
class DiscordKillFeed {
    protected string m_KillFeedChannel = "channel-id";
    
    void OnPlayerKilled(PlayerBase victim, PlayerBase killer) {
        if (!killer) return;
        
        string victimName = victim.GetIdentity().GetName();
        string killerName = killer.GetIdentity().GetName();
        string weapon = GetWeaponName(killer);
        
        autoptr UDiscordEmbed embed = new UDiscordEmbed();
        embed.Title = "â˜ ï¸ Kill";
        embed.Color = 0xFF0000;  // Red
        embed.Description = "**" + killerName + "** killed **" + victimName + "**";
        
        embed.AddField("Weapon", weapon, true);
        embed.AddField("Distance", GetKillDistance(victim, killer) + "m", true);
        embed.AddField("Time", UUtil.GetTimeStamp(), true);
        
        U().ds().ChannelSendEmbed(m_KillFeedChannel, embed);
    }
    
    protected string GetWeaponName(PlayerBase player) {
        EntityAI weapon = player.GetHumanInventory().GetEntityInHands();
        if (weapon) return weapon.GetDisplayName();
        return "Unknown";
    }
    
    protected int GetKillDistance(PlayerBase victim, PlayerBase killer) {
        return vector.Distance(victim.GetPosition(), killer.GetPosition());
    }
}
```


## Best Practices

### Cache User Data
Discord API calls have rate limits. Don't fetch the user info every time they perform an action.
*   **Recommended**: Fetch `GetUser` on player connect, store it in a variable, and refresh only if necessary (e.g., every 15 minutes or on reconnect).

### Async Handling
All Discord operations are asynchronous.
*   **Never** try to block execution waiting for a result (like `while(!result)`).
*   Always use callbacks (`OnUserInfo`, `OnRoleCheck`) to handle the data when it arrives.

### Security
*   Use `CheckRoleDiscord` on the Server side to validate VIP perks.
*   Don't trust client-side claims about Discord status.

### Error Handling
Always handle `UF_NOTSETUP` (not linked) and `UF_ERROR` (API error/timeout).

## Common Use Cases

### VIP Systems
Grant in-game perks based on Discord roles.
1. Player connects.
2. Server calls `CheckRoleDiscord(guid, "VIP_ROLE_ID")`.
3. If true, give them a custom loadout or access to a donor base.

### In-Game Reporting
Allow players to report issues directly to Discord.
1. Player types `/report hacker123 aimbot`.
2. Mod calls `U().ds().ChannelSend("ADMIN_CHANNEL_ID", "Report: ...")`.
3. Admins get a ping on their phone instantly.

### Linked Account Verification
Force players to link accounts to play.
1. On join, check `CheckDiscord`.
2. If `UF_NOTSETUP`, show a GUI with a "Link Account" button that calls `OpenURL(U().ds().Link())`.
3. Kick the player after 2 minutes if they haven't linked.

## Tags
`discord`, `integration`, `roles`, `bot`, `linked-accounts`, `notifications`, `UniversalDSEndpoint`, `how-to`, `reference`, `doc-usage`, `modder`

