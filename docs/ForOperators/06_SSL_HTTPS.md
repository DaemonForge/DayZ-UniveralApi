# Universal Framework - SSL/HTTPS Configuration

This document covers SSL/TLS certificate configuration for the UF Server Service.

## Table of Contents

1. [Overview](#overview)
2. [Default Self-Signed Certificate](#default-self-signed-certificate)
3. [Custom Certificates](#custom-certificates)
4. [Let's Encrypt Automatic SSL](#lets-encrypt-automatic-ssl)
5. [Reverse Proxy Configuration](#reverse-proxy-configuration)
6. [Troubleshooting](#troubleshooting)

---

## Overview

The UF Server Service always runs over HTTPS. There are three options for SSL certificates:

1. **Default Self-Signed Certificate** - Works out of the box, suitable for most setups
2. **Custom Certificate** - Bring your own certificate files
3. **Let's Encrypt** - Automatic free SSL certificates

### Which Should You Choose?

| Scenario | Recommended Option |
|----------|-------------------|
| Testing or local network | Default self-signed |
| DayZ server on same machine | Default self-signed |
| Behind a reverse proxy (nginx, Cloudflare) | Default self-signed (proxy handles SSL) |
| Public-facing service with domain | Let's Encrypt |
| Enterprise with existing certificates | Custom certificate |

---

## Default Self-Signed Certificate

The UF Server Service includes a bundled self-signed certificate that is used automatically if no other certificate is configured.

### How It Works

- No configuration needed
- Works immediately on startup
- DayZ's RestApi accepts self-signed certificates
- Browsers will show a security warning (this is expected)

### Configuration

Leave these fields empty in `config.json`:

```json
{
    "Certificate": "",
    "CertificateKey": ""
}
```

And ensure Let's Encrypt is disabled:

```json
{
    "LetsEncypt": {
        "Enabled": false
    }
}
```

### Browser Access

When accessing the UF Service in a browser with a self-signed certificate:
1. You'll see a security warning
2. Click "Advanced" or "Show Details"
3. Click "Proceed" or "Accept the Risk"
4. The page will load normally

This warning is expected and doesn't affect the DayZ mod connection.

---

## Custom Certificates

Use your own SSL certificate files.

### Prerequisites

You need:
- Certificate file (.crt or .pem)
- Private key file (.key)

### Configuration

Update `config.json` with the file paths:

```json
{
    "Certificate": "C:/path/to/certificate.crt",
    "CertificateKey": "C:/path/to/private.key"
}
```

**Linux Example:**
```json
{
    "Certificate": "/etc/ssl/certs/uf-service.crt",
    "CertificateKey": "/etc/ssl/private/uf-service.key"
}
```

### Certificate Format

The certificate file should be in PEM format:
```
-----BEGIN CERTIFICATE-----
MIIDXTCCAkWgAwIBAgIJAJC1HiIAZAiUMA0Gcq...
-----END CERTIFICATE-----
```

The key file should also be PEM format:
```
-----BEGIN PRIVATE KEY-----
MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwgg...
-----END PRIVATE KEY-----
```

### Certificate Chain

If you have intermediate certificates, concatenate them into the certificate file:
```
-----BEGIN CERTIFICATE-----
(Your certificate)
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
(Intermediate certificate)
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
(Root certificate - optional)
-----END CERTIFICATE-----
```

### File Permissions (Linux)

Ensure the service user can read the certificate files:

```bash
sudo chown ufservice:ufservice /etc/ssl/certs/uf-service.crt
sudo chown ufservice:ufservice /etc/ssl/private/uf-service.key
sudo chmod 644 /etc/ssl/certs/uf-service.crt
sudo chmod 600 /etc/ssl/private/uf-service.key
```

---

## Let's Encrypt Automatic SSL

Let's Encrypt provides free, automatically renewed SSL certificates.

### Requirements

1. A domain name pointing to your server's public IP
2. Port 80 accessible from the internet (for ACME challenges)
3. Port 443 accessible from the internet (for HTTPS)
4. Server must be publicly accessible (not localhost)

### Configuration

Update `config.json`:

```json
{
    "Certificate": "",
    "CertificateKey": "",
    "LetsEncypt": {
        "Enabled": true,
        "Domain": "uf.yourdomain.com",
        "Email": "admin@yourdomain.com",
        "AltNames": []
    }
}
```

### Configuration Options

#### Domain (Required)
The primary domain name for the certificate.

**Valid examples:**
- `uf.example.com`
- `dayz.myserver.net`

**Invalid examples:**
- `localhost`
- `192.168.1.100`
- IP addresses are not supported

#### Email (Required)
Email address for Let's Encrypt notifications:
- Certificate expiry warnings
- Account recovery
- Terms of service updates

#### AltNames (Optional)
Additional domain names to include in the certificate:

```json
{
    "AltNames": ["uf2.example.com", "api.example.com"]
}
```

### How It Works

1. On startup, UF Service starts the Greenlock certificate manager
2. An ACME challenge server starts on port 80
3. Let's Encrypt verifies domain ownership via HTTP challenge
4. Certificate is issued and stored in the `greenlock/` directory
5. HTTPS server starts on configured port (usually 443)
6. Certificates auto-renew approximately 30 days before expiry

### Directory Structure

Certificates and Greenlock data are stored in:

**Windows Electron:** `%APPDATA%\ufserverservice\greenlock\`
**Linux:** `/var/lib/ufserverservice/greenlock/`

```
greenlock/
â”œâ”€â”€ greenlock.d/
â”‚   â””â”€â”€ config.json
â”œâ”€â”€ accounts/
â”‚   â””â”€â”€ (Let's Encrypt account data)
â””â”€â”€ live/
    â””â”€â”€ yourdomain.com/
        â”œâ”€â”€ cert.pem
        â”œâ”€â”€ chain.pem
        â”œâ”€â”€ fullchain.pem
        â””â”€â”€ privkey.pem
```

### First-Time Setup

1. Configure the domain and email in `config.json`
2. Ensure ports 80 and 443 are open and not in use by other services
3. Start the UF Service
4. Watch the logs for certificate issuance:
   ```
   [WebServer] Let's Encrypt event: certificate_issued
   ```

### Rate Limits

Let's Encrypt has rate limits:
- 50 certificates per registered domain per week
- 5 failed validation attempts per hour per domain

Avoid repeatedly deleting the `greenlock/accounts/` directory, as this creates new accounts and may trigger rate limits.

### Fallback Behavior

When Let's Encrypt is enabled, the service also maintains the self-signed certificate for:
- Localhost connections
- Connections to IP addresses
- Hostnames not in the configured domain list

This allows local testing while using Let's Encrypt for public access.

---

## Reverse Proxy Configuration

If you're running behind a reverse proxy (nginx, Apache, Cloudflare, etc.), the UF Service doesn't need a public certificate.

### Benefits of Reverse Proxy

- SSL termination at the proxy
- Load balancing capability
- DDoS protection (with Cloudflare)
- Easier certificate management

### Configuration

Use the default self-signed certificate for UF Service:

```json
{
    "Certificate": "",
    "CertificateKey": "",
    "LetsEncypt": {
        "Enabled": false
    },
    "Port": 8443
}
```

### Nginx Example

```nginx
server {
    listen 443 ssl;
    server_name uf.example.com;
    
    ssl_certificate /etc/letsencrypt/live/uf.example.com/fullchain.pem;
    ssl_certificate_key /etc/letsencrypt/live/uf.example.com/privkey.pem;
    
    location / {
        proxy_pass https://127.0.0.1:8443;
        proxy_ssl_verify off;  # Accept self-signed cert from UF Service
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
    }
}
```

### Cloudflare Configuration

1. Add your domain to Cloudflare
2. Set SSL/TLS mode to "Full" (not "Full (strict)")
   - "Full" accepts self-signed certificates from origin
3. Configure A record pointing to your server
4. UF Service uses default self-signed certificate

### Rate Limit Whitelist

Add your proxy's IP to the whitelist:

```json
{
    "RateLimitWhiteList": ["127.0.0.1", "proxy-ip-address"]
}
```

---

## Troubleshooting

### Let's Encrypt Issues

#### "LetsEncrypt disabled: no valid hostnames provided"

**Cause:** The domain is invalid or empty.

**Solution:** 
- Verify `Domain` is set correctly
- Domain must be a valid hostname, not an IP address
- Domain must be publicly accessible

#### "ACME challenge failed"

**Cause:** Let's Encrypt couldn't verify domain ownership.

**Check:**
1. Port 80 is open in firewall
2. No other service is using port 80
3. Domain DNS points to this server's public IP
4. Server is publicly accessible

#### "Rate limit exceeded"

**Cause:** Too many certificate requests.

**Solution:**
- Wait for the rate limit window to reset (usually 1 hour or 1 week)
- Avoid repeatedly deleting the `greenlock/` directory
- Use staging endpoint for testing (advanced)

#### "Greenlock accounts directory missing" warning

This is normal on first run. A new Let's Encrypt account will be created.

### Certificate Errors

#### "Certificate file not found"

**Cause:** The path in `Certificate` or `CertificateKey` is wrong.

**Solution:**
- Verify the file paths are correct
- Use absolute paths
- Check file exists and is readable

#### "Key and certificate don't match"

**Cause:** The private key doesn't match the certificate.

**Solution:**
- Regenerate the certificate/key pair
- Verify you're using the correct files

### Connection Issues

#### "SSL_ERROR_RX_RECORD_TOO_LONG"

**Cause:** Client is trying HTTP on the HTTPS port.

**Solution:**
- Use `https://` in URLs, not `http://`
- Verify the correct port is being used

#### "Certificate expired"

**Cause:** Let's Encrypt renewal failed or custom certificate expired.

**Solution:**
- For Let's Encrypt: Check logs for renewal errors, ensure ports are open
- For custom: Renew with your certificate provider

#### Browser shows "Not Secure"

**Cause:** Self-signed certificate or certificate chain issues.

**Solution:**
- For self-signed: This is expected, proceed through the warning
- For Let's Encrypt: Check that the full chain is being served
- For custom: Ensure intermediate certificates are included

## Tags
`operators`, `ssl`, `https`, `certificates`, `security`, `reverse-proxy`, `how-to`, `doc-usage`
