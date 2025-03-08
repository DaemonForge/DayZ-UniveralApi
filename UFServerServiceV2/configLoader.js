const { readFileSync, writeFileSync, existsSync, mkdirSync } = require('fs');
const { makeAuthToken } = require('./utils');

const Defaultconfig = require('./sample-config.json');
const ConfigPath = "config.json";
const ConfigFilePath = global.SAVEPATH + ConfigPath;

// Get the logger from global
const logger = global.logger;

// Helper function to safely update config and write to file
function updateConfig(updates) {
  Object.assign(config, updates);
  try {
    writeFileSync(ConfigFilePath, JSON.stringify(config, undefined, 4));
  } catch (e) {
    logger.error('Failed to update config', { error: e.message });
  }
}

// Load or create config
let config;
try {
  config = JSON.parse(readFileSync(ConfigFilePath));
} catch (err) {
  // Set up default config with generated auth token
  config = { ...Defaultconfig, ServerAuth: makeAuthToken() };
  writeFileSync(ConfigFilePath, JSON.stringify(config, undefined, 4));
  
  logger.info(`Installing for the first time the default config "${ConfigPath}" was created with the following values`);
  ['DBServer', 'DB', 'ServerAuth', 'AllowClientWrite', 'Port'].forEach(key => {
    logger.info(`   ${key}: ${config[key]}`);
  });
}

// Apply default values if missing
const defaults = {
  IP: "0.0.0.0",
  LetsEncypt: { Enabled: false, Domain: "yourdomain.com", Email: "jon@example.com", AltNames: [] },
  CreateIndexes: true
};

// Update config with any missing defaults
let needsUpdate = false;
Object.entries(defaults).forEach(([key, value]) => {
  if (config[key] === undefined || config[key] === null) {
    config[key] = value;
    needsUpdate = true;
  }
});

if (needsUpdate) {
  updateConfig({});
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
