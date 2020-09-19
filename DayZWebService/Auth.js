const express = require('express');
const { MongoClient } = require("mongodb");
const config = require('./config.json');
 
const router = express.Router();

router.post('/:GUID/:auth', (req, res)=>{
    if ( req.params.auth == config.ServerAuth ){
        console.log("Auth Token Requested for: " + req.params.GUID);
        runGetAuth(req, res, req.params.GUID, req.params.auth);
    }else{
        res.json({ GUID: req.params.GUID, AuthToken: "ERROR" });
    }
});

async function runGetAuth(req, res, GUID, auth) {
    const client = new MongoClient(config.DBServer, { useUnifiedTopology: true });
    try{

        var StringData = JSON.stringify(req.body);
        var RawData = req.body;
        // Connect the client to the server       
        await client.connect(); 
        console.log("ID " + GUID + " RawData" + RawData);
        const db = client.db(config.DB);
        var collection = db.collection("Players");
        var query = { GUID: GUID };
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