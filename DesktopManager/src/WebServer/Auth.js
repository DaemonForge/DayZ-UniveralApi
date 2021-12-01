const {Router} = require('express');
const { MongoClient, MongoError  } = require("mongodb");
let {createHash} = require('crypto');
const {makeAuthToken, CheckServerAuth} = require('./AuthChecker')

const log = require("./log");

const router = Router();

/**
 *  Generate Auth Token
 *  Post: Auth/[GUID]
 *  
 *  Description: This generates a auth token for the specified GUID and 
 *   updates the database so that way we can validate that the user has 
 *   already be issued a new. The AUTHTOKEN will expire 
 * 
 *  Returns: `{ 
 *                "GUID": "|THEPASSEDGUID|", 
 *                "AUTH": "|AUTHTOKEN|" 
 *            }`
 * 
 */
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
        let result = await collection.updateOne(query, updateDoc, options);
        if ( result.matchedCount === 1 || result.upsertedCount === 1 ){
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