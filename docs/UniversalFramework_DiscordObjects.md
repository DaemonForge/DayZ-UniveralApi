# Universal Framework - Discord Objects

## Overview

Data classes for Discord API interactions. Use these when sending messages, embeds, or processing Discord user data.

## UDiscordUser

Represents a Discord user with roles and voice channel info.

```enforce
class UDiscordUser extends StatusObject {
    string id;              // Discord user ID
    string Username;        // Display name
    string GlobalName;      // Global display name
    string Avatar;          // Avatar hash/URL
    autoptr TStringArray Roles;    // Array of role IDs
    string VoiceChannel;    // Current voice channel ID
    
    // Check if user has a role
    bool HasRole(string roleId);
    
    // Add role (makes API call)
    int AddRole(string roleId);
    
    // Remove role (makes API call)
    int RemoveRole(string roleId);
}
```

### Usage

```enforce
void OnUserLoaded(int cid, int status, string oid, UDiscordUser user) {
    if (status == UF_SUCCESS && user) {
        Print("User: " + user.Username);
        
        if (user.HasRole("123456789")) {
            Print("User is VIP");
        }
    }
}
```

## UDiscordMessage

Represents an incoming Discord message.

```enforce
class UDiscordMessage extends UFObject_Base {
    string id;           // Message ID
    string AuthorId;     // Discord ID of sender
    string AuthorGUID;   // Player GUID if linked
    string Content;      // Message text
    string ChannelId;    // Channel ID
    string RepliedTo;    // ID of replied message (if reply)
    autoptr UDiscordEmbed Embed;  // Embed if present
    int TimeStamp;       // Unix timestamp
}
```

## UDiscordEmbed

Rich embed for Discord messages.

```enforce
class UDiscordEmbed extends UFObject_Base {
    autoptr UDiscordAuthor author;
    string title;
    string url;
    string description;
    int color;           // Hex color as integer (e.g., 0xFF5500)
    autoptr array<autoptr UDiscordField> embeds;  // Fields array
    autoptr UDiscordImage thumbnail;
    autoptr UDiscordImage image;
    autoptr UDiscordFooter footer;
}
```

### Creating Embeds

```enforce
UDiscordEmbed embed = new UDiscordEmbed();
embed.title = "Server Alert";
embed.description = "Airdrop incoming at NWAF!";
embed.color = 0xFF5500;  // Orange

// Add author
embed.author = new UDiscordAuthor();
embed.author.name = "DayZ Server";
embed.author.icon_url = "https://example.com/icon.png";

// Add fields
embed.embeds = new array<autoptr UDiscordField>;
embed.embeds.Insert(new UDiscordField());
embed.embeds.Get(0).name = "Location";
embed.embeds.Get(0).value = "NWAF";
embed.embeds.Get(0).inline = true;

// Add footer
embed.footer = new UDiscordFooter("Server Time", "");
```

## UDiscordObject

Webhook-style message with embeds.

```enforce
class UDiscordObject extends UFObject_Base {
    string username;     // Override webhook username
    string avatar_url;   // Override webhook avatar
    string content;      // Text content
    autoptr array<autoptr UDiscordEmbed> embeds;
}
```

### Sending Webhook Message

```enforce
UDiscordObject msg = new UDiscordObject();
msg.username = "Game Bot";
msg.content = "Player joined the server!";

UDiscordEmbed embed = new UDiscordEmbed();
embed.title = "Player Join";
embed.description = "Welcome!";
msg.embeds.Insert(embed);

U().ds().ChannelSendEmbed("channelId", msg);
```

## UDiscordBasicMessage

Simple text message.

```enforce
class UDiscordBasicMessage extends UFObject_Base {
    string Message;
    
    void UDiscordBasicMessage(string message);
}
```

## Supporting Classes

### UDiscordAuthor

```enforce
class UDiscordAuthor extends UFObject_Base {
    string name;
    string url;
    string icon_url;
}
```

### UDiscordField

```enforce
class UDiscordField extends UFObject_Base {
    string name;
    string value;
    bool inline;   // Display inline with other fields
}
```

### UDiscordImage

```enforce
class UDiscordImage extends UFObject_Base {
    string url;
    int height;
    int width;
    
    void UDiscordImage(string url);
}
```

### UDiscordFooter

```enforce
class UDiscordFooter extends UFObject_Base {
    string text;
    string icon_url;
    
    void UDiscordFooter(string text, string iconUrl);
}
```

## Channel Objects

### UChannelOptions

Base options for channel operations.

```enforce
class UChannelOptions extends Managed {
    string reason;       // Audit log reason
    string topic;        // Channel topic
    bool nsfw;           // NSFW flag
    string parent;       // Parent category ID
    int position;        // Channel position (-1 = default)
    int rateLimitPerUser; // Slowmode seconds (-1 = none)
    autoptr array<autoptr UChannelPermissions> permissionOverwrites;
    
    void AddPerm(string id, string perm, bool isAllow = true);
    void SetPerms(string id, TStringArray perms, bool isAllow = true);
}
```

### UChannelCreateOptions

```enforce
class UChannelCreateOptions extends UChannelOptions {
    string type;   // "text", "voice", "category"
    
    void UChannelCreateOptions(string reason, string type = "text", string topic = "");
}
```

### UChannelUpdateOptions

```enforce
class UChannelUpdateOptions extends UChannelOptions {
    string name;   // New channel name
    
    void UChannelOptions(string reason, string name, string topic = "");
}
```

### UChannelPermissions

```enforce
class UChannelPermissions {
    string id;              // Role or user ID
    TStringArray allow;     // Allowed permissions
    TStringArray deny;      // Denied permissions
    
    void UChannelPermissions(string id, TStringArray allow, TStringArray deny);
}
```

## Utility Classes

### UDiscordStatusObject

Response status from Discord operations.

```enforce
class UDiscordStatusObject extends StatusObject {
    string oid;   // Object ID related to operation
}
```

### UDiscordMute

```enforce
class UDiscordMute extends UFObject_Base {
    bool State;   // true = muted
    
    void UDiscordMute(bool state);
}
```

### UDiscordNickname

```enforce
class UDiscordNickname extends UFObject_Base {
    string Nickname;
    
    void UDiscordNickname(string nickname);
}
```

## Complete Embed Example

```enforce
void SendKillFeed(string killer, string victim, string weapon) {
    UDiscordEmbed embed = new UDiscordEmbed();
    embed.title = "☠️ Kill Feed";
    embed.color = 0xCC0000;
    
    embed.author = new UDiscordAuthor();
    embed.author.name = "DayZ Server";
    
    embed.embeds = new array<autoptr UDiscordField>;
    
    autoptr UDiscordField f1 = new UDiscordField();
    f1.name = "Killer";
    f1.value = killer;
    f1.inline = true;
    embed.embeds.Insert(f1);
    
    autoptr UDiscordField f2 = new UDiscordField();
    f2.name = "Victim";
    f2.value = victim;
    f2.inline = true;
    embed.embeds.Insert(f2);
    
    autoptr UDiscordField f3 = new UDiscordField();
    f3.name = "Weapon";
    f3.value = weapon;
    f3.inline = false;
    embed.embeds.Insert(f3);
    
    embed.footer = new UDiscordFooter(UUtil.GetTimestamp(), "");
    
    UDiscordObject msg = new UDiscordObject();
    msg.embeds = new array<autoptr UDiscordEmbed>;
    msg.embeds.Insert(embed);
    
    U().ds().ChannelSendEmbed("killfeed-channel-id", msg);
}
```
