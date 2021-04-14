const {Router} = require('express');
const { MongoClient } = require("mongodb");

const log = require("./log")
const fetch  = require('node-fetch');

const {CheckAuth,CheckServerAuth} = require('./AuthChecker')

const router = Router();

let QnAconf;
router.post('/:auth', (req, res)=>{
    try{
        //Use this meathod to find the file so that way the config file can be outside of the packaged Applications
        if (QnAconf === undefined && global.config.QnA !== undefined && global.config.QnA["main"] !== undefined){
            QnAconf = global.config.QnA["main"];
        } else if (global.config.QnA === undefined || global.config.QnA["main"] === undefined){
            log("A QnA Request came in but it seems QnAMaker is not set up yet, please go to https://github.com/daemonforge/DayZ-UniveralApi/wiki/Setting-Up-QnA-Maker to learn how to set it up");
            return res.json({answer: "error", score: 0});
        }
        runQnA(req, res, QnAconf);
    } catch (err){ //If the file doesn't exsit give a nice usable json for DayZ
        log("A QnA Request came in but it seems QnAMaker is not set up yet, please go to https://github.com/daemonforge/DayZ-UniveralApi/wiki/Setting-Up-QnA-Maker to learn how to set it up");
        res.json({answer: "error", score: 0});
    }
});
router.post('/:key/:auth', (req, res)=>{
    let key = req.params.key;
    if (global.config.QnA !== undefined && global.config.QnA[key] !== undefined){
        runQnA(req, res, global.config.QnA[key]);
    } else { //If the file doesn't exsit give a nice usable json for DayZ
        log(`A QnA Request came in but it seems QnAMaker for ${key} is not set up yet, please go to https://github.com/daemonforge/DayZ-UniveralApi/wiki/Setting-Up-QnA-Maker to learn how to set it up`);
        res.json({Status: "Error", Error: "Key not configured", answer: "error", score: 0});
    }
});

async function runQnA(req, res, QnAconfig){
    if ( CheckServerAuth(req.params.auth) || (await CheckAuth( req.params.auth )) ){
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
            res.status(200);
            res.json({Status: "Error", answer: "", score: 0, Error:  `${e}`});
            log("AUTH ERROR: " + req.url + " Invalid Server Token", "warn");
            log('Catch an error: ', e)
        }
    }else{
        res.status(401);
        res.json({Status: "Error", answer: "Error", score: 0, Error: ""});
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
        if (result.result.ok == 1){
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