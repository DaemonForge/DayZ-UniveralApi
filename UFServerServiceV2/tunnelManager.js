/**
 * tunnelManager.js
 * 
 * Manages an embedded Cloudflare Tunnel (cloudflared) process within the Electron app.
 * Downloads the cloudflared binary on first use, checks for updates via GitHub releases,
 * and spawns/kills the process as needed.
 * 
 * Usage:
 *   const tunnel = require('./tunnelManager');
 *   await tunnel.start(token);          // Start tunnel with a Cloudflare tunnel token
 *   tunnel.stop();                      // Gracefully stop the tunnel
 *   tunnel.getStatus();                 // { running, pid, connectedAt, url, errors, ... }
 *   await tunnel.checkForUpdate();      // Check GitHub for newer version
 *   await tunnel.update();              // Download latest (stops tunnel first if running)
 */

const { spawn, execFile } = require('child_process');
const path = require('path');
const { existsSync, mkdirSync, createWriteStream, unlinkSync, readFileSync, writeFileSync, renameSync } = require('fs');
const https = require('https');
const http = require('http');
const { createLogger } = require('./utils');
const logger = createLogger(global.logger, 'tunnel');

// Platform-specific binary info
const PLATFORM_MAP = {
  win32:  { file: 'cloudflared.exe', url: 'https://github.com/cloudflare/cloudflared/releases/latest/download/cloudflared-windows-amd64.exe' },
  linux:  { file: 'cloudflared',     url: 'https://github.com/cloudflare/cloudflared/releases/latest/download/cloudflared-linux-amd64' },
  darwin: { file: 'cloudflared',     url: 'https://github.com/cloudflare/cloudflared/releases/latest/download/cloudflared-darwin-amd64.tgz' }
};

const GITHUB_LATEST_API = 'https://api.github.com/repos/cloudflare/cloudflared/releases/latest';
const UPDATE_CHECK_INTERVAL = 24 * 60 * 60 * 1000; // 24 hours

class TunnelManager {
  constructor() {
    this._process = null;
    this._status = {
      running: false,
      pid: null,
      connectedAt: null,
      url: null,
      hostname: null,
      errors: [],
      downloading: false,
      installedVersion: null,
      latestVersion: null,
      updateAvailable: false,
      lastUpdateCheck: null
    };
    this._listeners = [];
    this._updateCheckTimer = null;
    this._metaLoaded = false;
  }

  /**
   * Dynamic getter — always resolves against SAVEPATH at call time.
   * Ensures correct path even if module was required before SAVEPATH was set.
   */
  get _binDir() {
    return path.join(global.SAVEPATH || '.', 'bin');
  }

  get _metaPath() {
    return path.join(this._binDir, 'cloudflared-meta.json');
  }

  /**
   * Lazy-load metadata on first access (deferred until SAVEPATH is guaranteed).
   */
  _ensureMetaLoaded() {
    if (!this._metaLoaded) {
      this._metaLoaded = true;
      this._loadMeta();
    }
  }

  // ---- Metadata persistence ----

  _loadMeta() {
    try {
      if (existsSync(this._metaPath)) {
        const data = JSON.parse(readFileSync(this._metaPath, 'utf-8'));
        this._status.installedVersion = data.version || null;
        this._status.lastUpdateCheck = data.lastUpdateCheck || null;
      }
    } catch (_) { /* ignore corrupt meta */ }
  }

  _saveMeta() {
    try {
      if (!existsSync(this._binDir)) {
        mkdirSync(this._binDir, { recursive: true });
      }
      writeFileSync(this._metaPath, JSON.stringify({
        version: this._status.installedVersion,
        lastUpdateCheck: this._status.lastUpdateCheck
      }, null, 2));
    } catch (err) {
      logger.warn('Failed to save cloudflared metadata: ' + err.message);
    }
  }

  /**
   * Get the path to the cloudflared binary for this platform.
   */
  getBinaryPath() {
    this._ensureMetaLoaded();
    const info = PLATFORM_MAP[process.platform];
    if (!info) {
      throw new Error(`Unsupported platform: ${process.platform}`);
    }
    return path.join(this._binDir, info.file);
  }

  /**
   * Check if the cloudflared binary exists locally.
   */
  isBinaryInstalled() {
    return existsSync(this.getBinaryPath());
  }

  /**
   * Download the cloudflared binary for the current platform.
   * After download, detects the installed version from the binary itself.
   */
  async downloadBinary(onProgress) {
    const info = PLATFORM_MAP[process.platform];
    if (!info) {
      throw new Error(`Unsupported platform: ${process.platform}`);
    }

    if (!existsSync(this._binDir)) {
      mkdirSync(this._binDir, { recursive: true });
    }

    const dest = this.getBinaryPath();

    // Clean up any leftover partial download from a previous interrupted attempt
    const tmpDest = dest + '.tmp';
    if (existsSync(tmpDest)) {
      try { unlinkSync(tmpDest); } catch (_) { /* ignore */ }
    }

    this._status.downloading = true;
    this._emit();

    logger.info(`Downloading cloudflared from ${info.url}`);

    try {
      await this._downloadFile(info.url, dest, onProgress);
      // Make executable on Unix
      if (process.platform !== 'win32') {
        const { chmodSync } = require('fs');
        chmodSync(dest, 0o755);
      }
      // Detect installed version from the binary
      const version = await this._detectVersion();
      this._status.installedVersion = version;
      this._saveMeta();
      logger.info(`cloudflared downloaded successfully (version: ${version || 'unknown'})`);
    } finally {
      this._status.downloading = false;
      this._emit();
    }
  }

  /**
   * Run `cloudflared --version` to detect installed version string.
   * Returns something like "2025.2.0" or null on failure.
   */
  _detectVersion() {
    return new Promise((resolve) => {
      const binPath = this.getBinaryPath();
      if (!existsSync(binPath)) {
        return resolve(null);
      }
      try {
        execFile(binPath, ['--version'], { timeout: 10000, windowsHide: true }, (err, stdout, stderr) => {
          if (err) {
            logger.warn(`Failed to detect cloudflared version: ${err.message}`);
            return resolve(null);
          }
          // Output is like: "cloudflared version 2025.2.0 (built 2025-02-10-...)"
          const output = (stdout || '') + (stderr || '');
          const match = output.match(/(\d+\.\d+\.\d+)/);
          resolve(match ? match[1] : null);
        });
      } catch (_) {
        resolve(null);
      }
    });
  }

  /**
   * Check GitHub releases for the latest cloudflared version.
   * Returns { latestVersion, updateAvailable, installedVersion }.
   */
  async checkForUpdate() {
    this._ensureMetaLoaded();
    logger.info('Checking for cloudflared updates...');
    try {
      const release = await this._fetchJSON(GITHUB_LATEST_API);
      const latestVersion = (release.tag_name || '').replace(/^v/, '');
      
      this._status.latestVersion = latestVersion;
      this._status.lastUpdateCheck = new Date().toISOString();
      this._status.updateAvailable = !!(
        latestVersion &&
        this._status.installedVersion &&
        latestVersion !== this._status.installedVersion
      );
      this._saveMeta();
      this._emit();

      logger.info(`Installed: ${this._status.installedVersion || 'none'}, Latest: ${latestVersion}, Update available: ${this._status.updateAvailable}`);
      return {
        installedVersion: this._status.installedVersion,
        latestVersion,
        updateAvailable: this._status.updateAvailable
      };
    } catch (err) {
      logger.error('Failed to check for updates: ' + err.message);
      throw err;
    }
  }

  /**
   * Update cloudflared by downloading the latest binary.
   * Stops the tunnel first if it's running, then optionally restarts.
   * @param {string} [token] - If provided and tunnel was running, restarts after update
   */
  async update(token) {
    const wasRunning = this._status.running;
    
    if (wasRunning) {
      logger.info('Stopping tunnel for update...');
      this.stop();
      // Wait for process to exit
      await new Promise(resolve => setTimeout(resolve, 2000));
    }

    // Delete old binary — with retry logic for Windows file locking
    const binPath = this.getBinaryPath();
    if (existsSync(binPath)) {
      const maxRetries = 5;
      for (let attempt = 1; attempt <= maxRetries; attempt++) {
        try {
          unlinkSync(binPath);
          break; // success
        } catch (err) {
          if (attempt === maxRetries) {
            logger.error('Failed to remove old binary after retries: ' + err.message);
            throw new Error('Cannot remove old cloudflared binary. Is it still running?');
          }
          logger.warn(`File still locked (attempt ${attempt}/${maxRetries}), retrying in 1s...`);
          await new Promise(resolve => setTimeout(resolve, 1000));
        }
      }
    }

    await this.downloadBinary();

    // Restart if it was running and we have a token
    if (wasRunning && token) {
      logger.info('Restarting tunnel after update...');
      await this.start(token);
    }
  }

  /**
   * Start periodic update checks (every 24h by default).
   * Non-blocking — just logs and sets updateAvailable flag.
   */
  startUpdateChecks() {
    // Check once on startup (delayed to avoid blocking)
    setTimeout(() => {
      this.checkForUpdate().catch(() => {});
    }, 30000); // 30s after startup

    // Then every 24 hours
    this._updateCheckTimer = setInterval(() => {
      this.checkForUpdate().catch(() => {});
    }, UPDATE_CHECK_INTERVAL);
  }

  /**
   * Stop periodic update checks.
   */
  stopUpdateChecks() {
    if (this._updateCheckTimer) {
      clearInterval(this._updateCheckTimer);
      this._updateCheckTimer = null;
    }
  }

  /**
   * Fetch JSON from a URL (for GitHub API).
   */
  _fetchJSON(url) {
    return new Promise((resolve, reject) => {
      https.get(url, { headers: { 'User-Agent': 'UFServerService', 'Accept': 'application/vnd.github.v3+json' } }, (res) => {
        if (res.statusCode >= 300 && res.statusCode < 400 && res.headers.location) {
          res.destroy();
          return this._fetchJSON(res.headers.location).then(resolve, reject);
        }
        if (res.statusCode !== 200) {
          res.destroy();
          return reject(new Error(`GitHub API returned ${res.statusCode}`));
        }
        let data = '';
        res.on('data', chunk => { data += chunk; });
        res.on('end', () => {
          try { resolve(JSON.parse(data)); }
          catch (e) { reject(e); }
        });
        res.on('error', reject);
      }).on('error', reject);
    });
  }

  /**
   * Download a file, following redirects.
   * Downloads to a .tmp file first and renames on success to prevent corrupt partial downloads.
   */
  _downloadFile(url, dest, onProgress, redirectCount = 0) {
    if (redirectCount > 5) {
      return Promise.reject(new Error('Too many redirects'));
    }
    const tmpDest = dest + '.tmp';
    return new Promise((resolve, reject) => {
      const mod = url.startsWith('https') ? https : http;
      const request = mod.get(url, { headers: { 'User-Agent': 'UFServerService' } }, (response) => {
        // Handle redirects
        if (response.statusCode >= 300 && response.statusCode < 400 && response.headers.location) {
          response.destroy();
          return this._downloadFile(response.headers.location, dest, onProgress, redirectCount + 1)
            .then(resolve, reject);
        }
        if (response.statusCode !== 200) {
          response.destroy();
          return reject(new Error(`Download failed with status ${response.statusCode}`));
        }

        const totalBytes = parseInt(response.headers['content-length'] || '0', 10);
        let downloadedBytes = 0;

        const file = createWriteStream(tmpDest);
        response.on('data', (chunk) => {
          downloadedBytes += chunk.length;
          if (onProgress && totalBytes > 0) {
            onProgress(Math.round((downloadedBytes / totalBytes) * 100));
          }
        });
        response.pipe(file);
        file.on('finish', () => {
          file.close(() => {
            try {
              // Atomic rename: tmp -> final destination (prevents corrupt partial files)
              renameSync(tmpDest, dest);
              resolve();
            } catch (err) {
              try { unlinkSync(tmpDest); } catch (_) { /* ignore */ }
              reject(err);
            }
          });
        });
        file.on('error', (err) => {
          file.close();
          try { unlinkSync(tmpDest); } catch (_) { /* ignore */ }
          reject(err);
        });
      });
      request.on('error', (err) => {
        try { unlinkSync(tmpDest); } catch (_) { /* ignore */ }
        reject(err);
      });
      request.setTimeout(60000, () => {
        request.destroy();
        try { unlinkSync(tmpDest); } catch (_) { /* ignore */ }
        reject(new Error('Download timed out'));
      });
    });
  }

  /**
   * Start the cloudflared tunnel with the given token.
   * @param {string} token - Cloudflare tunnel token from Zero Trust dashboard
   */
  async start(token) {
    if (this._process) {
      logger.warn('Tunnel already running (PID: ' + this._process.pid + ')');
      return;
    }
    if (!token) {
      throw new Error('Tunnel token is required. Get it from Cloudflare Zero Trust dashboard.');
    }

    // Download binary if needed
    if (!this.isBinaryInstalled()) {
      logger.info('cloudflared binary not found, downloading...');
      await this.downloadBinary();
    }

    const binPath = this.getBinaryPath();
    logger.info('Starting cloudflared tunnel...');

    this._status.errors = [];

    const args = ['tunnel', '--no-autoupdate', 'run', '--token', token];
    
    const proc = spawn(binPath, args, {
      stdio: ['ignore', 'pipe', 'pipe'],
      windowsHide: true
    });

    this._process = proc;
    this._status.running = true;
    this._status.pid = proc.pid;
    this._status.connectedAt = null;
    this._status.url = null;
    this._status.hostname = null;
    this._emit();

    proc.stdout.on('data', (data) => {
      const lines = data.toString().split('\n');
      for (const raw of lines) {
        const line = raw.trim();
        if (line) {
          this._logCloudflaredLine(line);
          this._parseOutput(line);
        }
      }
    });

    proc.stderr.on('data', (data) => {
      const lines = data.toString().split('\n');
      for (const raw of lines) {
        const line = raw.trim();
        if (line) {
          this._logCloudflaredLine(line);
          this._parseOutput(line);
        }
      }
    });

    proc.on('error', (err) => {
      logger.error('Failed to start cloudflared: ' + err.message);
      this._status.errors.push(err.message);
      this._cleanup();
    });

    proc.on('exit', (code, signal) => {
      logger.info(`cloudflared exited (code=${code}, signal=${signal})`);
      this._cleanup();
    });
  }

  /**
   * Log a cloudflared output line at the appropriate level.
   * Noisy/routine lines (JSON timestamps, heartbeats) go to debug; meaningful lines go to info.
   */
  _logCloudflaredLine(line) {
    // Try to parse JSON log lines and log at appropriate level
    try {
      const parsed = JSON.parse(line);
      if (parsed && typeof parsed === 'object') {
        const level = (parsed.level || '').toLowerCase();
        const msg = parsed.message || parsed.msg || '';
        // Suppress any JSON line with no meaningful message
        if (!msg) {
          return;
        }
        // Route by cloudflared's own log level
        if (level === 'error' || level === 'fatal') {
          logger.error('[cloudflared] ' + msg);
        } else if (level === 'warn' || level === 'warning') {
          logger.warn('[cloudflared] ' + msg);
        } else if (level === 'debug') {
          return; // suppress debug-level cloudflared noise
        } else {
          logger.debug('[cloudflared] ' + msg);
        }
        return;
      }
    } catch (_) { /* not JSON, continue */ }
    // Suppress any line that looks like just a JSON timestamp (fallback for parse failures)
    if (/^\{?"?timestamp"?\s*:/.test(line) && line.length < 100) {
      return;
    }
    // Plain-text cloudflared lines: route by level token (INF/ERR/WRN/DBG)
    // Only promote errors/warnings to visible levels; everything else is debug noise.
    if (/\bERR\b/.test(line)) {
      logger.error('[cloudflared] ' + line);
    } else if (/\bWRN\b/.test(line)) {
      logger.warn('[cloudflared] ' + line);
    } else {
      logger.debug('[cloudflared] ' + line);
    }
  }

  /**
   * Parse cloudflared output for connection info.
   * cloudflared outputs JSON-structured log lines, so we try JSON parsing first.
   */
  _parseOutput(line) {
    // Try parsing as JSON first (cloudflared's default output format)
    let json = null;
    try { json = JSON.parse(line); } catch (_) { /* not JSON */ }

    if (json && typeof json === 'object') {
      this._parseJsonOutput(json);
      return;
    }

    // Fallback: plain-text parsing for older cloudflared versions
    if (line.includes('Registered tunnel connection') || line.includes('Connection registered')) {
      if (!this._status.connectedAt) {
        this._status.connectedAt = new Date().toISOString();
        logger.info('Tunnel connected');
        this._emit();
      }
    }

    // Extract hostname from config update lines (plain-text format):
    // e.g. config="{\"ingress\":[{\"hostname\":\"example.com\",...}]}"
    // Quotes may be escaped (\" ) or unescaped (" ) depending on output format
    if (!this._status.hostname) {
      const hostnameMatch = line.match(/\\?"hostname\\?"\s*:\s*\\?"([a-zA-Z0-9][a-zA-Z0-9._-]+\.[a-zA-Z]{2,})\\?"/);
      if (hostnameMatch) {
        this._status.hostname = hostnameMatch[1];
        logger.info('Tunnel hostname detected: ' + this._status.hostname);
        this._emit();
      }
    }

    const quickUrlMatch = line.match(/https?:\/\/[^\s]+\.trycloudflare\.com/);
    if (quickUrlMatch) {
      this._status.url = quickUrlMatch[0];
      this._emit();
    }
    if (line.toLowerCase().includes('error') || line.toLowerCase().includes('failed')) {
      this._status.errors.push(line);
      if (this._status.errors.length > 10) this._status.errors.shift();
      this._emit();
    }
  }

  /**
   * Parse a JSON-structured cloudflared log line.
   */
  _parseJsonOutput(json) {
    const msg = (json.message || json.msg || '').toLowerCase();
    const level = (json.level || json.logLevel || '').toLowerCase();

    // Detect connection registration
    if (msg.includes('registered tunnel connection') || msg.includes('connection registered') || msg.includes('registered connindex')) {
      if (!this._status.connectedAt) {
        this._status.connectedAt = new Date().toISOString();
        logger.info('Tunnel connected');
        this._emit();
      }
    }

    // Detect hostname — search the entire JSON structure for hostname values
    if (!this._status.hostname) {
      let hostname = this._findHostname(json);
      // Also check embedded config strings (cloudflared sends config as a JSON string field)
      if (!hostname && json.config && typeof json.config === 'string') {
        const configMatch = json.config.match(/\\?"hostname\\?"\s*:\s*\\?"([a-zA-Z0-9][a-zA-Z0-9._-]+\.[a-zA-Z]{2,})\\?"/);
        if (configMatch) hostname = configMatch[1];
      }
      if (hostname) {
        this._status.hostname = hostname;
        logger.info('Tunnel hostname detected: ' + hostname);
        this._emit();
      }
    }

    // Detect quick tunnel URL (trycloudflare.com)
    const fullMsg = json.message || json.msg || '';
    const quickUrlMatch = fullMsg.match(/https?:\/\/[^\s"]+\.trycloudflare\.com/);
    if (quickUrlMatch) {
      this._status.url = quickUrlMatch[0];
      this._emit();
    }
    if (json.url && typeof json.url === 'string' && json.url.includes('trycloudflare.com')) {
      this._status.url = json.url;
      this._emit();
    }

    // Detect errors
    if (level === 'error' || level === 'fatal' || msg.includes('error') || msg.includes('failed')) {
      const errMsg = json.message || json.msg || JSON.stringify(json);
      this._status.errors.push(errMsg);
      if (this._status.errors.length > 10) this._status.errors.shift();
      this._emit();
    }
  }

  /**
   * Recursively search a JSON object for a hostname value.
   * Looks for keys like "hostname", "publicHostname", "originUrl" etc.
   * Returns the first valid public hostname found, or null.
   */
  _findHostname(obj, depth = 0) {
    if (!obj || typeof obj !== 'object' || depth > 5) return null;
    const IGNORED = ['localhost', '127.0.0.1', '0.0.0.0', 'trycloudflare.com', 'github.com', 'cloudflare.com', 'argotunnel.com'];

    // Check arrays (e.g. ingress rules)
    if (Array.isArray(obj)) {
      for (const item of obj) {
        const found = this._findHostname(item, depth + 1);
        if (found) return found;
      }
      return null;
    }

    // Check known hostname keys on this object
    const hostnameKeys = ['hostname', 'publicHostname', 'public_hostname', 'originUrl', 'origin'];
    for (const key of hostnameKeys) {
      const val = obj[key];
      if (val && typeof val === 'string' && val.length > 0) {
        // Extract hostname from URL if needed
        let host = val;
        const urlMatch = val.match(/https?:\/\/([a-zA-Z0-9][a-zA-Z0-9.-]+\.[a-zA-Z]{2,})/);
        if (urlMatch) host = urlMatch[1];
        // Validate it looks like a real domain and isn't in the ignore list
        if (host.includes('.') && !IGNORED.some(ig => host.includes(ig))) {
          return host;
        }
      }
    }

    // Recurse into nested objects
    for (const key of Object.keys(obj)) {
      if (typeof obj[key] === 'object' && obj[key] !== null) {
        const found = this._findHostname(obj[key], depth + 1);
        if (found) return found;
      }
    }
    return null;
  }

  /**
   * Stop the cloudflared tunnel.
   */
  stop() {
    if (!this._process) {
      logger.info('No tunnel process running');
      return;
    }
    logger.info('Stopping cloudflared tunnel...');
    try {
      if (process.platform === 'win32') {
        // On Windows, kill the process tree
        spawn('taskkill', ['/pid', String(this._process.pid), '/T', '/F'], { windowsHide: true });
      } else {
        this._process.kill('SIGTERM');
      }
    } catch (err) {
      logger.error('Error stopping tunnel: ' + err.message);
    }
    // Force kill after 5 seconds if still running
    const proc = this._process;
    setTimeout(() => {
      try {
        if (proc && !proc.killed) {
          proc.kill('SIGKILL');
        }
      } catch (_) { /* already dead */ }
    }, 5000);
  }

  /**
   * Clean up state after process exits.
   */
  _cleanup() {
    this._process = null;
    this._status.running = false;
    this._status.pid = null;
    this._status.hostname = null;
    this._status.errors = [];
    this._emit();
  }

  /**
   * Get current tunnel status.
   */
  getStatus() {
    this._ensureMetaLoaded();
    return { ...this._status };
  }

  /**
   * Register a status change listener.
   * @param {Function} fn - Called with status object on each change
   * @returns {Function} Unsubscribe function
   */
  onStatusChange(fn) {
    this._listeners.push(fn);
    return () => {
      this._listeners = this._listeners.filter(l => l !== fn);
    };
  }

  /**
   * Emit status change to all listeners.
   */
  _emit() {
    const status = this.getStatus();
    for (const fn of this._listeners) {
      try { fn(status); } catch (_) { /* ignore listener errors */ }
    }
  }
}

// Singleton
module.exports = new TunnelManager();
