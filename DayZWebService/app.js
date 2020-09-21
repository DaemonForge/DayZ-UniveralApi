const express = require('express');
const { MongoClient } = require("mongodb");
const fs = require('fs')
const configStr = fs.readFileSync('config.json');
const config = JSON.parse(configStr);
const https = require('https')
const bodyParser = require('body-parser');
const RouterItem = require('./Items');
const RouterPlayer = require('./player');
const RouterGlobals = require('./gobals');
const RouterAuth = require('./Auth');
const RouterStatus = require('./Status');

const app = express();


app.use(bodyParser.json());
app.use('/Item', RouterItem);
app.use('/Player', RouterPlayer);
app.use('/Gobals', RouterGlobals);
app.use('/GetAuth', RouterAuth);
app.use('/Status', RouterStatus);
app.use('/', (req,res)=>{

    console.log("Error in URL:" + req.url);
    res.json({Error: "Error"});
});

https.createServer({
    key: fs.readFileSync('server.key'),
    cert: fs.readFileSync('server.cert')
  }, app)
  .listen(config.Port, function () {
    console.log('API Webservice  listening on port "' + config.Port +'"! Go to https://localhost:'+config.Port+'/')
  });

module.exports = https;