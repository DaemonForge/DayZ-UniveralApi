const {Wit} = require('node-wit');
const {Router} = require('express');
const {isArray, isObject, RemoveBadProperties,GenerateLimiter,IncermentAPICount} = require('../utils')


const log = require("../log");

const router = Router();

// apply rate limiter to all requests
router.use(GenerateLimiter(global.config.RequestLimitWit || 300, 10));

const clients = {};
  
  
router.post('/:key', (req, res)=>{
    let key = req.params.key;
    //console.log(key);
    if (clients[key] === undefined || clients[key] === null) {
        let token = key;
        if (req.ClientInfo.Wit !== undefined && req.ClientInfo.Wit[key] !== undefined){
            token = req.ClientInfo.Wit[req.params.key];
        }
        log(`Adding New Wit(${key}) to Cache`);
        clients[key] = new Wit({accessToken: token});
        //console.log(clients);
    }
    AskWit(req, res, key, req.headers['auth-key']);
});



async function AskWit(req, res, key, auth){
        let RawData = req.body;
        try {
            let question =  RawData.Question || RawData.Text
            let answer = await Ask(question, key);
            log("Wit AI Query: " + question + " Status: " + answer.Status);
            res.status(200);
            res.json(answer);
            IncermentAPICount(req.ClientInfo.ClientId);
        } catch (e){
            res.status(401);
            res.json({Status: "Error", Error: `${e}`});
            log(`Error: ${e}`, "warn");
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