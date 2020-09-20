const express = require('express');
const { MongoClient } = require("mongodb");
const config = require('./config.json');

const router = express.Router();

router.post('/Load/:mod/:auth', (req, res)=>{
    runGet(req, res, req.params.mod, req.params.auth);
});

router.post('/Save/:mod/:auth', (req, res)=>{
    runUpdate(req, res, req.params.mod, req.params.auth);
});
async function runGet(req, res, mod, auth) {
    if (auth == config.ServerAuth || (await CheckPlayerAuth(auth)) ){
        var RawData = req.body;
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
                    console.log("result: "+ results)
                }
                res.status(201);
                res.json(RawData);
            } else {
                var data = await results.toArray(); 
                console.log("Found " + mod + " data: " + data[0].Data)
                res.json(data[0].Data);
            }
        }catch(err){
            console.log("Found Server with ID " + err)
            res.status(203);
            res.json(RawData);
        }finally{
            // Ensures that the client will close when you finish/error
            await client.close();
        }
    } else {
        res.status(401);
        res.json(RawData);
    }
};
async function runUpdate(req, res, mod, auth) {
    if (auth == config.ServerAuth || ((await CheckPlayerAuth(auth)) && config.AllowClientWrite) ){
        const client = new MongoClient(config.DBServer, { useUnifiedTopology: true });
        var StringData = JSON.stringify(req.body);
        var RawData = req.body;

        try{
            await client.connect();
            // Connect the client to the server
            console.log("ID " + mod + " req" + req.body);
            const db = client.db(config.DB);
            var collection = db.collection("Globals");
            var query = { Mod: mod };
            const options = { upsert: true };
            const updateDoc  = {
                $set: { Mod: mod, Data: RawData }
            };
            const result = await collection.updateOne(query, updateDoc, options);
            console.log("Posted New Data result: " + result.result)
            res.json(RawData);
        }catch(err){
            console.log("err " + err)
            res.status(203);
            res.json(RawData);
        }finally{
            // Ensures that the client will close when you finish/error
            await client.close();
        }
    } else {
        res.status(401);
        res.json(req.body);
    }
};

async function CheckPlayerAuth(auth){
    var isAuth = false;
    const client = new MongoClient(config.DBServer, { useUnifiedTopology: true });
    try{
        await client.connect();
        // Connect the client to the server       
        const db = client.db(config.DB);
        var collection = db.collection("Players");
        var query = { AUTH: auth };
        var results = collection.find(query);
            if ((await results.count()) != 0){
                isAuth = true;
            }
    } catch(err){
        console.log(" auth" + auth + " err" + err);
    } finally{
        await client.close();
        return isAuth;
    }

}
module.exports = router;