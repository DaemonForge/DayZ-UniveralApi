# Cloudflare Tunnel Setup

Expose your UF Server Service to the internet securely using a Cloudflare Tunnel — **without opening any ports** on your router or firewall.

---

## Table of Contents

1. [Overview](#overview)
2. [Cost](#cost)
3. [Prerequisites](#prerequisites)
4. [Step 1 — Cloudflare Account & Domain](#step-1--cloudflare-account--domain)
5. [Step 2 — Create a Tunnel](#step-2--create-a-tunnel)
6. [Step 3 — Configure the Public Hostname](#step-3--configure-the-public-hostname)
7. [Step 4 — Configure the UF Service](#step-4--configure-the-uf-service)
8. [Step 5 — Configure the DayZ Mod](#step-5--configure-the-dayz-mod)
9. [Managing the Tunnel](#managing-the-tunnel)
10. [Updating cloudflared](#updating-cloudflared)
11. [Troubleshooting](#troubleshooting)
12. [FAQ](#faq)

---

## Overview

Cloudflare Tunnel creates a secure outbound-only connection from your machine to Cloudflare's edge. Visitors hit Cloudflare's network, and traffic is relayed through the tunnel to your service. This means:

- **No open ports** — your firewall stays closed.
- **DDoS protection** — Cloudflare absorbs attacks before they reach you.
- **Automatic SSL** — Cloudflare handles the public HTTPS certificate.
- **No static IP required** — works behind CGNAT, dynamic IPs, etc.

### How It Works

```
DayZ Client/Server
        │
        ▼ (HTTPS)
  Cloudflare Edge  ◄── public internet ──►  Players
        │
        ▼ (encrypted tunnel)
  cloudflared binary (on your machine)
        │
        ▼ (HTTPS / self-signed)
  UF Server Service (localhost)
```

---

## Cost

Cloudflare Tunnel is part of **Cloudflare Zero Trust**, which has a **free tier** supporting up to **50 users**. This is more than enough for virtually any DayZ server setup.

| What You Need | Cost |
|---------------|------|
| Cloudflare Zero Trust (free plan) | **Free** |
| Domain name (required) | ~$10/year if registering through Cloudflare |
| Already own a domain? | **Free** — just transfer DNS to Cloudflare |

You do **not** need a paid Cloudflare plan unless you require advanced access policies or have more than 50 users.

---

## Prerequisites

Before starting, make sure you have:

- [ ] A running UF Server Service (Windows Electron app or Linux service)
- [ ] A domain name (any TLD works — `.com`, `.dev`, `.onl`, etc.)
- [ ] A Cloudflare account (free)

---

## Step 1 — Cloudflare Account & Domain

1. **Create a Cloudflare account** at [dash.cloudflare.com/sign-up](https://dash.cloudflare.com/sign-up) if you don't have one.

2. **Add your domain** to Cloudflare:
   - Click **Add a site** in the Cloudflare dashboard.
   - Enter your domain name and select the **Free** plan.
   - Cloudflare will scan your existing DNS records.
   - Update your domain registrar's **nameservers** to the ones Cloudflare provides.
   - Wait for DNS propagation (usually a few minutes, up to 24 hours).

> **Tip:** If you don't have a domain yet, you can register one directly through Cloudflare at [dash.cloudflare.com/domains](https://dash.cloudflare.com). Cheap TLDs like `.onl` or `.site` often cost under $10/year.

---

## Step 2 — Create a Tunnel

1. Go to the **Cloudflare Zero Trust dashboard**: [one.dash.cloudflare.com](https://one.dash.cloudflare.com/)
   - If this is your first time, select the **Free** plan when prompted.

2. Navigate to **Networks → Tunnels**.

3. Click **Create a tunnel**.

4. Select **Cloudflared** as the connector type.

5. Give your tunnel a name (e.g. `my-dayz-server`).

6. On the **Install connector** page, Cloudflare will display an install command like:

   ```
   cloudflared.exe service install aCEiJlnpR0tVxBDF1H2FhN3Prtvw4z ...
   ```

7. **Copy the token** — this is the `eyJ...` part after `service install`. You can copy the entire command if you like; the UF Service app will automatically strip the prefix and extract just the token.

> **Important:** Do NOT run this command yourself. The UF Service manages the cloudflared binary for you.

---

## Step 3 — Configure the Public Hostname

After creating the tunnel, Cloudflare asks you to set up a **Public Hostname**. This is the URL that your DayZ mod will connect to.

1. **Subdomain**: Enter a subdomain of your choice (e.g. `api`, `uf`, `dayz`).
2. **Domain**: Select your domain from the dropdown.
3. **Path**: Leave empty.

4. **Service**:
   - **Type**: `HTTPS`
   - **URL**: `localhost` (just `localhost` — Cloudflare appends the port from the tunnel config)

   > If your UF Service runs on a non-default port, enter `localhost:<port>` (e.g. `localhost:3000`). The default port is `443`.

5. Expand **Additional application settings → TLS**.

6. **Enable "No TLS Verify"** (toggle it ON).

   > This is **required** because the UF Service uses a self-signed certificate locally. Without this, Cloudflare will reject the connection to your origin.

7. Click **Save tunnel**.

Your public URL will be something like: `https://uf.yourdomain.com`

---

## Step 4 — Configure the UF Service

### Using the Settings UI (Windows)

1. Open **Settings** from the tray menu.
2. In the **SSL/TLS** section, set **Certificate Type** to **Self Signed**.
   - Cloudflare handles the public SSL certificate. The self-signed cert is only for the local connection between cloudflared and the service.
3. Open the **Cloudflare Tunnel** section.
4. **Paste the tunnel token** into the *Tunnel Token* field.
   - You can paste the full command (`cloudflared.exe service install eyJ...`) — the prefix is stripped automatically.
5. Check **Enable Tunnel**.
6. Optionally check **Auto-Start on Launch** (recommended).
7. Click **Save** (bottom-right) and **restart the app**.
8. After restart, the tunnel will start automatically (or click **Start** manually).

### Using config.json (Linux / Manual)

Add the following to your `config.json`:

```json
{
  "Certificate": "",
  "CertificateKey": "",
  "Tunnel": {
    "enabled": true,
    "token": "aCEiJlnpR0tVxBDF1H2FhN3Prtvw4z...",
    "autoStart": true
  }
}
```

> Leave `Certificate` and `CertificateKey` as empty strings to use the built-in self-signed certificate.

---

## Step 5 — Configure the DayZ Mod

Once the tunnel is running, update the DayZ mod configuration file (`UF/UFramework.json` in your DayZ server profile folder):

```json
{
  "ServerURL": "https://api.yourdomain.com",
  "ServerID": "your-server-id",
  "ServerAuth": "your-auth-key"
}
```

Replace `api.yourdomain.com` with the actual subdomain + domain you configured in Step 3 (e.g. `https://mytestserver.dayz.onl`).

> **Note:** No port number is needed in the URL — Cloudflare routes traffic on standard HTTPS port 443.

---

## Managing the Tunnel

### Status Indicators

| Indicator | Meaning |
|-----------|---------|
| ⚫ Not Running | Tunnel is stopped |
| 🟡 Connecting... | cloudflared is starting up |
| 🟢 Connected | Tunnel is active and routing traffic |
| ⏳ Downloading... | cloudflared binary is being downloaded |

### Start / Stop

- **Settings UI**: Use the **Start** and **Stop** buttons in the Cloudflare Tunnel section.
- **Tray menu**: The tray icon shows tunnel status and provides Start/Stop actions.
- **Auto-Start**: When enabled, the tunnel starts automatically when the service launches.

### Download cloudflared

The first time you start the tunnel (or click **Download cloudflared**), the app automatically downloads the correct cloudflared binary for your platform from [GitHub releases](https://github.com/cloudflare/cloudflared/releases).

---

## Updating cloudflared

The UF Service manages cloudflared updates automatically:

- **Automatic checks**: Every 24 hours, the app checks GitHub for a newer version.
- **Update badge**: If an update is available, an amber "Update Available" badge appears in the settings.
- **Manual check**: Click **🔄 Check for Update** to check immediately.
- **Update**: Click **⬆️ Update Now** to download the latest version. If the tunnel is running, it will be stopped, updated, and restarted automatically.

Version information is shown in the settings UI below the status indicator.

---

## Troubleshooting

### Tunnel won't start — "No tunnel token configured"

**Cause**: You entered a token in the UI but haven't saved and restarted yet. The tunnel reads the token from the saved config file, not from the UI.

**Fix**: Click **Save**, restart the app, then try starting the tunnel again.

---

### Tunnel shows 🟡 Connecting but never turns 🟢

**Possible causes**:
- Invalid or revoked token — check the [Zero Trust dashboard](https://one.dash.cloudflare.com/) to verify your tunnel is active.
- Network issues — ensure the machine can reach `api.cloudflare.com` outbound on port 443.
- Firewall blocking outbound connections — cloudflared needs outbound HTTPS access.

---

### DayZ mod gets connection errors

1. **Verify the tunnel is 🟢 Connected** in the UF Service settings.
2. **Check the ServerURL** in `UFramework.json` — it should match your tunnel hostname exactly (e.g. `https://api.yourdomain.com`). No port number.
3. **Check Certificate Type** is set to **Self Signed** in UF Service settings.
4. **Check "No TLS Verify"** is enabled in your Cloudflare tunnel hostname config (Additional application settings → TLS).
5. **Test in a browser** — visit `https://api.yourdomain.com/status` and you should get a JSON response.

---

### 502 Bad Gateway from Cloudflare

**Cause**: Cloudflare can reach the tunnel but the tunnel can't reach your local service.

**Fix**:
- Ensure the UF Service is actually running and listening.
- Verify the port in your tunnel hostname config matches the UF Service port.
- Make sure "No TLS Verify" is enabled (self-signed certificates are rejected by default).

---

### cloudflared download fails

- Check your internet connection.
- If behind a corporate proxy/firewall, ensure access to `github.com` and `objects.githubusercontent.com`.
- Try clicking **Download cloudflared** again.

---

## FAQ

### Do I need to open any ports?

**No.** That's the primary benefit of Cloudflare Tunnel. All connections are outbound from your machine.

### Can I use this with a dynamic IP?

**Yes.** The tunnel is outbound-only, so your IP can change without affecting anything.

### Can I use this alongside the built-in proxy?

Yes, but it's not recommended to use both simultaneously for the same purpose. The Cloudflare Tunnel replaces the need for the built-in DaemonForge proxy. Use one or the other.

### Does this work on Linux?

Yes. The cloudflared binary is downloaded for your platform automatically. On Linux, configure the tunnel via `config.json` as shown in Step 4.

### Can multiple DayZ servers share one tunnel?

Yes. Each DayZ server can point to the same `ServerURL` if they all connect to the same UF Service instance. If you run multiple UF Service instances, create separate tunnels (or hostnames) for each.

### Is my tunnel token sensitive?

**Yes.** Treat it like a password. Anyone with your token could run a connector for your tunnel. The token is stored locally in your `config.json` and is never transmitted anywhere except to Cloudflare for authentication.

### How do I delete a tunnel?

Go to [one.dash.cloudflare.com](https://one.dash.cloudflare.com/) → Networks → Tunnels → select your tunnel → Delete. Then remove the token from your UF Service settings.
