
const {readFileSync,writeFileSync,unlink,existsSync,mkdirSync} = require('fs');
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
  console.log("Installing for the first time the default config \"" + ConfigPath + "\" was created with the following values");
  console.log("   DBServer: "  + config.DBServer);
  console.log("   DB: "  + config.DB);
  console.log("   ServerAuth: "  + config.ServerAuth);
  console.log("   AllowClientWrite: "  + config.AllowClientWrite);
  console.log("   Port: "  + config.Port);
  
  if (global.mainWindow !== undefined){
    global.mainWindow.send("log",{type: "warn", message: "Installing for the first time the default config \"" + ConfigPath + "\" was created with the following values"})
    global.mainWindow.send("log",{type: "warn", message: "   DBServer: "  + config.DBServer})
    global.mainWindow.send("log",{type: "warn", message: "   DB: "  + config.DB})
    global.mainWindow.send("log",{type: "warn", message: "   ServerAuth: "  + config.ServerAuth})
    global.mainWindow.send("log",{type: "warn", message: "   AllowClientWrite: "  + config.AllowClientWrite})
    global.mainWindow.send("log",{type: "warn", message: "   Port: "  + config.Port})
  }
  if (global.logs !== undefined){
    global.logs.push({type: "warn", message: "Installing for the first time the default config \"" + ConfigPath + "\" was created with the following values"})
    global.logs.push({type: "warn", message: "   DBServer: "  + config.DBServer})
    global.logs.push({type: "warn", message: "   DB: "  + config.DB})
    global.logs.push({type: "warn", message: "   ServerAuth: "  + config.ServerAuth})
    global.logs.push({type: "warn", message: "   AllowClientWrite: "  + config.AllowClientWrite})
    global.logs.push({type: "warn", message: "   Port: "  + config.Port})
  }
}

if (config.IP === undefined || config.IP === null){
  config.IP = "0.0.0.0";
  try {
   writeFileSync(global.SAVEPATH + ConfigPath, JSON.stringify(config, undefined, 4))
  } catch(e) {
    console.log(e)
  }
}

if (config.LetsEncypt === undefined || config.LetsEncypt === null){
  config.LetsEncypt = {Enabled: false, Domain: "yourdomain.com", Email: "jon@example.com", AltNames: []};
  try {
   writeFileSync(global.SAVEPATH + ConfigPath, JSON.stringify(config, undefined, 4))
  } catch(e) {
    console.log(e)
  }
}

if (config.CreateIndexes === undefined || config.CreateIndexes === null){
  config.CreateIndexes = true;
  try {
   writeFileSync(global.SAVEPATH + ConfigPath, JSON.stringify(config, undefined, 4))
  } catch(e) {
    console.log(e)
  }
}

if (config.LetsEncypt.Enabled === true){
  if (!config.LetsEncypt.AltNames.find(element => element === config.LetsEncypt.Domain) ){
    config.LetsEncypt.AltNames.push(config.LetsEncypt.Domain);
  }
  let LEconfigjson = {
    sites: [
      {
        subject: config.LetsEncypt.Domain,
        altnames: config.LetsEncypt.AltNames
      }
    ]

  }
  try {
    let path = global.SAVEPATH + "greenlock.d";
    if (!existsSync(path)){
        mkdirSync(path);
    }
    if (!existsSync(`${path}/config.json`)){
      writeFileSync( `${path}/config.json`, JSON.stringify(LEconfigjson, undefined, 4))
    } else {
      let file = JSON.parse(readFileSync(`${path}/config.json`));
      if (LEconfigjson.sites[0].subject != file.sites[0].subject || LEconfigjson.sites[0].altnames != file.sites[0].altnames){
        writeFileSync( `${path}/config.json`, JSON.stringify(LEconfigjson, undefined, 4))
      }
    }
   } catch(e) {
    console.log(e)
    config.LetsEncypt.Enabled = false;
   }
}

module.exports = config;
