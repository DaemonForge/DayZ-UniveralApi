const { Router } = require('express');
const { MongoClient } = require("mongodb");


const log = require("./log");

const {CheckAuth,CheckServerAuth} = require('./AuthChecker');


const config = require('./configLoader');

const router = Router();

router.post('/:mod/:auth', (req, res)=>{
    runQuery(req, res, req.params.mod, req.params.auth, GetCollection(req.baseUrl));
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
        const client = new MongoClient(config.DBServer, { useUnifiedTopology: true });
        try{

            // Connect the client to the server
            await client.connect();
            
            const db = client.db(config.DB);
            var collection = db.collection(COLL);
            var query = JSON.parse(RawData.Query);
            var orderBy = JSON.parse(RawData.OrderBy);
            var ReturnCol = "Data";
            if (COLL == "Players"){
                ReturnCol = mod;
                if (query && Object.keys(query).length === 0 && query.constructor === Object){
                    query[mod] = { "$exists": true };
                }
            }
            if (COLL == "Objects" && (query.Mod === undefined || query.Mod === null)){
                query.Mod = mod;
            }
            var results = collection.find(query).sort(orderBy);
            if (RawData.MaxResults >= 1){
                results.limit(RawData.MaxResults);
            }
            var theData = await results.toArray();
            var RetrunData = [];
            var count = 0;
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
                    var simpleReturn = { Status: "NoResults", Count: 0, Results: [] }
                    log("Query:  " + JSON.stringify(query) + " against " + COLL + " for " + ReturnCol + " Got 0 Results", "info");
                    res.json(simpleReturn);
                } else {
                    var simpleReturn = {Status: "Success", Count: count, Results: RetrunData }
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
        res.status(203);
        res.json({Status: "NoAuth", Count: 0, Results: [] });
    }
};

module.exports = router;