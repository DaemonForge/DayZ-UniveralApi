const { readFileSync, writeFileSync, existsSync, mkdirSync } = require('fs');
const path = require('path');
const { makeAuthToken } = require('./utils');

const Defaultconfig = require('./sample-config.json');
const ConfigPath = "config.json";
const ConfigFilePath = path.join(global.SAVEPATH, ConfigPath);
const HOSTNAME_REGEX = /^(?=.{1,253}$)(?!-)[a-z0-9](?:[a-z0-9-]{0,61}[a-z0-9])?(?:\.(?!-)[a-z0-9](?:[a-z0-9-]{0,61}[a-z0-9])?)*$/i;

// Get the logger from global
const logger = global.logger;

const BASE_DEFAULT_CONFIG = {
  DBServer: "mongodb://localhost:27017",
  DB: "DayZ",
  AllowClientWrite: false,
  IP: "0.0.0.0",
  Port: 443,
  CreateIndexes: true,
  LogToFile: true,
  CheckForNewVersion: true,
  RequestLimit: 500,
  RequestLimitQuery: 400,
  RequestLimitStatus: 100,
  RequestLimitServerQuery: 200,
  RequestLimitTranslate: 200,
  RequestLimitLogger: 500,
  RequestLimitCrypto: 150,
  RateLimitWhiteList: ["127.0.0.1"],
  TrustProxyHeaders: false,
  MaxBodySize: "32mb",
  ServerAuth: [],
  ServerAuthLabels: [],
  Certificate: "",
  CertificateKey: "",
  Discord: {
    Client_Id: "",
    Client_Secret: "",
    Bot_Token: "",
    Guild_Id: "",
    AllowToReRegister: false,
    Restrict_Sign_Up: false,
    Required_Role: "",
    BlackList_Role: "",
    Restrict_Sign_Up_Countries: []
  },
  OpenAIApi: {
    ApiKey: "",
    BaseURL: "",
    DefaultModel: "",
    EmbeddingModel: "",
    enablePromptProtection: true
  },
  Functions: {},
  LetsEncypt: {
    Enabled: false,
    Domain: "",
    Email: "",
    AltNames: []
  },
  Proxy: {
    primaryDomain: "",
    subdomain: "",
    token: "",
    lastRenew: "",
    autoRenew: false
  },
  Tunnel: {
    enabled: false,
    token: "",
    autoStart: true
  }
};

function ensureDirectory() {
  const dir = path.dirname(ConfigFilePath);
  if (!existsSync(dir)) {
    mkdirSync(dir, { recursive: true });
  }
}

function ensureArray(value, fallback = []) {
  return Array.isArray(value) ? [...value] : [...fallback];
}

function ensureObject(value, fallback = {}) {
  return value && typeof value === 'object' && !Array.isArray(value) ? { ...value } : { ...fallback };
}

function collectValidHostnames(primary, extras = []) {
  const values = [];
  const seen = new Set();
  const pushCandidate = (candidate) => {
    if (typeof candidate !== 'string') {
      if (candidate !== undefined && candidate !== null) {
        logger.warn('Ignoring non-string LetsEncrypt hostname', { value: candidate });
      }
      return;
    }
    const normalized = candidate.trim().toLowerCase();
    if (!normalized) {
      return;
    }
    if (!HOSTNAME_REGEX.test(normalized)) {
      logger.warn('Ignoring invalid LetsEncrypt hostname (fails RFC-1123 check)', { value: candidate });
      return;
    }
    if (!seen.has(normalized)) {
      seen.add(normalized);
      values.push(normalized);
    }
  };

  pushCandidate(primary);
  if (Array.isArray(extras)) {
    extras.forEach(pushCandidate);
  }

  return values;
}

function writeConfigFile(data) {
  ensureDirectory();
  writeFileSync(ConfigFilePath, JSON.stringify(data, undefined, 4));
}

function normalizeConfig(raw, { ensureAuthToken = false } = {}) {
  const result = {
    ...BASE_DEFAULT_CONFIG,
    ...(raw && typeof raw === 'object' ? raw : {})
  };

  result.DBServer = result.DBServer || BASE_DEFAULT_CONFIG.DBServer;
  result.DB = result.DB || BASE_DEFAULT_CONFIG.DB;
  result.IP = result.IP || BASE_DEFAULT_CONFIG.IP;
  result.Port = Number(result.Port) || BASE_DEFAULT_CONFIG.Port;
  result.AllowClientWrite = Boolean(result.AllowClientWrite);
  result.CreateIndexes = result.CreateIndexes !== false;
  result.LogToFile = result.LogToFile !== false;
  result.CheckForNewVersion = result.CheckForNewVersion !== false;

  result.RequestLimit = Number(result.RequestLimit) || BASE_DEFAULT_CONFIG.RequestLimit;
  result.RequestLimitQuery = Number(result.RequestLimitQuery) || BASE_DEFAULT_CONFIG.RequestLimitQuery;
  result.RequestLimitStatus = Number(result.RequestLimitStatus) || BASE_DEFAULT_CONFIG.RequestLimitStatus;
  result.RequestLimitServerQuery = Number(result.RequestLimitServerQuery) || BASE_DEFAULT_CONFIG.RequestLimitServerQuery;
  result.RequestLimitTranslate = Number(result.RequestLimitTranslate) || BASE_DEFAULT_CONFIG.RequestLimitTranslate;
  result.RequestLimitLogger = Number(result.RequestLimitLogger) || BASE_DEFAULT_CONFIG.RequestLimitLogger;
  result.RequestLimitCrypto = Number(result.RequestLimitCrypto) || BASE_DEFAULT_CONFIG.RequestLimitCrypto;

  result.RateLimitWhiteList = ensureArray(result.RateLimitWhiteList, BASE_DEFAULT_CONFIG.RateLimitWhiteList).map(String);
  result.TrustProxyHeaders = Boolean(result.TrustProxyHeaders);
  result.MaxBodySize = (typeof result.MaxBodySize === 'string' && result.MaxBodySize.trim())
    ? result.MaxBodySize.trim()
    : BASE_DEFAULT_CONFIG.MaxBodySize;

  result.ServerAuth = ensureArray(result.ServerAuth);
  if (ensureAuthToken) {
    const sampleToken = Array.isArray(Defaultconfig.ServerAuth) ? Defaultconfig.ServerAuth[0] : null;
    if (result.ServerAuth.length === 0) {
      result.ServerAuth.push(makeAuthToken());
    } else if (result.ServerAuth.length === 1 && sampleToken && result.ServerAuth[0] === sampleToken) {
      result.ServerAuth[0] = makeAuthToken();
    }
  }
  result.ServerAuth = result.ServerAuth.map(String);

  result.ServerAuthLabels = ensureArray(result.ServerAuthLabels);
  while (result.ServerAuthLabels.length < result.ServerAuth.length) {
    result.ServerAuthLabels.push("");
  }
  if (result.ServerAuthLabels.length > result.ServerAuth.length) {
    result.ServerAuthLabels = result.ServerAuthLabels.slice(0, result.ServerAuth.length);
  }

  result.Discord = {
    ...BASE_DEFAULT_CONFIG.Discord,
    ...ensureObject(result.Discord)
  };
  result.Discord.Restrict_Sign_Up_Countries = ensureArray(result.Discord.Restrict_Sign_Up_Countries);

  result.OpenAIApi = {
    ...BASE_DEFAULT_CONFIG.OpenAIApi,
    ...ensureObject(result.OpenAIApi)
  };

  result.Functions = ensureObject(result.Functions, {});

  result.LetsEncypt = {
    ...BASE_DEFAULT_CONFIG.LetsEncypt,
    ...ensureObject(result.LetsEncypt)
  };
  result.LetsEncypt.AltNames = ensureArray(result.LetsEncypt.AltNames);

  result.Proxy = {
    ...BASE_DEFAULT_CONFIG.Proxy,
    ...ensureObject(result.Proxy)
  };
  result.Proxy.autoRenew = Boolean(result.Proxy.autoRenew);

  result.Tunnel = {
    ...BASE_DEFAULT_CONFIG.Tunnel,
    ...ensureObject(result.Tunnel)
  };
  result.Tunnel.enabled = Boolean(result.Tunnel.enabled);
  result.Tunnel.autoStart = result.Tunnel.autoStart !== false;

  return result;
}

// Helper function to safely update config and write to file
function updateConfig(updates) {
  Object.assign(config, updates);
  try {
    writeConfigFile(config);
  } catch (e) {
    logger.error('Failed to update config', { error: e.message });
  }
}

// Load or create config
let config;
try {
  ensureDirectory();
  const rawConfig = JSON.parse(readFileSync(ConfigFilePath));
  config = normalizeConfig(rawConfig, { ensureAuthToken: true });
  const rawString = JSON.stringify(rawConfig, undefined, 4);
  const normalizedString = JSON.stringify(config, undefined, 4);
  if (rawString !== normalizedString) {
    writeConfigFile(config);
  }
} catch (err) {
  // Set up default config with generated auth token
  const mergedDefaults = normalizeConfig(Defaultconfig, { ensureAuthToken: true });
  // Ensure at least one auth label exists
  if (mergedDefaults.ServerAuthLabels.length === 0) {
    mergedDefaults.ServerAuthLabels = new Array(mergedDefaults.ServerAuth.length).fill("");
  }
  config = mergedDefaults;
  writeConfigFile(config);
  
  logger.info(`Installing for the first time the default config "${ConfigPath}" was created with the following values`);
  ['DBServer', 'DB', 'ServerAuth', 'AllowClientWrite', 'Port'].forEach(key => {
    logger.info(`   ${key}: ${config[key]}`);
  });
}

// Handle LetsEncrypt configuration
if (config.LetsEncypt.Enabled === true) {
  const validHosts = collectValidHostnames(config.LetsEncypt.Domain, config.LetsEncypt.AltNames);

  if (validHosts.length === 0) {
    logger.error('LetsEncrypt disabled: no valid hostnames provided. Update config.json -> LetsEncypt.Domain/AltNames with publicly reachable domains.');
    config.LetsEncypt.Enabled = false;
    updateConfig({ LetsEncypt: config.LetsEncypt });
  } else {
    const primaryHost = validHosts[0];
    const additionalHosts = validHosts.slice(1);
    config.LetsEncypt.Domain = primaryHost;
    config.LetsEncypt.AltNames = additionalHosts;
    logger.info('LetsEncrypt hostnames sanitized', { primary: primaryHost, altNames: additionalHosts });

    try {
      const greenlockRoot = path.join(global.SAVEPATH, 'greenlock');
      const configDir = path.join(greenlockRoot, 'greenlock.d');
      if (!existsSync(greenlockRoot)) {
        mkdirSync(greenlockRoot, { recursive: true });
      }
      if (!existsSync(configDir)) {
        mkdirSync(configDir, { recursive: true });
      }
      const leConfig = {
        defaults: {
          store: {
            module: 'greenlock-store-fs'
          },
          challenges: {
            "http-01": {
              module: "acme-http-01-standalone"
            }
          },
          renewOffset: "-45d",
          renewStagger: "3d",
          accountKeyType: "EC-P256",
          serverKeyType: "RSA-2048",
          subscriberEmail: config.LetsEncypt.Email
        },
        sites: [{
          subject: primaryHost,
          altnames: [primaryHost, ...additionalHosts]
        }]
      };
      writeFileSync(path.join(configDir, 'config.json'), JSON.stringify(leConfig, undefined, 4));
    } catch (err) {
      logger.error('Unable to write LetsEncrypt config', { error: err.message });
    }

    updateConfig({ LetsEncypt: config.LetsEncypt });
  }
}

module.exports = config;
