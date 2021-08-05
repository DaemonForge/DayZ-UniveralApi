const {Router} = require('express');
const { MongoClient, MongoError  } = require("mongodb");
let {createHash} = require('crypto');
const {makeAuthToken, CheckServerAuth} = require('./AuthChecker')

const log = require("./log");

const router = Router();

router.post('/:GUID', (req, res)=>{
    if ( CheckServerAuth(req.headers['auth-key']) ){
        runGetAuth(req, res, req.params.GUID);
    }else{
        res.status(401);
        res.json({ GUID: req.params.GUID, AuthToken: "ERROR" });
        log("AUTH ERROR: " + req.url + " Invalid Server Token", "warn");
    }
});



async function runGetAuth(req, res, GUID) {
    const client = new MongoClient(global.config.DBServer, { useUnifiedTopology: true });
    try{
        // Connect the client to the server       
        await client.connect(); 
        const db = client.db(global.config.DB);
        let collection = db.collection("Players");
        let query = { GUID: GUID };
        const options = { upsert: true };
        let AuthToken = makeAuthToken(GUID);
        let SaveToken = createHash('sha256').update(AuthToken).digest('base64');
        const updateDocValue  = { GUID: GUID, AUTH: SaveToken }
        const updateDoc = { $set: updateDocValue, };
        const result = await collection.updateOne(query, updateDoc, options);
        if (result.result.ok == 1){
            res.json({GUID: GUID, AUTH: AuthToken});
            log("Auth Token Generated for: " + GUID);
        } else {
            res.json({GUID: GUID, AUTH: "ERROR"});
            log("Error Generating Auth Token  for: " + GUID, "warn");
        }
    }catch(err){
        res.json({GUID: GUID, AUTH: "ERROR"});
        log("AUTH ERROR: " + req.url + " error: " + err, "warn");
    }finally{
        await client.close();
    }
};


module.exports = router;