const express = require('express');
const { MongoClient } = require("mongodb");

const log = require("./log");

const CheckAuth = require('./AuthChecker');


const config = require('./configLoader');

const router = express.Router();

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
    if (auth === config.ServerAuth || ((await CheckAuth(auth)) && COLL === "Objects") ){
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
                    res.json(simpleReturn);
                } else {
                    var simpleReturn = {Status: "Success", Count: count, Results: RetrunData }
                    log("Query:  " + JSON.stringify(query) + " against " + COLL + " for " + ReturnCol + " Got " + count + " Results", "info");
                    res.json(simpleReturn);
                }
            }
        }catch(err){
            log("Error in Query:  " + query + " against " + COLL + " for mod " + mod + " error: " + err, "info");
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
function dynamicSortMultiple( props ) {
    /*
     * save the arguments object as it will be overwritten
     * note that arguments object is an array-like object
     * consisting of the names of the properties to sort by
     */
    return function (obj1, obj2) {
        var i = 0, result = 0, numberOfProperties = props.length;
        /* try getting a different result from 0 (equal)
         * as long as we have extra properties to compare
         */
        while(result === 0 && i < numberOfProperties) {
            result = dynamicSort(props[i])(obj1, obj2);
            i++;
        }
        return result;
    }
}
function dynamicSort(property) {
    var sortOrder = 1;
    if(property[0] === "-") {
        sortOrder = -1;
        property = property.substr(1);
    }
    return function (a,b) {
        /* next line works with strings and numbers, 
         * and you may want to customize it to your needs
         */
        var result = (a[property] < b[property]) ? -1 : (a[property] > b[property]) ? 1 : 0;
        return result * sortOrder;
    }
}
module.exports = router;