const express = require('express');
const { MongoClient } = require("mongodb");
const fs = require('fs');
const https = require('https')
const bodyParser = require('body-parser');
const DefaultCert = require('./defaultkeys.json');
const app = express();

/* Config File */
const config = require('./configLoader');

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
    console.log("Error invalid or is not a post Requested URL is:" + req.url);
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


module.exports = https;