const {Router} = require('express');
const { MongoClient } = require("mongodb");

const {isArray, isObject, isEmpty, HandleBadAuthkey,IncermentAPICount} = require('../utils')

const log = require("../log");

const router = Router();

router.post('/Load/:mod', (req, res)=>{
    runGet(req, res, req.params.mod);
});

router.post('/Save/:mod', (req, res)=>{
    if (req.IsServer !== true) return HandleBadAuthkey(res);
    runSave(req, res, req.params.mod);
});

router.post('/Transaction/:mod', (req, res)=>{
    if (req.IsServer !== true) return HandleBadAuthkey(res);
    runTransaction(req, res, req.params.mod);
});

router.post('/Update/:mod', (req, res)=>{
    if (req.IsServer !== true) return HandleBadAuthkey(res);
    runUpdate(req, res, req.params.mod);
});


async function runGet(req, res, mod) {
    let RawData = req.body;
        const client = new MongoClient(global.config.DBServer, { useUnifiedTopology: true });
        try{
            // Connect the client to the server
            await client.connect();
            
            const db = client.db(req.ClientInfo.DB);
            let collection = db.collection("Globals");
            let query = { Mod: mod };
            let results = collection.find(query);
            if ((await results.count()) == 0){
                if (req.IsServer && !isEmpty(RawData)){
                    let doc = { Mod: mod, Data: RawData };
                    let result = await collection.insertOne(doc);
                    if ( result.matchedCount === 1 || result.upsertedCount === 1 ){ 
                        log("Created "+ mod + " Globals");
                        res.status(201);
                    }
                }
                res.json(RawData);
                IncermentAPICount(req.ClientInfo.ClientId);
            } else {
                let data = await results.toArray(); 
                log("Retrieving "+ mod + " Globals");
                res.json(data[0].Data);
                IncermentAPICount(req.ClientInfo.ClientId);
            }
        }catch(err){
            res.status(203);
            res.json(RawData);
            log("ERROR: " + err, "warn");
        }finally{
            // Ensures that the client will close when you finish/error
            await client.close();
        }
};
async function runSave(req, res, mod) {
    let RawData = req.body;
        const client = new MongoClient(global.config.DBServer, { useUnifiedTopology: true });
        try{
            await client.connect();
            // Connect the client to the server
            const db = client.db(req.ClientInfo.DB);
            let collection = db.collection("Globals");
            let query = { Mod: mod };
            const options = { upsert: true };
            const updateDoc  = {
                $set: { Mod: mod, Data: RawData }
            };
            const result = await collection.updateOne(query, updateDoc, options);
            if ( result.matchedCount === 1 || result.upsertedCount === 1 ){
                log("Updated "+ mod + " Globals");
                res.status(201);
                res.json(RawData);
                IncermentAPICount(req.ClientInfo.ClientId);
            } else {
                log("Error with Updating "+ mod + "Globals", "warn");
                res.status(203);
                res.json(RawData);
                IncermentAPICount(req.ClientInfo.ClientId);
            }
        }catch(err){
            res.status(203);
            res.json(RawData);
            log("ERROR: " + err, "warn");
        }finally{
            // Ensures that the client will close when you finish/error
            await client.close();
        }
};

async function runTransaction(req, res, mod){
        let RawData = req.body;
        const client = new MongoClient(global.config.DBServer, { useUnifiedTopology: true });
        try{

            // Connect the client to the server
            await client.connect();
            
            const db = client.db(req.ClientInfo.DB);
            let collection = db.collection("Globals");
            let  query = { Mod: mod };
            let Element =  "Data." + RawData.Element;
            let stringData = "{ \"$inc\": { \""+Element+"\": " + RawData.Value + " } }"
            let options = { upsert: false };
            let Results = await collection.updateOne(query, JSON.parse(stringData), options);
            if (Results.modifiedCount >= 1 || Results.upsertedCount >= 1){
                let Value = await collection.distinct(Element, query);
                log("Transaction " + mod + " incermented " + Element + " by " + RawData.Value + " now " + Value[0], "warn");
                res.json({Status: "Success", ID: mod,  Value: Value[0], Element: RawData.Element})
                IncermentAPICount(req.ClientInfo.ClientId);
            } else {
                log("Error in Transaction:  " + mod + " for Globals error: Invaild mod", "warn");
                res.json({Status: "Error", ID: mod,  Value: 0, Element: RawData.Element})
                IncermentAPICount(req.ClientInfo.ClientId);
            }
        }catch(err){
            log("Error in Transaction: :  " + mod + " for Globals error: " + err, "warn");
            res.status(203);
            res.json({Status: "Error", ID: mod, Value: 0, Element: RawData.Element });
        }finally{
            // Ensures that the client will close when you finish/error
            await client.close();
        }
}
async function runUpdate(req, res, mod) {
        let RawData = req.body;
        const client = new MongoClient(global.config.DBServer, { useUnifiedTopology: true });
        try{
            await client.connect();

            let element = RawData.Element;
            let operation = RawData.Operation || "set";
            let StringData = RawData.Value;
            if (isObject(RawData.Value)){
                StringData = JSON.stringify(RawData.Value);
            } else if (isArray(RawData.Value)) {
                StringData = JSON.stringify(RawData.Value);
            } else if (`${StringData}`.match(/^-?(0|[1-9]\d*)(\.\d+)?$/g)){
                StringData = RawData.Value * 1;
            } else {
                StringData = RawData.Value;
            }
            // Connect the client to the server
            const db = client.db(req.ClientInfo.DB);
            let collection = db.collection("Globals");
            let query = { Mod: mod };
            const options = { upsert: false };
            let jsonString = `{ "Data.${element}": ${StringData} }`;
            let updateDocValue ;
            
            try { //should use regex to check for a string without " but being lazy
                updateDocValue  = JSON.parse(jsonString);
            } catch (e){
                jsonString = `{ "Data.${element}": "${StringData}" }`;
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
            if ( result.matchedCount >= 1 || result.upsertedCount >= 1){
                log("Updated " + element +" for "+ mod + " Globals");
                res.status(200);
                res.json({ Status: "Success", Element: element, Mod: mod, ID: "Globals"});
                IncermentAPICount(req.ClientInfo.ClientId);
            } else {
                log("Error with Updating " + element +" for "+ mod + " Globals", "warn");
                res.status(203);
                res.json({ Status: "Error", Element: element, Mod: mod, ID: "Globals"});
                IncermentAPICount(req.ClientInfo.ClientId);
            }
        }catch(err){
            //console.log(err)
            log(`ERROR: ${err}`, "warn");
            res.status(203);
            res.json({ Status: "Error", Element: RawData.Element, Mod: RawData.mod, ID: "Globals"});
        }finally{
            // Ensures that the client will close when you finish/error
            await client.close();
        }
};
module.exports = router;