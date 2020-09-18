const express = require('express');
const { MongoClient } = require("mongodb");
const config = require('./config.json');
const fs = require('fs')
const https = require('https')
const bodyParser = require('body-parser');
const RouterItem = require('./Items');
const RouterPlayer = require('./player');
const RouterGlobals = require('./gobals');
const RouterAuth = require('./Auth');
 
// Create a new MongoClient
const client = new MongoClient(config.DBServer, { useUnifiedTopology: true });


const app = express();


app.use(bodyParser.json())
app.use('/Item', RouterItem)
app.use('/Player', RouterPlayer)
app.use('/Gobals', RouterGlobals)
app.use('/GetAuth', RouterAuth)


https.createServer({
    key: fs.readFileSync('server.key'),
    cert: fs.readFileSync('server.cert')
  }, app)
  .listen(config.Port, function () {
    console.log('API Webservice  listening on port 3000! Go to https://localhost:3000/')
  });
