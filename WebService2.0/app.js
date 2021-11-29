if (global.APIVERSION === undefined) {
  let pjson = require('./package.json');
  global.APIVERSION = process.env.npm_package_version || pjson.version;
}
global.STABLEVERSION = '0.0.0';
global.NEWVERSIONDOWNLOAD = `https://github.com/daemonforge/DayZ-UniveralApi/releases`;
if (global.SAVEPATH === undefined) {
  global.SAVEPATH = "./";
}
const nodeFetch = require('node-fetch');
global.fetch = nodeFetch;
const log = require('./log')
const express = require('express');
const {
  json
} = require('body-parser');
const https = require('https');
global.config = require('./configLoader')
const cluster = require('cluster');
const {
  ValidateAuth
} = require('./authValidator');

let totalCPUs = global.config.cpuCount || 1;
if (totalCPUs < 1) {
  totalCPUs = require('os').cpus().length
}

const CryptoEndpoint = require('./apis/crypto');
const TrueRandomEndpoint = require('./apis/TrueRandom');
const AuthEndpoint = require('./Auth');
const StatusEndpoint = require('./Status');
const QnAEndpoint = require('./apis/QnAMaker');
const FowarderEndpoint = require('./apis/apiFowarder'); //Switching to /QnA for new ai interface
const LoggerEndpoint = require('./logger');
const DiscordConnectorEndpoint = require('./discord/discordConnector');
const WitEndpoint = require('./apis/witConnector');
const LUISEndpoint = require('./apis/luisConnector');
const TranslateEndpoint = require('./apis/TranslateConnector');
const ServerQueryEndpoint = require('./apis/serverquery');
const ToxicityEndpoint = require('./apis/toxicityConnector');
const dbEndpoint = require('./db/db');
const GlobalsEndpoint = require('./db/globals')
const {
  publicLoad
} = require("./db/public");

const {
  isArray,
  CheckRecentVersion,
  CheckIndexes,
  ExtractAuthKey,
  GenerateCerts,
  GenerateLimiter
} = require('./utils');

async function startWebServer() {
  const webapp = express();

  // apply rate limiter to all requests
  webapp.use(new GenerateLimiter(500, 5));

  webapp.use(ExtractAuthKey);
  webapp.use(ValidateAuth);

  webapp.use((req, res, next) => {
    json({
      limit: '128mb'
    })(req, res, (err) => {
      if (err) {
        log(`Bad Request Sent to "${req.url}" Error: ${err}`);
        res.status(400);
        res.json({
          Status: "Error",
          Error: `Bad Request ${err}`
        });
        return;
      }
      next();
    });
  });

  webapp.use('/GetAuth', AuthEndpoint);
  webapp.use('/Player', dbEndpoint);
  webapp.use('/Object', dbEndpoint);
  webapp.use('/Globals', GlobalsEndpoint);

  webapp.use('/QnAMaker', QnAEndpoint);
  webapp.use('/QnA', QnAEndpoint); //Switching to /QnA for new ai interface
  webapp.use('/Forward', FowarderEndpoint);
  webapp.use('/Logger', LoggerEndpoint);
  webapp.use('/Discord', DiscordConnectorEndpoint);
  webapp.use('/Wit', WitEndpoint);
  webapp.use('/LUIS', LUISEndpoint);
  webapp.use('/Translate', TranslateEndpoint);
  webapp.use('/ServerQuery', ServerQueryEndpoint);
  webapp.use('/Toxicity', ToxicityEndpoint);
  webapp.use('/Status', StatusEndpoint);
  webapp.use('/Random', TrueRandomEndpoint)
  webapp.use('/Crypto', CryptoEndpoint);
  webapp.post('/:ClientId/Player/PublicLoad/:GUID/:mod', publicLoad);

  webapp.use('/', (req, res) => {
    if (req.url != '/') {
      log("Error invalid or is not a post Requested URL is:" + req.url);
    }
    res.status(501);
    res.json({
      Status: "Error",
      Error: "Reqested bad URL"
    });
  });
  let DefaultCert = await GenerateCerts();
  let ServerKey = DefaultCert.private;
  let ServerCert = DefaultCert.cert;
  if (global.config.Certificate != "" && global.config.CertificateKey != "") {
    if (existsSync(global.config.Certificate) && existsSync(global.config.CertificateKey)) {
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
      log('API Webservice started and is now listening on port "' + Port + '"!')
    });
  server.on('error', function (e) {
    // Handle your error here
    log(e, "warn");
  });

}

startWebServer();