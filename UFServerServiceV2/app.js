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

// Load configuration
global.config = require('./configLoader');

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
const nodeFetchModule = require('node-fetch');
const RateLimit = require('express-rate-limit');


// Import utility functions
const { isArray, CheckRecentVersion, CheckIndexes, ExtractAuthKey } = require('./utils');

// Resolve node-fetch CommonJS/ESM default and set global fetch if needed
const resolvedFetch = (nodeFetchModule && typeof nodeFetchModule === 'object' && 'default' in nodeFetchModule)
  ? nodeFetchModule.default
  : nodeFetchModule;

if (typeof global.fetch !== 'function') {
  global.fetch = resolvedFetch;
}

// Determine CPU count for clustering
const totalCPUs = Math.max(1, global.config.cpuCount || os.cpus().length);

// Import route handlers
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
  max: global.config.RequestLimit || 500, // Max requests per window
  message: '{ "Status": "Error", "Error": "RateLimited" }',
  standardHeaders: 'draft-7', // Use recommended draft-7 standard headers
  keyGenerator: (req) => {
    // Identify clients by IP, checking various header options
    return req.headers['CF-Connecting-IP'] || 
           req.headers['x-forwarded-for'] || 
           req.socket.remoteAddress || 
           req.ip;
  },
  handler: (request, response, next, options) => {
    if (request.rateLimit.used === request.rateLimit.limit + 1) {
      // Original onLimitReached code
      const ip = request.headers['CF-Connecting-IP'] || 
                 request.headers['x-forwarded-for'] || 
                 request.socket.remoteAddress || 
                 request.ip;
      logger.warn('[WebServer] RateLimit reached - possible DDoS attack or need to increase request limit', { ip });
    }
    response.status(options.statusCode).send(options.message);
  },
  skip: (req) => {
    // Skip rate limiting for whitelisted IPs
    const ip = req.headers['CF-Connecting-IP'] || 
               req.headers['x-forwarded-for'] || 
               req.socket.remoteAddress || 
               req.ip;
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
      limit: '64mb'
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
 * Load SSL certificates for HTTPS
 * @returns {Object} Object containing key and cert for HTTPS server
 */
function loadCertificates() {
  let serverKey = DefaultCert.Key;
  let serverCert = DefaultCert.Cert;
  
  if (global.config.Certificate != "" && global.config.CertificateKey != ""){
    if (existsSync(global.config.Certificate) && existsSync(global.config.CertificateKey)){
      serverKey = readFileSync(global.config.Certificate);
      serverCert = readFileSync(global.config.CertificateKey);
    }
  }
  return { key: serverKey, cert: serverCert };
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

    // Check for existing accounts to warn about rate limits if missing
    const accountsDir = path.join(greenlockRootDir, 'accounts');
    if (!existsSync(accountsDir)) {
      logger.warn('[WebServer] Greenlock accounts directory missing. A new Let\'s Encrypt account will be registered. Warning: Frequent deletions may hit rate limits.');
    }

    require("greenlock-express").init({
      packageRoot: greenlockRootDir,
      packageAgent,
      configDir: greenlockDir,
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
            raw: details
          });
        } else {
          logger.info(`[WebServer] Let's Encrypt event: ${type}`, { details });
        }
      },
      maintainerEmail: letsEncrypt.Email,
      cluster: false
    }).ready(setupGreenlockServer);

    // Load fallback certificates for localhost/non-configured hostnames
    const fallbackCerts = loadCertificates();
    const letsEncryptHostsSet = new Set(letsEncryptHosts.map(h => h.toLowerCase()));

    function setupGreenlockServer(glx) {
      // Create SNI callback that uses Let's Encrypt certs for configured domains
      // and falls back to self-signed cert for localhost/other hostnames
      const greenlockSNI = glx.httpsServer().listeners('secureConnection')[0];
      
      // Setup HTTPS server with SNI callback for fallback support
      const tlsOptions = {
        SNICallback: (servername, cb) => {
          const hostname = (servername || '').toLowerCase();
          // For localhost or non-configured hostnames, use fallback cert
          if (hostname === 'localhost' || hostname === '127.0.0.1' || !letsEncryptHostsSet.has(hostname)) {
            //logger.debug('[WebServer] Using fallback certificate', { hostname });
            const tls = require('tls');
            const ctx = tls.createSecureContext({
              key: fallbackCerts.key,
              cert: fallbackCerts.cert
            });
            cb(null, ctx);
          } else {
            // Use greenlock's SNI for configured Let's Encrypt domains
            glx.tlsOptions.SNICallback(servername, cb);
          }
        },
        key: fallbackCerts.key,
        cert: fallbackCerts.cert
      };

      const httpsServer = https.createServer(tlsOptions, webapp);
      
      httpsServer.listen(port, ip, function() {
        logger.info("[WebServer] Server started", { 
          address: httpsServer.address().address, 
          port: httpsServer.address().port, 
          ssl: "Let's Encrypt" 
        });
      });
      
      httpsServer.on('error', function(e) {
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
  } else {
    // Single process mode
    startWebServer();
    
    if (totalCPUs <= 1) {
      if (global.config?.CheckForNewVersion) {
        CheckRecentVersion();
      }
    }
  }
  setTimeout(CheckIndexes, 1000);
  
  // Ensure KB indexes are created for all existing KBs
  const { ensureAllKBIndexes, ensureAllEmbeddings } = require('./models/kb');
  setTimeout(ensureAllKBIndexes, 2000);
  
  // Ensure all KB documents have embeddings (runs after indexes are created)
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


Start(global.isElectron);