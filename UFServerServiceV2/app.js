/**
 * DayZ Universal API Server - Main Application
 * This service provides a RESTful API for DayZ server management and data exchange
 */

// Set global constants
if (global.APIVERSION === undefined) {
  global.APIVERSION = process.env.npm_package_version || require('./package.json').version;
}
global.STABLEVERSION = '0.0.0';
global.NEWVERSIONDOWNLOAD = 'https://github.com/daemonforge/DayZ-UniveralApi/releases';
if (global.SAVEPATH === undefined) {
  global.SAVEPATH = './';
}

// Load configuration
global.config = require('./configLoader');

// Import dependencies
const express = require('express');
const favicon = require('serve-favicon');
const { existsSync, readFileSync } = require('fs');
const https = require('https');
const { json } = require('body-parser');
const DefaultCert = require('./defaultkeys.json');
const cluster = require('cluster');
const path = require('path');
const os = require('os');
const nodeFetch = require('node-fetch');
const RateLimit = require('express-rate-limit');

// Initialize logger
const logger = require('./log').initializeLogger();
global.logger = logger;

// Import utility functions
const { isArray, CheckRecentVersion, CheckIndexes, ExtractAuthKey } = require('./utils');

// Set global fetch for use throughout the application
global.fetch = nodeFetch;

// Determine CPU count for clustering
const totalCPUs = Math.max(1, global.config.cpuCount || os.cpus().length);

// Import route handlers
const RouterItem = require('./controllers/object');
const RouterPlayer = require('./controllers/player');
const RouterGlobals = require('./controllers/global');
const RouterAuth = require('./auth/controller');
const RouterStatus = require('./controllers/Status');
const RouterLogger = require('./controllers/logger');
const RouterDiscordConnector = require('./discord/router');
const RouterServerQuery = require('./controllers/serverquery');
const RouterTrueRandom = require('./controllers/TrueRandom');
const RouterCrypto = require('./controllers/crypto');
const messagesRouter = require('./controllers/messages');
const AIChatRouter = require('./controllers/aiChat');

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
      logger.warn('RateLimit reached - possible DDoS attack or need to increase request limit', { ip });
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
        logger.error('Bad Request', { url: req.url, error: err.message });
        res.status(400).json({ Status: "error", Error: `Bad Request ${err}` });
        return;
      }
      next();
    });
  });
  
  // Serve favicon
  app.use(favicon(path.join(__dirname, 'public', 'favicon.ico')));
  
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
  
  // Handle invalid routes
  app.use('/', (req, res) => {
    if (req.url !== '/') {
      logger.warn('Invalid URL requested', { url: req.url, ip: req.ip });
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

  // Check if Let's Encrypt is enabled
  const letsEncrypt = global.config.LetsEncypt;
  if (letsEncrypt?.Enabled === true && letsEncrypt?.Email) {
    // Let's Encrypt SSL setup
    require("greenlock-express").init({
      packageRoot: path.resolve('./'),
      configDir: `${global.SAVEPATH}/greenlock.d`,
      notify: function(type, object) {
        if (type === 'error') {
          logger.warn("Let's Encrypt error", { error: JSON.stringify(object) });
        }
      },
      maintainerEmail: letsEncrypt.Email,
      cluster: false
    }).ready(setupGreenlockServer);

    function setupGreenlockServer(glx) {
      // Setup HTTPS server with Let's Encrypt
      const httpsServer = glx.httpsServer(null, webapp);
      
      httpsServer.listen(port, ip, function() {
        logger.info("Server started", { 
          address: httpsServer.address().address, 
          port: httpsServer.address().port, 
          ssl: "Let's Encrypt" 
        });
      });
      
      httpsServer.on('error', function(e) {
        logger.error("HTTPS server error", { error: e.message, stack: e.stack });
      });

      // Also listen on port 80 for ACME challenges
      const httpServer = glx.httpServer(function(req, res) {
        res.statusCode = 301;
        res.setHeader("Location", "https://" + req.headers.host + req.path);
        res.end("Insecure connections are not allowed. Redirecting...");
      });

      httpServer.listen(80, ip, function() {
        logger.info("HTTP redirect server started", { 
          address: httpServer.address().address, 
          port: 80, 
          purpose: "Let's Encrypt ACME challenges" 
        });
      });
      
      httpServer.on('error', function(e) {
        logger.error("HTTP server error", { error: e.message, stack: e.stack });
      });
    }
  } else {
    // Standard SSL setup
    const certificates = loadCertificates();
    const server = https.createServer(certificates, webapp)
      .listen(port, function() {
        logger.info("API Webservice started", { port, address: ip });
      });
      
    server.on('error', function(e) {
      logger.error("Server error", { error: e.message, stack: e.stack });
    });
  }
}

/**
 * Main application startup function
 * @param {boolean} isElectron - Whether the app is running in Electron environment
 */
function Start(isElectron = false) {
  // Setup clustering if enabled and not in Electron
  if (cluster.isMaster && totalCPUs > 1 && !isElectron) {
    logger.info("Starting server in cluster mode", { workers: totalCPUs });
    
    // Fork worker processes
    for (let i = 0; i < totalCPUs; i++) {
      cluster.fork();
    }
    
    // Restart worker if it crashes
    cluster.on('exit', (worker, code, signal) => {
      logger.warn("Worker died, restarting", { 
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
    setTimeout(CheckIndexes, 1000);
  } else {
    // Single process mode
    startWebServer();
    
    if (totalCPUs <= 1) {
      if (global.config?.CheckForNewVersion) {
        CheckRecentVersion();
      }
      setTimeout(CheckIndexes, 1000);
    }
  }
}

// Export the start function for potential use as a module
module.exports = Start;

// Auto-start the server if not imported as a module
if (require.main === module) {
  Start();
}