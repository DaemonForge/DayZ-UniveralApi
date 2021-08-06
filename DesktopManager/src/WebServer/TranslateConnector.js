const { Router } = require('express');

const log = require("./log")
const fetch  = require('node-fetch');
const LanguageDetect = require('languagedetect');
const lngDetector = new LanguageDetect();
const { CheckAuth, CheckServerAuth } = require('./AuthChecker')
const {isArray,GenerateLimiter} = require('./utils');

const querystring = require('querystring');

const router = Router();


router.use(GenerateLimiter(global.config.RequestLimitTranslate || 200, 10));

router.post('', (req, res)=>{
    if (global.config.Translate !== undefined && global.config.Translate.Type == "Microsoft" && global.config.Translate.SubscriptionKey !== ""){

        runTranslate(req, res, req.headers['auth-key']);
    } else if (global.config.Translate !== undefined &&  global.config.Translate.Type == "LibreTranslate"){
        runLibreTranslate(req, res, req.headers['auth-key']);
    } else { //If the file doesn't exsit give a nice usable json for DayZ
        log(`A Tranlation Request came in but isn't set up yet`);
        res.json({Status: "Error"});
    }
});

async function runTranslate(req, res, auth){
    if ( CheckServerAuth( auth ) || (await CheckAuth( auth )) ){
        try {
            let Tconfig = global.config.Translate;
            let text = req.body.Text;
            let lang = req.body.From;
            let queryobj = {
                "api-version": '3.0',
                to: req.body.To
            }
            if (lang !== undefined && lang != "" && lang.toLowerCase() != "auto" ) { 
                queryobj.from = lang;
            }
            let querystr = querystring.stringify(queryobj);
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
            //console.log(json)
            if (json[0] !== undefined && json[0].translations !== undefined ){
                response = {
                    Status: "Success",
                    Error: "",
                    Translations: json[0].translations,
                    Detected: json[0].detectedLanguage.language
                }
            } else {
                let error = "Not a valid response from the API"
                if (json.error !== undefined ){
                    error = json.error.message;
                }
                log(`Translation an error: ${error}`)
                response = {
                    Status: "Error",
                    Error: error,
                    Translations: [{ text: "NA", to: "NA"} ],
                    Detected: "NA"
                }
            }
            log(`Translation successfully request proccessed with Microsoft Translate`);
            res.status(200);
            res.json(response);
            
        }catch(e) {
            res.status(200);
            res.json({Status: "Error", Error: `${e}`, Translations: [{ text: "NA", to: "NA"} ], Detected: "NA"});
            log(`Translation an error: ${e}`)
        }
    }else{
        res.status(401);
        res.json({Status: "Error", Error: "Invlaid Auth", Translations: [{ text: "NA", to: "NA"} ], Detected: "NA"});
        log("AUTH ERROR: " + req.url + " Invalid Server Token", "warn");
    }
}

async function runLibreTranslate(req, res, auth){
    if ( CheckServerAuth( auth ) || (await CheckAuth( auth )) ){
        try {
            let Tconfig = global.config.Translate;
            //console.log(req.body)
            let text = req.body.Text;
            let lang = req.body.From;
            let to = req.body.To[0];
            to = to.toLowerCase();
            lang = lang.toLowerCase();
            //console.log(text);
            if (lang === undefined || lang === ""){
                lngDetector.setLanguageType("iso2")
                let DetectedLang = await lngDetector.detect(text, 10);
                //console.log(DetectedLang[0]);
                lang =DetectedLang[0][0];
                //console.log(lang);
                //console.log(body)
            }
            let json = await fetch(`${Tconfig.Endpoint}`,{
                method: "post",
                headers: {
                    "Content-Type":"application/json"
                },
                body: JSON.stringify({q: text, source: `${lang}`, target: to})
            }).then(response => { return response.json()});
            //console.log(await json)
            if (json !== undefined && json.translatedText !== undefined ){
                response = {
                    Status: "Success",
                    Error: "",
                    Translations: [{ text: json.translatedText, to: to.toUpperCase()} ],
                    Detected: lang.toUpperCase()
                }
            } else {
                let error = "Not a valid response from the API"
                if (json.error !== undefined ){
                    error = json.error.message;
                }
                log(`Translation an error: ${error}`)
                response = {
                    Status: "Error",
                    Error: error,
                    Translations: [{ text: "NA", to: "NA"} ],
                    Detected: "NA"
                }
            }
            log(`Translation successfully request proccessed with Libre Translate`);
            res.status(200);
            res.json(response);
            
        }catch(e) {
            console.log(e);
            res.status(200);
            res.json({Status: "Error", Error: `${e}`, Translations: [{ text: "NA", to: "NA"} ], Detected: "NA"});
            log(`Translation an error: ${e}`)
        }
    }else{
        res.status(401);
        res.json({Status: "Error", Error: "Invlaid Auth", Translations: [{ text: "NA", to: "NA"} ], Detected: "NA"});
        log("AUTH ERROR: " + req.url + " Invalid Server Token", "warn");
    }
}

module.exports = router;
/*  **** Default Supported Languages ****
    +-----------------------+----------+
    | Language              | Code     |
    +-----------------------+----------+
    | Arabic                | ar       |
    +-----------------------+----------+
    | Chinese   Simplified  | zh-Hans  |
    +-----------------------+----------+
    | English               | en       |
    +-----------------------+----------+
    | French                | fr       |
    +-----------------------+----------+
    | German                | de       |
    +-----------------------+----------+
    | Hindi                 | hi       |
    +-----------------------+----------+
    | Indonesian            | id       |
    +-----------------------+----------+
    | Irish                 | ga       |
    +-----------------------+----------+
    | Italian               | it       |
    +-----------------------+----------+
    | Japanese              | ja       |
    +-----------------------+----------+
    | Korean                | ko       |
    +-----------------------+----------+
    | Polish                | pl       |
    +-----------------------+----------+
    | Portuguese            | pt       |
    +-----------------------+----------+
    | Russian               | ru       |
    +-----------------------+----------+
    | Spanish               | es       |
    +-----------------------+----------+
    | Turkish               | tr       |
    +-----------------------+----------+
    | Vietnamese            | vi       | 
    +-----------------------+----------+
*/


/*  *** Supported Languages Microsoft ***
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