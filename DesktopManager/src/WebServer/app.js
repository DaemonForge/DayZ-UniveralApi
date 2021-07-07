
global.APIVERSION = '0.9.4';
global.STABLEVERSION = '0.0.0';
global.NEWVERSIONDOWNLOAD = `https://github.com/daemonforge/DayZ-UniveralApi/releases`;
if (global.SAVEPATH === undefined){
  global.SAVEPATH = "./";
}
const express = require('express');
const favicon = require("serve-favicon")
const {existsSync,readFileSync} = require('fs');
const https = require('https')
const fetch  = require('node-fetch');
const {json} = require('body-parser');
const DefaultCert = require('./defaultkeys.json');
const app = express();
const {isArray, versionCompare} = require('./utils');

const nodeFetch = require('node-fetch');
global.fetch = nodeFetch;

/* Config File */
global.config = require('./configLoader');

const log = require("./log");

const RouterItem = require('./Object');
const RouterPlayer = require('./player');
const RouterGlobals = require('./globals');
const RouterAuth = require('./Auth');
const RouterStatus = require('./Status');
const RouterQnA = require('./QnAMaker');
const RouterFowarder = require("./apiFowarder");
const RouterLogger = require("./logger");
const RouterDiscordConnector = require("./discordConnector");
const RouterWit = require("./witConnector");
const RouterLUIS = require("./luisConnector");
const RouterTranslate = require("./TranslateConnector");
const RouterServerQuery = require("./serverQuery");
const RouterToxicity = require("./toxicityConnector");
const RouterTrueRandom = require("./TrueRandom");

var RateLimit = require('express-rate-limit');
var limiter = new RateLimit({
  windowMs: 10*1000, // 50 req/sec
  max: global.config.RequestLimit || 500,
  message:  '{ "Status": "Error", "Error": "RateLimited" }',
  keyGenerator: function (req /*, res*/) {
    return req.headers['CF-Connecting-IP'] || req.headers['x-forwarded-for'] || req.socket.remoteAddress || req.ip;
  },
  onLimitReached: function (req, res, options) {
    let ip = req.headers['CF-Connecting-IP'] || req.headers['x-forwarded-for'] || req.socket.remoteAddress || req.ip;
    log("RateLimit Reached("  + ip + ") you may be under a DDoS Attack or you may need to increase your request limit");
  },
  skip: function (req, res) {
    let ip = req.headers['CF-Connecting-IP'] || req.headers['x-forwarded-for'] || req.socket.remoteAddress || req.ip;
    if (global.config.RateLimitWhiteList !== undefined && ip !== undefined && ip !== null && isArray(global.config.RateLimitWhiteList) && (global.config.RateLimitWhiteList.find(element => element === ip) === ip)) return true;
    return false;
  }
});

// apply rate limiter to all requests
app.use(limiter);

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
app.use('/QnA', RouterQnA); //Switching to /QnA for new ai interface
app.use('/Forward', RouterFowarder);
app.use('/Logger', RouterLogger);
app.use('/Discord', RouterDiscordConnector);
app.use('/Wit', RouterWit);
app.use('/LUIS', RouterLUIS);
app.use('/Translate', RouterTranslate);
app.use('/ServerQuery', RouterServerQuery);
app.use('/Toxicity', RouterToxicity);
app.use('/Random', RouterTrueRandom);

app.use('/', (req,res)=>{
    log("Error invalid or is not a post Requested URL is:" + req.url);
    res.status(501);
    res.json({Status: "error", Error: "Reqested bad URL"});
});
let ServerKey = DefaultCert.Key;
let ServerCert = DefaultCert.Cert;
if (global.config.Certificate != "" && global.config.CertificateKey != ""){
  if (existsSync(global.config.Certificate) && existsSync(global.config.CertificateKey)){
    ServerKey = readFileSync(global.config.Certificate);
    ServerCert = readFileSync(global.config.CertificateKey);
  }
}
let Port = process.env.PORT || global.config.Port || 8443
const server = https.createServer({
    key: ServerKey,
    cert: ServerCert
  }, app)
  .listen(Port, function () {
    log('API Webservice started and is now listening on port "' + Port +'"!')
  });
server.on('error', function (e) {
    // Handle your error here
    log(e, "warn");
  });

async function CheckRecentVersion(){
  try {
    const data = await fetch("https://api.github.com/repos/daemonforge/DayZ-UniveralApi/releases").then( response => response.json()).catch(e => console.log(e));
    if (data[0] !== undefined && data[0].tag_name !== undefined ){
      global.STABLEVERSION = data[0].tag_name;
      global.NEWVERSIONDOWNLOAD = data[0].html_url;
    }
    let vc = versionCompare(global.APIVERSION, global.STABLEVERSION);
    if (global.STABLEVERSION === "0.0.0"){
      log(`WARNING!!! Could check for the current stable version`, "warn");
    } else if (vc > 0){
      log(`WARNING!!! You are running a unpublished version, note it may not work as expected`, "warn")
      log(`Installed Version: ${global.APIVERSION} Stable Version: ${global.STABLEVERSION} `);
    } else if (vc < 0){
      log(`!!!WARNING!!! You're API is currently out of date `, "warn")
      log(`Installed Version: ${global.APIVERSION} Stable Version: ${global.STABLEVERSION}`);
      log(`WARNING!!! Download Link - ${global.NEWVERSIONDOWNLOAD}`, "warn");
    } else {
      log(`API Is currently running the most recent Stable Version: ${global.APIVERSION}`);
    }
  } catch (err){
    log(`WARNING!!! Couldn't check for the current stable version`, "warn");
    console.log(err);
  }
}
if (global.config?.CheckForNewVersion){
  CheckRecentVersion();
}

module.exports = https;