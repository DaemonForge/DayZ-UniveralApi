# Universal Framework - Discord Template Customization

This document provides complete reference for customizing the Discord linking pages that players see when connecting their Steam and Discord accounts.

---

## Overview

The Discord linking process shows players three possible pages:
- **Login Page** - Where players initiate the Discord connection
- **Success Page** - Shown after successful account linking
- **Error Page** - Shown when something goes wrong

These pages are rendered using EJS (Embedded JavaScript) templates, which you can fully customize to match your server's branding.

---

## Template Files

| Template | Filename | Purpose |
|----------|----------|---------|
| Login Page | `discordLogin.ejs` | Shown when player clicks the linking URL |
| Success Page | `discordSuccess.ejs` | Shown after successful account linking |
| Error Page | `discordError.ejs` | Shown when an error occurs |

---

## Template Locations

| Platform | Templates Directory |
|----------|---------------------|
| Windows (Electron) | `%APPDATA%\ufserverservice\templates\` |
| Linux (systemd install) | `/var/lib/ufserverservice/templates/` |
| Linux (manual) | `./templates/` or set via `UF_SAVE_PATH` environment variable |

### Accessing Templates

**Windows (Electron App):**
1. Right-click the UF Service tray icon
2. Click "📁 Discord Templates"
3. The templates folder will open in Explorer

**Or via PowerShell:**
```powershell
explorer "$env:APPDATA\ufserverservice\templates"
```

**Linux (systemd install):**
```bash
cd /var/lib/ufserverservice/templates/
ls -la
```

**Linux (manual run):**
If you run the binary manually without the install script, templates are in `./templates/` relative to where you execute the binary. You can override this with the `UF_SAVE_PATH` environment variable:
```bash
export UF_SAVE_PATH=/path/to/data/
./ufserverservice-linux
# Templates will be at /path/to/data/templates/
```

---

## Template Variables Reference

### Login Page (`discordLogin.ejs`)

| Variable | Type | Description |
|----------|------|-------------|
| `SteamId` | string | The player's Steam ID (17-digit) |
| `Login_URL` | string | The Discord OAuth2 redirect URL |
| `Connected` | boolean | `true` if player already has a linked account |

**Example usage:**
```ejs
<p>Steam ID: <%= SteamId %></p>

<% if (Connected) { %>
    <p>Your account is already linked!</p>
<% } else { %>
    <button onclick="window.location='<%= Login_URL %>'">Connect to Discord</button>
<% } %>
```

### Success Page (`discordSuccess.ejs`)

| Variable | Type | Description |
|----------|------|-------------|
| `SteamId` | string | The player's Steam ID |
| `DiscordId` | string | The player's Discord user ID |
| `DiscordUsername` | string | The player's Discord username |
| `DiscordName` | string | The player's Discord display name |
| `DiscordAvatar` | string | URL to the player's Discord avatar |

**Example usage:**
```ejs
<div class="success-card">
    <img src="<%= DiscordAvatar %>" alt="Discord Avatar">
    <h2>Welcome, <%= DiscordName %>!</h2>
    <p>Discord: <%= DiscordUsername %></p>
    <p>Steam ID: <%= SteamId %></p>
    <p>Your accounts are now linked.</p>
</div>
```

### Error Page (`discordError.ejs`)

| Variable | Type | Description |
|----------|------|-------------|
| `TheError` | string | The error message text |
| `Type` | string | Error type code (see table below) |

**Error Types:**

| Type | Meaning | Suggested User Message |
|------|---------|------------------------|
| `AlreadyLinked` | Steam account already has a Discord linked | "Your Steam account is already connected to Discord" |
| `Conflict` | Discord account is linked to a different Steam ID | "This Discord account is linked to another Steam ID" |
| `UserNotFound` | Player is not a member of your Discord server | "Please join our Discord server first" |
| `RoleRequired` | Player doesn't have the required role | "You need a specific role to link your account" |
| `Blacklisted` | Player has a blacklisted role | "You cannot link with your current Discord role" |
| `NotSetup` | Discord integration not configured | "Discord integration is not available" |
| `BadURL` | Invalid linking URL | "Invalid link - please try again from in-game" |
| `ValidationError` | Geographic/IP validation failed | "Unable to verify your connection" |

**Example usage:**
```ejs
<div class="error-card">
    <% if (Type === "UserNotFound") { %>
        <h2>Join Our Discord First!</h2>
        <p>You must be a member of our Discord server before linking.</p>
        <a href="https://discord.gg/YOUR_INVITE" class="btn">Join Discord</a>
    <% } else if (Type === "AlreadyLinked") { %>
        <h2>Already Connected</h2>
        <p>Your Steam account is already linked to a Discord account.</p>
    <% } else if (Type === "Conflict") { %>
        <h2>Discord Account In Use</h2>
        <p>This Discord account is already linked to a different Steam ID.</p>
    <% } else if (Type === "RoleRequired") { %>
        <h2>Role Required</h2>
        <p>You need a specific role in our Discord to link your account.</p>
    <% } else if (Type === "Blacklisted") { %>
        <h2>Access Denied</h2>
        <p>Your account cannot be linked at this time.</p>
    <% } else { %>
        <h2>Error</h2>
        <p><%= TheError %></p>
    <% } %>
</div>
```

---

## EJS Syntax Reference

EJS (Embedded JavaScript) allows you to mix HTML with JavaScript logic. This section provides everything you need to write and troubleshoot EJS templates.

### Basic Tags

| Tag | Purpose | Example |
|-----|---------|---------|
| `<%= %>` | Output escaped value (safe for user input) | `<%= SteamId %>` |
| `<%- %>` | Output raw HTML (unescaped - use carefully) | `<%- htmlContent %>` |
| `<% %>` | Execute JavaScript (no output) | `<% if (x) { %>` |
| `<%# %>` | Comment (not rendered in output) | `<%# This is hidden %>` |

### Outputting Variables

```ejs
<%# This outputs the variable value escaped (safe) %>
<p>Your Steam ID is: <%= SteamId %></p>

<%# This outputs raw HTML (use carefully!) %>
<div><%- someHtmlContent %></div>
```

### Conditionals

```ejs
<% if (Connected) { %>
    <p>You are connected!</p>
<% } else { %>
    <p>Please connect your account.</p>
<% } %>
```

### Else-If Chains

```ejs
<% if (Type === "UserNotFound") { %>
    <p>Join our Discord first!</p>
<% } else if (Type === "AlreadyLinked") { %>
    <p>Already linked!</p>
<% } else if (Type === "Conflict") { %>
    <p>Discord already in use!</p>
<% } else { %>
    <p>Unknown error: <%= TheError %></p>
<% } %>
```

### Ternary Operator (Inline Conditionals)

```ejs
<p class="<%= Connected ? 'success' : 'pending' %>">
    Status: <%= Connected ? 'Linked' : 'Not Linked' %>
</p>
```

### Loops

```ejs
<% const items = ['Item 1', 'Item 2', 'Item 3']; %>
<ul>
    <% items.forEach(function(item) { %>
        <li><%= item %></li>
    <% }); %>
</ul>
```

### JavaScript in Templates

You can use any JavaScript within `<% %>` tags:

```ejs
<%
  // Define variables
  const greeting = Connected ? 'Welcome back!' : 'Hello!';
  const statusClass = Connected ? 'status-linked' : 'status-pending';
%>

<h1><%= greeting %></h1>
<div class="<%= statusClass %>">
    <!-- content -->
</div>
```

### Common Mistakes and Fixes

| ❌ Wrong | ✅ Correct | Reason |
|----------|-----------|--------|
| `<% SteamId %>` | `<%= SteamId %>` | Need `=` to output the value |
| `<% if (x) { }` | `<% if (x) { %> ... <% } %>` | Must close bracket with `%>` |
| `<%= <b>text</b> %>` | `<%- '<b>text</b>' %>` | Use `%-` for raw HTML, wrap in quotes |
| `if (Connected) {` | `<% if (Connected) { %>` | Need EJS tags around JavaScript |
| `<%= steamid %>` | `<%= SteamId %>` | Variables are case-sensitive |

---

## Step-by-Step Template Customization

### Step 1: Backup Existing Templates

Before making changes, back up the originals:

**Windows (PowerShell):**
```powershell
$templatesPath = "$env:APPDATA\ufserverservice\templates"
Copy-Item -Path $templatesPath -Destination "$templatesPath-backup" -Recurse
```

**Linux:**
```bash
sudo cp -r /var/lib/ufserverservice/templates /var/lib/ufserverservice/templates-backup
```

### Step 2: Open the Template for Editing

**Windows (PowerShell):**
```powershell
notepad "$env:APPDATA\ufserverservice\templates\discordLogin.ejs"
```

**Linux:**
```bash
sudo nano /var/lib/ufserverservice/templates/discordLogin.ejs
```

### Step 3: Make Your Changes

Edit the HTML, CSS, and EJS tags as needed. See the [Default Templates](#default-template-reference) section below for complete examples.

### Step 4: Save and Test

1. Save the template file
2. Open a browser and navigate to: `https://your-uf-service/Discord/login/76561198000000000`
   (Use any valid 17-digit Steam ID for testing)
3. Verify the page renders correctly

### Step 5: Check for Errors

If your template has syntax errors:
- The service will fall back to the default template
- An error will be logged: `ERROR IN [TEMPLATE_NAME] TEMPLATE`

**Windows:** Right-click tray icon → "📁 Logs"
**Linux:** `journalctl -u ufserverservice -f`

---

## Default Template Reference

The following sections contain the **complete default templates** that ship with the service. Use these as a reference or starting point for your customizations.

### Default Error Template (`discordError.ejs`)

```html
<!DOCTYPE html>
<html lang="en">
    <head>
        <meta charset="UTF-8">
        <meta http-equiv="X-UA-Compatible" content="IE=edge">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <link rel="icon" type="image/x-icon" href="/favicon.ico">
        <link rel="preconnect" href="https://fonts.googleapis.com">
        <link href="https://fonts.googleapis.com/css2?family=Roboto:wght@400;700&display=swap" rel="stylesheet">
        
        <!--
            Passed Variables:
            - TheError : Contains the error text to be shown.
            - Type     : The type of error being handled. This can be one of:
                         "AlreadyLinked", "Conflict", "UserNotFound", "RoleRequired", etc.
        -->
        <title>Universal Framework - Error - <%= Type %></title>
        
        <style>
            body {
                font-family: 'Roboto', sans-serif;
                background: #121212;
                color: #DCEDC8;
                margin: 0;
                padding: 0;
            }
            .container {
                max-width: 800px;
                margin: 40px auto;
                padding: 20px;
                background: rgba(0, 0, 0, 0.7);
                border-radius: 8px;
                box-shadow: 0 4px 8px rgba(0, 0, 0, 0.3);
                text-align: center;
            }
            .header {
                display: flex;
                align-items: center;
                justify-content: center;
                margin-bottom: 20px;
                animation: slideDown 0.8s ease-out;
            }
            .header img.logo {
                width: 50px;
                height: 50px;
                margin-right: 15px;
            }
            .header h1 {
                font-size: 2em;
                margin: 0;
            }
            .error-card {
                background: #1e1e1e;
                padding: 20px;
                border-radius: 8px;
                text-align: left;
                margin: 0 auto;
                box-shadow: 0 2px 4px rgba(0, 0, 0, 0.5);
            }
            .error-card h2 {
                color: #ff5555;
                margin-top: 0;
            }
            pre {
                background: #333;
                padding: 10px;
                border-radius: 5px;
                color: #DCEDC8;
                overflow-x: auto;
            }
        </style>
    </head>
    <body>
        <div class="container">
            <div class="header">
                <img src="/icon.svg" alt="Project Logo" class="logo">
                <h1>Universal Framework</h1>
            </div>
            <div class="error-card">
                <% if (Type === "AlreadyLinked") { %>
                    <h2>You have already linked your Discord account</h2>
                <% } else if (Type === "Conflict") { %>
                    <h2>This Discord account is already linked with another Steam ID</h2>
                <% } else if (Type === "UserNotFound") { %>
                    <h2>User not found in our Discord</h2>
                <% } else if (Type === "RoleRequired") { %>
                    <h2>You are required to have a specific role in Discord</h2>
                <% } else { %>
                    <h2>Error: Invalid Link or Request</h2>
                    <p>Please check the details below or contact support.</p>
                    <pre><%= TheError %></pre>
                <% } %>
            </div>
        </div>
    </body>
</html>
```

### Default Login Template (`discordLogin.ejs`)

```html
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <meta http-equiv="X-UA-Compatible" content="IE=edge" />
    <link rel="icon" type="image/x-icon" href="/favicon.ico" />
    <link rel="preconnect" href="https://fonts.googleapis.com" />
    <link href="https://fonts.googleapis.com/css2?family=Roboto:wght@400;700&display=swap" rel="stylesheet" />

    <!--
      Passed Variables:
      • SteamId     - The Steam ID of the user.
      • Login_URL   - The URL for Discord connection/login.
      • Connected   - Boolean; true if the Steam account is already linked.
    -->
    <title>Universal Framework - Connect to Discord</title>

    <style>
      body {
        font-family: 'Roboto', sans-serif;
        background: linear-gradient(45deg, #1a1a1a, #141414, #1a1a1a);
        background-size: 400% 400%;
        animation: gradientBackground 20s ease infinite;
        color: #dcedc8;
        margin: 0;
        padding: 0;
        filter: contrast(0.9) saturate(0.8);
      }
      @keyframes gradientBackground {
        0%   { background-position: 0% 50%; }
        50%  { background-position: 100% 50%; }
        100% { background-position: 0% 50%; }
      }

      .container {
        max-width: 900px;
        margin: 40px auto;
        padding: 20px;
        background: rgba(0, 0, 0, 0.95);
        border-radius: 10px;
        box-shadow: 0 8px 16px rgba(0, 0, 0, 0.85);
        animation: fadeIn 1s ease-out;
        position: relative;
      }
      @keyframes fadeIn {
        from { opacity: 0; transform: translateY(30px); }
        to   { opacity: 1; transform: translateY(0); }
      }
      
      .header {
        display: flex;
        flex-direction: column;
        align-items: center;
        margin-bottom: 30px;
        animation: slideDown 1s ease-out;
      }
      .header img.logo {
        width: 60px;
        height: 60px;
        margin-bottom: 10px;
        filter: brightness(0.8);
      }
      .header .title {
        font-size: 3em;
        margin: 0;
        background: linear-gradient(45deg, #00bfff, #1e90ff, #00bfff);
        background-size: 200%;
        -webkit-background-clip: text;
        -webkit-text-fill-color: transparent;
        animation: gradientAnimation 4s ease infinite;
        text-shadow: 1px 1px 5px #000;
      }
      .header .subtitle {
        font-size: 1.5em;
        margin-top: 8px;
        color: #4fc3f7;
        animation: glow 2s infinite alternate;
      }
      @keyframes slideDown {
        from { opacity: 0; transform: translateY(-40px); }
        to   { opacity: 1; transform: translateY(0); }
      }
      @keyframes gradientAnimation {
        0% { background-position: 0% 50%; }
        50% { background-position: 100% 50%; }
        100% { background-position: 0% 50%; }
      }
      @keyframes glow {
        from { text-shadow: 0 0 10px #4fc3f7; }
        to { text-shadow: 0 0 20px #4fc3f7; }
      }

      .steam-card {
        background: #1e1e1e;
        border-radius: 8px;
        padding: 20px;
        margin-bottom: 20px;
        text-align: center;
        box-shadow: 0 4px 8px rgba(0, 0, 0, 0.7);
        animation: popIn 1s ease-out;
        position: relative;
      }
      @keyframes popIn {
        0% { opacity: 0; transform: scale(0.5) rotate(-5deg); }
        100% { opacity: 1; transform: scale(1) rotate(0deg); }
      }
      .account-label {
        font-size: 1.4em;
        color: #4fc3f7;
        margin-bottom: 10px;
        letter-spacing: 0.05em;
      }
      .steam-card img.avatar {
        border-radius: 50%;
        width: 100px;
        height: 100px;
        margin-bottom: 15px;
        transition: transform 0.3s ease;
      }
      .steam-card img.avatar:hover { transform: scale(1.1); }
      .steam-card .name { font-size: 1.2em; margin: 10px 0; }
      .steam-card .id { font-size: 0.9em; color: #b0b0b0; }

      .connect-btn {
        display: block;
        width: 100%;
        max-width: 300px;
        margin: 10px auto;
        padding: 12px 25px;
        font-size: 1.3em;
        color: #ffffff;
        background: linear-gradient(135deg, #00796b, #004d40);
        border: none;
        border-radius: 50px;
        cursor: pointer;
        transition: transform 0.2s ease, box-shadow 0.2s ease;
        box-shadow: 0 4px 8px rgba(0, 0, 0, 0.3);
      }
      .connect-btn:hover {
        transform: scale(1.05);
        box-shadow: 0 6px 12px rgba(0, 0, 0, 0.4);
      }
      .connect-btn:active { transform: scale(0.98); }
      .connect-btn svg {
        height: 28px;
        width: 28px;
        fill: #ffffff;
        margin-right: 10px;
        vertical-align: middle;
      }

      .discord-join {
        margin-top: 30px;
        padding: 20px;
        background: rgba(0, 0, 0, 0.8);
        border: 2px solid #333;
        border-radius: 8px;
        text-align: center;
        animation: fadeIn 1s ease-out;
      }
      .discord-join img.discord-logo {
        width: 50px;
        height: 50px;
        vertical-align: middle;
        margin-right: 10px;
        filter: grayscale(1);
      }
      .discord-join .join-text {
        font-size: 1.2em;
        display: inline-block;
        vertical-align: middle;
        color: #aaa;
      }
      .discord-join a.discord-btn {
        display: inline-block;
        margin-top: 10px;
        padding: 10px 20px;
        background: #3a3a3a;
        color: #fff;
        text-decoration: none;
        border-radius: 50px;
        transition: transform 0.2s ease, box-shadow 0.2s ease;
      }
      .discord-join a.discord-btn:hover {
        transform: scale(1.05);
        box-shadow: 0 4px 8px rgba(0, 0, 0, 0.4);
      }

      .footer {
        margin-top: 40px;
        font-size: 0.8em;
        color: #777;
        text-align: center;
        animation: fadeIn 1.5s ease-out;
      }
    </style>
  </head>
  <body>
    <div class="container">
      <div class="header">
        <img src="/icon.svg" alt="Universal Framework Logo" class="logo" />
        <h1 class="title">Universal Framework</h1>
        <h2 class="subtitle">Connect to Discord</h2>
      </div>

      <% if (Connected) { %>
        <div class="steam-card">
          <p>Your Steam account is already linked to a Discord account.</p>
        </div>
      <% } else { %>
        <div class="steam-card" id="SteamCard">
          <p class="account-label">Your Steam Account</p>
          <img id="cardAvatar" class="avatar" src="/placeholder.png" alt="Your Steam Avatar" />
          <p class="name" id="cardName">Loading Steam Info...</p>
          <p class="id">Steam ID: <%= SteamId %></p>
          <button class="connect-btn" onclick="login()">
            <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 200 200">
              <path d="M118.4 84.2c-5.7 0-10.2 4.9-10.2 11s4.6 11 10.2 11c5.7 0 10.2-4.9 10.2-11s-4.6-11-10.2-11zm-36.5 0c-5.7 0-10.2 4.9-10.2 11s4.6 11 10.2 11c5.7 0 10.2-4.9 10.2-11 .1-6.1-4.5-11-10.2-11zM167 1H33c-11.3 0-20.5 9.2-20.5 20.5v134c0 11.3 9.2 20.5 20.5 20.5h113.4l-5.3-18.3 12.8 11.8 12.1 11.1 21.6 18.7V21.5C187.5 10.2 178.3 1 167 1zm-38.6 129.5l-6.6-8c13.1-3.7 18.1-11.8 18.1-11.8-4.1 2.7-8 4.6-11.5 5.9-5 2.1-9.8 3.4-14.5 4.3-9.6 1.8-18.4 1.3-25.9-.1-5.7-1.1-10.6-2.6-14.7-4.3-2.3-.9-4.8-2-7.3-3.4-.3-.2-.6-.3-.9-.5-.2-.1-.3-.2-.4-.2-1.8-1-2.8-1.7-2.8-1.7s4.8 7.9 17.5 11.7c-3 3.8-6.7 8.2-6.7 8.2-22.1-.7-30.5-15.1-30.5-15.1 0-31.9 14.4-57.8 14.4-57.8 14.4-10.7 28-10.4 28-10.4l1 1.2c-18 5.1-26.2 13-26.2 13s2.2-1.2 5.9-2.8C76 54 84.5 52.8 88 52.4c.6-.1 1.1-.2 1.7-.2 6.1-.8 13-1 20.2-.2 9.5 1.1 19.7 3.9 30.1 9.5 0 0-7.9-7.5-24.9-12.6l1.4-1.6s13.7-.3 28 10.4c0 0 14.4 25.9 14.4 57.8 0-.1-8.4 14.3-30.5 15z"/>
            </svg>
            <span>Connect</span>
          </button>
        </div>
      <% } %>

      <div class="discord-join">
        <img src="/icon.svg" alt="Discord Logo" class="discord-logo" />
        <div class="join-text">
          You must join our Discord <strong>before</strong> connecting your account!
        </div>
        <br />
        <a href="https://discord.gg/[REPLACEME]" target="_blank" class="discord-btn">
          Join Discord
        </a>
      </div>

      <div class="footer">
        <p>
          Notice: This mod collects minimal Discord account data, and is able to send you messages 
          even when disconnected from the server. By connecting your account, you consent to this 
          limited use and agree that the mod creator assumes no liability for any abuse by server 
          operators, including privacy or security breaches.
        </p>
        <p>
          <a href="https://github.com/sponsors/DaemonF0rge?o=esb" style="color: #716dcf;">
            Support this mod by donating on GitHub
          </a>
        </p>
      </div>
    </div>

    <script>
      const fetch_retry = async (url, options, n) => {
        try {
          return await fetch(url, options);
        } catch (err) {
          if (n === 1) throw err;
          return await fetch_retry(url, options, n - 1);
        }
      };

      function resolveID(res) {
        console.log("Steam data fetched:", res);
        document.getElementById("cardName").innerText = res.personaname || "Unknown User";
        document.getElementById("cardAvatar").src = res.avatarmedium || "/defaultSteamAvatar.png";
      }

      function login() {
        window.location.replace(`<%= Login_URL %>`);
      }

      document.addEventListener("DOMContentLoaded", function () {
        const steamUrl = `https://daemonforge.dev/SteamId/id/?id=<%= SteamId %>`;
        fetch_retry(steamUrl, {}, 3)
          .then((response) => response.json().then((json) => resolveID(json)))
          .catch((e) => console.error("Fetch error:", e));
      });
    </script>
  </body>
</html>
```

### Default Success Template (`discordSuccess.ejs`)

```html
<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <link rel="icon" type="image/x-icon" href="/favicon.ico" />
    <link rel="preconnect" href="https://fonts.googleapis.com" />
    <link href="https://fonts.googleapis.com/css2?family=Roboto:wght@400;700&display=swap" rel="stylesheet" />

    <!--
      Passed Variables:
      • SteamId          - The Steam ID of the user.
      • DiscordId        - The Discord Id.
      • DiscordUsername  - The Discord Username.
      • DiscordName      - The DisplayName for the Discord user.
      • DiscordAvatar    - The URL to the Discord avatar.
    -->
    <title>Account Linked Successfully</title>
    
    <style>
      body {
        font-family: 'Roboto', sans-serif;
        background: linear-gradient(45deg, #1a1a1a, #141414, #1a1a1a);
        background-size: 400% 400%;
        animation: gradientBackground 20s ease infinite;
        color: #DCEDC8;
        margin: 0;
        padding: 0;
        filter: contrast(0.9) saturate(0.8);
      }
      @keyframes gradientBackground {
        0%   { background-position: 0% 50%; }
        50%  { background-position: 100% 50%; }
        100% { background-position: 0% 50%; }
      }
      
      .container {
        max-width: 900px;
        margin: 40px auto;
        padding: 20px;
        background: rgba(0, 0, 0, 0.95);
        border-radius: 10px;
        box-shadow: 0 8px 16px rgba(0, 0, 0, 0.85);
        animation: fadeIn 1s ease-out;
        position: relative;
        overflow: hidden;
      }
      @keyframes fadeIn {
        from { opacity: 0; transform: translateY(30px); }
        to   { opacity: 1; transform: translateY(0); }
      }
      
      .header {
        display: flex;
        flex-direction: column;
        align-items: center;
        margin-bottom: 30px;
        animation: slideDown 1s ease-out;
      }
      .header img.logo {
        width: 60px;
        height: 60px;
        margin-bottom: 10px;
        filter: brightness(0.8);
      }
      .header .title {
        font-size: 3em;
        margin: 0;
        background: linear-gradient(45deg, #00bfff, #1e90ff, #00bfff);
        background-size: 200%;
        -webkit-background-clip: text;
        -webkit-text-fill-color: transparent;
        animation: gradientAnimation 4s ease infinite;
        text-shadow: 1px 1px 5px #000;
      }
      .header .subtitle {
        font-size: 1.5em;
        margin-top: 8px;
        color: #4fc3f7;
        animation: glow 2s infinite alternate;
      }
      @keyframes slideDown {
        from { opacity: 0; transform: translateY(-40px); }
        to   { opacity: 1; transform: translateY(0); }
      }
      @keyframes gradientAnimation {
        0% { background-position: 0% 50%; }
        50% { background-position: 100% 50%; }
        100% { background-position: 0% 50%; }
      }
      @keyframes glow {
        from { text-shadow: 0 0 10px #4fc3f7; }
        to { text-shadow: 0 0 20px #4fc3f7; }
      }
      
      .accounts {
        display: flex;
        justify-content: space-around;
        flex-wrap: wrap;
        gap: 30px;
      }
      
      .account-card {
        background: #1e1e1e;
        border-radius: 8px;
        padding: 20px;
        flex: 1;
        min-width: 280px;
        box-shadow: 0 4px 8px rgba(0, 0, 0, 0.7);
        text-align: center;
        animation: fadeIn 1s ease-out;
      }
      .account-card h2 { margin-top: 0; color: #DCEDC8; }
      .account-card img.avatar {
        border-radius: 50%;
        width: 100px;
        height: 100px;
        margin-bottom: 15px;
        transition: transform 0.3s ease;
      }
      .account-card img.avatar:hover { transform: scale(1.1); }
      .account-card .name { font-size: 1.2em; margin: 10px 0; }
      .account-card .id { font-size: 0.9em; color: #B0B0B0; }
      .account-card a.profile-link {
        margin-top: 10px;
        display: inline-block;
        text-decoration: none;
        color: #4FC3F7;
        background: #0D47A1;
        padding: 8px 12px;
        border-radius: 4px;
        transition: background 0.2s, transform 0.2s;
      }
      .account-card a.profile-link:hover {
        background: #1565C0;
        transform: scale(1.05);
      }
      
      .footer {
        margin-top: 30px;
        font-size: 0.8em;
        color: #B0B0B0;
        text-align: center;
        animation: fadeIn 1.5s ease-out;
      }
    </style>
  </head>
  <body>
    <div class="container">
      <div class="header">
        <img src="/icon.svg" alt="Universal Framework Logo" class="logo" />
        <h1 class="title">Universal Framework</h1>
        <h2 class="subtitle">Account Linked Successfully</h2>
      </div>

      <div class="accounts">
        <div class="account-card discord">
          <h2>Discord Account</h2>
          <img src="<%= DiscordAvatar %>" alt="Discord Avatar" class="avatar">
          <p class="name">
            <%= DiscordName %>
            <br><small>(<%= DiscordUsername %>)</small>
          </p>
          <p class="id">ID: <%= DiscordId %></p>
        </div>
        
        <div class="account-card steam">
          <h2>Steam Account</h2>
          <img id="cardAvatar" class="avatar" src="/placeholder.png" alt="Your Steam Avatar" />
          <p class="name" id="cardName">Loading Steam Info...</p>
          <p class="id">ID: <%= SteamId %></p>
          <a id="profileLink" href="#" class="profile-link" target="_blank" style="display: none;">
            View Steam Profile
          </a>
        </div>
      </div>

      <div class="footer">
        <p>Thank you for linking your accounts. Enjoy our mod's features and services.</p>
      </div>
    </div>

    <script>
      const fetch_retry = async (url, options, n) => {
        try {
          return await fetch(url, options);
        } catch (err) {
          if (n === 1) throw err;
          return await fetch_retry(url, options, n - 1);
        }
      };

      function resolveID(res) {
        console.log("Steam data fetched:", res);
        document.getElementById("cardName").innerText = res.personaname || "Unknown User";
        document.getElementById("cardAvatar").src = res.avatarmedium || "/defaultSteamAvatar.png";
        if (res.profileurl) {
          const profileLink = document.getElementById("profileLink");
          profileLink.href = res.profileurl;
          profileLink.style.display = "inline-block";
        }
      }

      document.addEventListener("DOMContentLoaded", function () {
        const steamUrl = `https://daemonforge.dev/SteamId/id/?id=<%= SteamId %>`;
        fetch_retry(steamUrl, {}, 3)
          .then((response) => response.json().then((json) => resolveID(json)))
          .catch((e) => console.error("Fetch error:", e));
      });
    </script>
  </body>
</html>
```

---

## Additional Template Assets

You can also customize these files in the templates directory:

| File | Purpose | Recommended Size |
|------|---------|-----------------|
| `favicon.ico` | Browser tab icon | 32x32 or 16x16 |
| `icon.png` | Page icon image | 128x128 |
| `icon.svg` | Scalable vector icon | Any |

These are served at:
- `/favicon.ico`
- `/icon.png`
- `/icon.svg`

---

## Troubleshooting Templates

### Template Not Loading

**Windows:**
1. Verify the file exists:
   ```powershell
   Test-Path "$env:APPDATA\ufserverservice\templates\discordLogin.ejs"
   ```
2. Ensure file extension is `.ejs` (not `.ejs.txt`)
3. Check logs: Right-click tray icon → "📁 Logs"

**Linux:**
1. Check file permissions:
   ```bash
   ls -la /var/lib/ufserverservice/templates/
   sudo chown ufservice:ufservice /var/lib/ufserverservice/templates/*
   sudo chmod 644 /var/lib/ufserverservice/templates/*.ejs
   ```
2. Verify file extension is `.ejs`
3. Check logs: `journalctl -u ufserverservice -f`

### Syntax Errors

If your template has EJS syntax errors:
- The service falls back to the default template
- Check logs for: `ERROR IN [TEMPLATE_NAME] TEMPLATE`

**Common syntax errors and fixes:**

| Error Message | Cause | Fix |
|---------------|-------|-----|
| `SyntaxError: missing )` | Unclosed parenthesis | Check all `<%` and `%>` pairs match |
| `SyntaxError: Unexpected token` | Invalid JS in `<% %>` | Verify JavaScript syntax |
| `ReferenceError: X is not defined` | Typo in variable | Check spelling (case-sensitive) |
| `Cannot read property of undefined` | Missing variable | Ensure variable is passed to template |

### Variables Not Displaying

- Ensure you're using `<%= variable %>` not `<% variable %>`
- Check variable spelling - they are case-sensitive:
  - ✅ `<%= SteamId %>`
  - ❌ `<%= steamid %>`
  - ❌ `<%= steamId %>`
- Verify you're editing the correct template file

### Testing Templates

You can test templates by visiting the login URL with any Steam ID:
```
https://your-service-url/Discord/login/76561198000000000
```

Use browser DevTools (F12) to:
- Check for JavaScript errors in Console
- Inspect HTML elements
- Test CSS styling

---

## Restoring Default Templates

To restore default templates, delete your custom templates and restart the service. Defaults are recreated automatically.

**Windows (PowerShell):**
```powershell
# Delete custom templates
Remove-Item "$env:APPDATA\ufserverservice\templates\discord*.ejs"

# Restart from tray icon or:
Stop-Process -Name "ufserverservice" -Force
# Then relaunch from Start Menu
```

**Linux:**
```bash
# Delete custom templates
sudo rm /var/lib/ufserverservice/templates/discord*.ejs

# Restart service
sudo systemctl restart ufserverservice
```

The service automatically recreates missing templates with defaults on startup.

## Tags
`operators`, `discord`, `templates`, `branding`, `ejs`, `ui`, `how-to`, `doc-usage`
