const {Router} = require('express');
const { MongoClient } = require("mongodb");
const {CheckAuth,CheckServerAuth} = require('./AuthChecker')
const {isArray,GenerateLimiter} = require('./utils');
let {createHash} = require('crypto');

 
const log = require("./log");


const router = Router();


router.use(GenerateLimiter(global.config.RequestLimitLogger || 500, 10));

router.post('/One/:id', (req, res)=>{
    runLoggerOne(req, res,req.params.id, req.headers['auth-key']);
});

router.post('/Many/:id', (req, res)=>{
    runLoggerMany(req, res,req.params.id, req.headers['auth-key']);
});


async function runLoggerOne(req, res, id, auth) {
    const client = new MongoClient(global.config.DBServer, { useUnifiedTopology: true });
    let RawData = req.body;
	let hasServerAuth = CheckServerAuth(auth);
	let hasClientAuth = await CheckAuth(auth, true);
    if ( hasClientAuth || hasServerAuth ){  
        try{
            //console.log(RawData);
            // Connect the client to the server       
            await client.connect(); 
            const db = client.db(global.config.DB);
            let collection = db.collection("Logs");
            let datetime = new Date();
            let ClientId = GetClientID(req);
            RawData.ServerId = id;
            RawData.LoggedDateTime = datetime;
            RawData.ClientId = ClientId;
			if (hasServerAuth){
				RawData.ClientType = "Server";
			} else if (hasClientAuth){
				RawData.ClientType = "Client";
            }
            
            const result = await collection.insertOne(RawData);
            if (result.insertedId != undefined ){
                res.json({Status: "Success", Error: ""});
                log(`New Log Registered from ${RawData.ClientType} - Device ID: ${RawData.ClientId}`);
            // log("Status Check Called", "info"); 
            } else {
                log("ERROR: Database Write Error", "warn");
                res.status(500);
                res.json({Status: "Error", Error: "Database Write Error"});
            }
        }catch(err){
            log("ERROR: " + err, "warn");
            res.status(500);
            res.json({Status: "error", Error: err});
        }finally{
            // Ensures that the client will close when you finish/error
            client.close();
        }
    }
}

async function runLoggerMany(req, res, id, auth) {
    const client = new MongoClient(global.config.DBServer, { useUnifiedTopology: true });
    let RawData = req.body;
	let hasServerAuth = CheckServerAuth(auth);
	let hasClientAuth = await CheckAuth(auth, true);
    if (hasClientAuth || hasServerAuth){  
        try{
            // Connect the client to the server       
            await client.connect(); 
            const db = client.db(global.config.DB);
            let collection = db.collection("Logs");
            let datetime = new Date();
            let ClientId = GetClientID(req);
            let ClientType = "Server";
            RawData.forEach(element => {
                element.ServerId = id;
                element.LoggedDateTime = datetime;
				if (hasServerAuth){
					element.ClientType = "Server";
                    ClientType = "Server";
				} else if (hasClientAuth){
					element.ClientType = "Client";
                    ClientType = "Client";
				}
                element.ClientId = ClientId;
            });
            const result = await collection.insertMany(RawData);
            console.log(result)
            if (result.insertedCount > 0 ){
                res.json({Status: "Success", Error: "" });
                log(`New Log Array Registered from ${ClientType} - Device ID: ${ClientId}`);
            } else {
                res.status(500);
                res.json({Status: "Error", Error: "Database Write Error"});
                log("ERROR: Database Write Error", "warn");
            }
        }catch(err){
            log("ERROR: " + err, "warn");
            res.status(500);
            res.json({Status: "Error", Error: err});
        }finally{
            // Ensures that the client will close when you finish/error
            client.close();
        }
    }
}

function GetClientID(req){
    let ip = req.headers['CF-Connecting-IP'] || req.headers['x-forwarded-for'] || req.socket.remoteAddress || req.ip;
    let  hash = createHash('sha256');
    let theHash = hash.update(ip).digest('base64');
    return theHash.substr(0,32); //Cutting the last few digets to save a bit of data and make sure people don't mistake it for the GUIDS
}


module.exports = router;