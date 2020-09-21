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

router.post('/:mod/:auth', (req, res)=>{
    runQuery(req, res, req.params.mod, req.params.auth, GetCollection(req.url));
});

function GetCollection(URL){
    if (URL.includes("/Player/")){
        return "Players"
    }
    if (URL.includes("/Item/")){
        return "Items"
    }
}

async function runQuery(req, res, mod, auth, COLL) {
    if (auth == config.ServerAuth || (await CheckPlayerAuth(auth)) ){
        var RawData = req.body;
        const client = new MongoClient(config.DBServer, { useUnifiedTopology: true });
        try{

            // Connect the client to the server
            await client.connect();
    
            const db = client.db(config.DB);
            var collection = db.collection(COLL);
            var query = { Mod: mod };
            var results = collection.find(query);
            if ((await results.count()) == 0){
                if (auth == config.ServerAuth || config.AllowClientWrite){
                    var doc = { Mod: mod, Data: RawData };
                    var result = await collection.insertOne(doc);
                    console.log("result: "+ results)
                }
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
        res.status(203);
        res.json(RawData);
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