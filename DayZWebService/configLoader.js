
const {readFileSync,writeFileSync,unlink,existsSync} = require('fs');
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


//Some Code to update configs for people
try {
  if (existsSync(global.SAVEPATH + "QnAMaker.json")) {
        //file exists
      try {
        let QnAconfig = JSON.parse(readFileSync(global.SAVEPATH + "QnAMaker.json")); 
        log(`QnAConfig Present Converting to new format`);
        if (config.QnA === undefined){
          config.QnA = {};
        }
        config.QnA["main"] =  QnAconfig;
        unlink(global.SAVEPATH + "QnAMaker.json", (err) => {
          if (err) {
            console.error(err)
            return
          }
        
          //file removed
        });
        writeFileSync(global.SAVEPATH + ConfigPath, JSON.stringify(config, undefined, 4))
      } catch(e){
		log(`${e}`, "warn")
      }
  }
} catch(err) {
  console.error(err)
}



if (config.Discord === undefined){
  config.Discord = {};
  if (config.Discord_Client_Id !== undefined){
    config.Discord.Client_Id = config.Discord_Client_Id;
    delete config.Discord_Client_Id;
  } else { config.Discord.Client_Id = ""; }

  if (config.Discord_Client_Secret !== undefined){
    config.Discord.Client_Secret = config.Discord_Client_Secret;
    delete config.Discord_Client_Secret;
  } else { config.Discord.Client_Secret = ""; }

  if (config.Discord_Bot_Token !== undefined){
    config.Discord.Bot_Token = config.Discord_Bot_Token;
    delete config.Discord_Bot_Token;
  } else {  config.Discord.Bot_Token = "";  }

  if (config.Discord_Guild_Id !== undefined){
    config.Discord.Guild_Id = config.Discord_Guild_Id;
    delete config.Discord_Guild_Id;
  } else { config.Discord.Guild_Id = ""; }

  if (config.Discord_Required_Role !== undefined){
    config.Discord.Required_Role = config.Discord_Required_Role;
    delete config.Discord_Required_Role;
  } else { config.Discord.Required_Role = ""; }

  if (config.Discord_BlackList_Role !== undefined){
    config.Discord.BlackList_Role = config.Discord_BlackList_Role;
    delete config.Discord_BlackList_Role;
  } else { config.Discord.BlackList_Role = ""; }
  if (config.Discord_Restrict_Sign_Up !== undefined){
    config.Discord.Restrict_Sign_Up = config.Discord_Restrict_Sign_Up;
    delete config.Discord_Restrict_Sign_Up;
  } else { config.Discord.Restrict_Sign_Up = false;}

  if (config.Discord_Sign_Up_Countries !== undefined){
    config.Discord.Sign_Up_Countries = config.Discord_Sign_Up_Countries;
    delete config.Discord_Sign_Up_Countries;
  } else {config.Discord.Sign_Up_Countries = ["blacklist", "CN"]; }

 try {
  writeFileSync(global.SAVEPATH + ConfigPath, JSON.stringify(config, undefined, 4))
 } catch(e) {
   log(`${e}`, "warn")
 }
}

if (config.CheckForNewVersion === undefined || config.CheckForNewVersion === null){
  config.CheckForNewVersion = true;
  try {
   writeFileSync(global.SAVEPATH + ConfigPath, JSON.stringify(config, undefined, 4))
  } catch(e) {
   log(`${e}`, "warn")
  }
}

if (config.RateLimitWhiteList === undefined || config.RateLimitWhiteList === null){
  config.RateLimitWhiteList = ["127.0.0.1"];
  try {
   writeFileSync(global.SAVEPATH + ConfigPath, JSON.stringify(config, undefined, 4))
  } catch(e) {
   log(`${e}`, "warn")
  }
}

if (config.CreateIndexes === undefined || config.CreateIndexes === null){
  config.CreateIndexes = true;
  try {
   writeFileSync(global.SAVEPATH + ConfigPath, JSON.stringify(config, undefined, 4))
  } catch(e) {
   log(`${e}`, "warn")
  }
}


module.exports = config;
