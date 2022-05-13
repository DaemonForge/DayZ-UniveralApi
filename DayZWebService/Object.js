const {Router} = require('express');
const { MongoClient } = require("mongodb");
const {CheckAuth,CheckServerAuth} = require('./AuthChecker')
let {createHash} = require('crypto');

const {isArray, isObject, makeObjectId, isEmpty} = require('./utils')
 
const log = require("./log");
// Create a new MongoClient

const queryHandler = require("./Query");
const TransactionHandler = require("./Transaction");

const router = Router();

router.use('/Query', queryHandler);
router.use('/Transaction', TransactionHandler);
router.post('/Load/:ObjectId/:mod', (req, res)=>{
    runGet(req, res, req.params.ObjectId, req.params.mod, req.headers['auth-key']);
});

router.post('/Save/:ObjectId/:mod', (req, res)=>{
    runSave(req, res, req.params.ObjectId, req.params.mod, req.headers['auth-key']);
});

router.post('/Update/:ObjectId/:mod', (req, res)=>{
    runUpdate(req, res, req.params.ObjectId, req.params.mod, req.headers['auth-key']);
});


async function runGet(req, res, ObjectId, mod, auth) {
    if (CheckServerAuth(auth) || (await CheckAuth(auth)) ){
        const client = new MongoClient(global.config.DBServer, { useUnifiedTopology: true });
        let StringData = JSON.stringify(req.body);
        let RawData = req.body;
        try{

            // Connect the client to the server
            await client.connect();
            const db = client.db(global.config.DB);
            let collection = db.collection("Objects");
            let query = { ObjectId: ObjectId, Mod: mod };
            let results = collection.find(query);
            if ((await collection.countDocuments(query)) == 0){
                if ((CheckServerAuth(auth) || global.config.AllowClientWrite) && !isEmpty(RawData)){
                    if (ObjectId == "NewObject"){
                        ObjectId = makeObjectId();
                        RawData.ObjectId = ObjectId;
                        console.log("Item called as NewObject for " + mod + " Generating ID " + ObjectId);
                    }
                    console.log("Can't find Object for mod " + mod + " with ID " + ObjectId + " Creating it now");
                    const doc  = {ObjectId: ObjectId, Mod: mod, Data: RawData}
                    let result = await collection.insertOne(doc);
                    let Data = result.ops[0];
                }
                res.status(201);
                res.json(RawData);
            } else {
                let dataarr = await results.toArray(); 
                let data = dataarr[0]; 
                if (typeof data.Data !== 'undefined' && data.Data){
                    res.status(200);
                    res.json(data.Data);
                } else {
                    res.status(203);
                    res.json(RawData);
                }
            }
        }catch(err){
            console.log("ERROR: " + err);
            res.status(203);
            res.json(RawData);
        }finally{
            // Ensures that the client will close when you finish/error
            client.close();
        }
    }  else {
        res.status(401);
        res.json(req.body);
        console.log("ERROR: Bad Auth Token");
    }
};

async function runSave(req, res, ObjectId, mod, auth) {  
    if (CheckServerAuth(auth) || ((await CheckAuth(auth)) && global.config.AllowClientWrite) ){
        const client = new MongoClient(global.config.DBServer, { useUnifiedTopology: true });
        let RawData = req.body;
        try{

            // Connect the client to the server
            await client.connect();
            const db = client.db(global.config.DB);
            let collection = db.collection("Objects");
            if (ObjectId == "NewObject"){
                ObjectId = makeObjectId();
                RawData.ObjectId = ObjectId;
            }
            let query = { ObjectId: ObjectId, Mod: mod };
            const options = { upsert: true };
            const updateDocValue  =  {ObjectId: ObjectId, Mod: mod, Data: RawData};
            const updateDoc = { $set: updateDocValue, };
            const result = await collection.updateOne(query, updateDoc, options);
            if (result.matchedCount === 1 || result.upsertedCount === 1 ){
                log("Updated "+ mod + " Data for Object: " + ObjectId);
                res.status(201);
                res.json(RawData);
            } else {
                log("Error with Updating "+ mod + " Data for Object: " + ObjectId, "warn");
                res.status(203);
                res.json(RawData);
            }
        }catch(err){
            log("err " + err, "warn")
            res.json(RawData);
        }finally{
            // Ensures that the client will close when you finish/error
            await client.close();
        }
    }  else {
        res.status(401);
        res.json(req.body);
        log("ERROR: Bad Auth Token", "warn");
    }
};

async function runUpdate(req, res, ObjectId, mod, auth) {
    if ( CheckServerAuth(auth) || ((await CheckAuth(auth)) && global.config.AllowClientWrite) ){
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
            let collection = db.collection("Objects");
            let query = { ObjectId: ObjectId, Mod: mod };
            const options = { upsert: false };

            let jsonString = `{ "Data.${element}": ${StringData} }`;
            let updateDocValue;
            try { //should use regex to check for a string without " but being lazy
                updateDocValue  = JSON.parse(jsonString);
            } catch (e){
                jsonString = `{ "Data.${element}": "${StringData}" }`;
                updateDocValue  = JSON.parse(jsonString);
            }
            //console.log(updateDocValue);
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
            //console.log(updateDoc)
            
            const result = await collection.updateOne(query, updateDoc, options);
            //console.log(result.result)
            if (result.matchedCount >= 1 || result.upsertedCount >= 1){
                log("Updated " + element +" for "+ mod + " Data for ObjectId: " + ObjectId);
                res.status(200);
                res.json({ Status: "Success", Element: element, Mod: mod, ID: ObjectId});
            } else {
                //console.log(result.result)
                log("Error with Updating " + element +" for "+ mod + " Data for ObjectId: " + ObjectId, "warn");
                res.status(203);
                res.json({ Status: "NotFound", Element: element, Mod: mod, ID: ObjectId});
            }
        }catch(err){
            log(`ERROR: ${err}`, "warn");
            res.status(203);
            res.json({ Status: "Error", Element: RawData.Element, Mod: mod, ID: ObjectId});
            //console.log(err)
        }finally{
            // Ensures that the client will close when you finish/error
            await client.close();
        }
    } else {
        res.status(401);
        res.json({ Status: "Error", Error: "Invalid Auth", Element: "", Mod: mod, ID: ObjectId});
        log("AUTH ERROR: " + req.url, "warn");
    }
};

module.exports = router;
