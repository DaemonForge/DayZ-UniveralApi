
const fs = require('fs');

const Defaultconfig = require('./sample-config.json');
const ConfigPath = "config.json"
var config;
try{
  config = JSON.parse(fs.readFileSync(ConfigPath));
} catch (err){
  Defaultconfig.ServerAuth = makeAuthToken();
  config = Defaultconfig;
  fs.writeFileSync("./config.json", JSON.stringify(Defaultconfig, undefined, 4))
  console.log("Installing for the first time the default config \"config.json\" was created with the following values");
  console.log("   DBServer: "  + config.DBServer);
  console.log("   DB: "  + config.DB);
  console.log("   ServerAuth: "  + config.ServerAuth);
  console.log("   AllowClientWrite: "  + config.AllowClientWrite);
  console.log("   Port: "  + config.Port);
}
function makeAuthToken() {
    var result           = '';
    var characters       = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-.~()*:@,;';
    var charactersLength = characters.length;
    for ( var i = 0; i < 48; i++ ) {
       result += characters.charAt(Math.floor(Math.random() * charactersLength));
    }
    return result;
 }
module.exports = config;
