const express = require('express');
const { MongoClient } = require("mongodb");
const CheckAuth = require('./AuthChecker')
var crypto = require('crypto');

const config = require('./configLoader');
 
const log = require("./log");


const router = express.Router();


router.post('/One/:id/:auth', (req, res)=>{
    runLoggerOne(req, res,req.params.id, req.params.auth);
});

router.post('/Many/:id/:auth', (req, res)=>{
    runLoggerMany(req, res,req.params.id, req.params.auth);
});
async function runLoggerOne(req, res, id, auth) {
    const client = new MongoClient(config.DBServer, { useUnifiedTopology: true });
    var RawData = req.body;
    if (auth === config.ServerAuth || (await CheckAuth(auth, true))){  
        try{
            //console.log(RawData);
            // Connect the client to the server       
            await client.connect(); 
            const db = client.db(config.DB);
            var collection = db.collection("Logs");
            var datetime = new Date();
            RawData.ServerId = id;
            RawData.LoggedDateTime = datetime;
            console.log(RawData);
            const result = await collection.insertOne(RawData);
            if (result.result.ok == 1){
                res.json({Status: "ok"});
            // log("Status Check Called", "info"); 
            } else {
                res.status(500);
                res.json({Status: "error", Error: "Database Write Error"});
                log("ERROR: Database Write Error", "warn");
            }
        }catch(err){
            res.status(500);
            res.json({Status: "error", Error: err});
            log("ERROR: " + err, "warn");
        }finally{
            // Ensures that the client will close when you finish/error
            client.close();
        }
    }
}
async function runLoggerMany(req, res, id, auth) {
    const client = new MongoClient(config.DBServer, { useUnifiedTopology: true });
    var RawData = req.body;
    if (auth === config.ServerAuth || (await CheckAuth(auth, true))){  
        try{
            // Connect the client to the server       
            await client.connect(); 
            const db = client.db(config.DB);
            var collection = db.collection("Logs");
            var datetime = new Date();
            RawData.forEach(element => {
                element.ServerId = id;
                element.LoggedDateTime = datetime;
            });
            const result = await collection.insertMany(RawData);
            if (result.result.ok == 1){
                res.json({Status: "ok"});
            } else {
                res.status(500);
                res.json({Status: "error", Error: "Database Write Error"});
                log("ERROR: Database Write Error", "warn");
            }
        }catch(err){
            res.status(500);
            res.json({Status: "error", Error: err});
            log("ERROR: " + err, "warn");
        }finally{
            // Ensures that the client will close when you finish/error
            client.close();
        }
    }
}

module.exports = router;