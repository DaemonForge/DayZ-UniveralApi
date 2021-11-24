const {Router} = require('express');
const log = require("./log");
const cryptoConvert = require('crypto-convert');
const {isArray,GenerateLimiter} = require('./utils');

const { CheckAuth, CheckServerAuth} = require('./AuthChecker')

const router = Router();

router.use(GenerateLimiter(global.config.RequestLimitCrypto || 150, 5));
cryptoConvert.set({
	crypto_interval: 3000, //Crypto cache update interval, default every 5 seconds
	fiat_interval: (180 * 1e3 * 60), 
	binance: false,
	bitfinex: true,
	okex: true
});

router.post('/Convert/:from/:to', (req, res)=>{
    let RawData = req.body; 
    let value = RawData.Value;
    DoCryptoConvert(res, req, req.headers['auth-key'], req.params.from, req.params.to, value);
});

router.post('/Price/:from/:to', (req, res)=>{
    DoCryptoConvert(res, req,req.headers['auth-key'], req.params.from, req.params.to, 1);
});

router.post('/Bulk/:to', (req, res)=>{
    DoBulkCryptoConvert(res, req,req.headers['auth-key'], req.params.to, 1);
});

async function DoCryptoConvert(res, req, auth, from, to, amount){
    if (CheckServerAuth(auth) || (await CheckAuth(auth))){
        try {
            //Cache is not yet loaded on application start
            if(!cryptoConvert.isReady){
                await cryptoConvert.ready();
            }
            let rvalue = new cryptoConvert.from(from).to(to).amount(amount);
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
async function DoBulkCryptoConvert(res, req, auth, to){
    if (CheckServerAuth(auth) || (await CheckAuth(auth))){
        try {
            //Cache is not yet loaded on application start
            if(!cryptoConvert.isReady){
                await cryptoConvert.ready();
            }
            let RawData = req.body; 
            let value = RawData.From;
            let rValues = {};
            value.forEach(element => {
                rValues[element] = new cryptoConvert.from(element).to(to).amount(1);
            });
            res.json({Status: "Success", Error: "", Values: rValues})
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

module.exports = router;