const jwt = require('jsonwebtoken');
const config = require('./configLoader');

const log = require("./log");


module.exports = async function CheckAuth(auth, ignoreError = false){
    return jwt.verify(auth, config.ServerAuth, function(err, decoded) {
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
