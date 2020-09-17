const express = require('express');
const { MongoClient } = require("mongodb");
const config = require('./config.json');
const bodyParser = require('body-parser');
const RouterServer = require('./Items');
const RouterPlayers = require('./player');
const RouterGlobals = require('./gobals');
 
// Create a new MongoClient
const client = new MongoClient(config.DBServer, { useUnifiedTopology: true });

const app = express();
app.use(bodyParser.json())
app.use('/Items', RouterServer)
app.use('/Player', RouterPlayers)
app.use('/Gobals', RouterGlobals)

app.listen(config.Port);