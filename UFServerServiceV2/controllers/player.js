const express = require('express');
const router = express.Router();
const { isArray, isObject, isEmpty, NormalizeToGUID, createLogger } = require('../utils');
const logger = createLogger(global.logger, 'c.player');

const { CheckAuth, CheckPlayerAuth, CheckServerAuth, requireServerAuth, requirePlayerOrServerAuth } = require('../auth/utils')

// Import your player model functions
const { getPlayer, getPlayerModData, playerExists, newPlayer, updatePlayer, updatePlayerModData, updatePlayerField, runPlayerTransaction, runValidatedPlayerTransaction } = require('../models/player');

// ----- Endpoint Handlers -----
const queryHandler = require("./query");

router.use('/Query', queryHandler);

/**
 * Non-public load.
 * Read requires either valid server auth OR valid player auth (handled by middleware).
 */
router.post('/Load/:GUID/:mod', requirePlayerOrServerAuth, runGet);
router.post('/Save/:GUID/:mod', requireServerAuth, runSave);
router.post('/Update/:GUID/:mod', requireServerAuth, runUpdate);
router.post('/PublicLoad/:GUID/:mod', runGetPublic);
router.post('/PublicSave/:GUID/:mod', requireServerAuth, runSavePublic);
router.post('/Transaction/:GUID/:mod', requireServerAuth, runTransaction);

async function runGet(req, res) {
    const GUID = req.params.GUID;
    const mod = req.params.mod;
    logger.debug(`Player data load request for GUID ${GUID} from: ${ req.isServer ? "Server": "Client"} and mod ${mod}`, { GUID, mod, clientIP: req.ip });
    try {
        const data = await getPlayerModData(GUID, mod);
        if (!data) {
            logger.info(`Player or mod data not found for GUID ${GUID} and mod ${mod}`, { GUID, mod });
            return res.status(404).json({ error: 'Player or mod data not found' });
        }
        logger.info(`Player data loaded successfully for GUID ${GUID} and mod ${mod}`, { GUID, mod });
        return res.json(data);
    } catch (err) {
        logger.error(`Error loading player data for GUID ${GUID} and mod ${mod}: ${err.message}`, { error: err });
        return res.status(500).json({ error: err.message });
    }
}

/**
 * Save mod data (write operation).
 * Requires server auth (handled by middleware).
 */
async function runSave(req, res) {
    const GUID = req.params.GUID;
    const mod = req.params.mod;
    logger.debug(`Player data save request for GUID ${GUID} and mod ${mod}`, { GUID, mod, clientIP: req.ip });
    
    try {
        const modData = req.body;
        let result;
        const playerExistsFlag = await playerExists(GUID);
        
        if (playerExistsFlag) {
            logger.debug(`Updating existing player mod data for GUID ${GUID} and mod ${mod}`, { GUID, mod });
            result = await updatePlayerModData(GUID, mod, modData);
        } else {
            logger.info(`Creating new player record for GUID ${GUID} and mod ${mod}`, { GUID, mod });
            const newDoc = { GUID, [mod]: modData };
            result = await newPlayer(GUID, newDoc);
        }
        
        logger.info(`Player data saved successfully for GUID ${GUID} and mod ${mod} (isNewPlayer: ${!playerExistsFlag})`, { GUID, mod, isNewPlayer: !playerExistsFlag });
        return res.json(result);
    } catch (err) {
        logger.error(`Error saving player data for GUID ${GUID} and mod ${mod}: ${err.message}`, { error: err });
        return res.status(500).json({ error: err.message });
    }
}

/**
 * Updates a specific field in a player's document.
 */
async function runUpdate(req, res) {
    const GUID = req.params.GUID;
    const mod = req.params.mod;
    logger.debug(`Player field update request for GUID ${GUID} and mod ${mod}`, { GUID, mod, clientIP: req.ip });
    
    try {
        const { Element, Operation, Value } = req.body;
        if (!Element || !Operation || Value === undefined) {
            logger.warn(`Invalid update payload for GUID ${GUID} and mod ${mod}`, { GUID, mod, body: req.body });
            return res.status(400).json({ error: 'Invalid update payload. Must include Element, Operation, and Value.' });
        }
        
        logger.debug(`Updating player field ${Element} with operation ${Operation} for GUID ${GUID} and mod ${mod}`, { GUID, mod, element: Element, operation: Operation });
        const result = await updatePlayerField(GUID, mod, Element, Operation, tryConvertToObject(Value));
        
        logger.info(`Player field ${Element} updated successfully for GUID ${GUID} and mod ${mod}`, { GUID, mod, element: Element });
        return res.json(result);
    } catch (err) {
        logger.error(`Error updating player field ${req.body.Element} for GUID ${GUID} and mod ${mod}: ${err.message}`, { error: err });
        return res.status(500).json({ error: err.message });
    }
}

/**
 * Public load.
 * No auth required, but loads mod data with the "Public." prefix.
 */
async function runGetPublic(req, res) {
    const GUID = NormalizeToGUID(req.params.GUID);
    const mod = req.params.mod;
    logger.debug(`Public player data load request for GUID ${GUID} and mod ${mod}`, { GUID, mod, clientIP: req.ip });
    
    try {
        const publicMod = `Public.${mod}`;
        const data = await getPlayerModData(GUID, mod, publicMod);
        if (!data) {
            logger.info(`Public player data not found for GUID ${GUID} and mod ${mod}`, { GUID, mod });
            return res.status(404).json({ error: 'Player or public mod data not found' });
        }
        
        logger.info(`Public player data loaded successfully for GUID ${GUID} and mod ${mod}`, { GUID, mod });
        return res.json(data);
    } catch (err) {
        logger.error(`Error loading public player data for GUID ${GUID} and mod ${mod}: ${err.message}`, { error: err });
        return res.status(500).json({ error: err.message });
    }
}

/**
 * Public save.
 * Write operations require server auth (handled by middleware).
 */
async function runSavePublic(req, res) {
    const GUID = NormalizeToGUID(req.params.GUID);
    const mod = req.params.mod;
    logger.debug(`Public player data save request for GUID ${GUID} and mod ${mod}`, { GUID, mod, clientIP: req.ip });
    
    try {
        const publicMod = `Public.${mod}`;
        const modData = req.body;
        let result;
        const playerExistsFlag = await playerExists(GUID);
        
        if (playerExistsFlag) {
            logger.debug(`Updating existing public player data for GUID ${GUID} and mod ${mod}`, { GUID, mod });
            result = await updatePlayerModData(GUID, publicMod, modData);
        } else {
            logger.info(`Creating new player with public data for GUID ${GUID} and mod ${mod}`, { GUID, mod });
            const newDoc = { GUID, [publicMod]: modData };
            result = await newPlayer(GUID, newDoc);
        }
        
        logger.info(`Public player data saved successfully for GUID ${GUID} and mod ${mod} (isNewPlayer: ${!playerExistsFlag})`, { GUID, mod, isNewPlayer: !playerExistsFlag });
        return res.json(result);
    } catch (err) {
        logger.error(`Error saving public player data for GUID ${GUID} and mod ${mod}: ${err.message}`, { error: err });
        return res.status(500).json({ error: err.message });
    }
}

/**
 * Runs a transaction that increments a specified field within a player's document.
 */
async function runTransaction(req, res) {
    const GUID = req.params.GUID;
    const mod = req.params.mod;
    try {
        const transactionData = req.body;
        if (!transactionData.Element || transactionData.Value === undefined) {
            logger.warn(`Invalid transaction payload for GUID ${GUID} and mod ${mod}`, { GUID, mod, body: req.body });
            return res.status(400).json({ Status: "Error", ID: GUID, Mod: mod, Error: 'Invalid transaction payload. Must include Element and Value.' });
        }
        
        logger.debug(`Running player transaction on element ${transactionData.Element} with value ${transactionData.Value} for GUID ${GUID} and mod ${mod}`, { GUID, mod, element: transactionData.Element });
        const result = await runPlayerTransaction(transactionData, mod, GUID);
        
        logger.info(`Player transaction completed for GUID ${GUID} and mod ${mod} on element ${transactionData.Element}`, { GUID, mod, result });
        return res.json(result);
    } catch (err) {
        logger.error(`Error in player transaction for GUID ${GUID} and mod ${mod} on element ${req.body.Element}: ${err.message}`, { error: err });
        return res.status(500).json({ Status: "Error", ID: GUID, Mod: mod, Error: err.message });
    }
}

/**
 * Runs a validated transaction that increments a field within limits.
 */
async function runValidatedTx(req, res) {
    const GUID = req.params.GUID;
    const mod = req.params.mod;

    try {
        const transactionData = req.body;
        if (!transactionData.Element || transactionData.Value === undefined ||
            transactionData.Min === undefined || transactionData.Max === undefined) {
            logger.warn(`Invalid validated transaction payload for GUID ${GUID} and mod ${mod}`, { GUID, mod, body: req.body });
            return res.status(400).json({ Status: "Error", ID: GUID, Mod: mod,
                Error: 'Invalid transaction payload. Must include Element, Value, Min, and Max.' 
            });
        }
        
        logger.debug(`Running validated player transaction for GUID ${GUID} and mod ${mod} on element ${transactionData.Element} with value ${transactionData.Value} (min ${transactionData.Min}, max ${transactionData.Max})`, { GUID, mod });
        
        const result = await runValidatedPlayerTransaction(transactionData, mod, GUID);
        
        logger.info(`Validated player transaction completed for GUID ${GUID}, mod ${mod} on element ${transactionData.Element}`, { GUID, mod, result });
        return res.json(result);
    } catch (err) {
        logger.error(`Error in validated player transaction for GUID ${GUID} and mod ${mod} on element ${req.body.Element}: ${err.message}`, { error: err });
        return res.status(500).json({ Status: "Error", ID: GUID, Mod: mod, Error: err.message });
    }
}

async function Transaction(req, res) {
    const GUID = req.params.GUID;
    const mod = req.params.mod;
    let RawData = req.body;
    if (RawData.Min !== undefined && RawData.Max !== undefined && RawData.Min !== RawData.Max) {
        RunValidatedTransaction(req, res);
    } else {
        RunTransaction(req, res);
    }
}

module.exports = router;
