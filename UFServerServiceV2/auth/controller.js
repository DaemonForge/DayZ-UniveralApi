const {Router} = require('express');
const { requireServerAuth, makeAuthToken} = require('./utils');
const { saveAuthToken } = require('../models/player');

// Use logger from global object instead of direct import
const logger = global.logger;

const router = Router();

/**
 *  Generate Auth Token
 *  Post: Auth/[GUID]
 *  
 *  Description: This generates a auth token for the specified GUID and 
 *   updates the database so that way we can validate that the user has 
 *   already be issued a new. The AUTHTOKEN will expire ion 46 minutes
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
    try{
        let AUTH = makeAuthToken(GUID);
        if ((await saveAuthToken(GUID, AUTH))){
            res.json({GUID, AUTH});
            logger.info("Auth Token Generated", { GUID });
        } else {
            res.json({GUID, AUTH: "ERROR"});
            logger.warn("Error Generating Auth Token", { GUID });
        }
    }catch(err){
        res.json({GUID: GUID, AUTH: "ERROR"});
        logger.error("AUTH ERROR", { 
            url: req.url, 
            error: err.message, 
            stack: err.stack 
        });
    }
};

module.exports = router;
