
const tf = require('@tensorflow/tfjs');
const toxicity  = require('@tensorflow-models/toxicity');
const {Router} = require('express');
const nodeFetch = require('node-fetch');
global.fetch = nodeFetch;
const log = require("./log");

const {CheckAuth,CheckServerAuth} = require('./AuthChecker')

const router = Router();

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