const {Router} = require('express');
// Remove the old logger import
const {promisedProperties,GenerateLimiter} = require('../utils');
const logger = global.logger;
const {requirePlayerOrServerAuth} = require("../auth/utils");

const router = Router();

router.use(GenerateLimiter(global.config.RequestLimitCrypto || 150, 5));

let CryptoCache = {};
let CurrencyCache = {};
let LastCache = 0;


router.post('/Convert/:from/:to', requirePlayerOrServerAuth, (req, res)=>{
    let RawData = req.body; 
    let value = RawData.Value;
    DoCryptoConvert(res, req, req.params.from, req.params.to, value);
});

router.post('/Price/:from/:to', requirePlayerOrServerAuth, (req, res)=>{
    DoCryptoConvert(res, req.params.from, req.params.to, 1);
});

router.post('/:from', requirePlayerOrServerAuth, (req, res)=>{
    DoBulkCryptoConvert(res, req, req.params.from);
});


async function DoCryptoConvert(res, req, from, to, amount){
        try {
            let rvalue = (await GetRate(from,to)) * amount;
            res.json({Status: "Success", Error: "", Value: rvalue})
        }
        catch (err){
            res.json({Status: "Error", Error: `${err}`, Value: -1});
        }
}
async function DoBulkCryptoConvert(res, req, from){
        try {
            if ((Date.now() - 5000) > LastCache){
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
                logger.error("Error Getting data from Binance.com", { error: err });
            });
        }
        if (!coinconvertIsPending){
            coinconvertIsPending = true;
            coinconvert = (await fetch(`https://api.coinconvert.net/ticker`)).json().then(value => { 
                coinconvertIsPending = false;
                return value;
            }).catch(err =>{
                console.log(err);
                logger.error("Error Getting data from Binance.com", { error: err });
            });
        }
        if (!currencyIsPending){
            currencyIsPending = true;
            currency = (await fetch(`https://open.er-api.com/v6/latest/EUR`)).json().then(value => { 
                currencyIsPending = false;
                return value;
            }).catch(err =>{
                console.log(err);
                logger.error("Error Getting data from Binance.com", { error: err });
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
                CryptoCache[`${key[1]}`]= 1 / (element.price * 1);
            }
        });
    } catch (err) {
        logger.error("Error Getting data from Binance.com", { error: err });
    } 
    try {
        while (coinconvert === undefined) await wait(9);
        coinconvert = await coinconvert;
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
        logger.error("Error Getting data from CoinConvert.net", { error: err });
    }
    try {
        while (currency === undefined) await wait(9);
        currency = await currency;
        for (const [key, value] of Object.entries(currency.rates)) {
            CurrencyCache[`${key}`]= value * 1;
        }
    } catch(err) {
        logger.error("Error Getting data from open.er-api.com", { error: err });
    }
    if (!skiplog) logger.info("Crypto currencies cache updated", { count: Object.keys(CryptoCache).length });
    LastCache = Date.now();
    return;
}
async function wait(ms) {
    return new Promise(resolve => {
      setTimeout(resolve, ms);
    });
  }

module.exports = router;
