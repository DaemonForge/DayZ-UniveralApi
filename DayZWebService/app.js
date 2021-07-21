
var pjson = require('./package.json');
global.APIVERSION = process.env.npm_package_version || pjson.version;
global.STABLEVERSION = '0.0.0';
global.NEWVERSIONDOWNLOAD = `https://github.com/daemonforge/DayZ-UniveralApi/releases`;
if (global.SAVEPATH === undefined){
  global.SAVEPATH = "./";
}
/* Config File */
global.config = require('./configLoader');

const express = require('express');
const favicon = require("serve-favicon");
const {existsSync,readFileSync} = require('fs');
const https = require('https')
const {json} = require('body-parser');
const DefaultCert = require('./defaultkeys.json');
const cluster = require('cluster');

const {isArray, CheckRecentVersion, CheckIndexes} = require('./utils');

const nodeFetch = require('node-fetch');
global.fetch = nodeFetch;

let totalCPUs = global.config.cpuCount || 1;

if (totalCPUs < 1){
  totalCPUs = require('os').cpus().length 
}

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
function ExtractAuthKey (req, res, next) {
  req.headers['Auth-Key'] = req.headers['Auth-Key'] || req.headers['content-type'] || '';
  req.headers['content-type'] = 'application/json';
  next();
}




function startWebServer() {
  const webapp = express();

  // apply rate limiter to all requests
  webapp.use(limiter);

  webapp.use(ExtractAuthKey);

  webapp.use((req, res, next) => {
    json({
        limit: '64mb'
    })(req, res, (err) => {
        if (err) {
            console.log("Bad Request Sent");
            res.status(400);
            res.json({Status: "error", Error: `Bad Request ${err}`});
            return;
        }
        next();
    });
  });
  webapp.use(favicon(__dirname + '/public/favicon.ico'));
  webapp.use('/Object', RouterItem);
  webapp.use('/Player', RouterPlayer);
  webapp.use('/Gobals', RouterGlobals); //For Backwards Compatblity 
  webapp.use('/Globals', RouterGlobals);
  webapp.use('/GetAuth', RouterAuth);
  webapp.use('/Status', RouterStatus);
  webapp.use('/QnAMaker', RouterQnA);
  webapp.use('/QnA', RouterQnA); //Switching to /QnA for new ai interface
  webapp.use('/Forward', RouterFowarder);
  webapp.use('/Logger', RouterLogger);
  webapp.use('/Discord', RouterDiscordConnector);
  webapp.use('/Wit', RouterWit);
  webapp.use('/LUIS', RouterLUIS);
  webapp.use('/Translate', RouterTranslate);
  webapp.use('/ServerQuery', RouterServerQuery);
  webapp.use('/Toxicity', RouterToxicity);
  webapp.use('/Random', RouterTrueRandom);

  webapp.use('/', (req,res)=>{
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
    }, webapp)
    .listen(Port, function () {
      log('API Webservice started and is now listening on port "' + Port +'"!')
    });
  server.on('error', function (e) {
      // Handle your error here
      log(e, "warn");
    });

}

function Start(isElectron){
  if (cluster.isMaster && totalCPUs > 1 && !isElectron) {
    // Fork workers.
    for (let i = 0; i < totalCPUs; i++) {
      cluster.fork();
    }
    cluster.on('exit', (worker, code, signal) => {
      cluster.fork();
    });
    if (global.config?.CheckForNewVersion){
      CheckRecentVersion();
    }
    setTimeout(CheckIndexes, 1000);
  } else {
    startWebServer();
    if (totalCPUs <= 1){

      if (global.config?.CheckForNewVersion){
        CheckRecentVersion();
      }
      
      setTimeout(CheckIndexes, 1000);

    }
  }
}

module.exports = Start;

Start();