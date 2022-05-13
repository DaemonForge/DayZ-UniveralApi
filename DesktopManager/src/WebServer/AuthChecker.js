const {verify, sign} = require('jsonwebtoken');
const { MongoClient } = require("mongodb");
const {createHash} = require('crypto');
const {isArray} = require('./utils');

const log = require("./log");

module.exports = {
    CheckAuth, 
    CheckAuthAgainstGUID,
    CheckPlayerAuth,
    AuthPlayerGuid,
    CheckServerAuth,
    GetSigningAuth,
    makeAuthToken
}

async function CheckAuth(auth, ignoreError = false){
    return verify(auth, GetSigningAuth(), function(err, decoded) {
        if (err) {
            if (err.name == "TokenExpiredError"){
                log("Error: Auth Token is expired, it expired at " + err.expiredAt, "warn");
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
    return verify(auth, GetSigningAuth(), function(err, decoded) {
        if (err) {
            if (err.name == "TokenExpiredError"){
                log("Error: Auth Token for " + decoded?.GUID + "is expired, it expired at " + err.expiredAt, "warn");
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
    let guid = verify(auth, GetSigningAuth(), function(err, decoded) {
        if (err) {
            if (err.name == "TokenExpiredError"){
                log("Error: Auth Token for " + decoded?.GUID + " is expired, it expired at " + err.expiredAt, "warn");
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
    const client = new MongoClient(global.config.DBServer, { useUnifiedTopology: true });
    if ((await CheckAuthAgainstGUID(auth, guid, true))){
        try{
            await client.connect();
            // Connect the client to the server        
            const db = client.db(global.config.DB);
            let collection = db.collection("Players");
            let SavedAuth = createHash('sha256').update(auth).digest('base64');
            let query = { GUID: guid, AUTH: SavedAuth };
            if ((await collection.countDocuments(query)) != 0){
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

function CheckServerAuth(auth){
    if (auth === undefined || auth === null) return false;
    if (isArray(global.config.ServerAuth) && (global.config.ServerAuth.find(element => element === auth) === auth)) return true;
    if (!isArray(global.config.ServerAuth) && global.config.ServerAuth === auth) return true;
    return false; 
}

function GetSigningAuth(){
    if(isArray(global.config.ServerAuth)){
        return global.config.ServerAuth[0];
    } else {
        return global.config.ServerAuth;
    }
}



function makeAuthToken(GUID) {
    const player = { GUID: GUID }; 
    //Token expires in 46.5 minutes, tokens renew every 21-23 Minutes ensuring that if the API is down at the time of the renewal token will last till next retry
    let result = sign(player, GetSigningAuth(), { expiresIn: 2800 }); 
    return result;
 }