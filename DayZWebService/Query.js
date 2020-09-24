const express = require('express');
const { MongoClient } = require("mongodb");

const CheckAuth = require('./AuthChecker')


const config = require('./configLoader');

const router = express.Router();

router.post('/:mod/:auth', (req, res)=>{
    runQuery(req, res, req.params.mod, req.params.auth, GetCollection(req.url));
});

function GetCollection(URL){
    if (URL.includes("/Player/")){
        return "Players"
    }
    if (URL.includes("/Item/")){
        return "Items"
    }
}

async function runQuery(req, res, mod, auth, COLL) {
    if (auth == config.ServerAuth || (await CheckAuth(auth)) ){
        var RawData = req.body;
        const client = new MongoClient(config.DBServer, { useUnifiedTopology: true });
        try{

            // Connect the client to the server
            await client.connect();
            
            const db = client.db(config.DB);
            var collection = db.collection(COLL);
            var query = JSON.parse(RawData.Query);
            var results =  collection.distinct(mod, query);
            var Count =  await results.count();
            if (Count == 0){
                var simpleReturn = { Status: "NoResults", Count: 0, Results: [] }
                res.json(simpleReturn);
            } else {
                var data = await results.toArray(); 

                var simpleReturn = {Status: "Success", Count: Count, Results: data }
                console.log("Found " + mod + " data: " + data)
                res.json(simpleReturn);
            }
        }catch(err){
            console.log("Error " + err)
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