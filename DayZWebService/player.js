const { json } = require('body-parser');
const e = require('express');
const express = require('express');
const { MongoClient } = require("mongodb");
const config = require('./config.json');
 
// Create a new MongoClient


const router = express.Router();

router.post('/Load/:GUID/:mod/:auth', (req, res)=>{
    runGet(req, res, req.params.GUID, req.params.mod, req.params.auth, false);
});

router.post('/Save/:GUID/:mod/:auth', (req, res)=>{
    runUpdate(req, res, req.params.GUID, req.params.mod, req.params.auth, true);
});

router.post('/GetAuth/:GUID/:auth', (req, res)=>{
    if ( req.params.auth == config.ServerAuth ){
        console.log("Auth Token Requested for: " + req.params.GUID);
        runGetAuth(req, res, req.params.GUID, req.params.auth);
    }else{
        res.json({ GUID: req.params.GUID, AuthToken: "NULL" });
    }
});


async function runGet(req, res, GUID, mod, auth) {
    if ((await CheckPlayerAuth(GUID, auth)) || (auth == config.ReadAllAuth) || (auth == config.ServerAuth) ){
        const client = new MongoClient(config.DBServer, { useUnifiedTopology: true });
        try{

            await client.connect();
            // Connect the client to the server
            console.log("ID " + GUID + " req" + req);
            const db = client.db(config.DB);
            var collection = db.collection("players");
            var query = { GUID: GUID };
            var results = collection.find(query);
            var StringData = JSON.stringify(req.body);
            var RawData = req.body;
            
            if ((await results.count()) == 0){
                console.log("Can't find Player with ID " + GUID + "Creating it now");

                const doc  = JSON.parse("{ \"GUID\": \"" + GUID + "\", \""+mod+"\": "+ StringData + " }");
                var result = await collection.insertOne(doc);
                var Data = result.ops[0];
                res.json(RawData);
                
            } else {
                var dataarr = await results.toArray(); 
                var data = dataarr[0]; 
                console.log("GUID " + GUID + " data: " + data)
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
                    const result = await collection.updateOne(query, updateDoc, options);
                    res.json(RawData);
                }
            }
        }catch(err){
            console.log("runGet Error: " + err)
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
    if ( (write != false && auth == config.ReadAllAuth) || auth == config.ServerAuth || ((await CheckPlayerAuth(GUID, auth)) && config.AllowClientWrite) ){
        const client = new MongoClient(config.DBServer, { useUnifiedTopology: true });
        try{
            await client.connect();

            var StringData = JSON.stringify(req.body);
            var RawData = req.body;
            // Connect the client to the server
            console.log("ID " + GUID + " StringData" + StringData);
            const db = client.db(config.DB);
            var collection = db.collection("players");
            var query = { GUID: GUID };
            const options = { upsert: true };
            const jsonString = "{ \"GUID\": \""+GUID+"\", \""+mod+"\": "+ StringData + " }";
            
            console.log("jsonString" + jsonString);
            const updateDocValue  = JSON.parse(jsonString);
            console.log(updateDocValue);
            const updateDoc = { $set: updateDocValue, };
            const result = await collection.updateOne(query, updateDoc, options);
            res.json(RawData);
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
    console.log("Checking Player AUTH");
    var isAuth = false;
    const client = new MongoClient(config.DBServer, { useUnifiedTopology: true });
    try{
        await client.connect();
        // Connect the client to the server        
        console.log("ID " + guid + " auth" + auth);
        const db = client.db(config.DB);
        var collection = db.collection("players");
        var query = { GUID: guid, AUTH: auth };
        var results = collection.find(query);
            if ((await results.count()) != 0){
                isAuth = true;
            }
    } catch(err){
        console.log("ID " + guid + " auth" + auth + " err" + err);
    } finally{
        await client.close();
        return isAuth;
    }

}

async function runGetAuth(req, res, GUID, auth) {
    const client = new MongoClient(config.DBServer, { useUnifiedTopology: true });
    try{

        var StringData = JSON.stringify(req.body);
        var RawData = req.body;
        // Connect the client to the server       
        await client.connect(); 
        await client.db(config.DB).command({ ping: 1 });
        console.log("ID " + GUID + " RawData" + RawData);
        const db = client.db(config.DB);
        var collection = db.collection("players");
        var query = { GUID: GUID };
        var results = collection.find(query);
        var reqData = req.body.Data;
        const options = { upsert: true };
        var AuthToken = makeAuthToken();
        const updateDocValue  = { GUID: GUID, AUTH: AuthToken }
        const updateDoc = { $set: updateDocValue, };
        const result = await collection.updateOne(query, updateDoc, options);
        console.log(result.result);
        res.json({GUID: GUID, AUTH: AuthToken});
    }catch(err){
        console.log("Found Server with ID " + err)
        res.json({GUID: GUID, AUTH: "ERROR"});
    }finally{
        // Ensures that the client will close when you finish/error

        await client.close();
        return res;
    }
};
function makeAuthToken() {
    var result           = '';
    var characters       = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-.~()*:@,;';
    var charactersLength = characters.length;
    for ( var i = 0; i < 44; i++ ) {
       result += characters.charAt(Math.floor(Math.random() * charactersLength));
    }
    return result;
 }

module.exports = router;