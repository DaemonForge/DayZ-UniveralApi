const express = require('express');
const { MongoClient } = require("mongodb");
const config = require('./config.json');
const queryHandler = require("./Query");
// Create a new MongoClient


const router = express.Router();
router.use('/Query', queryHandler);
router.post('/Load/:GUID/:mod/:auth', (req, res)=>{
    runGet(req, res, req.params.GUID, req.params.mod, req.params.auth, false);
});

router.post('/Save/:GUID/:mod/:auth', (req, res)=>{
    runUpdate(req, res, req.params.GUID, req.params.mod, req.params.auth, true);
});


async function runGet(req, res, GUID, mod, auth) {
    if (  (auth == config.ServerAuth) || (await CheckPlayerAuth(GUID, auth))){
        const client = new MongoClient(config.DBServer, { useUnifiedTopology: true });
        try{

            await client.connect();
            // Connect the client to the server
            const db = client.db(config.DB);
            var collection = db.collection("Players");
            var query = { GUID: GUID };
            var results = collection.find(query);
            var StringData = JSON.stringify(req.body);
            var RawData = req.body;
            
            if ((await results.count()) == 0){
                if (auth == config.ServerAuth || config.AllowClientWrite){
                    console.log("Can't find Player with ID " + GUID + "Creating it now");
                    const doc  = JSON.parse("{ \"GUID\": \"" + GUID + "\", \""+mod+"\": "+ StringData + " }");
                    await collection.insertOne(doc);
                }
                res.json(RawData);
            } else {
                var dataarr = await results.toArray(); 
                var data = dataarr[0]; 
                var sent = false;
                for (const [key, value] of Object.entries(data)) {
                    console.log(`${key}: ${value}`);
                    if(key === mod){
                        var sent = true;
                        res.json(value);
                    }
                }
                if (sent != true){
                    const updateDocValue  = JSON.parse("{ \""+mod+"\": "+ StringData + " }");
                    const updateDoc = { $set: updateDocValue, };
                    const options = { upsert: false };
                    await collection.updateOne(query, updateDoc, options);
                    res.json(RawData);
                }
            }
        }catch(err){
            res.json(req.body);
        }finally{
            // Ensures that the client will close when you finish/error
            client.close();
            return res;
        }
    } else {
        res.json(req.body);
        return res;
    }
};
async function runUpdate(req, res, GUID, mod, auth, write) {
    if ( auth == config.ServerAuth || ((await CheckPlayerAuth(GUID, auth)) && config.AllowClientWrite) ){
        const client = new MongoClient(config.DBServer, { useUnifiedTopology: true });
        try{
            await client.connect();

            var StringData = JSON.stringify(req.body);
            var RawData = req.body;
            // Connect the client to the server
            const db = client.db(config.DB);
            var collection = db.collection("Players");
            var query = { GUID: GUID };
            const options = { upsert: true };
            const jsonString = "{ \"GUID\": \""+GUID+"\", \""+mod+"\": "+ StringData + " }";
            const updateDocValue  = JSON.parse(jsonString);
            console.log(updateDocValue);
            const updateDoc = { $set: updateDocValue, };
            const result = await collection.updateOne(query, updateDoc, options);
            if (result.result.ok == 1){
                res.json(RawData);
            } else {
                res.json(req.body);
            }
        }catch(err){
            console.log("Found Server with ID " + err)
            res.json(req.body);
        }finally{
            // Ensures that the client will close when you finish/error
            await client.close();
        }
    } else {
        res.json(req.body);
    }
};

async function CheckPlayerAuth(guid, auth){
    console.log("Checking Player AUTH " + guid);
    var isAuth = false;
    const client = new MongoClient(config.DBServer, { useUnifiedTopology: true });
    try{
        await client.connect();
        // Connect the client to the server        
        const db = client.db(config.DB);
        var collection = db.collection("Players");
        var query = { GUID: guid, AUTH: auth };
        var results = collection.find(query);
            if ((await results.count()) != 0){
                isAuth = true;
            }
    } catch(err){
        console.log("ID " + guid + " err" + err);
    } finally{
        await client.close();
        return isAuth;
    }

}


module.exports = router;