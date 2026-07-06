#!/usr/bin/env node
/**
 * Headless entry point for Universal Framework Service
 * Used for Linux/non-GUI deployments via pkg
 */

const path = require('path');
const { readFileSync, writeFileSync, existsSync, mkdirSync } = require('fs');

// Set global flags before loading app
global.isElectron = false;
global.SAVEPATH = process.env.UF_SAVE_PATH || './';
global.APIVERSION = require('./package.json').version;
global.rootPath = __dirname;

// --------------- Config helper ---------------
function loadConfigSync() {
  const configPath = path.join(global.SAVEPATH, 'config.json');
  try {
    const configDir = path.dirname(configPath);
    if (!existsSync(configDir)) {
      mkdirSync(configDir, { recursive: true });
    }
    return JSON.parse(readFileSync(configPath, 'utf-8'));
  } catch (err) {
    console.error('Error reading config:', err);
    return {};
  }
}

// --------------- Tunnel auto-start ---------------
let tunnelManager;

function startTunnelIfConfigured() {
  try {
    const cfg = loadConfigSync();
    if (cfg.Tunnel && cfg.Tunnel.enabled && cfg.Tunnel.token && cfg.Tunnel.autoStart) {
      tunnelManager = require('./tunnelManager');
      (global.logger || console).info('[Tunnel] Auto-starting cloudflared tunnel...');
      tunnelManager.start(cfg.Tunnel.token).catch(err => {
        (global.logger || console).error('[Tunnel] Auto-start failed:', err.message);
      });
      tunnelManager.startUpdateChecks();
    }
  } catch (err) {
    (global.logger || console).error('[Tunnel] Error checking tunnel config for auto-start:', err.message);
  }
}

// --------------- Proxy auto-renew ---------------
let proxyRenewInterval;

async function renewProxyToken() {
  const config = loadConfigSync();
  if (!config.Proxy || !config.Proxy.subdomain || !config.Proxy.token || !config.Proxy.autoRenew) {
    return;
  }
  const keepaliveUrl = 'https://ufapi.daemonforge.dev/keepalive';
  try {
    const res = await fetch(keepaliveUrl, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ subdomain: config.Proxy.subdomain, token: config.Proxy.token })
    });
    if (res.ok) {
      (global.logger || console).info('Proxy token renewed successfully.');
      config.Proxy.lastRenew = new Date().toISOString();
      writeFileSync(path.join(global.SAVEPATH, 'config.json'), JSON.stringify(config, null, 2));
    } else {
      const text = await res.text();
      (global.logger || console).error('Failed to renew proxy token: ' + text);
    }
  } catch (e) {
    (global.logger || console).error('Error renewing proxy token: ' + e.message);
  }
}

function startProxyAutoRenew() {
  renewProxyToken();
  proxyRenewInterval = setInterval(renewProxyToken, 24 * 60 * 60 * 1000);
}

// --------------- Graceful shutdown ---------------
async function gracefulShutdown(signal) {
  const cluster = require('cluster');
  const isPrimary = cluster.isMaster || cluster.isPrimary;

  // Only the primary process logs the shutdown message to avoid 8x noise
  if (isPrimary) {
    console.log(`\nReceived ${signal}. Shutting down gracefully...`);
  }

  // Stop cloudflared tunnel & update checks (only set in primary)
  try {
    if (tunnelManager) {
      tunnelManager.stopUpdateChecks();
      tunnelManager.stop();
    }
  } catch (err) {
    (global.logger || console).warn('Error stopping tunnel on exit', { error: err.message });
  }

  // Stop proxy auto-renew interval (only set in primary)
  if (proxyRenewInterval) {
    clearInterval(proxyRenewInterval);
  }

  // Close the shared MongoDB connection pool (each worker process has its own)
  try {
    const { closeDb } = require('./models/db');
    await closeDb();
  } catch (_) { /* closing silently is fine for workers */ }

  process.exit(0);
}

process.on('SIGINT', () => gracefulShutdown('SIGINT'));
process.on('SIGTERM', () => gracefulShutdown('SIGTERM'));

// --------------- Start the application ---------------
require('./app');

// Start tunnel and proxy only in the master/primary process (not in cluster workers)
const cluster = require('cluster');
setTimeout(() => {
  if (cluster.isMaster || cluster.isPrimary) {
    startTunnelIfConfigured();
    startProxyAutoRenew();
  }
}, 3000);