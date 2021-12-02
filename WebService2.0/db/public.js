const {
    MongoClient
} = require("mongodb");
const {
    IncermentAPICount,
    GetClientInfoById,
    HandleBadAuthkey,
    NormalizeToGUID,
    byteSize
} = require('../utils');
const log = require("../log");
module.exports = {
    publicPlayerLoad,
    runGetPublic,
    runSavePublic
}

async function publicPlayerLoad(req, res, next) {
    let clientData = await GetClientInfoById(req.params.ClientId);
    if (clientData === undefined) return HandleBadAuthkey(res)
    let GUID = NormalizeToGUID(req.params.GUID);
    runGetPublic(req, res, GUID, req.params.mod, clientData.DB, "Players")
}

async function runGetPublic(req, res, GUID, mod, database, COLL) {
    const client = new MongoClient(global.config.DBServer, {
        useUnifiedTopology: true
    });
    try {
        await client.connect();
        // Connect the client to the server
        const db = client.db(database); //req.ClientInfo.DB
        let collection = db.collection(COLL);
        let query = {
            GUID: GUID
        };
        let results = collection.find(query);
        let RawData = req.body;

        if ((await results.count()) == 0) {
            if (req.KeyType !== "null" && req.IsServer) {
                log("Can't find Player with ID " + GUID + " Creating it now");
                const doc = JSON.parse(`{ "GUID": "${GUID}", "Public": { "${mod}": "${RawData.Value}" } }`);
                await collection.insertOne(doc);
            } else {
                log("Can't find Player with ID " + GUID, "warn");
            }
            res.status(201);
            res.json(RawData);
        } else {
            let dataarr = await results.toArray();
            let data = dataarr[0];
            let sent = false;
            if (data !== undefined && data.Public !== undefined)
                for (const [key, value] of Object.entries(data.Public)) {
                    if (key === mod) {
                        sent = true;
                        res.json({
                            "Value": value
                        });
                        log("Retrieving " + mod + " Data for GUID: " + GUID);
                    }
                }
            if (sent !== true) {
                if (req.IsServer === true) {
                    const updateDocValue = JSON.parse(`{ "Public.${mod}": "${RawData.Value}" }`);
                    const updateDoc = {
                        $set: updateDocValue,
                    };
                    const options = {
                        upsert: false
                    };
                    await collection.updateOne(query, updateDoc, options);
                    log("Can't find " + mod + " Data for GUID: " + GUID + " Creating it now");
                } else {
                    log("Can't find " + mod + " Data for GUID: " + GUID, "warn");
                }
                res.status(203);
                res.json(RawData);
                if (req.KeyType !== "null") {
                    IncermentAPICount(req.ClientInfo.ClientId, byteSize(RawData));
                }
            }
        }
    } catch (err) {
        res.status(203);
        res.json({
            Value: "Error"
        });
        log("ERROR: " + err, "warn");
    } finally {
        // Ensures that the client will close when you finish/error
        client.close();
    }
};
async function runSavePublic(req, res, GUID, mod) {
    const client = new MongoClient(global.config.DBServer, {
        useUnifiedTopology: true
    });
    try {
        await client.connect();

        let RawData = req.body;
        // Connect the client to the server
        const db = client.db(req.ClientInfo.DB);
        let collection = db.collection(req.Collection);
        let query = {
            GUID: GUID
        };
        const options = {
            upsert: true
        };
        const jsonString = `{ "GUID": "${GUID}", "Public.${mod}": "${RawData.Value}" }`;
        const updateDocValue = JSON.parse(jsonString);
        const updateDoc = {
            $set: updateDocValue,
        };
        const result = await collection.updateOne(query, updateDoc, options);
        if (result.matchedCount === 1 || result.upsertedCount === 1) {
            log("Updated " + mod + " Data for GUID: " + GUID);
            res.status(200);
            res.json(RawData);
            IncermentAPICount(req.ClientInfo.ClientId, byteSize(RawData));
        } else {
            log("Error with Updating " + mod + " Data for GUID: " + GUID, "warn");
            res.status(203);
            res.json(RawData);
        }
    } catch (err) {
        res.status(203);
        res.json(req.body);
        log("ERROR: " + err, "warn");
    } finally {
        // Ensures that the client will close when you finish/error
        await client.close();
    }
};