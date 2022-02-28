const {
    Router
} = require('express');
const {
    MongoClient
} = require("mongodb");


const log = require("../log");
const {
    isArray,
    isObject,
    CleanRegEx,
    GenerateLimiter,
    IncermentAPICount,
    byteSize
} = require('../utils');




const router = Router();

// apply rate limiter to all requests
router.use(GenerateLimiter(global.config.RequestLimitQuery || 400, 10));

/**
 * Post: /[Collection]/[Mod]
 * 
 */
router.post('/:mod', (req, res) => {
    if (req.IsServer !== true && COLL === "Players") return HandleBadAuthkey(res);
    runQuery(req, res, req.params.mod, req.Collection);
});

router.post('/Update/:mod', (req, res) => {
    if (req.IsServer !== true) return HandleBadAuthkey(res);
    runUpdateFromQuery(req, res, req.params.mod, req.Collection);
});

async function runQuery(req, res, mod, COLL) {
        var RawData = req.body;
        const client = new MongoClient(global.config.DBServer, {
            useUnifiedTopology: true
        });
        try {

            // Connect the client to the server
            await client.connect();

            const db = client.db(req.ClientInfo.DB);
            let collection = db.collection(COLL);
            let query = JSON.parse(RawData.Query);
            let orderBy = JSON.parse(RawData.OrderBy);
            let fixQuery = RawData.FixQuery || 0;
            let ReturnCol = "Data";
            if (COLL == "Players") {
                ReturnCol = mod;
            }
            if (fixQuery === 1) {
                query = FixQuery(query, ReturnCol);
            }
            if (COLL == "Players") {
                if (query && Object.keys(query).length === 0 && query.constructor === Object) {
                    query[mod] = {
                        "$exists": true
                    };
                }
            }
            if (COLL == "Objects" && (query.Mod === undefined || query.Mod === null)) {
                query.Mod = mod;
            }
            let results = collection.find(query).sort(orderBy);
            if (RawData.MaxResults >= 1) {
                results.limit(RawData.MaxResults);
            }
            let theData = await results.toArray();
            let RetrunData = [];
            let count = 0;
            for (result of theData) {
                for (const [key, value] of Object.entries(result)) {
                    if (key === ReturnCol) {
                        if (RawData.ReturnObject != "" && RawData.ReturnObject != null) {
                            count++;
                            RetrunData.push(value[RawData.ReturnObject]);
                        } else {
                            count++;
                            RetrunData.push(value);
                        }
                    }
                }
            }
            if (RetrunData) {
                if (count == 0) {
                    let simpleReturn = {
                        Status: "NoResults",
                        Count: 0,
                        Results: []
                    }
                    log("Query:  " + JSON.stringify(query) + " against " + COLL + " for " + ReturnCol + " Got 0 Results", "info");
                    res.json(simpleReturn);
                    IncermentAPICount(req.ClientInfo.ClientId, byteSize(simpleReturn));
                } else {
                    let simpleReturn = {
                        Status: "Success",
                        Count: count,
                        Results: RetrunData
                    }
                    log("Query:  " + JSON.stringify(query) + " against " + COLL + " for " + ReturnCol + " Got " + count + " Results", "info");
                    res.json(simpleReturn);
                    IncermentAPICount(req.ClientInfo.ClientId, byteSize(simpleReturn));
                }
            }
        } catch (err) {
            log("Error in Query:  " + query + " against " + COLL + " for mod " + mod + " error: " + err, "warn");
            res.status(203);
            let response = {
                Status: "Error",
                Count: 0,
                Results: []
            };
            res.json(response);
            IncermentAPICount(req.ClientInfo.ClientId, byteSize(response));
        } finally {
            // Ensures that the client will close when you finish/error
            await client.close();
        }
};

async function runUpdateFromQuery(req, res, mod, COLL) {
        let RawData = req.body;
        const client = new MongoClient(global.config.DBServer, { useUnifiedTopology: true });
        try{
            await client.connect();
            let query = JSON.parse(RawData.Query.Query);
            let orderBy = JSON.parse(RawData.Query.OrderBy);
            let fixQuery = RawData.Query.FixQuery || 0;
            let ReturnCol = "Data";
            if (COLL == "Players"){
                ReturnCol = mod;
            }
            if (fixQuery === 1){
                query = FixQuery(query,ReturnCol);
            }
            if (COLL == "Players"){
                if (query && Object.keys(query).length === 0 && query.constructor === Object){
                    query[mod] = { "$exists": true };
                }
            }
            if (COLL == "Objects" && (query.Mod === undefined || query.Mod === null)){
                query.Mod = mod;
            }
            let element = RawData.Element;
            let operation = RawData.Operation || "set";
            let StringData;
            if (isObject(RawData.Value)){
                StringData = JSON.stringify(RawData.Value);
            } else if (isArray(RawData.Value)) {
                StringData = JSON.stringify(RawData.Value);
            } else if (`${StringData}`.match(/^-?(0|[1-9]\d*)(\.\d+)?$/g)){
                StringData = RawData.Value * 1;
            } else {
                StringData = RawData.Value;
            }
            // Connect the client to the server
            const db = client.db(global.config.DB);
            let collection = db.collection(COLL);
            const options = { upsert: false };

            let jsonString = `{ "Data.${element}": ${StringData} }`;
            let updateDocValue;
            try { //should use regex to check for a string without " but being lazy
                updateDocValue  = JSON.parse(jsonString);
            } catch (e){
                jsonString = `{ "Data.${element}": "${StringData}" }`;
                updateDocValue  = JSON.parse(jsonString);
            }
            //console.log(updateDocValue);
            let updateDoc = { $set: updateDocValue, };
            
            if (operation === "pull"){
                updateDoc = { $pull: updateDocValue, };
            } else if (operation === "push"){
                updateDoc = { $push: updateDocValue, };
            } else if (operation === "unset"){
                updateDoc = { $unset: updateDocValue, };
            } else if (operation === "mul"){
                updateDoc = { $mul: updateDocValue, };
            } else if (operation === "rename"){
                updateDoc = { $rename: updateDocValue, };
            } else if (operation === "pullAll"){
                updateDoc = { $pullAll: updateDocValue, };
            }
            //console.log(updateDoc)
            let simpleReturn = {};
            const result = await collection.updateOne(query, updateDoc, options);
            //console.log(result.result)
            if (result.matchedCount >= 1 || result.upsertedCount >= 1){
                log("Updated " +  result.matchedCount + " items " + element +" for "+ mod + " Data for Query: " + JSON.stringify(query));
                simpleReturn = { Status: "Success", Element: element, Mod: mod, Count:  result.matchedCount};
                res.status(200);
                res.json(simpleReturn);
            } else {
                //console.log(result.result)
                log("Updated " +  result.matchedCount + " items " + element +" for "+ mod + " Data for Query: " + JSON.stringify(query));
                simpleReturn = { Status: "NoResults", Element: element, Mod: mod, Count: 0}
                res.status(203);
                res.json(simpleReturn);
            }
            IncermentAPICount(req.ClientInfo.ClientId, byteSize(simpleReturn));
        }catch(err){
            log(`ERROR: ${err}`, "warn");
            res.status(203);
            res.json({ Status: "Error", Element: RawData.Element, Mod: mod, Count: 0});
            //console.log(err)
        }finally{
            // Ensures that the client will close when you finish/error
            await client.close();
        }
};

function FixQuery(query, prefix) {
    if (isObject(query)) {
        for (const [key, value] of Object.entries(query)) {
            if (!key.match(/^\$/i) && !key.match(new RegExp(`^${CleanRegEx(prefix)}\\.`, "g"))) {
                query[`${prefix}.${key}`] = FixQuery(value, prefix);
                delete query[key]
            } else {
                query[key] = FixQuery(value, prefix);
            }
        }
        return query;
    } else if (isArray(query)) {
        let newArr = [];
        query.forEach(e => {
            newArr.push(FixQuery(e, prefix));
        });
        return newArr;
    }
    return query;
}

module.exports = router;