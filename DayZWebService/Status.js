const express = require('express');
const { MongoClient } = require("mongodb");
const CheckAuth = require('./AuthChecker');
const log = require("./log");
const config = require('./configLoader');
 
const router = express.Router();

// Create a new MongoClient
router.post('/', (req, res)=>{
    runStatusCheck(req, res);
});

// Create a new MongoClient
router.post('/:Auth', (req, res)=>{
    runStatusCheck(req, res, req.params.Auth);
});

router.get('/', (req, res)=>{
    runStatusCheck(req, res);
});



async function runStatusCheck(req, res, auth) {
    const client = new MongoClient(config.DBServer, { useUnifiedTopology: true });
    var returnError = "noauth"
    if (auth === config.ServerAuth || (await CheckAuth(auth, true))){
        returnError = "noerror"
    }
    try{
        // Connect the client to the server       
        await client.connect(); 
        const db = client.db(config.DB);
        var collection = db.collection("Globals");
        var query = { Mod: "UniversalApiStatus"};
        const options = { upsert: true };
        var TestValue = Math.random();
        const updateDocValue  = { Mod: "UniversalApiStatus", Description: "This Object Exsits as a test when ever the status url is called to make sure the database is writeable", TestVar: TestValue }
        const updateDoc = { $set: updateDocValue, };
        const result = await collection.updateOne(query, updateDoc, options);
        if (result.result.ok == 1){
            res.json({Status: "ok", Error: returnError});
            log("Status Check Called", "info");
        } else {
            res.status(500);
            res.json({Status: "error", Error: "Database Write Error"});
            log("ERROR: Database Write Error", "warn");
        }
    }catch(err){
        console.log(err);
        res.status(500);
        res.json({Status: "error", Error: err});
        log("ERROR: " + err, "warn");
    }finally{
        // Ensures that the client will close when you finish/error
        client.close();
    }
}

module.exports = router;