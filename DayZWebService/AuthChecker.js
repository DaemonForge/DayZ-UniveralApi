const {verify} = require('jsonwebtoken');
const config = require('./configLoader');
const { MongoClient } = require("mongodb");
const {createHash} = require('crypto');

const log = require("./log");

module.exports = {
    CheckAuth, 
    CheckAuthAgainstGUID,
    CheckPlayerAuth,
    AuthPlayerGuid
}

async function CheckAuth(auth, ignoreError = false){
    return verify(auth, config.ServerAuth, function(err, decoded) {
        if (err) {
            if (err.name == "TokenExpiredError"){
                log("Error: Auth Token for " + decoded.GUID + "is expired, it expired at " + err.expiredAt, "warn");
            } else if (ignoreError){ //Used in the status check to avoid the logs from filling up
                return false;
            } else if (err.name == "JsonWebTokenError") {
                log("Auth Token is not valid", "warn");
            } else {
                log(err, "warn");
            }
            return false;
        } else {
            //log("User Auth " + decoded.GUID)
            return true;
        }
    });

}
async function CheckAuthAgainstGUID(auth, guid, ignoreError = false){
    return verify(auth, config.ServerAuth, function(err, decoded) {
        if (err) {
            if (err.name == "TokenExpiredError"){
                log("Error: Auth Token for " + decoded.GUID + "is expired, it expired at " + err.expiredAt, "warn");
            } else if (ignoreError){ //Used in the status check to avoid the logs from filling up
                return false;
            } else if (err.name == "JsonWebTokenError") {
                log("Auth Token is not valid", "warn");
            } else {
                log(err, "warn");
            }
            return false;
        } else {
            
            return (guid === decoded.GUID);
        }
    });

}

function AuthPlayerGuid(auth, ignoreError = false){
    let guid = verify(auth, config.ServerAuth, function(err, decoded) {
        if (err) {
            if (err.name == "TokenExpiredError"){
                log("Error: Auth Token for " + decoded.GUID + "is expired, it expired at " + err.expiredAt, "warn");
            } else if (ignoreError){ //Used in the status check to avoid the logs from filling up
                return "";
            } else if (err.name == "JsonWebTokenError") {
                log("Auth Token is not valid", "warn");
            } else {
                log(err, "warn");
            }
            return "";
        } else {
            //console.log(decoded)
            return decoded.GUID;
        }
    });
    //console.log(guid);
    return guid;
}

async function CheckPlayerAuth(guid, auth){
    let isAuth = false;
    const client = new MongoClient(config.DBServer, { useUnifiedTopology: true });
    if ((await CheckAuthAgainstGUID(auth, guid, true))){
        try{
            await client.connect();
            // Connect the client to the server        
            const db = client.db(config.DB);
            let collection = db.collection("Players");
            let SavedAuth = createHash('sha256').update(auth).digest('base64');
            let query = { GUID: guid, AUTH: SavedAuth };
            let results = collection.find(query);
                if ((await results.count()) != 0){
                    isAuth = true;
                }
        } catch(err){
            log("ID " + guid + " err" + err, "warn");
        } finally{
            await client.close();
            return isAuth;
        }
    }
    return isAuth;
}