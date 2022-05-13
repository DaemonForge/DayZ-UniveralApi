const {Router} = require('express');
const { MongoClient } = require("mongodb");
const {isArray, isObject, isEmpty, NormalizeToGUID} = require('./utils')
const log = require("./log");

const {CheckAuth, CheckPlayerAuth,CheckServerAuth} = require('./AuthChecker')


const queryHandler = require("./Query");
const TransactionHandler = require("./Transaction");
// Create a new MongoClient


const router = Router();
router.use('/Query', queryHandler);
router.use('/Transaction', TransactionHandler);
router.post('/Load/:GUID/:mod', (req, res)=>{
    let GUID = NormalizeToGUID(req.params.GUID);
    runGet(req, res, GUID, req.params.mod, req.headers['auth-key']);
});
router.post('/Save/:GUID/:mod', (req, res)=>{
    let GUID = NormalizeToGUID(req.params.GUID);
    runSave(req, res, GUID, req.params.mod, req.headers['auth-key']);
});

router.post('/Update/:GUID/:mod', (req, res)=>{
    let GUID = NormalizeToGUID(req.params.GUID);
    runUpdate(req, res, GUID, req.params.mod, req.headers['auth-key']);
});

router.post('/PublicLoad/:GUID/:mod', (req, res)=>{
    let GUID = NormalizeToGUID(req.params.GUID);
    runGetPublic(req, res, GUID, req.params.mod, req.headers['auth-key']);
});

router.post('/PublicSave/:GUID/:mod', (req, res)=>{
    let GUID = NormalizeToGUID(req.params.GUID);
    runSavePublic(req, res, GUID, req.params.mod, req.headers['auth-key']);
});


async function runGet(req, res, GUID, mod, auth) {
    if (  CheckServerAuth(auth) || (await CheckPlayerAuth(GUID, auth))){
        const client = new MongoClient(global.config.DBServer, { useUnifiedTopology: true });
        try{

            await client.connect();
            // Connect the client to the server
            const db = client.db(global.config.DB);
            let collection = db.collection("Players");
            let query = { GUID: GUID };
            let results = collection.find(query);
            let StringData = JSON.stringify(req.body);
            let RawData = req.body;
            
            if ((await collection.countDocuments(query)) == 0){
                if ((CheckServerAuth(auth) || global.config.AllowClientWrite) && !isEmpty(RawData)){
                    log("Can't find Player with ID " + GUID + "Creating it now");
                    const doc  = JSON.parse("{ \"GUID\": \"" + GUID + "\", \""+mod+"\": "+ StringData + " }");
                    await collection.insertOne(doc);
                } else {
                    log("Can't find Player with ID " + GUID, "warn");
                }
                res.status(201);
                res.json(RawData);
            } else {
                let dataarr = await results.toArray(); 
                let data = dataarr[0]; 
                let sent = false;
                for (const [key, value] of Object.entries(data)) {
                    if(key === mod){
                        sent = true;
                        res.json(value);
                        log("Retrieving "+ mod + " Data for GUID: " + GUID);
                    }
                }
                if (sent != true){
                    if ((CheckServerAuth(auth) || global.config.AllowClientWrite) && !isEmpty(RawData)){
                        const updateDocValue  = JSON.parse("{ \""+mod+"\": "+ StringData + " }");
                        const updateDoc = { $set: updateDocValue, };
                        const options = { upsert: false };
                        await collection.updateOne(query, updateDoc, options);
                        log("Can't find "+ mod + " Data for GUID: " + GUID +  " Creating it now");
                    } else {
                        log("Can't find "+ mod + " Data for GUID: " + GUID, "warn");
                    }
                    res.status(203);
                    res.json(RawData);
                }
            }
        }catch(err){
            res.status(203);
            res.json(req.body);
            log("ERROR: " + err, "warn");
        }finally{
            // Ensures that the client will close when you finish/error
            client.close();
        }
    } else {
        res.status(401);
        res.json(req.body);
    }
};
async function runSave(req, res, GUID, mod, auth) {
    if ( CheckServerAuth(auth) || ((await CheckPlayerAuth(GUID, auth)) && global.config.AllowClientWrite) ){
        const client = new MongoClient(global.config.DBServer, { useUnifiedTopology: true });
        try{
            await client.connect();

            let StringData = JSON.stringify(req.body);
            let RawData = req.body;
            // Connect the client to the server
            const db = client.db(global.config.DB);
            let collection = db.collection("Players");
            let query = { GUID: GUID };
            const options = { upsert: true };
            const jsonString = "{ \"GUID\": \""+GUID+"\", \""+mod+"\": "+ StringData + " }";
            const updateDocValue  = JSON.parse(jsonString);
            const updateDoc = { $set: updateDocValue, };
            const result = await collection.updateOne(query, updateDoc, options);
            if (result.matchedCount === 1 || result.upsertedCount >= 1){
                log("Updated "+ mod + " Data for GUID: " + GUID);
                res.status(200);
                res.json(RawData);
            } else {
                log("Error with Updating "+ mod + " Data for GUID: " + GUID, "warn");
                res.status(203);
                res.json(req.body);
            }
        }catch(err){
            res.status(203);
            res.json(req.body);
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


async function runUpdate(req, res, GUID, mod, auth) {
    if ( CheckServerAuth(auth) || ((await CheckPlayerAuth(GUID, auth)) && global.config.AllowClientWrite) ){
        let RawData = req.body;
        const client = new MongoClient(global.config.DBServer, { useUnifiedTopology: true });
        try{
            await client.connect();
            let element = RawData.Element;
            let operation = RawData.Operation || "set";
            let StringData;
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
            const db = client.db(global.config.DB);
            let collection = db.collection("Players");
            let query = { GUID: GUID };
            const options = { upsert: false };
            let jsonString = `{ "${mod}.${element}": ${StringData} }`;
            let updateDocValue;
            try { //should use regex to check for a string without " but being lazy
                updateDocValue  = JSON.parse(jsonString);
            } catch (e){
                jsonString = `{ "${mod}.${element}": "${StringData}" }`;
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
            if (result.matchedCount >= 1 || result.upsertedCount >= 1){
                log("Updated " + element +" for "+ mod + " Data for GUID: " + GUID);
                res.status(200);
                res.json({ Status: "Success", Element: element, Mod: mod, ID: GUID});
            } else {
                log("Error with Updating " + element +" for "+ mod + " Data for GUID: " + GUID, "warn");
                res.status(203);
                res.json({ Status: "NotFound", Element: element, Mod: mod, ID: GUID});
            }
        }catch(err){
            res.status(203);
            res.json({ Status: "Error", Element: RawData.Element, Mod: mod, ID: GUID});
            log("ERROR: " + err, "warn");
        }finally{
            // Ensures that the client will close when you finish/error
            await client.close();
        }
    } else {
        res.status(401);
        res.json({ Status: "Error", Error: "Invalid Auth", Element: "", Mod: mod, ID: GUID});
        log("AUTH ERROR: " + req.url, "warn");
    }
};

async function runGetPublic(req, res, GUID, mod, auth) {
    const client = new MongoClient(global.config.DBServer, { useUnifiedTopology: true });
    try{ 
        await client.connect();
        // Connect the client to the server
        const db = client.db(global.config.DB);
        let collection = db.collection("Players");
        let query = { GUID: GUID };
        let results = collection.find(query);
        let RawData = req.body;
        
        if ((await collection.countDocuments(query)) == 0){
            if (auth !== "null" && (CheckServerAuth(auth) || ((await CheckPlayerAuth(GUID, auth)) && global.config.AllowClientWrite))){
                log("Can't find Player with ID " + GUID + " Creating it now");
                const doc  = JSON.parse(`{ "GUID": "${GUID}", "Public": { "${mod}": "${RawData.Value}" } }`);
                await collection.insertOne(doc);
            } else {
                 log("Can't find Player with ID " + GUID, "warn");
            }
            res.status(201);
            res.json(RawData);
        } else {
            let dataarr = await results.toArray(); 
            let data = dataarr[0]; 
            let sent = false;
            if (data !== undefined && data.Public !== undefined)
            for (const [key, value] of Object.entries(data.Public)) {
                if(key === mod){
                    sent = true;
                    res.json({ "Value": value });
                    log("Retrieving "+ mod + " Data for GUID: " + GUID);
                }
            }
            if (sent !== true){
                if (auth !== "null" && (CheckServerAuth(auth) || ((await CheckPlayerAuth(GUID, auth)) && global.config.AllowClientWrite))){
                    const updateDocValue  = JSON.parse(`{ "Public.${mod}": "${RawData.Value}" }`);
                    const updateDoc = { $set: updateDocValue, };
                    const options = { upsert: false };
                    await collection.updateOne(query, updateDoc, options);
                    log("Can't find "+ mod + " Data for GUID: " + GUID +  " Creating it now");
                } else {
                    log("Can't find "+ mod + " Data for GUID: " + GUID, "warn");
                }
                res.status(203);
                res.json(RawData);
            }
        }
    }catch(err){
        res.status(203);
        res.json({Value: "Error"});
        log("ERROR: " + err, "warn");
    }finally{
        // Ensures that the client will close when you finish/error
        client.close();
    }
};
async function runSavePublic(req, res, GUID, mod, auth) {
    if ( CheckServerAuth(auth) || ((await CheckPlayerAuth(GUID, auth)) && global.config.AllowClientWrite) ){
        const client = new MongoClient(global.config.DBServer, { useUnifiedTopology: true });
        try{
            await client.connect();

            let RawData = req.body;
            // Connect the client to the server
            const db = client.db(global.config.DB);
            let collection = db.collection("Players");
            let query = { GUID: GUID };
            const options = { upsert: true };
            const jsonString = `{ "GUID": "${GUID}", "Public.${mod}": "${RawData.Value}" }`;
            const updateDocValue  = JSON.parse(jsonString);
            const updateDoc = { $set: updateDocValue, };
            const result = await collection.updateOne(query, updateDoc, options);
            if ( result.matchedCount === 1 || result.upsertedCount === 1 ){
                log("Updated "+ mod + " Data for GUID: " + GUID);
                res.status(200);
                res.json(RawData);
            } else {
                log("Error with Updating "+ mod + " Data for GUID: " + GUID, "warn");
                res.status(203);
                res.json(RawData);
            }
        }catch(err){
            res.status(203);
            res.json(req.body);
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

