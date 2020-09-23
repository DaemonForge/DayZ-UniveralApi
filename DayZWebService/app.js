const express = require('express');
const { MongoClient } = require("mongodb");
const fs = require('fs');
const https = require('https')
const bodyParser = require('body-parser');
const DefaultCert = require('./defaultkeys.json');
const app = express();
/* Config File */
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

const RouterItem = require('./Items');
const RouterPlayer = require('./player');
const RouterGlobals = require('./gobals');
const RouterAuth = require('./Auth');
const RouterStatus = require('./Status');
const RouterQnA = require('./QnAMaker');

app.use(bodyParser.json());
app.use('/Item', RouterItem);
app.use('/Player', RouterPlayer);
app.use('/Gobals', RouterGlobals);
app.use('/GetAuth', RouterAuth);
app.use('/Status', RouterStatus);
app.use('/QnAMaker', RouterQnA);
app.use('/', (req,res)=>{

    console.log("Error in URL:" + req.url);
    res.json({Error: "Error"});
});
var ServerKey = DefaultCert.Key;
var ServerCert = DefaultCert.Cert;
if (config.Certificate != "" && config.CertificateKey != ""){
  if (fs.existsSync(config.Certificate) && fs.existsSync(config.CertificateKey)){
    ServerKey = fs.readFileSync(config.Certificate);
    ServerCert = fs.readFileSync(config.CertificateKey);
  }
}

https.createServer({
    key: ServerKey,
    cert: ServerCert
  }, app)
  .listen(config.Port, function () {
    console.log('API Webservice started and is now listening on port "' + config.Port +'"!')
  });
  function makeAuthToken() {
    var result           = '';
    var characters       = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-.~()*:@,;';
    var charactersLength = characters.length;
    for ( var i = 0; i < 44; i++ ) {
       result += characters.charAt(Math.floor(Math.random() * charactersLength));
    }
    return result;
 }

module.exports = https;