const express = require('express');
const { MongoClient } = require("mongodb");
const fetch  = require('node-fetch');
const fs = require('fs');
const Defaultconfig = require('./sample-config.json');
const ConfigPath = "config.json"
var config;
try{
  config = JSON.parse(fs.readFileSync(ConfigPath));
} catch (err){
  config = Defaultconfig;
}
 
const router = express.Router();
router.post('/:auth', (req, res)=>{
    try{
        QnAconfig = JSON.parse(fs.readFileSync("QnAMaker.json"));
        runQnA(req, res, QnAconfig);
    } catch (err){
        res.json({answer: "error", score: 0});
    }
});

async function runQnA(req, res, QnAconfig){
    if ( req.params.auth == config.ServerAuth || (await CheckPlayerAuth( req.params.auth )) ){
        var EndpointKey = "EndpointKey " + QnAconfig.EndpointKey;
        json = await fetch(QnAconfig.Endpoint, { 
            method: 'post', 
            body: JSON.stringify(req.body),
            headers: { 'Content-Type': 'application/json',
            "Authorization": EndpointKey
        }
        }).then(response => response.json())
        //console.log(json);
        var answer = GetHighestAnwser(json.answers, QnAconfig, req.body.question);
        res.json(answer);
    }else{
        res.status(401);
        res.json({ Message: "Error"});
        console.log("AUTH ERROR: " + req.url + " Invalid Server Token");
    }
}

function GetHighestAnwser(answers, QnAconfig, question){
    var highestScore = QnAconfig.MinScore;
    var answer = {answer: "null", score: 0};
    for (var value of answers) {
        console.log("QnA Maker: Question: \"" + question + "\" Possible Anwser: \"" + value.answer + "\" Score: " + value.score);
        if(value.score > highestScore){
            highestScore = value.score;
            answer.answer = value.answer;
            answer.score = value.score;
        }
    }
    return answer;
}




async function CheckPlayerAuth(auth){
    var isAuth = false;
    const client = new MongoClient(config.DBServer, { useUnifiedTopology: true });
    try{
        await client.connect();
        // Connect the client to the server      
        const db = client.db(config.DB);
        var collection = db.collection("Players");
        var query = { AUTH: auth };
        var results = collection.find(query);
            if ((await results.count()) != 0){
                isAuth = true;
            }
    } catch(err){
        console.log(" auth" + auth + " err" + err);
    } finally{
        await client.close();
        return isAuth;
    }

}

module.exports = router;