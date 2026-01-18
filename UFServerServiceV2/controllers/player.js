const express = require('express');
const router = express.Router();
const { NormalizeToGUID, createLogger, tryConvertToObject } = require('../utils');
const logger = createLogger(global.logger, 'c.player');

const { requireServerAuth, requirePlayerOrServerAuth } = require('../auth/utils')

// Import your player model functions
const { getPlayerModData, playerExists, newPlayer, updatePlayerModData, updatePlayerField, runPlayerTransaction, runValidatedPlayerTransaction, deletePlayerModData } = require('../models/player');

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
router.post('/Delete/:GUID/:mod', requireServerAuth, runDelete);

async function runGet(req, res) {
    const GUID = NormalizeToGUID(req.params.GUID);
    const mod = req.params.mod;
    logger.info(`Player load request`, { GUID, mod });
    try {
        const data = await getPlayerModData(GUID, mod);
        if (!data) {
            logger.debug(`Player or mod data not found`, { GUID, mod });
            return res.status(404).json({ error: 'Player or mod data not found' });
        }
        logger.debug(`Player data loaded successfully`, { GUID, mod });
        return res.status(200).json(data);
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
    const GUID = NormalizeToGUID(req.params.GUID);
    const mod = req.params.mod;
    logger.info(`Player save request`, { GUID, mod });
    
    try {
        const modData = req.body;
        let result;
        const playerExistsFlag = await playerExists(GUID);
        
        if (playerExistsFlag) {
            logger.debug(`Updating existing player mod data`, { GUID, mod });
            result = await updatePlayerModData(GUID, mod, modData);
        } else {
            logger.info(`Creating new player record`, { GUID, mod });
            const newDoc = { GUID, [mod]: modData };
            result = await newPlayer(GUID, newDoc);
        }
        
        logger.info(`Player data saved successfully`, { GUID, mod, isNewPlayer: !playerExistsFlag });
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
    const GUID = NormalizeToGUID(req.params.GUID);
    const mod = req.params.mod;
    logger.info(`Player update request`, { GUID, mod, Element, Operation });
    
    try {
        const { Element, Operation, Value } = req.body;
        logger.debug('Request body for update', { Element, Operation, Value });
        if (!Element || !Operation || Value === undefined) {
            logger.warn(`Invalid update payload for GUID ${GUID} and mod ${mod}`, { GUID, mod, body: req.body });
            return res.status(400).json({ error: 'Invalid update payload. Must include Element, Operation, and Value.' });
        }
        
        logger.debug(`Updating player field ${Element} with operation ${Operation} for GUID ${GUID} and mod ${mod}`, { GUID, mod, Element, Operation });
        const result = await updatePlayerField(GUID, mod, Element, Operation, tryConvertToObject(Value));
        logger.debug('updatePlayerField returned', { result });
        
        logger.info(`Player field ${Element} updated successfully for GUID ${GUID} and mod ${mod}`, { GUID, mod, Element });
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
    logger.info(`Player public load request`, { GUID, mod });
    
    try {
        const publicMod = `Public.${mod}`;
        logger.debug(`Using public mod key: ${publicMod}`, { GUID, mod });
        const data = await getPlayerModData(GUID, mod, publicMod);
        logger.debug('getPlayerModData for public load returned', { data });
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
    logger.info(`Player public save request`, { GUID, mod });
    
    try {
        const publicMod = `Public.${mod}`;
        logger.debug(`Using public mod key for save: ${publicMod}`, { GUID, mod });
        const modData = req.body;
        logger.debug('Request body for public save', { modData });
        let result;
        const playerExistsFlag = await playerExists(GUID);
        logger.debug(`playerExists returned ${playerExistsFlag} for GUID ${GUID}`, { GUID });
        
        if (playerExistsFlag) {
            logger.debug(`Updating existing public player data for GUID ${GUID} and mod ${mod}`, { GUID, mod });
            result = await updatePlayerModData(GUID, publicMod, modData);
        } else {
            logger.info(`Creating new player with public data for GUID ${GUID} and mod ${mod}`, { GUID, mod });
            const newDoc = { GUID, [publicMod]: modData };
            result = await newPlayer(GUID, newDoc);
        }
        
        logger.info(`Public player data saved successfully for GUID ${GUID} and mod ${mod} (isNewPlayer: ${!playerExistsFlag})`, { GUID, mod, isNewPlayer: !playerExistsFlag });
        logger.debug('runSavePublic result', { result });
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
    const GUID = NormalizeToGUID(req.params.GUID);
    const mod = req.params.mod;
    logger.info(`Player transaction request`, { GUID, mod });
    
    try {
        const transactionData = req.body;
        logger.debug('Transaction payload received', { transactionData });
        if (!transactionData.Element || transactionData.Value === undefined) {
            logger.warn(`Invalid transaction payload for GUID ${GUID} and mod ${mod}`, { GUID, mod, body: req.body });
            return res.status(400).json({ Status: "Error", ID: GUID, Mod: mod, Error: 'Invalid transaction payload. Must include Element and Value.' });
        }
        
        logger.debug(`Processing transaction on Element ${transactionData.Element} with value ${transactionData.Value}`, { GUID, mod });
        const result = await runPlayerTransaction(transactionData, mod, GUID);
        logger.debug('runPlayerTransaction returned', { result });
        
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
    const GUID = NormalizeToGUID(req.params.GUID);
    const mod = req.params.mod;
    logger.info(`Validated transaction request`, { GUID, mod });

    try {
        const transactionData = req.body;
        logger.debug('Validated transaction payload received', { transactionData });
        if (!transactionData.Element || transactionData.Value === undefined ||
            transactionData.Min === undefined || transactionData.Max === undefined) {
            logger.warn(`Invalid validated transaction payload for GUID ${GUID} and mod ${mod}`, { GUID, mod, body: req.body });
            return res.status(400).json({ Status: "Error", ID: GUID, Mod: mod,
                Error: 'Invalid transaction payload. Must include Element, Value, Min, and Max.' 
            });
        }
        
        logger.debug(`Processing validated transaction on Element ${transactionData.Element} with value ${transactionData.Value} (Min: ${transactionData.Min}, Max: ${transactionData.Max})`, { GUID, mod });
        const result = await runValidatedPlayerTransaction(transactionData, mod, GUID);
        logger.debug('runValidatedPlayerTransaction returned', { result });
        
        logger.info(`Validated player transaction completed for GUID ${GUID}, mod ${mod} on element ${transactionData.Element}`, { GUID, mod, result });
        return res.json(result);
    } catch (err) {
        logger.error(`Error in validated player transaction for GUID ${GUID} and mod ${mod} on element ${req.body.Element}: ${err.message}`, { error: err });
        return res.status(500).json({ Status: "Error", ID: GUID, Mod: mod, Error: err.message });
    }
}

async function Transaction(req, res) {
    const GUID = NormalizeToGUID(req.params.GUID);
    const mod = req.params.mod;
    logger.debug(`Received Transaction call for GUID ${GUID} and mod ${mod}`, { GUID, mod, clientIP: req.ip });
    let RawData = req.body;
    logger.debug('Raw transaction payload', { RawData });
    if (RawData.Min !== undefined && RawData.Max !== undefined && RawData.Min !== RawData.Max) {
        logger.debug('Routing to validated transaction');
        runValidatedTx(req, res);
    } else {
        logger.debug('Routing to normal transaction');
        runTransaction(req, res);
    }
}

/**
 * Deletes a mod's data from a player document
 */
async function runDelete(req, res) {
    const GUID = NormalizeToGUID(req.params.GUID);
    const mod = req.params.mod;
    
    logger.info(`Player delete request`, { GUID, mod });
    
    try {
        const result = await deletePlayerModData(GUID, mod);
        
        if (!result.success || !result.deleted) {
            logger.warn(`Player mod data not found or not deleted`, { GUID, mod, result });
            return res.status(404).json({ 
                error: 'Player or mod data not found',
                deleted: false
            });
        }
        
        logger.info(`Player mod data deleted successfully`, { 
            GUID, 
            mod, 
            modifiedCount: result.modifiedCount
        });
        
        return res.status(200).json({
            success: true,
            deleted: true,
            modifiedCount: result.modifiedCount
        });
    } catch (err) {
        logger.error(`Error deleting player mod data for GUID ${GUID} and mod ${mod}: ${err.message}`, { error: err });
        return res.status(500).json({ error: err.message });
    }
}

module.exports = router;
