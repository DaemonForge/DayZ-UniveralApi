
const {isArray, isObject, createLogger, NormalizeToGUID} = require('../utils')

const jwt = require('jsonwebtoken');
const { verify, sign } = jwt;
const { MongoClient } = require("mongodb");
const { createHash } = require('crypto');

// Use logger from global object instead of direct import
const logger = createLogger(global.logger, 'auth');

/**
 * Middleware to authenticate server requests using the auth-key header
 * 
 * @async
 * @function requireServerAuth
 * @param {Object} req - Express request object
 * @param {Object} res - Express response object
 * @param {Function} next - Express next middleware function
 * @returns {Promise<void>} Calls next() if authenticated or returns 403 response if unauthorized
 * @throws {Object} Returns a JSON error response with 403 status if authentication fails
 */
const requireServerAuth = async (req, res, next) => {
    const auth = req.headers['auth-key'];
    req.isServer = false;
    req.serverId = "undefined";
    if (CheckServerAuth(auth)) {
        req.isServer = true;
        req.serverId = findLabelByAuthKey(auth);
        logger.debug('Server auth successful', { mod: req.params.mod });
        return next();
    }
    logger.warn(`Unauthorized player data access attempt requireServerAuth ${req.url}`, { mod: req.params.mod, auth });
    return res.status(401).json({ Status: "NoAuth", Error: 'Unauthorized' });
};


/**
 * Express middleware that authenticates requests using either server auth or player auth.
 * First checks for server authentication, then falls back to player authentication if needed.
 * 
 * @async
 * @param {Object} req - Express request object
 * @param {Object} req.headers - Request headers
 * @param {string} req.headers['auth-key'] - Authentication key from request header
 * @param {Object} req.params - Request parameters
 * @param {string} [req.params.GUID] - Optional player GUID in request parameters
 * @param {string} [req.params.mod] - Optional mod identifier for logging
 * @param {Object} res - Express response object
 * @param {Function} next - Express next middleware function
 * @returns {Promise<void>} - Calls next() if authentication succeeds or returns error response
 * 
 * @throws {Object} Returns 403 status with error message if authentication fails
 * @throws {Object} Returns 500 status with error message if authentication process errors
 */
const requirePlayerOrServerAuth = async (req, res, next) => {
    const auth = req.headers['auth-key'];
    
    req.isServer = false;
    req.serverId = "undefined";
    // Check for server auth first
    if (CheckServerAuth(auth)) {
        logger.debug('Server auth successful', { mod: req.params.mod });
        req.isServer = true;
        req.serverId = findLabelByAuthKey(auth);
        return next();
    }

    // Get GUID from params or extract from auth token if not present
    let GUID = req.params.GUID;
    if (!GUID) {
        GUID = AuthPlayerGuid(auth);
        if (!GUID) {
            logger.warn('No GUID available for authentication', { mod: req.params.mod });
            return res.status(401).json({Status: "NoAuth", Error: 'Unauthorized' });
        }
    } else {
        GUID = NormalizeToGUID(GUID);
    }
    req.params.GUID = GUID;
    req.GUID = GUID;

    // Check for player auth
    try {
        if (await CheckPlayerAuth(GUID, auth)) {
            logger.debug('Player auth successful', { GUID });
            return next();
        }
        logger.warn(`Unauthorized player data access attempt requirePlayerOrServerAuth ${req.url}`, { GUID, mod: req.params.mod});
        return res.status(401).json({Status: "NoAuth", Error: 'Unauthorized' });
    } catch (err) {
        logger.error('Auth error', { error: err.message });
        return res.status(401).json({ Status: "NoAuth", Error: 'Authentication error' });
    }
};



/**
 * Verifies the authenticity of an authentication token.
 * 
 * @async
 * @param {string} auth - The authentication token to verify
 * @param {boolean} [ignoreError=false] - If true, suppresses error logging for failed verifications
 * @returns {Promise<boolean>} A promise that resolves to true if authentication is successful, false otherwise
 * @description This function verifies a JWT using the signing authority. It handles different error cases:
 * - TokenExpiredError: Logs that the token is expired
 * - JsonWebTokenError: Logs that the token is invalid (unless ignoreError is true)
 * - Other errors: Logs a general authentication error (unless ignoreError is true)
 */
async function CheckAuth(auth, ignoreError = false){
    return verify(auth, GetSigningAuth(), function(err, decoded) {
        if (err) {
            if (err.name == "TokenExpiredError"){
                logger.warn("Auth Token is expired", { expiredAt: err.expiredAt });
            } else if (ignoreError){ //Used in the status check to avoid the logs from filling up
                return false;
            } else if (err.name == "JsonWebTokenError") {
                logger.warn("Auth Token is not valid", { errorName: err.name });
            } else {
                logger.warn("Authentication error", { error: err });
            }
            return false;
        } else {
            return true;
        }
    });
}

/**
 * Verifies an authentication token against a provided GUID
 * @async
 * @param {string} auth - The authentication token to verify
 * @param {string} guid - The GUID to check against the token's payload
 * @param {boolean} [ignoreError=false] - Whether to suppress logging for certain errors (typically used in status checks)
 * @returns {Promise<boolean>} - Returns true if the token is valid and contains the matching GUID, otherwise false
 */
async function CheckAuthAgainstGUID(auth, guid, ignoreError = false){
    return verify(auth, GetSigningAuth(), function(err, decoded) {
        if (err) {
            if (err.name == "TokenExpiredError"){
                logger.warn("Auth Token is expired", { guid: decoded?.GUID, expiredAt: err.expiredAt });
            } else if (ignoreError){ //Used in the status check to avoid the logs from filling up
                return false;
            } else if (err.name == "JsonWebTokenError") {
                logger.warn("Auth Token is not valid", { errorName: err.name });
            } else {
                logger.warn("Authentication error", { error: err });
            }
            return false;
        } else {
            return (guid === decoded.GUID);
        }
    });
}

/**
 * Authenticates a player using a JWT token and returns their GUID
 * 
 * @param {string} auth - The JWT authentication token to verify
 * @param {boolean} [ignoreError=false] - When true, suppresses logging for certain errors (used in status checks)
 * @returns {string} The player's GUID if authentication is successful, empty string otherwise
 * 
 * @description
 * This function verifies the provided authentication token against the server's signing authority.
 * It handles various error cases including expired tokens and invalid formats.
 * When authentication fails, it logs appropriate warnings unless ignoreError is set to true.
 */
function AuthPlayerGuid(auth, ignoreError = false){
    let guid = verify(auth, GetSigningAuth(), function(err, decoded) {
        if (err) {
            if (err.name == "TokenExpiredError"){
                logger.warn("Auth Token is expired", { guid: decoded?.GUID, expiredAt: err.expiredAt });
            } else if (ignoreError){ //Used in the status check to avoid the logs from filling up
                return "";
            } else if (err.name == "JsonWebTokenError") {
                logger.warn("Auth Token is not valid", { errorName: err.name });
            } else {
                logger.warn("Authentication error", { error: err });
            }
            return "";
        } else {
            return decoded.GUID;
        }
    });
    return guid;
}

/**
 * Verifies a player's authentication by checking against the database
 * @async
 * @function CheckPlayerAuth
 * @param {string} guid - The Global Unique Identifier of the player
 * @param {string} auth - The authentication token to verify
 * @returns {Promise<boolean>} Returns true if authentication is valid, false otherwise
 * @throws {Error} Logs warning if database authentication process fails
 */
async function CheckPlayerAuth(guid, auth){
    let isAuth = false;
    try {
        return (await CheckAuthAgainstGUID(auth, guid, true));
    } catch (err) {
        logger.error(`CheckPlayerAuth Error ${err.message}`, err);
    }
    return isAuth;
}

/**
 * Verifies if the provided authentication token matches the server authentication configuration.
 * 
 * @param {string|any} auth - The authentication token to verify
 * @returns {boolean} - Returns true if the authentication token is valid, false otherwise
 * 
 * @description
 * This function validates the provided authentication token against the server configuration.
 * It can handle both single token and multiple token configurations:
 * - If global.config.ServerAuth is an array, it checks if auth is in that array
 * - If global.config.ServerAuth is a single value, it directly compares with auth
 * - Returns false if auth is undefined or null
 */
function CheckServerAuth(auth){
    if (auth === undefined || auth === null) return false;
    if (isArray(global.config.ServerAuth) && (global.config.ServerAuth.find(element => element === auth) === auth)) return true;
    if (!isArray(global.config.ServerAuth) && global.config.ServerAuth === auth) return true;
    return false; 
}

/**
 * Retrieves the authentication configuration used for signing operations.
 * If ServerAuth config is an array, returns the first element, otherwise returns the ServerAuth value directly.
 * 
 * @function GetSigningAuth
 * @returns {*} The signing authentication configuration
 */
function GetSigningAuth(){
    if(isArray(global.config.ServerAuth)){
        return global.config.ServerAuth[0];
    } else {
        return global.config.ServerAuth;
    }
}

/**
 * Creates an authentication token for a player based on their GUID.
 * The token expires in 900 seconds (~15 minutes), designed with a longer 
 * expiration to ensure API downtime doesn't interrupt authentication.
 * Tokens are typically renewed every 10 minutes.
 *
 * @param {string} GUID - Unique identifier for the player
 * @param {string} serverId - The server ID that requested the authentication token
 * @returns {string} Signed JWT authentication token
 * @throws {Error} When token generation fails
 */
function makeAuthToken(GUID, serverId) {
    try {
        const player = { GUID: GUID, ServerId: serverId || "undefined" }; 
        //Token expires in ~15 minutes, tokens renew every 10 Minutes
        return sign(player, GetSigningAuth(), { expiresIn: 900 });
    } catch (error) {
        logger.error("Failed to create auth token", { guid: GUID, error: error.message });
        throw new Error("Authentication token generation failed");
    }
}


/**
 * findLabelByAuthKey
 * -------------------
 * Finds the label corresponding to a given authentication key in the configuration.
 *
 * In our configuration, the authentication keys are stored in the array "ServerAuth"
 * and their corresponding labels are stored in a parallel array "ServerAuthLabels". 
 * The function finds the index of the provided auth key and returns the label stored at
 * the same index. If the key is not found or if the labels array is not present, it returns undefined.
 *
 * Note:
 *   - It is assumed that both arrays always maintain a one-to-one relationship:
 *     i.e. if there is an entry in ServerAuth, then ServerAuthLabels (even if empty) 
 *     exists at the same index.
 *
 * @param {string} authKey - The authentication key to search for.
 * @param {object} config - The configuration object that must include a ServerAuth array 
 *                          and, optionally, a ServerAuthLabels array.
 * @returns {string|"undefined"} - The corresponding label (which may be an empty string)
 *                               if found; otherwise, undefined.
 */
function findLabelByAuthKey(authKey) {
    // First, check if a valid configuration object is provided and contains the ServerAuth array.
    if (!config || !Array.isArray(global.ServerAuth)) {
      console.error("Invalid config: ServerAuth array is missing.");
      return "undefined";
    }
  
    // Find the index of the provided authKey in the ServerAuth array.
    const index = global.config.ServerAuth.indexOf(authKey);
  
    // If the authKey is not found, return undefined.
    if (index === -1) {
      return "undefined";
    }
  
    // Check if the ServerAuthLabels array exists in the config.
    if (!Array.isArray(global.config.ServerAuthLabels)) {
      // If the labels array does not exist, we could consider it as no label data present.
      return "undefined";
    }
  
    // Return the label at the corresponding index.
    // Note: The label may be an empty string, which is acceptable.
    return global.config.ServerAuthLabels[index];
  }

module.exports = { CheckAuth, CheckAuthAgainstGUID, findLabelByAuthKey, AuthPlayerGuid, CheckPlayerAuth, CheckServerAuth, GetSigningAuth, makeAuthToken,requireServerAuth,requirePlayerOrServerAuth };