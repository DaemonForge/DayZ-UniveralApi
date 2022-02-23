const {Router} = require('express');
const { MongoClient } = require("mongodb");

const log = require("./log");

const {CheckAuth,CheckServerAuth} = require('./AuthChecker');
const {NormalizeToGUID} = require('./utils');

const router = Router();

module.exports = router;

router.post('/:id/:mod', (req, res)=>{
    let GUID = req.params.id;
    let collection = GetCollection(req.baseUrl);
    if (collection === "Players") {
        GUID = NormalizeToGUID(req.params.id);
    }
    Transaction(req, res, req.params.mod, GUID, req.headers['auth-key'], collection);
});

function GetCollection(URL){
    if (URL.includes("/Player/")){
        return "Players"
    }
    if (URL.includes("/Object/")){
        return "Objects"
    }
}


async function Transaction(req, res, mod, id, auth, COLL){
    if (CheckServerAuth(auth) || ((await CheckAuth(auth)) && global.config.AllowClientWrite) ){
        let RawData = req.body;
        if (RawData.Min !== undefined && RawData.Max !== undefined && RawData.Min !== RawData.Max) {
            RunValidatedTransaction(RawData,res,mod,id,COLL)
        } else {
            RunTransaction(RawData, res, mod,id, COLL)
        }
    } else {
        res.status(401);
        res.json({Status: "Error", Error: "Invalid Auth", Mod: mod, ID: id, Value: 0, Element: RawData.Element });
    }

}

async function RunTransaction(data, res, mod, id, COLL){
    const client = new MongoClient(global.config.DBServer, { useUnifiedTopology: true });
    try{
        // Connect the client to the server
        await client.connect();
        
        const db = client.db(global.config.DB);
        let collection = db.collection(COLL);
        let query;
        let base;
        if (COLL == "Players"){
            query = { GUID: id };
            base = mod;
        }
        if (COLL == "Objects") {
            query = { ObjectId: id, Mod: mod };
            base = "Data";
        }
        let Element = base + "." + data.Element;
        let stringData = "{ \"$inc\": { \""+Element+"\": " + data.Value + " } }";
        let options = { upsert: false };
        let Results = await collection.updateOne(query, JSON.parse(stringData), options);
        if (Results.matchedCount >= 1 || Results.upsertedCount >= 1){
            let Value = await collection.distinct(Element, query);
            log("Transaction " + mod + " id " + id + " incermented " + Element + " by " + data.Value + " now " + Value[0], "info");
            res.json({Status: "Success", ID: id, Mod: mod,  Value: Value[0], Element: data.Element})
        } else {
            log("Error in Transaction:  " + mod + " id " + id + " for " + COLL + " error: Invaild ID", "warn");
            res.json({Status: "NotFound", ID: id, Mod: mod,  Value: 0, Element: data.Element})
        }
    }catch(err){
        log("Error in Transaction:  " + mod + " id " + id + " for " + COLL + " error: " + err, "warn");
        res.status(500);
        res.json({Status: "Error", ID: id, Mod: mod,  Value: 0, Element: data.Element });
    }finally{
        // Ensures that the client will close when you finish/error
        await client.close();
    }
}

async function RunValidatedTransaction(data, res, mod, id, COLL){

    const client = new MongoClient(global.config.DBServer, { useUnifiedTopology: true });
    try{
        // Connect the client to the server
        await client.connect();
        
        const db = client.db(global.config.DB);
        let collection = db.collection(COLL);
        let query;
        let base;
        if (COLL == "Players"){
            query = { GUID: id };
            base = mod;
        }
        if (COLL == "Objects") {
            query = { ObjectId: id, Mod: mod };
            base = "Data";
        }
        let Element = base + "." + data.Element;
        let stringData = "{ \"$inc\": { \""+Element+"\": " + data.Value + " } }";
        
        let OldValue = await collection.distinct(Element, query);
        OldValue = OldValue[0];
        if (OldValue !== undefined){
            let NewValue = OldValue + data.Value;
            if (NewValue <= data.Max && NewValue >= data.Min){
                let options = { upsert: false };
                let Results = await collection.updateOne(query, JSON.parse(stringData), options);
                if (Results.matchedCount >= 1 || Results.upsertedCount >= 1){
                    let Value = await collection.distinct(Element, query);
                    log("Transaction " + mod + " id " + id + " incermented " + Element + " by " + data.Value + " now " + Value[0], "info");
                    res.json({Status: "Success", Error: "", ID: id, Mod: mod,  Value: Value[0], Element: data.Element})
                } else {
                    log("Error in Transaction:  " + mod + " id " + id + " for " + COLL + " error: ", "warn");
                    res.json({Status: "Error", Error: "Failed to Update", ID: id, Mod: mod,  Value: 0, Element: data.Element})
                }
            } else {
                log("Transaction:  " + mod + " id " + id + " for " + COLL + " didn't incerment New Value would be above Max or below Min");
                res.json({Status: "Error", Error: "Out of Range", ID: id, Mod: mod,  Value: OldValue, Element: data.Element})
            }
        } else {
            log("Error in Transaction:  " + mod + " id " + id + " for " + COLL + " error: Invaild ID", "warn");
            res.json({Status: "NotFound", Error: "Element or object not found",ID: id, Mod: mod,  Value: 0, Element: data.Element})
        }
    }catch(err){
        log("Error in Transaction:  " + mod + " id " + id + " for " + COLL + " error: " + err, "warn");
        res.status(500);
        res.json({Status: "Error", Error: `Err: ${err}`, ID: id, Mod: mod, Value: 0, Element: data.Element });
    }finally{
        // Ensures that the client will close when you finish/error
        await client.close();
    }
}