const express = require('express');
const { MongoClient } = require("mongodb");

const CheckAuth = require('./AuthChecker')

const log = require("./log");

const config = require('./configLoader');

const router = express.Router();

router.post('/Load/:mod/:auth', (req, res)=>{
    runGet(req, res, req.params.mod, req.params.auth);
});

router.post('/Save/:mod/:auth', (req, res)=>{
    runUpdate(req, res, req.params.mod, req.params.auth);
});
async function runGet(req, res, mod, auth) {
    var RawData = req.body;
    if (auth == config.ServerAuth || (await CheckAuth(auth)) ){
        const client = new MongoClient(config.DBServer, { useUnifiedTopology: true });
        try{

            // Connect the client to the server
            await client.connect();
    
            const db = client.db(config.DB);
            var collection = db.collection("Globals");
            var query = { Mod: mod };
            var results = collection.find(query);
            if ((await results.count()) == 0){
                if (auth == config.ServerAuth || config.AllowClientWrite){
                    var doc = { Mod: mod, Data: RawData };
                    var result = await collection.insertOne(doc);
                    if (result.result.ok == 1){ 
                        log("Created "+ mod + " Globals");
                        res.status(201);
                    }
                }
                res.json(RawData);
            } else {
                var data = await results.toArray(); 
                log("Retrieving "+ mod + " Globals");
                res.json(data[0].Data);
            }
        }catch(err){
            res.status(203);
            res.json(RawData);
            log("ERROR: " + err, "warn");
        }finally{
            // Ensures that the client will close when you finish/error
            await client.close();
        }
    } else {
        res.status(401);
        res.json(RawData);
        log("AUTH ERROR: " + req.url, "warn");
    }
};
async function runUpdate(req, res, mod, auth) {
    var RawData = req.body;
    if (auth == config.ServerAuth || ((await CheckAuth(auth)) && config.AllowClientWrite) ){
        const client = new MongoClient(config.DBServer, { useUnifiedTopology: true });
        try{
            await client.connect();
            // Connect the client to the server
            const db = client.db(config.DB);
            var collection = db.collection("Globals");
            var query = { Mod: mod };
            const options = { upsert: true };
            const updateDoc  = {
                $set: { Mod: mod, Data: RawData }
            };
            const result = await collection.updateOne(query, updateDoc, options);
            if (result.result.ok == 1){
                log("Updated "+ mod + " Globals");
                res.status(201);
                res.json(RawData);
            } else {
                log("Error with Updating "+ mod + "Globals", "warn");
                res.status(203);
                res.json(RawData);
            }
        }catch(err){
            res.status(203);
            res.json(RawData);
            log("ERROR: " + err, "warn");
        }finally{
            // Ensures that the client will close when you finish/error
            await client.close();
        }
    } else {
        res.status(401);
        res.json(req.body);
        log("AUTH ERROR: " + req.url, "warn");
    }
};

module.exports = router;