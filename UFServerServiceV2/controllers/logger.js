const { Router } = require('express');
const { MongoClient } = require("mongodb");
const { CheckAuth, CheckServerAuth } = require("../auth/utils.js");
const { isArray, GenerateLimiter } = require('../utils.js');
const { createHash } = require('crypto');

// Remove the old logger import
// const log = require("./log");

const router = Router();
const logger = global.logger;

router.use(GenerateLimiter(global.config.RequestLimitLogger || 500, 10));

router.post('/One/:id', (req, res)=>{
    runLoggerOne(req, res,req.params.id, req.headers['auth-key']);
});

router.post('/Many/:id', (req, res)=>{
    runLoggerMany(req, res,req.params.id, req.headers['auth-key']);
});


async function runLoggerOne(req, res, id, auth) {
    const client = new MongoClient(global.config.DBServer);
    let RawData = req.body;
    let hasServerAuth = CheckServerAuth(auth);
    let hasClientAuth = await CheckAuth(auth, true);
    if ( hasClientAuth || hasServerAuth ){  
        try{
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
                logger.info('New Log Registered', { clientType: RawData.ClientType, clientId: RawData.ClientId });
            } else {
                logger.warn('Database Write Error', { operation: 'insertOne', collection: 'Logs' });
                res.status(500);
                res.json({Status: "Error", Error: "Database Write Error"});
            }
        }catch(err){
            logger.error('Error in runLoggerOne', { error: err.message, stack: err.stack });
            res.status(500);
            res.json({Status: "error", Error: err});
        }finally{
            client.close();
        }
    }
}

async function runLoggerMany(req, res, id, auth) {
    const client = new MongoClient(global.config.DBServer);
    let RawData = req.body;
    let hasServerAuth = CheckServerAuth(auth);
    let hasClientAuth = await CheckAuth(auth, true);
    if (hasClientAuth || hasServerAuth){  
        try{
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
            if (result.insertedCount > 0 ){
                res.json({Status: "Success", Error: "" });
                logger.info('New Log Array Registered', { clientType: ClientType, clientId: ClientId, count: result.insertedCount });
            } else {
                res.status(500);
                res.json({Status: "Error", Error: "Database Write Error"});
                logger.warn('Database Write Error', { operation: 'insertMany', collection: 'Logs' });
            }
        }catch(err){
            logger.error('Error in runLoggerMany', { error: err.message, stack: err.stack });
            res.status(500);
            res.json({Status: "Error", Error: err});
        }finally{
            client.close();
        }
    }
}

function GetClientID(req){
    let ip = req.headers['CF-Connecting-IP'] || req.headers['x-forwarded-for'] || req.socket.remoteAddress || req.ip;
    let  hash = createHash('sha256');
    let theHash = hash.update(ip).digest('base64');
    return theHash.substring(0, 32); 
}


module.exports = router;
