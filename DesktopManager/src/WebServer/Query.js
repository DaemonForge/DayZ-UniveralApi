const { Router } = require('express');
const { MongoClient } = require("mongodb");


const log = require("./log");
const {isArray,isObject,CleanRegEx, GenerateLimiter} = require('./utils');

const {CheckAuth,CheckServerAuth} = require('./AuthChecker');



const router = Router();

// apply rate limiter to all requests
router.use(GenerateLimiter(global.config.RequestLimitQuery || 400, 10));

/**
 * Post: /[Collection]/[Mod]
 * 
 */
router.post('/:mod', (req, res)=>{
    runQuery(req, res, req.params.mod, req.headers['auth-key'], GetCollection(req.baseUrl));
});

function GetCollection(URL){
    if (URL.includes("/Player/")){
        return "Players"
    }
    if (URL.includes("/Object/")){
        return "Objects"
    }
}

async function runQuery(req, res, mod, auth, COLL) {
    if (CheckServerAuth(auth) || ((await CheckAuth(auth)) && COLL === "Objects") ){
        var RawData = req.body;
        const client = new MongoClient(global.config.DBServer, { useUnifiedTopology: true });
        try{

            // Connect the client to the server
            await client.connect();
            
            const db = client.db(global.config.DB);
            let collection = db.collection(COLL);
            let query = JSON.parse(RawData.Query);
            let orderBy = JSON.parse(RawData.OrderBy);
            let fixQuery = RawData.FixQuery || 0;
            let ReturnCol = "Data";
            if (COLL == "Players"){
                ReturnCol = mod;
                if (query && Object.keys(query).length === 0 && query.constructor === Object){
                    query[mod] = { "$exists": true };
                }
            }
            if (fixQuery === 1){
                query = FixQuery(query,ReturnCol);
            }
            if (COLL == "Objects" && (query.Mod === undefined || query.Mod === null)){
                query.Mod = mod;
            }
            let results = collection.find(query).sort(orderBy);
            if (RawData.MaxResults >= 1){
                results.limit(RawData.MaxResults);
            }
            let theData = await results.toArray();
            let RetrunData = [];
            let count = 0;
            for (result of theData){
                for (const [key, value] of Object.entries(result)) {
                    if(key === ReturnCol){
                        if (RawData.ReturnObject != "" && RawData.ReturnObject != null){
                            count ++;
                            RetrunData.push(value[RawData.ReturnObject]);
                        } else {
                            count ++;
                            RetrunData.push(value);
                        }
                    }
                }
            }
            if (RetrunData){
                if (count == 0){
                    let simpleReturn = { Status: "NoResults", Count: 0, Results: [] }
                    log("Query:  " + JSON.stringify(query) + " against " + COLL + " for " + ReturnCol + " Got 0 Results", "info");
                    res.json(simpleReturn);
                } else {
                    let simpleReturn = {Status: "Success", Count: count, Results: RetrunData }
                    log("Query:  " + JSON.stringify(query) + " against " + COLL + " for " + ReturnCol + " Got " + count + " Results", "info");
                    res.json(simpleReturn);
                }
            }
        }catch(err){
            log("Error in Query:  " + query + " against " + COLL + " for mod " + mod + " error: " + err, "warn");
            res.status(203);
            res.json({Status: "Error", Count: 0, Results: [] });
        }finally{
            // Ensures that the client will close when you finish/error
            await client.close();
        }
    } else {
        res.status(401);
        res.json({Status: "Error", Error: "Invalid Auth", Count: 0, Results: [] });
    }
};

function FixQuery(query, prefix){
    if (isObject(query)){
        for (const [key, value] of Object.entries(query)) {
            if(!key.match(/^\$/i) && !key.match(new RegExp(`^${CleanRegEx(prefix)}\\.`, "g"))){
                query[`${prefix}.${key}`] = FixQuery(value, prefix);
                delete query[key]
            } else {
                query[key] = FixQuery(value, prefix);
            }
        }
        return query;
    } else if (isArray(query)){
        let newArr = [];
        query.forEach(e => {
            newArr.push(FixQuery(e, prefix));
        });
        return newArr;
    }
    return query;
}

module.exports = router;
