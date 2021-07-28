const {Router} = require('express');
const { MongoClient } = require("mongodb");

const log = require("./log");

const {CheckAuth,CheckServerAuth} = require('./AuthChecker');


const router = Router();

module.exports = router;

router.post('/:id/:mod', (req, res)=>{
    runTransaction(req, res, req.params.mod, req.params.id, req.headers['auth-key'], GetCollection(req.baseUrl));
});

//TO REMOVE
router.post('/:id/:mod/:auth', (req, res)=>{
    runTransaction(req, res, req.params.mod, req.params.id, req.params.auth, GetCollection(req.baseUrl));
});

function GetCollection(URL){
    if (URL.includes("/Player/")){
        return "Players"
    }
    if (URL.includes("/Object/")){
        return "Objects"
    }
}


async function runTransaction(req, res, mod, id, auth, COLL){

    if (CheckServerAuth(auth) || ((await CheckAuth(auth)) && global.config.AllowClientWrite) ){
        var RawData = req.body;
        const client = new MongoClient(global.config.DBServer, { useUnifiedTopology: true });
        try{

            // Connect the client to the server
            await client.connect();
            
            const db = client.db(global.config.DB);
            var collection = db.collection(COLL);
            var query;
            var base;
            if (COLL == "Players"){
                query = { GUID: id };
                base = mod;
            }
            if (COLL == "Objects") {
                query = { ObjectId: id, Mod: mod };
                base = "Data";
            }
            var Element = base + "." + RawData.Element;
            var stringData = "{ \"$inc\": { \""+Element+"\": " + RawData.Value + " } }"
            var options = { upsert: false };
            var Results = await collection.updateOne(query, JSON.parse(stringData), options);
            if (Results.result.ok == 1 && Results.result.n > 0){
                var Value = await collection.distinct(Element, query);
                log("Transaction " + mod + " id " + id + " incermented " + Element + " by " + RawData.Value + " now " + Value[0], "info");
                res.json({Status: "Success", ID: id,  Value: Value[0], Element: RawData.Element})
            } else {
                log("Error in Transaction:  " + mod + " id " + id + " for " + COLL + " error: Invaild ID", "warn");
                res.json({Status: "Error", ID: id,  Value: 0, Element: RawData.Element})
            }
        }catch(err){
            log("Error in Transaction:  " + mod + " id " + id + " for " + COLL + " error: " + err, "warn");
            res.status(203);
            res.json({Status: "Error", ID: id, Value: 0, Element: RawData.Element });
        }finally{
            // Ensures that the client will close when you finish/error
            await client.close();
        }
    } else {
        res.status(401);
        res.json({Status: "Error", Error: "Invalid Auth", ID: id, Value: 0, Element: RawData.Element });
    }

}