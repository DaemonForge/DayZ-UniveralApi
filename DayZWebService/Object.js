const express = require('express');
const { MongoClient } = require("mongodb");
const CheckAuth = require('./AuthChecker')
var crypto = require('crypto');

const config = require('./configLoader');
 
const log = require("./log");
// Create a new MongoClient

const queryHandler = require("./Query");
const TransactionHandler = require("./Transaction");

const router = express.Router();

router.use('/Query', queryHandler);
router.use('/Transaction', TransactionHandler);
router.post('/Load/:ObjectId/:mod/:auth', (req, res)=>{
    runGet(req, res, req.params.ObjectId, req.params.mod, req.params.auth);
});

router.post('/Save/:ObjectId/:mod/:auth', (req, res)=>{
    runUpdate(req, res, req.params.ObjectId, req.params.mod, req.params.auth);
});

async function runGet(req, res, ObjectId, mod, auth) {
    if (auth === config.ServerAuth || (await CheckAuth(auth)) ){
        const client = new MongoClient(config.DBServer, { useUnifiedTopology: true });
        var StringData = JSON.stringify(req.body);
        var RawData = req.body;
        try{

            // Connect the client to the server
            await client.connect();
            const db = client.db(config.DB);
            var collection = db.collection("Objects");
            var query = { ObjectId: ObjectId, Mod: mod };
            var results = collection.find(query);
            
            if ((await results.count()) == 0){
                if (auth === config.ServerAuth || config.AllowClientWrite ){
                    if (ObjectId == "NewObject"){
                        ObjectId = makeObjectId();
                        RawData.ObjectId = ObjectId;
                        console.log("Item called as NewObject for " + mod + " Generating ID " + ObjectId);
                    }
                    console.log("Can't find Object for mod " + mod + " with ID " + ObjectId + " Creating it now");
                    const doc  = {ObjectId: ObjectId, Mod: mod, Data: RawData}
                    var result = await collection.insertOne(doc);
                    var Data = result.ops[0];
                }
                res.status(201);
                res.json(RawData);
            } else {
                var dataarr = await results.toArray(); 
                var data = dataarr[0]; 
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
async function runUpdate(req, res, ObjectId, mod, auth) {  
    if (auth === config.ServerAuth || ((await CheckAuth(auth)) && config.AllowClientWrite) ){
        const client = new MongoClient(config.DBServer, { useUnifiedTopology: true });
        var RawData = req.body;
        try{

            // Connect the client to the server
            await client.connect();
            const db = client.db(config.DB);
            var collection = db.collection("Objects");
            if (ObjectId == "NewObject"){
                ObjectId = makeObjectId();
                RawData.ObjectId = ObjectId;
            }
            var query = { ObjectId: ObjectId, Mod: mod };
            const options = { upsert: true };
            const updateDocValue  =  {ObjectId: ObjectId, Mod: mod, Data: RawData};
            const updateDoc = { $set: updateDocValue, };
            const result = await collection.updateOne(query, updateDoc, options);
            if (result.result.ok == 1){
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
function makeObjectId() {
    var result           = '';
    var characters       = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-.~()*:@,;';
    var charactersLength = characters.length;
    for ( var i = 0; i < 16; i++ ) {
       result += characters.charAt(Math.floor(Math.random() * charactersLength));
    }
    var datetime = new Date();
    var date = datetime.toISOString()
    result += date;
    var SaveToken = crypto.createHash('sha256').update(result).digest('base64');
    //Making it URLSafe
    SaveToken = SaveToken.replace(/\+/g, '-'); 
    SaveToken = SaveToken.replace(/\//g, '_');
    SaveToken = SaveToken.replace(/=+$/, '');
    return SaveToken;
 }
module.exports = router;