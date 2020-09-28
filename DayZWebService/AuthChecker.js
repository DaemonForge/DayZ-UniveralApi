const jwt = require('jsonwebtoken');
const config = require('./configLoader');

const log = require("./log");


module.exports = async function CheckAuth(auth, ignoreError = false){
    return jwt.verify(auth, config.ServerAuth, function(err) {
        if (err) {
            if (err.name == "TokenExpiredError"){
                log("Error: Auth Token is expired, it expired at " + err.expiredAt, "warn");
            } else if (ignoreError){
                return false;
            } else if (err.name == "JsonWebTokenError") {
                log("Auth Token is not valid", "warn");
            } else {
                log(err, "warn");
            }
            return false;
        } else {
            return true;
        }
    });

}
