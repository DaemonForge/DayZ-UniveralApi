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
        const client = new MongoClient(config.DBServer, { useUnifiedTopology: true });
        try{

            // Connect the client to the server
            await client.connect();
            
            await client.db(config.DB).command({ ping: 1 });
            const db = client.db(config.DB);
            var collection = db.collection("server");
            var query = { Mod: mod };
            var results = collection.find(query);
            var reqData = req.body.Data;
            if ((await results.count()) == 0){
                console.log("Can't find Server with ID " + serverId + "Creating it now");

                var doc = { Mod: mod, Data: req.body };
                var result = await collection.insertOne(doc);
                res.json(result.ops[0]);
            } else {
                var data = await results.toArray(); 
                console.log("Found Server with ID " + serverId + " data: " + data)
                res.json(data[0].Data);
            }
        }catch(err){
            console.log("Found Server with ID " + err)
            res.json(err);
        }finally{
            // Ensures that the client will close when you finish/error
            await client.close();
            return res;
        }
    } else {
        res.json(req.body);
    }
};
async function runUpdate(req, res, mod, auth) {
    if (auth == config.ServerAuth || ((await CheckPlayerAuth(auth)) && config.AllowClientWrite) ){
        const client = new MongoClient(config.DBServer, { useUnifiedTopology: true });
        var StringData = JSON.stringify(req.body);
        var RawData = req.body;
        try{

            // Connect the client to the server
            await client.connect();
            
            await client.db(config.DB).command({ ping: 1 });
            console.log("ID " + mod + " req" + req.body);
            const db = client.db(config.DB);
            var collection = db.collection("server");
            var query = { Mod: mod };
            const options = { upsert: false };
            const updateDoc  = {
                $set: { Data: RawData }
            };
            const result = await collection.updateOne(query, updateDoc, options);
            res.json(result.result);
        }catch(err){
            console.log("err " + err)
            res.json(RawData);
        }finally{
            // Ensures that the client will close when you finish/error
            await client.close();
        }
    } else {
        res.json(req.body);
    }
};

async function CheckPlayerAuth(auth){
    console.log("Checking Player AUTH");
    var isAuth = false;
    const client = new MongoClient(config.DBServer, { useUnifiedTopology: true });
    try{
        await client.connect();
        // Connect the client to the server        
        console.log(" auth" + auth);
        const db = client.db(config.DB);
        var collection = db.collection("players");
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