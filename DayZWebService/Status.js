
const {Router} = require('express');
const { MongoClient } = require("mongodb");
const {CheckAuth,CheckServerAuth} = require('./AuthChecker');
const log = require("./log");
const config = require('./configLoader');
 
const router = Router();

var RateLimit = require('express-rate-limit');
var limiter = new RateLimit({
  windowMs: 10*1000, // 10 req/sec
  max: global.config.RequestLimitStatus || 100,
  message:  '{ "Status": "Error", "Error": "RateLimited" }',
  keyGenerator: function (req /*, res*/) {
    return req.headers['CF-Connecting-IP'] || req.headers['x-forwarded-for'] || req.socket.remoteAddress || req.ip;
  },
  onLimitReached: function (req, res, options) {
    let ip = req.headers['CF-Connecting-IP'] || req.headers['x-forwarded-for'] || req.socket.remoteAddress || req.ip;
    log("RateLimit Reached("  + ip + ") you may be under a DDoS Attack or you may need to increase your request limit");
  },
  skip: function (req, res) {
    let ip = req.headers['CF-Connecting-IP'] || req.headers['x-forwarded-for'] || req.socket.remoteAddress || req.ip;
    if (global.config.RateLimitWhiteList !== undefined && ip !== undefined && ip !== null && isArray(global.config.RateLimitWhiteList) && (global.config.RateLimitWhiteList.find(element => element === ip) === ip)) return true;
    return false;
  }
});

// apply rate limiter to all requests
router.use(limiter);

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

let WitsEnabled = [];
let TranslatesEnabled  = "Disabled";
let QnAEnabled = [];
let LUISEnabled = [];
if (global.config.Wit !== undefined){
    Object.keys(global.config.Wit).forEach(function (k) {
        WitsEnabled.push(k);
    })
}
if (global.config.LUIS !== undefined){
    Object.keys(global.config.LUIS).forEach(function (k) {
        if (global.config.LUIS[k].SubscriptionKey !== ""){
            LUISEnabled.push(k);
        }
    })
}
if (global.config.QnA !== undefined){
    Object.keys(global.config.QnA).forEach(function (k) {
        if (global.config.QnA[k].EndpointKey !== ""){
            QnAEnabled.push(k);
        }
    })
}
if (global.config.Translate !== undefined){
    if ( (global.config.Translate.Type === "LibreTranslate" && global.config.Translate.Endpoint !== "" ) || (global.config.Translate.SubscriptionKey !== "" &&  global.config.Translate.SubscriptionRegion !== "") ){
        TranslatesEnabled = "Enabled";
    }
}


async function runStatusCheck(req, res, auth) {
    const client = new MongoClient(global.config.DBServer, { useUnifiedTopology: true });
    var returnError = "noauth"
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
        if (result.result.ok == 1){
            res.json({Status: "Success", Error: returnError, Version: global.APIVERSION, Discord: global.DISCORDSTATUS, Translate: TranslatesEnabled, Wit: WitsEnabled, QnA: QnAEnabled, LUIS: LUISEnabled });
           // log("Status Check Called", "info"); 
        } else {
            res.status(500);
            res.json({Status: "Error", Error: "Database Write Error", Version: global.APIVERSION, Discord: global.DISCORDSTATUS, Translate: TranslatesEnabled, Wit: WitsEnabled, QnA: QnAEnabled, LUIS: LUISEnabled });
            log("ERROR: Database Write Error", "warn");
        }
    }catch(err){
        console.log(err);
        res.status(500);
        res.json({Status: "Error", Error: `${err}`, Version: global.APIVERSION, Discord: global.DISCORDSTATUS, Translate: TranslatesEnabled, Wit: WitsEnabled, QnA: QnAEnabled, LUIS: LUISEnabled });
        log("ERROR: " + err, "warn");
    }finally{
        // Ensures that the client will close when you finish/error
        client.close();
    }
}

module.exports = router;