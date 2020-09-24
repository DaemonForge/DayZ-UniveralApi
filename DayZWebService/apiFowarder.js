const express = require('express');
const { MongoClient } = require("mongodb");
const fetch  = require('node-fetch');

const CheckAuth = require('./AuthChecker')

const config = require('./configLoader');

/*
    var Request.URL
    var Request.Headers[].Key 
    var Request.Headers[].Value
    var Request.Type = "post"
    var Request.Body
    var Request.Method
    var Request.ReturnValue = "" //Will look for this specific Key to return as the data
*/

const router = express.Router();
router.post('/:auth', (req, res)=>{
    runFowarder(req, res, req.params.auth)
});
async function runFowarder(req, res, auth){
    var RawData = req.body;
    if ( auth == config.ServerAuth || (await CheckAuth( auth )) ){
        var strHeaders = "{";
        
        for (var header of RawData.Headers) {
            strHeaders = strHeaders + " \"" + header.Key + "\": \"" + header.Value + "\",";
        }
        var strHeadersLen = strHeaders.length() - 1; //Remove the last extra ','
        strHeaders = strHeaders.substring(0,strHeadersLen);
        strHeaders = strHeaders + " }";
        var Headers = JSON.parse(strHeaders);
        var response = {};
        json = await fetch(RawData.URL, { 
            method: RawData.Method, 
            body: JSON.stringify(RawData.body),
            headers: Headers
        }).then(response => response.json());
        if (RawData.ReturnValue != ""){
            for (const [key, value] of Object.entries(json)) {
                if(key === RawData.ReturnValue){
                    response = value;
                    console.log("Retrieving "+ RawData.ReturnValue + " Data for GUID: " + GUID);
                }
            }
        } else {
            response = json;
        }
        res.json(response);
    }else{
        res.status(401);
        res.json({});
        console.log("AUTH ERROR: " + req.url + " Invalid Token");
    }
}


module.exports = router;