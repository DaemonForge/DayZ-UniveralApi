/**
 * DayZ Universal API Server - Main Application
 * This service provides a RESTful API for DayZ server management and data exchange
 */

// Resolve package metadata so we can reference it even when packaged (where cwd lacks package.json)
const packageMetadata = (() => {
  try {
    return require('./package.json');
  } catch (error) {
    console.warn('[App] Unable to read package.json for metadata; falling back to defaults.', error?.message || error);
    return {
      name: 'Universal Framework Service',
      version: process.env.npm_package_version || '0.0.0'
    };
  }
})();

// Set global constants
if (global.APIVERSION === undefined) {
  global.APIVERSION = process.env.npm_package_version || packageMetadata.version || '0.0.0';
}
global.STABLEVERSION = '0.0.0';
global.NEWVERSIONDOWNLOAD = 'https://github.com/daemonforge/DayZ-UniveralApi/releases';
if (global.SAVEPATH === undefined) {
  global.SAVEPATH = './';
}
// Initialize logger
const logger = require('./log').initializeLogger();
global.logger = logger;
logger.debug('[App] Logger initialized, loading config...'); // ADDED LOG

// Process-level safety nets. Without these, a stray rejection or thrown error
// in async code is silent (unhandledRejection) or aborts the process with no
// log (uncaughtException). We always log; for a headless server we then exit so
// the supervisor/cluster restarts a clean worker (a process left running after
// an uncaught exception is in an undefined state). In the Electron desktop app
// we keep running so a background error doesn't kill the tray UI.
process.on('unhandledRejection', (reason) => {
  const err = reason instanceof Error ? reason : new Error(String(reason));
  logger.error('[App] Unhandled promise rejection', { error: err.message, stack: err.stack });
});
process.on('uncaughtException', (err) => {
  logger.error('[App] Uncaught exception', { error: err.message, stack: err.stack });
  if (!global.isElectron) {
    // Give the log a tick to flush, then exit for a clean restart.
    setTimeout(() => process.exit(1), 250);
  }
});

// Load configuration
global.config = require('./configLoader');
logger.debug('[App] Config loaded, importing dependencies...'); // ADDED LOG

// Import dependencies
const express = require('express');
const favicon = require('serve-favicon');
const { existsSync, readFileSync, mkdirSync, writeFileSync } = require('fs');
const https = require('https');
const { json } = require('body-parser');
const DefaultCert = require('./defaultkeys.json');
const cluster = require('cluster');
const path = require('path');
const os = require('os');
const RateLimit = require('express-rate-limit');


// Import utility functions
const { isArray, CheckRecentVersion, CheckIndexes, ExtractAuthKey, getClientIp } = require('./utils');

// fetch is a Node global (>=18); no polyfill needed. Fail loudly if run on an
// unsupported runtime rather than silently missing HTTP capability.
if (typeof global.fetch !== 'function') {
  throw new Error('global fetch is unavailable - Node.js 18+ is required');
}

// Determine CPU count for clustering
const totalCPUs = Math.max(1, global.config.cpuCount || os.cpus().length);

// Import route handlers
logger.debug('[App] Importing route handlers...');
const RouterItem = require('./controllers/object');
const RouterPlayer = require('./controllers/player');
const RouterGlobals = require('./controllers/global');
const RouterAuth = require('./auth/controller');
const RouterStatus = require('./controllers/status');
const RouterLogger = require('./controllers/logger');
const RouterDiscordConnector = require('./discord/router');
const RouterServerQuery = require('./controllers/serverquery');
const RouterTrueRandom = require('./controllers/trueRandom');
const RouterCrypto = require('./controllers/crypto');
const messagesRouter = require('./controllers/messages');
const AIChatRouter = require('./controllers/aiChat');
const AIAssistantRouter = require('./controllers/aiAssistant');
const AudioRouter = require('./controllers/tts');
const ImageRouter = require('./controllers/images');
const KBRouter = require('./controllers/kb');
const ModSettingsRouter = require('./controllers/modSettings');
const TranslateRouter = require('./controllers/TranslateConnector');

const HOSTNAME_REGEX = /^(?=.{1,253}$)(?!-)[a-z0-9](?:[a-z0-9-]{0,61}[a-z0-9])?(?:\.(?!-)[a-z0-9](?:[a-z0-9-]{0,61}[a-z0-9])?)*$/i;

function ensureDirectory(dirPath) {
  if (!existsSync(dirPath)) {
    mkdirSync(dirPath, { recursive: true });
  }
  return dirPath;
}

function collectValidHostnames(primary, extras = []) {
  const normalized = [];
  const seen = new Set();
  const push = (candidate) => {
    if (typeof candidate !== 'string') {
      return;
    }
    const value = candidate.trim().toLowerCase();
    if (!value) {
      return;
    }
    if (!HOSTNAME_REGEX.test(value)) {
      logger.warn('[WebServer] Ignoring invalid LetsEncrypt hostname', { host: candidate, reason: 'Fails RFC-1123 validation' });
      return;
    }
    if (!seen.has(value)) {
      seen.add(value);
      normalized.push(value);
    } else {
      logger.debug('[WebServer] Ignoring duplicate LetsEncrypt hostname', { host: candidate });
    }
  };

  push(primary);
  if (Array.isArray(extras)) {
    extras.forEach(push);
  }

  return normalized;
}

/**
 * Configure rate limiting for API protection
 */
const limiter = RateLimit({
  windowMs: 10 * 1000, // 10 seconds window
  limit: global.config.RequestLimit || 500, // Max requests per window
  message: '{ "Status": "Error", "Error": "RateLimited" }',
  standardHeaders: 'draft-7', // Use recommended draft-7 standard headers
  keyGenerator: (req) => getClientIp(req),
  handler: (request, response, next, options) => {
    if (request.rateLimit.used === request.rateLimit.limit + 1) {
      logger.warn('[WebServer] RateLimit reached - possible DDoS attack or need to increase request limit', { ip: getClientIp(request) });
    }
    response.status(options.statusCode).send(options.message);
  },
  skip: (req) => {
    // Skip rate limiting for whitelisted IPs
    const ip = getClientIp(req);
    const whitelist = global.config.RateLimitWhiteList;
    return ip && whitelist && isArray(whitelist) && whitelist.includes(ip);
  }
});

/**
 * Initialize and configure the Express application
 * @returns {Express} Configured Express application
 */
function createExpressApp() {
  const app = express();

  // Apply middleware
  app.use(limiter);
  app.use(ExtractAuthKey);

  // Configure JSON parser with extended size limit
  app.use((req, res, next) => {
    json({
      limit: global.config.MaxBodySize || '32mb'
    })(req, res, (err) => {
      if (err) {
        logger.error('[WebServer] Bad Request', { url: req.url, error: err.message });
        res.status(400).json({ Status: "error", Error: `Bad Request ${err}` });
        return;
      }
      next();
    });
  });

  const faviconFile = path.join(global.SAVEPATH, 'templates', 'favicon.ico');
  const defaultFavicon = path.join(__dirname, 'public', 'favicon.ico');
  const faviconPath = existsSync(faviconFile) ? faviconFile : defaultFavicon;
  app.use(favicon(faviconPath));

  // Register route handlers
  app.use('/Object', RouterItem);
  app.use('/Player', RouterPlayer);
  app.use('/Globals', RouterGlobals);
  app.use('/GetAuth', RouterAuth);
  app.use('/Status', RouterStatus);
  app.use('/Logger', RouterLogger);
  app.use('/Discord', RouterDiscordConnector);
  app.use('/ServerQuery', RouterServerQuery);
  app.use('/Random', RouterTrueRandom);
  app.use('/Crypto', RouterCrypto);
  app.use('/Messages', messagesRouter);
  app.use('/AI/Chat', AIChatRouter);
  app.use('/AI/Assistant', AIAssistantRouter);
  app.use('/TTS', AudioRouter);
  app.use('/Images', ImageRouter);
  app.use('/KB', KBRouter);
  app.use('/ModSettings', ModSettingsRouter);
  app.use('/Translate', TranslateRouter);
  
  const iconFile = path.join(global.SAVEPATH, 'templates', 'icon.png');
  const defaultIcon = path.join(__dirname, 'public', 'icon.png');
  const iconPath = existsSync(iconFile) ? iconFile : defaultIcon;
  app.get('/icon.png', (req, res) => {
    logger.debug(`[WebServer] Icon requested sending ${iconPath}`, { url: req.url, ip: req.ip });
    res.sendFile(iconPath);
  });
  const iconsvgFile = path.join(global.SAVEPATH, 'templates', 'icon.svg');
  const defaultsvgIcon = path.join(__dirname, 'public', 'icon.svg');
  const iconsvgPath = existsSync(iconsvgFile) ? iconsvgFile : defaultsvgIcon;
  app.get('/icon.svg', (req, res) => {
    logger.debug(`[WebServer] Icon requested sending ${iconsvgPath}`, { url: req.url, ip: req.ip });
    res.sendFile(iconsvgPath);
  });
  
  // Handle invalid routes
  app.use('/', (req, res) => {
    if (req.url !== '/') {
      logger.debug(`[WebServer] Invalid URL requested ${req.url}`, { url: req.url, ip: req.ip });
    }
    res.status(501).json({ Status: "Error", Error: "Requested bad URL" });
  });
  
  return app;
}

/**
 * Ensures a per-install self-signed certificate exists, generating one on first
 * run and persisting it under the data directory. This replaces the shared
 * bundled defaultkeys.json as the fallback so no two installs share a private
 * key. Returns { key, cert } PEM strings.
 *
 * @throws if generation fails (e.g. the 'selfsigned' package is unavailable),
 *         so the caller can fall back to the bundled keys.
 */
function ensureSelfSignedCertificate() {
  const certDir = ensureDirectory(path.join(path.resolve(global.SAVEPATH || process.cwd()), 'certs'));
  const keyPath = path.join(certDir, 'self-signed-key.pem');
  const certPath = path.join(certDir, 'self-signed-cert.pem');

  if (existsSync(keyPath) && existsSync(certPath)) {
    return { key: readFileSync(keyPath), cert: readFileSync(certPath) };
  }

  // Lazy require so a missing dependency degrades to the bundled fallback
  // rather than crashing startup.
  const selfsigned = require('selfsigned');
  const pems = selfsigned.generate(
    [{ name: 'commonName', value: 'localhost' }],
    {
      keySize: 2048,
      days: 3650,
      algorithm: 'sha256',
      extensions: [{
        name: 'subjectAltName',
        altNames: [
          { type: 2, value: 'localhost' }, // DNS
          { type: 7, ip: '127.0.0.1' }     // IP
        ]
      }]
    }
  );

  // 0o600 so the private key isn't world-readable (no-op on Windows ACLs).
  writeFileSync(keyPath, pems.private, { mode: 0o600 });
  writeFileSync(certPath, pems.cert);
  logger.info('[WebServer] Generated a unique self-signed certificate for this install', { certDir });
  return { key: pems.private, cert: pems.cert };
}

/**
 * Load SSL certificates for HTTPS.
 * Priority: operator-provided cert/key files -> per-install self-signed cert
 * (generated on first run) -> bundled defaultkeys.json (last-ditch fallback).
 * @returns {Object} Object containing key and cert for HTTPS server
 */
function loadCertificates() {
  // 1. Operator-provided certificate files take priority. (Mapping preserved
  //    from the original implementation for backward compatibility.)
  if (global.config.Certificate != "" && global.config.CertificateKey != ""){
    if (existsSync(global.config.Certificate) && existsSync(global.config.CertificateKey)){
      return {
        key: readFileSync(global.config.Certificate),
        cert: readFileSync(global.config.CertificateKey)
      };
    }
  }

  // 2. Per-install self-signed certificate (unique private key per install).
  try {
    return ensureSelfSignedCertificate();
  } catch (err) {
    // 3. Last-ditch fallback to the shared bundled keys so the server still
    //    starts (e.g. before `npm install` pulls in the selfsigned package).
    logger.warn('[WebServer] Self-signed certificate unavailable, using bundled fallback keys', { error: err.message });
    return { key: DefaultCert.Key, cert: DefaultCert.Cert };
  }
}

/**
 * Start the web server with appropriate SSL configuration
 */
function startWebServer() {
  const webapp = createExpressApp();
  const port = process.env.PORT || global.config.Port || 8443;
  const ip = global.config.IP || "0.0.0.0";
  const packageAgent = `${packageMetadata.name || 'UniversalFrameworkService'}/${global.APIVERSION || packageMetadata.version || '0.0.0'}`;

  // Check if Let's Encrypt is enabled
  const letsEncrypt = global.config.LetsEncypt;
  const letsEncryptHosts = collectValidHostnames(letsEncrypt?.Domain, letsEncrypt?.AltNames);
  const resolvedDataRoot = ensureDirectory(path.resolve(global.SAVEPATH || process.cwd()));
  const greenlockRootDir = ensureDirectory(path.join(resolvedDataRoot, 'greenlock'));
  const greenlockDir = ensureDirectory(path.join(greenlockRootDir, 'greenlock.d'));
  const greenlockPackageJsonPath = path.join(greenlockRootDir, 'package.json');
  if (!existsSync(greenlockPackageJsonPath)) {
    try {
      const minimalPackageJson = {
        name: packageMetadata.name || 'universal-framework-service',
        version: packageMetadata.version || '0.0.0'
      };
      writeFileSync(greenlockPackageJsonPath, JSON.stringify(minimalPackageJson, null, 2));
      logger.info('[WebServer] Created Greenlock package metadata', { path: greenlockPackageJsonPath });
    } catch (err) {
      logger.warn('[WebServer] Failed to write Greenlock package metadata', { error: err.message });
    }
  } else {
    logger.debug('[WebServer] Greenlock package metadata exists', { path: greenlockPackageJsonPath });
  }
  if (letsEncrypt?.Enabled === true && letsEncrypt?.Email && letsEncryptHosts.length > 0) {
    // Let's Encrypt SSL setup
    logger.info('[WebServer] Initializing LetsEncrypt', {
      packageRoot: greenlockRootDir,
      configDir: greenlockDir,
      email: letsEncrypt.Email,
      domains: letsEncryptHosts
    });

    // Check for existing ACME accounts (accounts live inside greenlock.d/, not the root)
    const accountsDir = path.join(greenlockDir, 'accounts');
    if (!existsSync(accountsDir)) {
      logger.warn('[WebServer] No existing ACME accounts found. A new Let\'s Encrypt account will be registered on first certificate order.');
    }

    // Seed greenlock config.json with proper site entries, defaults, and challenge config.
    // Without this, greenlock does not know which domains to manage and the ACME ordering
    // flow can hit undefined-challenges edge cases in @root/acme.
    const greenlockConfigPath = path.join(greenlockDir, 'config.json');
    try {
      let glConfig;
      try {
        glConfig = JSON.parse(readFileSync(greenlockConfigPath, 'utf8'));
      } catch (_e) {
        glConfig = {};
      }

      // Ensure defaults
      if (!glConfig.defaults) glConfig.defaults = {};
      glConfig.defaults.subscriberEmail = letsEncrypt.Email;
      glConfig.defaults.agreeToTerms = true;
      if (!glConfig.defaults.store) {
        glConfig.defaults.store = { module: 'greenlock-store-fs' };
      }
      if (!glConfig.defaults.challenges) {
        glConfig.defaults.challenges = {
          'http-01': { module: 'acme-http-01-standalone' }
        };
      }

      // Ensure sites (greenlock-manager-fs accepts both object and array formats)
      if (!glConfig.sites) glConfig.sites = {};
      if (Array.isArray(glConfig.sites)) {
        const sitesObj = {};
        glConfig.sites.forEach(s => { if (s.subject) sitesObj[s.subject] = s; });
        glConfig.sites = sitesObj;
      }

      const subject = letsEncryptHosts[0];
      if (!glConfig.sites[subject]) {
        glConfig.sites[subject] = {
          subject: subject,
          altnames: letsEncryptHosts.slice(0),
          renewAt: 1
        };
        logger.info('[WebServer] Adding site to Greenlock config', { subject, altnames: letsEncryptHosts });
      } else {
        // Update altnames in case the config changed
        glConfig.sites[subject].altnames = letsEncryptHosts.slice(0);
      }

      // Remove stale sites that are no longer in the user's LetsEncrypt config
      const letsEncryptSubjects = new Set(letsEncryptHosts);
      Object.keys(glConfig.sites).forEach(key => {
        if (!letsEncryptSubjects.has(key)) {
          logger.info('[WebServer] Removing stale site from Greenlock config', { subject: key });
          delete glConfig.sites[key];
        }
      });

      writeFileSync(greenlockConfigPath, JSON.stringify(glConfig, null, 2));
      logger.debug('[WebServer] Greenlock config.json written', { path: greenlockConfigPath });
    } catch (err) {
      logger.warn('[WebServer] Failed to seed Greenlock config', { error: err.message });
    }

    require("greenlock-express").init({
      packageRoot: greenlockRootDir,
      packageAgent,
      configDir: greenlockDir,
      subscriberEmail: letsEncrypt.Email,
      agreeToTerms: true,
      notify: function(type, object) {
        const details = typeof object === 'object' && object !== null ? object : { message: object };
        
        // Suppress expected errors caused by local health checks (Electron Tray) polling 'localhost'
        if (type === 'error' && details.code === 'INVALID_HOSTNAME' && (details.message || '').includes("'localhost'")) {
          logger.debug("[WebServer] Suppressed Greenlock error for localhost (local health check)", { code: details.code });
          return;
        }

        if (type === 'error') {
          logger.warn("[WebServer] Let's Encrypt error", {
            code: details.code,
            context: details.context,
            message: details.message || details.detail || details.reason,
            stack: details.stack,
            raw: details
          });
        } else {
          logger.info(`[WebServer] Let's Encrypt event: ${type}`, { details });
        }
      },
      maintainerEmail: letsEncrypt.Email,
      cluster: false
    }).ready(setupGreenlockServer);

    function setupGreenlockServer(glx) {
      // Load fallback certificates for localhost/non-configured hostnames
      const fallbackCerts = loadCertificates();
      const letsEncryptHostsSet = new Set(letsEncryptHosts.map(h => h.toLowerCase()));

      // Greenlock's httpsServer() mutates the secureOpts we pass in, installing its
      // own SNICallback. By keeping a reference to the opts object, we can grab that
      // callback after the call and wrap it with our localhost fallback logic.
      const secureOpts = {
        key: fallbackCerts.key,
        cert: fallbackCerts.cert
      };

      const greenlockHttpsServer = glx.httpsServer(secureOpts, webapp);

      // Greenlock's wrapDefaultSniCallback() sets secureOpts.SNICallback in-place
      const greenlockSNI = secureOpts.SNICallback || null;

      if (greenlockSNI) {
        const tls = require('tls');
        const fallbackCtx = tls.createSecureContext({
          key: fallbackCerts.key,
          cert: fallbackCerts.cert
        });

        // Replace the SNI callback on the actual server's secure context
        // so localhost/non-LE hostnames get the fallback cert
        greenlockHttpsServer.setSecureContext({
          SNICallback: (servername, cb) => {
            const hostname = (servername || '').toLowerCase();
            if (hostname === 'localhost' || hostname === '127.0.0.1' || !letsEncryptHostsSet.has(hostname)) {
              cb(null, fallbackCtx);
            } else {
              greenlockSNI(servername, cb);
            }
          },
          key: fallbackCerts.key,
          cert: fallbackCerts.cert
        });
      }

      greenlockHttpsServer.listen(port, ip, function() {
        logger.info("[WebServer] Server started", { 
          address: greenlockHttpsServer.address().address, 
          port: greenlockHttpsServer.address().port, 
          ssl: "Let's Encrypt" 
        });
      });
      
      greenlockHttpsServer.on('error', function(e) {
        logger.error("[WebServer] HTTPS server error", { error: e.message, stack: e.stack });
      });

      // Also listen on port 80 for ACME challenges
      const httpServer = glx.httpServer(function(req, res) {
        res.statusCode = 301;
        res.setHeader("Location", "https://" + req.headers.host + req.url);
        res.end("Insecure connections are not allowed. Redirecting...");
      });

      httpServer.listen(80, ip, function() {
        logger.info("[WebServer] HTTP redirect server started", { 
          address: httpServer.address().address, 
          port: 80, 
          purpose: "Let's Encrypt ACME challenges" 
        });
      });
      
      httpServer.on('error', function(e) {
        logger.error("[WebServer] HTTP server error", { error: e.message, stack: e.stack });
      });
    }
  } else {
    const certificates = loadCertificates();
    logger.info('[WebServer] Starting HTTPS server with bundled certificates', { port, address: ip });
    const server = https.createServer(certificates, webapp)
      .listen(port, function() {
        logger.info("[App] API Webservice started", { port, address: ip });
      });
    
    server.on('error', function(e) {
      logger.error("[WebServer] Server error", { error: e.message, stack: e.stack });
    });
  }
}

/**
 * Main application startup function
 * @param {boolean} isElectron - Whether the app is running in Electron environment
 */
function Start(isElectron = false) {
  logger.debug('[App] Start() called');
  logger.info(`[App] Starting Universal Framework Service v${global.APIVERSION}`, { 
    savePath: global.SAVEPATH,  
    isElectron,
    nodeVersion: process.version,
    platform: process.platform
  });

  // Setup clustering if enabled and not in Electron
  if (cluster.isMaster && totalCPUs > 1 && !isElectron) {
    logger.info("[App] Starting server in cluster mode", { workers: totalCPUs });
    
    // Fork worker processes
    for (let i = 0; i < totalCPUs; i++) {
      cluster.fork();
    }
    
    // Restart worker if it crashes
    cluster.on('exit', (worker, code, signal) => {
      logger.warn("[App] Worker died, restarting", { 
        workerId: worker.id, 
        exitCode: code, 
        signal 
      });
      cluster.fork();
    });
    
    // Check for updates and DB indexes in master process
    if (global.config?.CheckForNewVersion) {
      CheckRecentVersion();
    }
    // Master handles one-time DB index + KB setup
    setTimeout(CheckIndexes, 1000);
    const { ensureAllKBIndexes: ensureKB, ensureAllEmbeddings: ensureEmb } = require('./models/kb');
    setTimeout(ensureKB, 2000);
    const { ensureModSettingsIndexes } = require('./models/modSettings');
    setTimeout(ensureModSettingsIndexes, 2500);
    setTimeout(async () => {
      try {
        const kbController = require('./controllers/kb');
        if (kbController.generateEmbeddings) {
          await ensureEmb(kbController.generateEmbeddings);
        }
      } catch (err) {
        (global.logger || console).warn('[KB] Could not run embedding check on startup', { error: err.message });
      }
    }, 5000);
  } else {
    // Single process mode (worker or single-CPU)
    startWebServer();
    
    if (totalCPUs <= 1) {
      if (global.config?.CheckForNewVersion) {
        CheckRecentVersion();
      }
      // Single-process: handle one-time DB index + KB setup here
      setTimeout(CheckIndexes, 1000);
      const { ensureAllKBIndexes, ensureAllEmbeddings } = require('./models/kb');
      setTimeout(ensureAllKBIndexes, 2000);
      const { ensureModSettingsIndexes: ensureModSettingsIdx } = require('./models/modSettings');
      setTimeout(ensureModSettingsIdx, 2500);
      setTimeout(async () => {
        try {
          const kbController = require('./controllers/kb');
          if (kbController.generateEmbeddings) {
            await ensureAllEmbeddings(kbController.generateEmbeddings);
          }
        } catch (err) {
          (global.logger || console).warn('[KB] Could not run embedding check on startup', { error: err.message });
        }
      }, 5000);
    }
  }
}


Start(global.isElectron);