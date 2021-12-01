const {Router} = require('express');
const { MongoClient } = require("mongodb");

const log = require("./log")
const fetch  = require('node-fetch');

const {CheckAuth,CheckServerAuth} = require('./AuthChecker')
const {isArray,GenerateLimiter} = require('./utils');

const router = Router();

router.use(GenerateLimiter(global.config.RequestQnAMaker || 100, 5));

let QnAconf;
router.post('', (req, res)=>{
    try{
        //Use this meathod to find the file so that way the config file can be outside of the packaged Applications
        if (QnAconf === undefined && global.config.QnA !== undefined && global.config.QnA["main"] !== undefined){
            QnAconf = global.config.QnA["main"];
        } else if (global.config.QnA === undefined || global.config.QnA["main"] === undefined){
            log("A QnA Request came in but it seems QnAMaker is not set up yet, please go to https://github.com/daemonforge/DayZ-UniveralApi/wiki/Setting-Up-QnA-Maker to learn how to set it up");
            return res.json({Status: "Error", answer: "error", score: 0});
        }
        runQnA(req, res,req.headers['auth-key'], QnAconf);
    } catch (err){ //If the file doesn't exsit give a nice usable json for DayZ
        log("A QnA Request came in but it seems QnAMaker is not set up yet, please go to https://github.com/daemonforge/DayZ-UniveralApi/wiki/Setting-Up-QnA-Maker to learn how to set it up");
        res.json({Status: "Error",answer: "error", score: 0});
    }
});

router.post('/:key', (req, res)=>{
    let key = req.params.key;
    if (global.config.QnA !== undefined && global.config.QnA[key] !== undefined){
        runQnA(req, res, req.headers['auth-key'], global.config.QnA[key]);
    } else {
        //If the file doesn't exsit give a nice usable json for DayZ
        log("A QnA Request came in but it seems QnAMaker is not set up yet, please go to https://github.com/daemonforge/DayZ-UniveralApi/wiki/Setting-Up-QnA-Maker to learn how to set it up");
        res.json({Status: "Error", answer: "error", score: 0});
    }
});

async function runQnA(req, res, auth, QnAconfig){
    if ( CheckServerAuth(auth) || (await CheckAuth( auth )) ){
        let json;
        try {
            let EndpointKey = "EndpointKey " + QnAconfig.EndpointKey;
            let question = req.body.question || req.body.Question || req.body.Text;
            let quest = {question: question}
            json = await fetch(QnAconfig.Endpoint, { 
                method: 'post', 
                body: JSON.stringify(quest),
                headers: { "Content-Type": "application/json",
                "Authorization": EndpointKey
            }
            }).then(response => response.json())
            let answer = GetHighestAnwser(json.answers, QnAconfig, question);
            res.json(answer);
            if (answer.answer === "null" && QnAconfig.LogUnAnswerable){
                WriteQuestionToDataBase(question);
            }
        }catch(e) {
            res.status(400);
            res.json({Status: "Error", Error: `${e}`, answer: "", score: 0, Error:  `${e}`});
            log(`Error Getting response from QnA Endpoint: ${e}`, "warn");
        }
    }else{
        res.status(401);
        res.json({Status: "Error", answer: "Error", score: 0, Error: "Auth Error"});
        log("AUTH ERROR: " + req.url + " Invalid Server Token", "warn");
    }
}

function GetHighestAnwser(answers, QnAconfig, question){
    let highestScore = QnAconfig.MinScore;
    let answer = {Status: "NoAnswer", Error: "No Anwser found above the min score", answer: "null", score: 0};
    for (let value of answers) {
        log("QnA Maker: Question: \"" + question + "\" Possible Anwser: \"" + value.answer + "\" Score: " + value.score);
        if(value.score > highestScore){
            highestScore = value.score;
            answer.Status = "Success";
            answer.Error = "";
            answer.answer = value.answer;
            answer.score = value.score;
        }
    }
    return answer;
}

async function WriteQuestionToDataBase(question){
    const client = new MongoClient(global.config.DBServer, { useUnifiedTopology: true });
    try{
        // Connect the client to the server       
        await client.connect(); 
        const db = client.db(global.config.DB);
        let collection = db.collection("QnAMaker");
        const Doc  = { UnAnweredAbleQuestion: question }
        const result = await collection.insertOne(Doc);
        if (result.modifiedCount === 1 || result.upsertedCount === 1 ){
            log("Logged Question: " + question, "info");
        } else {
            log("Error Logging Question: " + question, "warn");
        }
    }catch(err){
        log("ERROR: " + err, "warn");
    }finally{
        await client.close();
    }
}

module.exports = router;