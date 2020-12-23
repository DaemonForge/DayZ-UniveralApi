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
	var hasServerAuth = (auth === config.ServerAuth);
	var hasClientAuth = await CheckAuth(auth, true);
    if ( hasClientAuth || hasServerAuth ){  
        try{
            //console.log(RawData);
            // Connect the client to the server       
            await client.connect(); 
            const db = client.db(config.DB);
            var collection = db.collection("Logs");
            var datetime = new Date();
            var ClientId = GetClientID(req);
            RawData.ServerId = id;
            RawData.LoggedDateTime = datetime;
            RawData.ClientId = ClientId;
			if (hasServerAuth){
				RawData.ClientType = "Server";
			} else if (hasClientAuth){
				RawData.ClientType = "Client";
            }
            
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
	var hasServerAuth = (auth === config.ServerAuth);
	var hasClientAuth = await CheckAuth(auth, true);
    if (hasClientAuth || hasServerAuth){  
        try{
            // Connect the client to the server       
            await client.connect(); 
            const db = client.db(config.DB);
            var collection = db.collection("Logs");
            var datetime = new Date();
            var ClientId = GetClientID(req);
            RawData.forEach(element => {
                element.ServerId = id;
                element.LoggedDateTime = datetime;
				if (hasServerAuth){
					element.ClientType = "Server";
				} else if (hasClientAuth){
					element.ClientType = "Client";
				}
                element.ClientId = ClientId;
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

function GetClientID(req){
    var ip = req.headers['CF-Connecting-IP'] ||  req.headers['x-forwarded-for'] || req.connection.remoteAddress;
    var  hash = crypto.createHash('sha256');
    var theHash = hash.update(ip).digest('base64');
    return theHash.substr(0,32); //Cutting the last few digets to save a bit of data and make sure people don't mistake it for the GUIDS
}


module.exports = router;