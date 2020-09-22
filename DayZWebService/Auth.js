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

router.post('/:GUID/:auth', (req, res)=>{
    if ( req.params.auth == config.ServerAuth ){
        runGetAuth(req, res, req.params.GUID);
    }else{
        res.status(401);
        res.json({ GUID: req.params.GUID, AuthToken: "ERROR" });
        console.log("AUTH ERROR: " + req.url + " Invalid Server Token");
    }
});

async function runGetAuth(req, res, GUID) {
    const client = new MongoClient(config.DBServer, { useUnifiedTopology: true });
    try{
        // Connect the client to the server       
        await client.connect(); 
        const db = client.db(config.DB);
        var collection = db.collection("Players");
        var query = { GUID: GUID };
        const options = { upsert: true };
        var AuthToken = makeAuthToken();
        const updateDocValue  = { GUID: GUID, AUTH: AuthToken }
        const updateDoc = { $set: updateDocValue, };
        const result = await collection.updateOne(query, updateDoc, options);
        if (result.result.ok == 1){
            res.json({GUID: GUID, AUTH: AuthToken});
            console.log("Auth Token Generated for: " + GUID);
        } else {
            res.json({GUID: GUID, AUTH: "ERROR"});
            console.log("Error Generating Auth Token  for: " + GUID);
        }
    }catch(err){
        res.json({GUID: GUID, AUTH: "ERROR"});
        console.log("AUTH ERROR: " + req.url);
    }finally{
        await client.close();
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