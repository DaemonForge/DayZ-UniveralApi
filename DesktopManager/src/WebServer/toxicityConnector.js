
const tf = require('@tensorflow/tfjs');
const toxicity  = require('@tensorflow-models/toxicity');
const {Router} = require('express');
const log = require("./log");

const {CheckAuth,CheckServerAuth} = require('./AuthChecker')
const {isArray} = require('./utils');

const router = Router();

var RateLimit = require('express-rate-limit');
var limiter = new RateLimit({
  windowMs: 10*1000, // 30 req/sec
  max: global.config.RequestLimitToxicity || 300,
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

router.post('', (req, res)=>{
    runToxicity(req, res, req.headers['Auth-Key']);
});

router.post('/:auth', (req, res)=>{
    runToxicity(req, res, req.params.auth);
});
// The minimum prediction confidence.
const threshold = 0.8;


            
async function runToxicity(req, res, auth, key){
    if ( CheckServerAuth( auth ) || (await CheckAuth( auth )) ){
        try {
            // The minimum prediction confidence.
            // Load the model. Users optionally pass in a threshold and an array of
            // labels to include.
            toxicity.load(threshold).then(model => {
            let query = req.body.Question || req.body.Text;
            const sentences = [query];
            try {
                    model.classify(sentences).then(predictions => {
                        let response = {Status: "Success"};
                        predictions.forEach(e => {
                            let label = e["label"];
                            switch (label){
                                case "identity_attack":
                                    label = "IdentityAttack";
                                    break;
                                case "insult":
                                    label = "Insult";
                                    break;
                                case "obscene":
                                    label = "Obscene";
                                    break;
                                case "severe_toxicity":
                                    label = "SevereToxicity";
                                    break;
                                case "sexual_explicit":
                                    label = "SexualExplicit";
                                    break;
                                case "threat":
                                    label = "Threat";
                                    break;
                                case "toxicity":
                                    label = "Toxicity";
                                    break;
                            }
                            response[label] = Math.round(e.results[0].probabilities[1] * 1000) / 1000;
                        })
                        res.status(200);
                        res.json(response);
                    });
                } catch (e){
                    console.log(e)
                    log(e, "warn")
                    res.status(203);
                    res.json({Status: "Error", Error: `${e}` });
                }
            });
           
        } catch (e){
            console.log(e)
            log(e, "warn")
            res.status(203);
            res.json({Status: "Error", Error: `${e}` });
        }
    } else {
        res.status(203);
        res.json({Status: "NoAuth", Error: "Invalid Auth" });
    }

}

module.exports = router;