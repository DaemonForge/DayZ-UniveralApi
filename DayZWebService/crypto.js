const {Router} = require('express');
const log = require("./log");
const {promisedProperties,GenerateLimiter} = require('./utils');

const { CheckAuth, CheckServerAuth} = require('./AuthChecker')

const router = Router();

router.use(GenerateLimiter(global.config.RequestLimitCrypto || 150, 5));

let CryptoCache = {};
let CurrencyCache = {};
let LastCache = 0;


router.post('/Convert/:from/:to', (req, res)=>{
    let RawData = req.body; 
    let value = RawData.Value;
    DoCryptoConvert(res, req, req.headers['auth-key'], req.params.from, req.params.to, value);
});

router.post('/Price/:from/:to', (req, res)=>{
    DoCryptoConvert(res, req,req.headers['auth-key'], req.params.from, req.params.to, 1);
});

router.post('/:from', (req, res)=>{
    DoBulkCryptoConvert(res, req,req.headers['auth-key'], req.params.from);
});


async function DoCryptoConvert(res, req, auth, from, to, amount){

    if (CheckServerAuth(auth) || (await CheckAuth(auth))){
        try {
            let rvalue = (await GetRate(from,to)) * amount;
            res.json({Status: "Success", Error: "", Value: rvalue})
        }
        catch (err){
            res.json({Status: "Error", Error: `${err}`, Value: -1});
        }
    } else {
        res.status(401);
        res.json({Status: "Error", Error: `Invalid Auth`, oid: ""});
        log("AUTH ERROR: " + req.url, "warn");
    }
}
async function DoBulkCryptoConvert(res, req, auth, from){
    if (CheckServerAuth(auth) || (await CheckAuth(auth))){
        try {
            let RawData = req.body; 
            let value = RawData.From || RawData.To;
            let rValues = {};
            value.forEach(element => {
                let v;
                if ((RawData.From !== undefined)){
                    v = GetRate(element,from);
                } else {
                    v = GetRate(from,element);
                }
                rValues[element] = v;
            });
            let rvalues = await promisedProperties(rValues)
            res.json({Status: "Success", Error: "", Values: rvalues})
        }
        catch (err){
            console.log(err)
            res.json({Status: "Error", Error: `${err}`, Values: []});
        }
    } else {
        res.status(401);
        res.json({Status: "Error", Error: `Invalid Auth`});
        log("AUTH ERROR: " + req.url, "warn");
    }
}

async function GetRate(from, to){
    if ((Date.now() - 4000) > LastCache){
        await UpdateCache();
    } else  if ((Date.now() - 2500) > LastCache){
        UpdateCache();
    } 
    let rate1 = -1;
    let rate2 = -1;
    if (CryptoCache[ to] !== undefined ){
        rate2 = CryptoCache[to];
    } else if (CurrencyCache[to] !== undefined){
        rate2 = CurrencyCache[to];
    }
    if (CurrencyCache[from] !== undefined ){
        rate1 = CurrencyCache[from];
    } else if (CryptoCache[from] !== undefined ){
        rate1 = CryptoCache[from];
    } 
    if (rate2 === -1 || rate1 === -1) {
        return -1;
    }
    //log(`Currency Rate Calcuated: ${from} to ${to}: ` + rate2/rate1)
    return rate2/rate1;
}

UpdateCache();
async function UpdateCache(){
    LastCache = Date.now();
    const regex = /([A-Z]{3,4})(EUR)$/gm;
    try {
        let binance = await (await fetch(`https://api.binance.com/api/v3/ticker/price`)).json();
        binance.forEach(element => { 
            let key = regex.exec(element.symbol);
            if (key){
                CryptoCache[`${key[1]}`]= 1 / (element.price * 1);
            }
        });
    } catch (err) {
        log(`Error Getting data from Binance.com ${err}`);
    } 
    try {
        let coinconvert = await (await fetch(`https://api.coinconvert.net/ticker`)).json();
        let crypto = coinconvert.crypto;
        for (const [key, value] of Object.entries(crypto)) {
            let mkey = regex.exec(key);
            if (mkey){
                CryptoCache[`${mkey[1]}`]= 1 / (value * 1);
            }
        }
        let cur = coinconvert.fiat;
        for (const [key, value] of Object.entries(cur)) {
            CurrencyCache[`${key}`]= value * 1;
        }
    } catch(err) {
        log(`Error Getting data from CoinConvert.net ${err}`);
    }
    try {
        let currency = await (await fetch(`https://open.er-api.com/v6/latest/EUR`)).json();
        for (const [key, value] of Object.entries(currency.rates)) {
            CurrencyCache[`${key}`]= value * 1;
        }
    } catch(err) {
        log(`Error Getting data from open.er-api.com ${err}`);
    }
    return;
}


module.exports = router;