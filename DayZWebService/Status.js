const express = require('express');
const { MongoClient } = require("mongodb");

const fs = require('fs');
const Defaultconfig = require('./sample-config.json');
const ConfigPath = "config.json"
var config;
try{
  config = JSON.parse(fs.readFileSync(ConfigPath));
} catch (err){
  config = Defaultconfig;
}
 
const router = express.Router();

// Create a new MongoClient
router.post('/', (req, res)=>{
    runStatusCheck(req, res);
});

// Create a new MongoClient
router.post('/:Auth', (req, res)=>{
    runStatusCheck(req, res);
});


async function runStatusCheck(req, res, auth) {
    const client = new MongoClient(config.DBServer, { useUnifiedTopology: true });
    try{
        // Connect the client to the server       
        await client.connect(); 
        await client.db(config.DB).command({ ping: 1 });
        const db = client.db(config.DB);
        var collection = db.collection("Globals");
        var query = { Mod: "UniversalApiSatus"};
        const options = { upsert: true };
        const updateDocValue  = { Mod: "UniversalApiSatus", Description: "This Object Exsits as a test when ever the status url is called to make sure the database is writeable" }
        const updateDoc = { $set: updateDocValue, };
        const result = await collection.updateOne(query, updateDoc, options);
        if (result.result.ok == 1){
            res.json({Status: "ok", Error: "null"});
            console.log("Status Check Called");
        } else {
            res.status(500);
            res.json({Status: "error", Error: "Database Write Error"});
            console.log("ERROR: Database Write Error");
        }
    }catch(err){
        console.log(err);
        res.status(500);
        res.json({Status: "error", Error: err});
        console.log("ERROR: " + err);
    }finally{
        // Ensures that the client will close when you finish/error
        client.close();
    }
};

module.exports = router;