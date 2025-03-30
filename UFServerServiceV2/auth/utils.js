
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
    if (CheckServerAuth(auth)) {
        req.isServer = true;
        logger.debug('Server auth successful', { mod: req.params.mod });
        return next();
    }
    logger.warn(`Unauthorized player data access attempt requireServerAuth ${req.url}`, { mod: req.params.mod, auth });
    return res.status(204).json({ Status: "NoAuth", Error: 'Unauthorized' });
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
    // Check for server auth first
    if (CheckServerAuth(auth)) {
        logger.debug('Server auth successful', { mod: req.params.mod });
        req.isServer = true;
        return next();
    }

    // Get GUID from params or extract from auth token if not present
    let GUID = req.params.GUID;
    if (!GUID) {
        GUID = AuthPlayerGuid(auth);
        if (!GUID) {
            req.params.GUID = GUID;
            logger.warn('No GUID available for authentication', { mod: req.params.mod });
            return res.status(204).json({Status: "NoAuth", Error: 'Unauthorized' });
        }
    } else {
        GUID = NormalizeToGUID(GUID);
    }

    // Check for player auth
    try {
        if (await CheckPlayerAuth(GUID, auth)) {
            logger.debug('Player auth successful', { GUID });
            return next();
        }
        logger.warn(`Unauthorized player data access attempt requirePlayerOrServerAuth ${req.url}`, { GUID, mod: req.params.mod});
        return res.status(204).json({Status: "NoAuth", Error: 'Unauthorized' });
    } catch (err) {
        logger.error('Auth error', { error: err.message });
        return res.status(204).json({ Status: "NoAuth", Error: 'Authentication error' });
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
    const client = new MongoClient(global.config.DBServer);
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
            logger.warn("Player authentication error", { guid: guid, error: err });
        } finally{
            await client.close();
            return isAuth;
        }
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
 * The token expires in 1300 seconds (~22 minutes), designed with a longer 
 * expiration to ensure API downtime doesn't interrupt authentication.
 * Tokens are typically renewed every 10 minutes.
 *
 * @param {string} GUID - Unique identifier for the player
 * @returns {string} Signed JWT authentication token
 * @throws {Error} When token generation fails
 */
function makeAuthToken(GUID) {
    try {
        const player = { GUID: GUID }; 
        //Token expires in ~22 minutes, tokens renew every 10 Minutes ensuring that if the API is down at the time of the renewal token will last till next retry
        return sign(player, GetSigningAuth(), { expiresIn: 1300 });
    } catch (error) {
        logger.error("Failed to create auth token", { guid: GUID, error: error.message });
        throw new Error("Authentication token generation failed");
    }
}

module.exports = { CheckAuth, CheckAuthAgainstGUID, AuthPlayerGuid, CheckPlayerAuth, CheckServerAuth, GetSigningAuth, makeAuthToken,requireServerAuth,requirePlayerOrServerAuth };