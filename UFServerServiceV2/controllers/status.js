const {Router} = require('express');
const { MongoClient } = require("mongodb");
const {CheckAuth,CheckServerAuth} = require('../auth/utils');
const {isArray,GenerateLimiter, createLogger} = require('../utils');
const logger = createLogger(global.logger, 'status');

const router = Router();

router.use(GenerateLimiter(global.config.RequestLimitStatus || 100, 10));

router.post('', (req, res)=>{
    runStatusCheck(req, res, req.headers['auth-key']);
});

router.get('', (req, res)=>{
    runStatusCheck(req, res, req.headers['auth-key']);
});

async function runStatusCheck(req, res, auth) {
    const { noLog } = req.query;
    const client = new MongoClient(global.config.DBServer);
    var returnError = "NoAuth"
    if (CheckServerAuth(auth) || (await CheckAuth(auth, true))){
        returnError = "noerror"
    }
    try{
        // Connect the client to the server       
        await client.connect(); 
        const db = client.db(global.config.DB);
        var collection = db.collection("Globals");
        var query = { Mod: "UniversalApiStatus"};
        const options = { upsert: true };
        var TestValue = Math.random();
        const updateDocValue  = { Mod: "UniversalApiStatus", Description: "This Object Exsits as a test when ever the status url is called to make sure the database is writeable", TestVar: TestValue }
        const updateDoc = { $set: updateDocValue, };
        const result = await collection.updateOne(query, updateDoc, options);
        if (result.modifiedCount >= 1 || result.upsertedCount >= 1 ){
            res.json({Status: "Success", Error: returnError, Version: global.APIVERSION, Discord: global.DISCORDSTATUS, OpenAI: global.OPENAISTATUS });
            if(!noLog) logger.debug("Status Check Called");
        } else {
            res.status(500);
            res.json({Status: "Error", Error: "Database Write Error", Version: global.APIVERSION, Discord: global.DISCORDSTATUS, OpenAI: global.OPENAISTATUS });
            if(!noLog) logger.warn("Database Write Error", { operation: "status check" });
        }
    }catch(err){
        console.log(err);
        res.status(500);
        res.json({Status: "Error", Error: `Error: ${err}`, Version: global.APIVERSION, Discord: global.DISCORDSTATUS, OpenAI: global.OPENAISTATUS });
        if(!noLog) logger.warn("Status check error", { error: err.message, stack: err.stack });
    }finally{
        // Ensures that the client will close when you finish/error
        client.close();
    }
}

module.exports = router;
