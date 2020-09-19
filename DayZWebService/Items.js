const express = require('express');
const { MongoClient } = require("mongodb");
const config = require('./config.json');
 
// Create a new MongoClient


const router = express.Router();

router.post('/Load/:ItemId/:mod/:auth', (req, res)=>{
    runGet(req, res, req.params.ItemId, req.params.mod, req.params.auth);
});

router.post('/Save/:ItemId/:mod/:auth', (req, res)=>{
    runUpdate(req, res, req.params.ItemId, req.params.mod, req.params.auth);
});

async function runGet(req, res, ItemId, mod, auth) {
    if (auth == config.ServerAuth || (await CheckPlayerAuth(auth)) ){
        const client = new MongoClient(config.DBServer, { useUnifiedTopology: true });
        var StringData = JSON.stringify(req.body);
        var RawData = req.body;
        try{

            // Connect the client to the server
            await client.connect();
            const db = client.db(config.DB);
            var collection = db.collection("Items");
            var query = { ItemId: ItemId };
            var results = collection.find(query);
            
            if ((await results.count()) == 0){
                if (auth == config.ServerAuth || config.AllowClientWrite ){
                    console.log("Can't find Item with ID " + ItemId + "Creating it now");

                    const doc  = JSON.parse("{ \"ItemId\": \"" + ItemId + "\", \""+mod+"\": "+ StringData + " }");
                    var result = await collection.insertOne(doc);
                    var Data = result.ops[0];
                }
                res.json(RawData);
            } else {
                var dataarr = await results.toArray(); 
                var data = dataarr[0]; 
                var sent = false;
                for (const [key, value] of Object.entries(data)) {
                    if(key === mod){
                        var sent = true;
                        res.json(value);
                    }
                }
                if (sent != true){
                    const updateDocValue  = JSON.parse("{ \""+mod+"\": "+ StringData + " }");
                    const updateDoc = { $set: updateDocValue, };
                    const options = { upsert: false };
                    const result = await collection.updateOne(query, updateDoc, options);
                    res.json(RawData);
                }
            }
        }catch(err){
            console.log("Found Item with ID " + err)
            res.json(RawData);
            return res;
        }finally{
            // Ensures that the client will close when you finish/error
            await client.close();
            return res;
        }
    }  else {
        res.json(req.body);
    }
};
async function runUpdate(req, res, ItemId, mod, auth) {  
    if (auth == config.ServerAuth || ((await CheckPlayerAuth(auth)) && config.AllowClientWrite) ){
        const client = new MongoClient(config.DBServer, { useUnifiedTopology: true });
        var StringData = JSON.stringify(req.body);
        var RawData = req.body;
        try{

            // Connect the client to the server
            await client.connect();
            
            await client.db(config.DB).command({ ping: 1 });
            console.log("ID " + ItemId + " req" + req);
            const db = client.db(config.DB);
            var collection = db.collection("Items");
            var query = { ItemId: ItemId };
            const options = { upsert: true };
            const updateDocValue  = JSON.parse("{ \"ItemId\": \"" + ItemId + "\", \""+mod+"\": "+ StringData + " }");
            const updateDoc = { $set: updateDocValue, };
            const result = await collection.updateOne(query, updateDoc, options);
            res.json(RawData);
        }catch(err){
            console.log("err " + err)
            res.json(RawData);
        }finally{
            // Ensures that the client will close when you finish/error
            await client.close();
        }
    }  else {
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