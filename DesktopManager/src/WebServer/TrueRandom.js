const {Router} = require('express');
const log = require("./log");

const { CheckAuth, CheckServerAuth} = require('./AuthChecker')

const router = Router();

var RateLimit = require('express-rate-limit');
var limiter = new RateLimit({
  windowMs: 10*1000, // 30 req/sec
  max: global.config.RequestLimitToxicity || 300,
  message:  '{ "Status": "Error", "Error": "RateLimited" }',
  keyGenerator: function (req /*, res*/) {
    return req.headers['CF-Connecting-IP'] || req.headers['x-forwarded-for'] || req.socket.remoteAddress || req.ip;
  },
  onLimitReached: function (req, res, options) {
    let ip = req.headers['CF-Connecting-IP'] || req.headers['x-forwarded-for'] || req.socket.remoteAddress || req.ip;
    log("RateLimit Reached("  + ip + ") you may be under a DDoS Attack or you may need to increase your request limit");
  }
});

// apply rate limiter to all requests
router.use(limiter);

router.post('/:auth', (req, res)=>{
    GetRandom(req, res, req.params.auth);
});

async function GetRandom(req, res, auth){
    if ( CheckServerAuth( auth ) || (await CheckAuth( auth )) ){
        let RawCount = req.body.Count || 2048;
        let count = RawCount;
        if (count > 2048 || count < 1){
            log("Failed to generate random numbers due to request size being too large", "warn");
            res.status(203);
            return res.json({Status: "Error", Error: `Invalid Array Request Size` });
        }
        try {
            let ints = []; //I know I could Impove this but meh it works and yeah
            if (count > 1024){
                count = count - 1024
                let resJson2 = await fetch(`https://qrng.anu.edu.au/API/jsonI.php?length=1024&type=uint16`)
                let data2 = await resJson2.json();
                if (data2.success){
                    ints = ints.concat(data2.data);
                }
            }
            let resJson = await fetch(`https://qrng.anu.edu.au/API/jsonI.php?length=${count}&type=uint16`)
            let data = await resJson.json();
            if (data.success){
                ints = ints.concat(data.data);
                log("Random numbers requested");
                res.status(200);
                res.json({Status: "Success", Error: ``, Numbers: ints });
            } else {
                res.status(203)
                log("Failed to generate random numbers due to error from qrng servers", "warn");
                res.json({Status: "Error", Error: `Error in request`,  });
            }
        } catch (e){
            console.log(e)
            log(e, "warn")
            res.status(203);
            res.json({Status: "Error", Error: `${e}` });
        }
    } else {
        res.status(203);
        res.json({Status: "NoAuth", Error: "Invalid Auth" });
    }

}

module.exports = router;