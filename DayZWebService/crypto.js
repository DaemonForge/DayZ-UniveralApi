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
            if ((Date.now() - 4000) > LastCache){
                await UpdateCache();
            }
            let RawData = req.body; 
            let value = RawData.From || RawData.To;
            let rValues = {};
            let skiplogs = false;
            value.forEach(element => {
                let v;
                if (RawData.From !== undefined){
                    v = GetRate(element,from,skiplogs);
                } else {
                    v = GetRate(from,element,skiplogs);
                }
                skiplogs = true;
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


async function GetRate(from, to, skiplog = false){
    if ((Date.now() - 4000) > LastCache){
        await UpdateCache(skiplog);
    } else  if ((Date.now() - 2000) > LastCache){
        UpdateCache(skiplog);
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



let binance;
let binanceIsPending = false;
let coinconvert;
let coinconvertIsPending = false;
let currency;
let currencyIsPending = false;
const regex = /([A-Z]{2,5})(EUR)/im;
async function UpdateCache(skiplog = false){
    try {
        if (!binanceIsPending){
            binanceIsPending = true;
            binance = (await fetch(`https://api.binance.com/api/v3/ticker/price`)).json().then(value => { 
                binanceIsPending = false;
                return value;
            }).catch(err =>{
                console.log(err);
                log(`Error Getting data from Binance.com ${err}`);
            });
        }
        if (!coinconvertIsPending){
            coinconvertIsPending = true;
            coinconvert = (await fetch(`https://api.coinconvert.net/ticker`)).json().then(value => { 
                coinconvertIsPending = false;
                return value;
            }).catch(err =>{
                console.log(err);
                log(`Error Getting data from Binance.com ${err}`);
            });
        }
        if (!currencyIsPending){
            currencyIsPending = true;
            currency = (await fetch(`https://open.er-api.com/v6/latest/EUR`)).json().then(value => { 
                currencyIsPending = false;
                return value;
            }).catch(err =>{
                console.log(err);
                log(`Error Getting data from Binance.com ${err}`);
            });
        }
    } catch (err){
        console.log(err);
    }
    try {
        while (binance === undefined) await wait(9);
        binance = await binance;
        binance.forEach(element => { 
            let key = regex.exec(element.symbol);
            if (key){
                //console.log(key[1]);
                CryptoCache[`${key[1]}`]= 1 / (element.price * 1);
            }
        });
    } catch (err) {
        log(`Error Getting data from Binance.com ${err}`);
    } 
    try {
        while (coinconvert === undefined) await wait(9);
        coinconvert = await coinconvert;
        let crypto = coinconvert.crypto;
        for (const [key, value] of Object.entries(crypto)) {
            let mkey = regex.exec(key);
            if (mkey){
                //console.log(mkey[1]);
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
        while (currency === undefined) await wait(9);
        currency = await currency;
        for (const [key, value] of Object.entries(currency.rates)) {
            CurrencyCache[`${key}`]= value * 1;
        }
    } catch(err) {
        log(`Error Getting data from open.er-api.com ${err}`);
    }
    if (!skiplog) log(`Crypto currencys cache updated for ${Object.keys(CryptoCache).length} currencys`);
    LastCache = Date.now();
    return;
}
async function wait(ms) {
    return new Promise(resolve => {
      setTimeout(resolve, ms);
    });
  }

module.exports = router;