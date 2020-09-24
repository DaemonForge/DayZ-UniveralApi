const jwt = require('jsonwebtoken');
const config = require('./configLoader');



module.exports = async function CheckAuth(auth){
    return jwt.verify(auth, config.ServerAuth, function(err) {
        if (err) {
            if (err.name == "TokenExpiredError"){
                console.log("Error: Auth Token is expired, it expired at " + err.expiredAt);
            } else if (err.name == "JsonWebTokenError") {
                console.log("Auth Token is Null");
            } else {
                console.log(err);
            }
            return false;
        } else {
            return true;
        }
    });

}
