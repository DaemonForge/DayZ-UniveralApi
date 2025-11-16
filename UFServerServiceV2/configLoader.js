const { readFileSync, writeFileSync, existsSync, mkdirSync } = require('fs');
const path = require('path');
const { makeAuthToken } = require('./utils');

const Defaultconfig = require('./sample-config.json');
const ConfigPath = "config.json";
const ConfigFilePath = path.join(global.SAVEPATH, ConfigPath);

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
    enablePromptProtection: true
  },
  Functions: {},
  LetsEncypt: {
    Enabled: false,
    Domain: "localhost.localhost",
    Email: "myemail@email.com",
    AltNames: []
  },
  Proxy: {
    primaryDomain: "",
    subdomain: "",
    token: "",
    lastRenew: "",
    autoRenew: false
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
  const domain = config.LetsEncypt.Domain;
  
  // Ensure domain is in AltNames
  if (!config.LetsEncypt.AltNames.includes(domain)) {
    config.LetsEncypt.AltNames.push(domain);
  }
  
  const LEconfigjson = {
    sites: [{
      subject: domain,
      altnames: config.LetsEncypt.AltNames
    }]
  };
  
  try {
    const path = global.SAVEPATH + "greenlock.d";
    const configPath = `${path}/config.json`;
    
    // Create directory if it doesn't exist
    if (!existsSync(path)) {
      mkdirSync(path);
    }
    
    let shouldWrite = true;
    if (existsSync(configPath)) {
      const file = JSON.parse(readFileSync(configPath));
      const currentSite = file.sites[0];
      const newSite = LEconfigjson.sites[0];
      
      // Only write if configuration changed
      shouldWrite = currentSite.subject !== newSite.subject || 
                    JSON.stringify(currentSite.altnames) !== JSON.stringify(newSite.altnames);
    }
    
    if (shouldWrite) {
      writeFileSync(configPath, JSON.stringify(LEconfigjson, undefined, 4));
    }
  } catch (e) {
    logger.error('LetsEncrypt configuration failed', { 
      error: e.message, 
      domain: domain
    });
    config.LetsEncypt.Enabled = false;
    updateConfig({});
  }
}

module.exports = config;
