const { Router } = require('express');

const log = require("./log")
const fetch  = require('node-fetch');

const { CheckAuth, CheckServerAuth } = require('./AuthChecker')

const querystring = require('querystring');

const router = Router();





router.post('/:key/:auth', (req, res)=>{
    let key = req.params.key;
    if (global.config.Translate !== undefined && global.config.Translate[key] !== undefined){
        runTranslate(req, res, req.params.auth, key);
    } else { //If the file doesn't exsit give a nice usable json for DayZ
        log(`A Tranlation Request for ${key} is not set up yet`);
        res.json({Status: "Error"});
    }
});

async function runTranslate(req, res, auth, key){
    if ( CheckServerAuth( auth ) || (await CheckAuth( auth )) ){
        try {
            let Tconfig = global.config.Translate[key];
            let text = req.body.Text;
            let querystr = querystring.stringify({
                "api-version": '3.0',
                to: req.body.To,
            });
            //console.log(body)
                let json = await fetch(`${Tconfig.Endpoint}?${querystr}`,{
                    method: "post",
                    headers: {
                    "Ocp-Apim-Subscription-Key": Tconfig.SubscriptionKey,
                    "Ocp-Apim-Subscription-Region": Tconfig.SubscriptionRegion,
                    "Content-Type":"application/json"
                },
                body: JSON.stringify([{"text": text}])
            }).then(response => response.json());
            let response;
            if (json[0] !== undefined && json[0].translations !== undefined ){
                response = {
                    Status: "Success",
                    Translations: json[0].translations,
                    Detected: json[0].detectedLanguage.language
                }
            } else {
                response = {
                    Status: "Error",
                    Translations: [{ text: "NA", to: "NA"} ],
                    Detected: "NA"

                }
            }
            res.status(200);
            res.json(response);
            
        }catch(e) {
            res.status(200);
            res.json({Status: "Error", Error: `${e}`});
            log('Catch an error: ', e)
        }
    }else{
        res.status(401);
        res.json({Status: "Error", Error: "Invlaid Auth Token"});
        log("AUTH ERROR: " + req.url + " Invalid Server Token", "warn");
    }
}

module.exports = router;


/*  ******* Supported Languages *******
    +-----------------------+----------+
    | Language              | Code     |
    +-----------------------+----------+
    | Afrikaans             | af       |
    +-----------------------+----------+
    | Albanian              | sq       |
    +-----------------------+----------+
    | Arabic                | ar       |
    +-----------------------+----------+
    | Bulgarian             | bg       |
    +-----------------------+----------+
    | Catalan               | ca       |
    +-----------------------+----------+
    | Chinese   Simplified  | zh-Hans  |
    +-----------------------+----------+
    | Chinese   Traditional | zh-Hant  |
    +-----------------------+----------+
    | Croatian              | hr       |
    +-----------------------+----------+
    | Czech                 | cs       |
    +-----------------------+----------+
    | Danish                | da       |
    +-----------------------+----------+
    | Dutch                 | nl       |
    +-----------------------+----------+
    | English               | en       |
    +-----------------------+----------+
    | Estonian              | et       |
    +-----------------------+----------+
    | Finnish               | fi       |
    +-----------------------+----------+
    | French                | fr       |
    +-----------------------+----------+
    | German                | de       |
    +-----------------------+----------+
    | Greek                 | el       |
    +-----------------------+----------+
    | Gujarati              | gu       |
    +-----------------------+----------+
    | Haitian   Creole      | ht       |
    +-----------------------+----------+
    | Hebrew                | he       |
    +-----------------------+----------+
    | Hindi                 | hi       |
    +-----------------------+----------+
    | Hungarian             | hu       |
    +-----------------------+----------+
    | Icelandic             | is       |
    +-----------------------+----------+
    | Indonesian            | id       |
    +-----------------------+----------+
    | Inuktitut             | iu       |
    +-----------------------+----------+
    | Irish                 | ga       |
    +-----------------------+----------+
    | Italian               | it       |
    +-----------------------+----------+
    | Japanese              | ja       |
    +-----------------------+----------+
    | Klingon               | tlh-Latn |
    +-----------------------+----------+
    | Korean                | ko       |
    +-----------------------+----------+
    | Kurdish   (Central)   | ku-Arab  |
    +-----------------------+----------+
    | Latvian               | lv       |
    +-----------------------+----------+
    | Lithuanian            | lt       |
    +-----------------------+----------+
    | Malay                 | ms       |
    +-----------------------+----------+
    | Maltese               | mt       |
    +-----------------------+----------+
    | Norwegian             | nb       |
    +-----------------------+----------+
    | Pashto                | ps       |
    +-----------------------+----------+
    | Persian               | fa       |
    +-----------------------+----------+
    | Polish                | pl       |
    +-----------------------+----------+
    | Portuguese            | pt       |
    +-----------------------+----------+
    | Romanian              | ro       |
    +-----------------------+----------+
    | Russian               | ru       |
    +-----------------------+----------+
    | Serbian   (Cyrillic)  | sr-Cyrl  |
    +-----------------------+----------+
    | Serbian   (Latin)     | sr-Latn  |
    +-----------------------+----------+
    | Slovak                | sk       |
    +-----------------------+----------+
    | Slovenian             | sl       |
    +-----------------------+----------+
    | Spanish               | es       |
    +-----------------------+----------+
    | Swahili               | sw       |
    +-----------------------+----------+
    | Swedish               | sv       |
    +-----------------------+----------+
    | Tahitian              | ty       |
    +-----------------------+----------+
    | Thai                  | th       |
    +-----------------------+----------+
    | Turkish               | tr       |
    +-----------------------+----------+
    | Ukrainian             | uk       |
    +-----------------------+----------+
    | Urdu                  | ur       |
    +-----------------------+----------+
    | Vietnamese            | vi       |
    +-----------------------+----------+
    | Welsh                 | cy       |
    +-----------------------+----------+
    | Yucatec   Maya        | yua      |
    +-----------------------+----------+
*/