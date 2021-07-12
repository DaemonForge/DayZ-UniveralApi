const {Router} = require('express');
const { MongoClient } = require("mongodb");

const {CheckAuth, CheckServerAuth} = require('./AuthChecker')
const {isArray, isObject, isEmpty} = require('./utils')

const log = require("./log");


const router = Router();

router.post('/Load/:mod', (req, res)=>{
    runGet(req, res, req.params.mod, req.headers['Auth-Key']);
});

router.post('/Save/:mod', (req, res)=>{
    runSave(req, res, req.params.mod, req.headers['Auth-Key']);
});

router.post('/Transaction/:mod', (req, res)=>{
    runTransaction(req, res, req.params.mod, req.headers['Auth-Key']);
});

router.post('/Update/:mod', (req, res)=>{
    runUpdate(req, res, req.params.mod, req.headers['Auth-Key']);
});

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
    let RawData = req.body;
    if (CheckServerAuth(auth)|| (await CheckAuth(auth)) ){
        const client = new MongoClient(global.config.DBServer, { useUnifiedTopology: true });
        try{

            // Connect the client to the server
            await client.connect();
    
            const db = client.db(global.config.DB);
            let collection = db.collection("Globals");
            let query = { Mod: mod };
            let results = collection.find(query);
            if ((await results.count()) == 0){
                if ((CheckServerAuth(auth) || global.config.AllowClientWrite) && !isEmpty(RawData)){
                    let doc = { Mod: mod, Data: RawData };
                    let result = await collection.insertOne(doc);
                    if (result.result.ok == 1){ 
                        log("Created "+ mod + " Globals");
                        res.status(201);
                    }
                }
                res.json(RawData);
            } else {
                let data = await results.toArray(); 
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
    let RawData = req.body;
    if (CheckServerAuth(auth) || ((await CheckAuth(auth)) && global.config.AllowClientWrite) ){
        const client = new MongoClient(global.config.DBServer, { useUnifiedTopology: true });
        try{
            await client.connect();
            // Connect the client to the server
            const db = client.db(global.config.DB);
            let collection = db.collection("Globals");
            let query = { Mod: mod };
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

    if (CheckServerAuth(auth) || ((await CheckAuth(auth)) && global.config.AllowClientWrite) ){
        let RawData = req.body;
        const client = new MongoClient(global.config.DBServer, { useUnifiedTopology: true });
        try{

            // Connect the client to the server
            await client.connect();
            
            const db = client.db(global.config.DB);
            let collection = db.collection("Globals");
            let  query = { Mod: mod };
            let Element =  "Data." + RawData.Element;
            let stringData = "{ \"$inc\": { \""+Element+"\": " + RawData.Value + " } }"
            let options = { upsert: false };
            let Results = await collection.updateOne(query, JSON.parse(stringData), options);
            if (Results.result.ok == 1 && Results.result.n > 0){
                let Value = await collection.distinct(Element, query);
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
    if ( CheckServerAuth(auth) || ((await CheckPlayerAuth(GUID, auth)) && global.config.AllowClientWrite) ){
        const client = new MongoClient(global.config.DBServer, { useUnifiedTopology: true });
        try{
            await client.connect();

            let RawData = req.body;
            let element = RawData.Element;
            let operation = RawData.Operation || "set";
            let StringData;
            if (isObject(RawData.Value)){
                StringData = JSON.stringify(RawData.Value);
            } else if (isArray(RawData.Value)) {
                StringData = JSON.stringify(RawData.Value);
            } else {
                StringData = RawData.Value;
            }
            // Connect the client to the server
            const db = client.db(global.config.DB);
            let collection = db.collection("Globals");
            let query = { Mod: mod };
            const options = { upsert: false };
            const jsonString = "{ \"Data."+element+"\": "+ StringData + " }";
            let updateDocValue ;
            
            try { //should use regex to check for a string without " but being lazy
                updateDocValue  = JSON.parse(jsonString);
            } catch (e){
                jsonString = "{ \"Data."+element+"\": \""+ StringData + "\" }";
                updateDocValue  = JSON.parse(jsonString);
            }
            
            let updateDoc = { $set: updateDocValue, };
            
            if (operation === "pull"){
                updateDoc = { $pull: updateDocValue, };
            } else if (operation === "push"){
                updateDoc = { $push: updateDocValue, };
            } else if (operation === "unset"){
                updateDoc = { $unset: updateDocValue, };
            } else if (operation === "mul"){
                updateDoc = { $mul: updateDocValue, };
            } else if (operation === "rename"){
                updateDoc = { $rename: updateDocValue, };
            } else if (operation === "pullAll"){
                updateDoc = { $pullAll: updateDocValue, };
            }
            
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