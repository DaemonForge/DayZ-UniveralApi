const {
    Router
} = require('express');
const log = require("../log");
const {
    isArray,
    GenerateLimiter,
    IncermentAPICount
} = require('../utils');


const router = Router();

router.use(GenerateLimiter(global.config.RequestLimitToxicity || 200, 10));

/**
 *  Quantum Random Number Generator 0 to 65535
 *  Post: /Random
 *  
 *  Description: This endpoint generates the specified amount of random numbers from 
 *    ANU's Quantum Random number API within the range of 0 to 65535
 * 
 *  Accepts: `{ "Count": |NumberToGenerate| }`
 *
 *  Returns: `{ "Status": "|STATUSOFREQUEST|", "Error": "|ANYERRORMESSAGE|", "Numbers": [|ARRAYOFINTEGERS|] }`
 * 
 */
router.post('', (req, res) => {
    GetRandom(req, res, req.headers['auth-key']);
});

/**
 *  Quantum Random Number Generator -2147483647 to 2147483647
 *  Post: /Random/Full
 *  
 *  Description: This endpoint generates the specified amount of random numbers from 
 *    ANU's Quantum Random number API within the range of -2147483647 to 2147483647
 * 
 *  Accepts: `{ "Count": |NumberToGenerate| }`
 *
 *  Returns: `{ 
 *                 "Status": "|STATUSOFREQUEST|", 
 *                 "Error": "|ANYERRORMESSAGE|",
 *                  "Numbers": [|ARRAYOFINTEGERS|] 
 *            }`
 * 
 */
router.post('/Full', (req, res) => {
    GetFullRandom(req, res, req.headers['auth-key']);
});

async function GetRandom(req, res, auth) {
    let RawCount = req.body.Count || 2048;
    let count = RawCount;
    if (count > 2048 || count < 1) {
        log("Failed to generate random numbers due to request size being too large", "warn");
        res.status(203);
        return res.json({
            Status: "Error",
            Error: `Invalid Array Request Size`
        });
    }
    try {
        let ints = []; //I know I could Impove this but meh it works and yeah
        if (count > 1024) {
            count = count - 1024
            let resJson2 = await fetch(`https://qrng.anu.edu.au/API/jsonI.php?length=1024&type=uint16`)
            let data2 = await resJson2.json();
            if (data2.success) {
                ints = ints.concat(data2.data);
            }
        }
        let resJson = await fetch(`https://qrng.anu.edu.au/API/jsonI.php?length=${count}&type=uint16`)
        let data = await resJson.json();
        if (data.success) {
            ints = ints.concat(data.data);
            log("Random numbers requested");
            res.status(200);
            res.json({
                Status: "Success",
                Error: ``,
                Numbers: ints
            });
            IncermentAPICount(req.ClientInfo.ClientId);
        } else {
            res.status(203)
            log("Failed to generate random numbers due to error from qrng servers", "warn");
            res.json({
                Status: "Error",
                Error: `Error in request`,
            });
        }
    } catch (e) {
        console.log(e)
        log(e, "warn")
        res.status(203);
        res.json({
            Status: "Error",
            Error: `${e}`
        });
    }

}

async function GetFullRandom(req, res, auth) {
    let RawCount = req.body.Count || 4096;
    let count = RawCount;
    if (count > 4096 || count < 1) {
        log("Failed to generate random numbers due to request size being too large", "warn");
        res.status(203);
        return res.json({
            Status: "Error",
            Error: `Invalid Array Request Size`
        });
    }
    let cachedints = await GetFromCache(count);
    if (cachedints.length === count) {
        res.status(200);
        res.json({
            Status: "Success",
            Error: `Cached`,
            Numbers: cachedints
        });
        IncermentAPICount(req.ClientInfo.ClientId);
        return;
    }
    try {
        let hexs = []; //I know I could Impove this but meh it works and yeah
        let ints = [];
        let resJson = await fetch(`https://qrng.anu.edu.au/API/jsonI.php?length=${Math.ceil(count/4)}&type=hex16&size=16`)
        let data = await resJson.json();
        if (data.success) {
            hexs = hexs.concat(data.data);
            hexs.forEach(e => {
                ints = ints.concat(ConvertToInts(e));
            });
            log("Random numbers requested");
            res.status(200);
            res.json({
                Status: "Success",
                Error: `Live`,
                Numbers: ints
            });
            IncermentAPICount(req.ClientInfo.ClientId);
        } else {
            res.status(203)
            log("Failed to generate random numbers due to error from qrng servers", "warn");
            res.json({
                Status: "Error",
                Error: `Error in request`,
            });
        }
    } catch (e) {
        console.log(e)
        log(e, "warn")
        res.status(203);
        res.json({
            Status: "Error",
            Error: `${e}`
        });
    }
}

function ConvertToInts(hex) {
    let buf = Buffer.from(hex, "hex");
    let start = 0;
    let end = 4;
    let ints = [];
    while (buf.length >= end) {
        let buf1 = buf.slice(start, end);
        ints.push(buf1.readInt32LE(0));
        start = end;
        end = end + 4;
    }
    return ints
}
let awaitingCache = false;

async function GetFromCache(count) {
    if (CachedInts.length < 100000 && !awaitingCache) {
        awaitingCache = true;
        GetAndCacheRandomNumber();
    }
    if (CachedInts.length < count) {
        return [];
    }
    return CachedInts.splice(0, count);
}
let CachedInts = [];
async function GetAndCacheRandomNumber() {
    try {
        let hexs = []; //I know I could Impove this but meh it works and yeah
        let resJson = await fetch(`https://qrng.anu.edu.au/API/jsonI.php?length=1024&type=hex16&size=512`)
        let data = await resJson.json();
        if (data.success) {
            hexs = hexs.concat(data.data);
            hexs.forEach(e => {
                CachedInts = CachedInts.concat(ConvertToInts(e));
            });
            awaitingCache = false;
            log(`True Random Number Cache Updated there is now ${CachedInts.length} random numbers in the cache`);
        }
    } catch (e) {
        console.log(e)
    }
}

module.exports = router;