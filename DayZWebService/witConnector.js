const {Wit} = require('node-wit');
const {Router} = require('express');
const {isArray, isObject, RemoveBadProperties} = require('./utils')
const {CheckAuth,CheckServerAuth} = require("./AuthChecker");


const log = require("./log");

const router = Router();

var RateLimit = require('express-rate-limit');
var limiter = new RateLimit({
  windowMs: 10*1000, // 30 req/sec
  max: global.config.RequestLimitWit || 300,
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

const clients = {};
  
  
router.post('/:key', (req, res)=>{
    let key = req.params.key;
    //console.log(key);
    if (clients[key] === undefined || clients[key] === null) {
        let token = key;
        if (global.config.Wit !== undefined && global.config.Wit[key] !== undefined){
            token = global.config.Wit[req.params.key];
        }
        log(`Adding New Wit(${key}) to Cache`);
        clients[key] = new Wit({accessToken: token});
        //console.log(clients);
    }
    AskWit(req, res, key, req.headers['auth-key']);
});



async function AskWit(req, res, key, auth){

    if (CheckServerAuth(auth) || (await CheckAuth(auth)) ){
        let RawData = req.body;
        try {
            let question =  RawData.Question || RawData.Text
            let answer = await Ask(question, key);
            log("Wit AI Query: " + question + " Status: " + answer.Status);
            res.status(200);
            res.json(answer);
        } catch (e){
            res.status(401);
            res.json({Status: "Error", Error: `${e}`});
            log(`Error: ${e}`, "warn");
        }
    } else {

        res.status(401);
        res.json({Status: "Error", Error: `Invalid Auth Key`});
        log("AUTH ERROR: " + req.url, "warn");
    }
}


async function Ask(msg, key){

    let res = await clients[key].message(msg)
    //console.log(res);
    let data = RemoveBadProperties(res);
    //console.log(data);
    if (data.intents !== undefined && isArray(data.intents) && data.intents.length > 0){
        data.Status = "Success";
    } else {
        data.Status = "NoIntents";
    }
    return data;
}

module.exports = router;