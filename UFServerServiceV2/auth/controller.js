const {Router} = require('express');
const { requireServerAuth, makeAuthToken } = require('./utils');
const { saveAuthToken } = require('../models/player');
const { createLogger } = require('../utils');

// Use logger from global object instead of direct import
const logger = createLogger(global.logger, 'auth');

const router = Router();

/**
 *  Generate Auth Token
 *  Post: Auth/[GUID]
 *  
 *  Description: This generates an auth token for the specified GUID and 
 *   updates the database so that way we can validate that the user has 
 *   already been issued a new token. The AUTHTOKEN will expire in 46 minutes
 * 
 *  Returns: `{ 
 *                "GUID": "|THEPASSEDGUID|", 
 *                "AUTH": "|AUTHTOKEN|" 
 *            }`
 * 
 */
router.post('/:GUID', requireServerAuth, runGetAuth);

async function runGetAuth(req, res) {
    let GUID = req.params.GUID;
    logger.debug("Received request for auth token generation", { GUID });
    try {
        let AUTH = makeAuthToken(GUID);
        logger.debug("Auth token generated", { GUID, AUTH });
        if ((await saveAuthToken(GUID, AUTH))) {
            res.json({ GUID, AUTH });
            logger.info("Auth Token Generated", { GUID });
        } else {
            res.json({ GUID, AUTH: "ERROR" });
            logger.warn("Error Generating Auth Token", { GUID });
        }
    } catch (err) {
        res.json({ GUID: GUID, AUTH: "ERROR" });
        logger.error("AUTH ERROR", { 
            url: req.url, 
            error: err.message, 
            stack: err.stack 
        });
        logger.debug("Error details", { GUID, errorObject: err });
    }
};

module.exports = router;
