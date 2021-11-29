const { Router } = require('express');
const { MongoClient } = require("mongodb");

const log = require("../log")
const fetch  = require('node-fetch');

const {isArray,GenerateLimiter,IncermentAPICount} = require('../utils');

const querystring = require('querystring');

const router = Router();

router.use(GenerateLimiter(global.config.RequestLimitLUIS || 200, 10));

router.post('/:key', (req, res)=>{
    let key = req.params.key;
    if (req.ClientInfo.LUIS !== undefined && req.ClientInfo.LUIS[key] !== undefined){
        runLUIS(req, res, req.headers['auth-key'], key);
    } else { //If the file doesn't exsit give a nice usable json for DayZ
        log(`A LUIS Request for ${key} is not set up yet`);
        res.json({Status: "Error"});
    }
});

async function runLUIS(req, res, auth, key){
        try {
            let luisconfig = req.ClientInfo.LUIS[key];
            let query = req.body.Question || req.body.Text;
            let showall = (req.body.showall === 1);
            let querystr = querystring.stringify({
                query: query,
                "show-all-intents": showall,
                "subscription-key": luisconfig.SubscriptionKey,
                "verbose": luisconfig.Verbose,
                "log": luisconfig.Log
            });
            let json = await fetch(`${luisconfig.Endpoint}?${querystr}`).then(response => response.json());
            if (json.prediction.topIntent != "None"){
                json["Status"] = "Success";
            } else {
                json["Status"] = "NoIntent";

            }
            res.status(200);
            res.json(json);
            IncermentAPICount(req.ClientInfo.ClientId);
            log(`Queries ${key} with question ${query} - ${json.Status}`)
            
        }catch(e) {
            res.status(200);
            res.json({Status: "Error", Error: `${e}`});
            log('Catch an error: ', e)
        }
}





module.exports = router;