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
    runSave(req, res, req.params.mod, req.params.auth);
});

router.post('/Transaction/:mod/:auth', (req, res)=>{
    runTransaction(req, res, req.params.mod, req.params.auth);
});

router.post('/Update/:mod/:auth', (req, res)=>{
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
async function runSave(req, res, mod, auth) {
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

async function runTransaction(req, res, mod, auth){

    if (auth === config.ServerAuth || ((await CheckAuth(auth)) && config.AllowClientWrite) ){
        var RawData = req.body;
        const client = new MongoClient(config.DBServer, { useUnifiedTopology: true });
        try{

            // Connect the client to the server
            await client.connect();
            
            const db = client.db(config.DB);
            var collection = db.collection("Globals");
            var  query = { Mod: mod };
            var Element =  "Data." + RawData.Element;
            var stringData = "{ \"$inc\": { \""+Element+"\": " + RawData.Value + " } }"
            var options = { upsert: false };
            var Results = await collection.updateOne(query, JSON.parse(stringData), options);
            if (Results.result.ok == 1 && Results.result.n > 0){
                var Value = await collection.distinct(Element, query);
                log("Transaction " + mod + " incermented " + Element + " by " + RawData.Value + " now " + Value[0], "warn");
                res.json({Status: "Success", ID: mod,  Value: Value[0], Element: RawData.Element})
            } else {
                log("Error in Transaction:  " + mod + " for Globals error: Invaild mod", "warn");
                res.json({Status: "Error", ID: mod,  Value: 0, Element: RawData.Element})
            }
        }catch(err){
            log("Error in Transaction: :  " + mod + " for Globals error: " + err, "warn");
            res.status(203);
            res.json({Status: "Error", ID: mod, Value: 0, Element: RawData.Element });
        }finally{
            // Ensures that the client will close when you finish/error
            await client.close();
        }
    } else {
        res.status(203);
        res.json({Status: "NoAuth", ID: mod, Value: 0, Element: RawData.Element });
    }

}
async function runUpdate(req, res, mod, auth) {
    if ( auth == config.ServerAuth || ((await CheckPlayerAuth(GUID, auth)) && config.AllowClientWrite) ){
        const client = new MongoClient(config.DBServer, { useUnifiedTopology: true });
        try{
            await client.connect();

            var RawData = req.body;
            var element = RawData.Element;
            var StringData;
            if (isObject(RawData.Value)){
                StringData = JSON.stringify(RawData.Value);
            } else if (isArray(RawData.Value)) {
                StringData = JSON.stringify(RawData.Value);
            } else {
                StringData = RawData.Value;
            }
            // Connect the client to the server
            const db = client.db(config.DB);
            var collection = db.collection("Objects");
            var query = { Mod: mod };
            const options = { upsert: false };
            const jsonString = "{ \"Data."+element+"\": "+ StringData + " }";
            const updateDocValue  = JSON.parse(jsonString);
            const updateDoc = { $set: updateDocValue, };
            const result = await collection.updateOne(query, updateDoc, options);
            if (result.result.ok == 1 && result.result.n > 0){
                log("Updated " + element +" for "+ mod + " Globals");
                res.status(200);
                res.json({ Status: "Success", Element: element, Mod: mod, ID: "Globals"});
            } else {
                log("Error with Updating " + element +" for "+ mod + " Globals", "warn");
                res.status(203);
                res.json({ Status: "Error", Element: element, Mod: mod, ID: "Globals"});
            }
        }catch(err){
            res.status(203);
            res.json({ Status: "Error", Element: element, Mod: mod, ID: "Globals"});
            log("ERROR: " + err, "warn");
        }finally{
            // Ensures that the client will close when you finish/error
            await client.close();
        }
    } else {
        res.status(401);
        res.json({ Status: "NoAuth", Element: element, Mod: mod, ID: "Globals"});
        log("AUTH ERROR: " + req.url, "warn");
    }
};
module.exports = router;
isObject = function(a) {
    return (!!a) && (a.constructor === Object);
};


isArray = function(a) {
    return (!!a) && (a.constructor === Array);
};