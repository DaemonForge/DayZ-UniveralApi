const {Router} = require('express');
const fetch  = require('node-fetch');

const log = require("./log");

const {CheckAuth,CheckServerAuth} = require('./AuthChecker')

const config = require('./configLoader');

/*
    let Request.URL
    let Request.Headers[].Key 
    let Request.Headers[].Value
    let Request.Body
    let Request.Method
    let Request.ReturnValue = "" //Will look for this specific Key to return as the data
*/

const router = Router();
router.post('/:auth', (req, res)=>{
    runFowarder(req, res, req.params.auth)
});
async function runFowarder(req, res, auth){
    let RawData = req.body;
    if ( CheckServerAuth(auth) || (await CheckAuth( auth )) ){
        log("Fowarded Called URL: " + RawData.URL );
        let strHeaders = "{";
        let json;
        for (let header of RawData.Headers) {
            strHeaders = strHeaders + " \"" + header.Key + "\": \"" + header.Value + "\",";
        }
        let strHeadersLen = strHeaders.length - 1; //Remove the last extra ','
        strHeaders = strHeaders.substring(0,strHeadersLen);
        strHeaders = strHeaders + " }";
        let Headers = JSON.parse(strHeaders);
        //console.log(RawData.Body)
        try {
        json = await fetch(RawData.URL, { 
            method: RawData.Method, 
            body: RawData.Body,
            headers: Headers
        }).then(response => response.json());
        }catch(e) {
            console.log('Catch an error: ', e)
        }
        //console.log(json);
        let ReturnValue;
        if (RawData.ReturnValue != ""){
            try{
                ReturnValue = json[RawData.ReturnValue];
                if (RawData.ReturnValueArrayIndex >= 0){
                    ReturnValue = json[RawData.ReturnValue][RawData.ReturnValueArrayIndex];
                }
            } catch(err) {
                log("Error Trying to get Return Value " + RawData.ReturnValue + " from response " + err, "warn");
                ReturnValue = json;
            }
        } else {
            ReturnValue = json;
            try{
                if (RawData.ReturnValueArrayIndex >= 0){
                    ReturnValue = json[RawData.ReturnValueArrayIndex];
                }
            } catch(err) {
                log("Error Trying to get Return index: " + err, "warn");
                ReturnValue = json;
            }
        }
        res.json(ReturnValue);
    }else{
        res.status(401);
        res.json({});
        log("AUTH ERROR: " + req.url + " Invalid Token", "warn");
    }
}


module.exports = router;