const {Router} = require('express');
const { MongoClient } = require("mongodb");

const log = require("./log")
const fetch  = require('node-fetch');

const {readFileSync} = require('fs');

const CheckAuth = require('./AuthChecker')
const config = require('./configLoader');


const router = Router();
router.post('/:auth', (req, res)=>{
    try{
        //Use this meathod to find the file so that way the config file can be outside of the packaged Applications
        QnAconfig = JSON.parse(readFileSync("QnAMaker.json")); 
        runQnA(req, res, QnAconfig);
    } catch (err){ //If the file doesn't exsit give a nice usable json for DayZ
        console.log("A QnA Request came in but it seems QnAMaker is not set up yet, please go to https://github.com/daemonforge/DayZ-UniveralApi/wiki/Setting-Up-QnA-Maker to learn how to set it up");
        res.json({answer: "error", score: 0});
    }
});

async function runQnA(req, res, QnAconfig){
    if ( req.params.auth == config.ServerAuth || (await CheckAuth( req.params.auth )) ){
        console.log(JSON.stringify(req.body));
        let json;
        try {
            let EndpointKey = "EndpointKey " + QnAconfig.EndpointKey;
            json = await fetch(QnAconfig.Endpoint, { 
                method: 'post', 
                body: JSON.stringify(req.body),
                headers: { "Content-Type": "application/json",
                "Authorization": EndpointKey
            }
            }).then(response => response.json())
        }catch(e) {
            console.log('Catch an error: ', e)
        }
        console.log(json);
        let answer = GetHighestAnwser(json.answers, QnAconfig, req.body.question);
        res.json(answer);
        if (answer.answer === "null" && QnAconfig.LogUnAnswerable){
            WriteQuestionToDataBase(req.body.question);
        }
    }else{
        res.status(401);
        res.json({ Message: "Error"});
        log("AUTH ERROR: " + req.url + " Invalid Server Token", "warn");
    }
}

function GetHighestAnwser(answers, QnAconfig, question){
    let highestScore = QnAconfig.MinScore;
    let answer = {answer: "null", score: 0};
    for (let value of answers) {
        console.log("QnA Maker: Question: \"" + question + "\" Possible Anwser: \"" + value.answer + "\" Score: " + value.score);
        if(value.score > highestScore){
            highestScore = value.score;
            answer.answer = value.answer;
            answer.score = value.score;
        }
    }
    return answer;
}

async function WriteQuestionToDataBase(question){
    const client = new MongoClient(config.DBServer, { useUnifiedTopology: true });
    try{
        // Connect the client to the server       
        await client.connect(); 
        const db = client.db(config.DB);
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