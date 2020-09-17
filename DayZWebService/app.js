const express = require('express');
const { MongoClient } = require("mongodb");
const config = require('./config.json');
const bodyParser = require('body-parser');
const RouterItem = require('./Items');
const RouterPlayer = require('./player');
const RouterGlobals = require('./gobals');
 
// Create a new MongoClient
const client = new MongoClient(config.DBServer, { useUnifiedTopology: true });


const app = express();


console.log("AP Started");
app.use(bodyParser.json())
app.use('/Item', RouterItem)
app.use('/Player', RouterPlayer)
app.use('/Gobals', RouterGlobals)
app.listen(config.Port);