const express = require('express');
const favicon = require("serve-favicon")
const {existsSync,readFileSync} = require('fs');
const https = require('https')
const {json} = require('body-parser');
const DefaultCert = require('./defaultkeys.json');
const app = express();
const log = require("./log");
/* Config File */
const config = require('./configLoader');

const RouterItem = require('./Object');
const RouterPlayer = require('./player');
const RouterGlobals = require('./globals');
const RouterAuth = require('./Auth');
const RouterStatus = require('./Status');
const RouterQnA = require('./QnAMaker');
const RouterFowarder = require("./apiFowarder");
const RouterLogger = require("./logger");
const RouterDiscordConnector = require("./discordConnector");



app.use((req, res, next) => {
  json({
      limit: '64mb'
  })(req, res, (err) => {
      if (err) {
          console.log("Bad Request Sent");
          res.status(400);
          res.json({Status: "error", Error: "Bad Request"});
          return;
      }
      next();
  });
});
app.use(favicon(__dirname + '/public/favicon.ico'));
app.use('/Object', RouterItem);
app.use('/Player', RouterPlayer);
app.use('/Gobals', RouterGlobals); //For Backwards Compatblity 
app.use('/Globals', RouterGlobals);
app.use('/GetAuth', RouterAuth);
app.use('/Status', RouterStatus);
app.use('/QnAMaker', RouterQnA);
app.use('/Forward', RouterFowarder);
app.use('/Logger', RouterLogger);
app.use('/Discord', RouterDiscordConnector);
app.use('/', (req,res)=>{
    log("Error invalid or is not a post Requested URL is:" + req.url);
    res.status(501);
    res.json({Status: "error", Error: "Reqested bad URL"});
});
let ServerKey = DefaultCert.Key;
let ServerCert = DefaultCert.Cert;
if (config.Certificate != "" && config.CertificateKey != ""){
  if (existsSync(config.Certificate) && existsSync(config.CertificateKey)){
    ServerKey = readFileSync(config.Certificate);
    ServerCert = readFileSync(config.CertificateKey);
  }
}

https.createServer({
    key: ServerKey,
    cert: ServerCert
  }, app)
  .listen(config.Port, function () {
    console.log('API Webservice started and is now listening on port "' + config.Port +'"!')
  });


module.exports = https;