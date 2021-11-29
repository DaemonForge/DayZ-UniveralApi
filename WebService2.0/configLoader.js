
const {readFileSync,writeFileSync} = require('fs');
const {makeAuthToken} = require('./utils')

const Defaultconfig = require('./sample-config.json');
const ConfigPath = "config.json"
let config;
try{
  config = JSON.parse(readFileSync(global.SAVEPATH + ConfigPath));
} catch (err){
  Defaultconfig.ServerAuth = makeAuthToken();
  config = Defaultconfig;
  writeFileSync(global.SAVEPATH + ConfigPath, JSON.stringify(Defaultconfig, undefined, 4))
}

module.exports = config;
