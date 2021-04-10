const {Wit} = require('node-wit');
const {Router} = require('express');
const {isArray, isObject, RemoveBadProperties} = require('./utils')
const {CheckAuth,CheckServerAuth} = require("./AuthChecker");
const config = require('./configLoader');


const log = require("./log");

const router = Router();


const clients = {};
  
  
router.post('/:key/:auth', (req, res)=>{
    let key = req.params.key;
    //console.log(key);
    if (clients[key] === undefined || clients[key] === null) {
        let token = key;
        if (config.Wit !== undefined && config.Wit[key] !== undefined){
            token = config.Wit[req.params.key];
        }
        log(`Adding New Wit(${key}) to Cache`);
        clients[key] = new Wit({accessToken: token});
        //console.log(clients);
    }
    AskWit(req, res, key, req.params.auth);
});



async function AskWit(req, res, key, auth){

    if (CheckServerAuth(auth) || (await CheckAuth(auth)) ){
        let RawData = req.body;
        try {
            let answer = await Ask(RawData.Question, key);
            log("Wit AI Query: " + RawData.Question);
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
    if (data["intents"] !== undefined && isArray(data['intents']) && data['intents'].length > 0){
        data["Status"] = "Success";
    } else {
        data["Status"] = "NoIntents";
    }
    return data;
}

module.exports = router;