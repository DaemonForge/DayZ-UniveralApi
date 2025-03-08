const express = require('express');
const router = express.Router();
const {isArray, isObject, isEmpty, NormalizeToGUID} = require('../utils')
const logger = global.logger;

const { CheckAuth, CheckPlayerAuth, CheckServerAuth, requireServerAuth, requirePlayerOrServerAuth } = require('../auth/utils')

// Import your player model functions
const { getPlayer, getPlayerModData, playerExists, newPlayer, updatePlayer, updatePlayerModData, updatePlayerField, runPlayerTransaction,runValidatedPlayerTransaction } = require('../models/player');

// ----- Endpoint Handlers -----
const queryHandler = require("./Query");

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
    logger.debug('Player data load request', { GUID, mod, clientIP: req.ip });
    
    try {
        const data = await getPlayerModData(GUID, mod);
        if (!data) {
            logger.info('Player or mod data not found', { GUID, mod });
            return res.status(404).json({ error: 'Player or mod data not found' });
        }
        logger.info('Player data loaded successfully', { GUID, mod });
        return res.json(data);
    } catch (err) {
        logger.error('Error loading player data', { GUID, mod, error: err.message, stack: err.stack });
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
    logger.debug('Player data save request', { GUID, mod, clientIP: req.ip });
    
    try {
        const modData = req.body;
        let result;
        const playerExistsFlag = await playerExists(GUID);
        
        if (playerExistsFlag) {
            logger.debug('Updating existing player mod data', { GUID, mod });
            result = await updatePlayerModData(GUID, mod, modData);
        } else {
            logger.info('Creating new player record', { GUID, mod });
            // Create a new document with the mod data
            const newDoc = { GUID, [mod]: modData };
            result = await newPlayer(GUID, newDoc);
        }
        
        logger.info('Player data saved successfully', { GUID, mod, isNewPlayer: !playerExistsFlag });
        return res.json(result);
    } catch (err) {
        logger.error('Error saving player data', { GUID, mod, error: err.message, stack: err.stack });
        return res.status(500).json({ error: err.message });
    }
}

/**
 * Updates a specific field in a player's document.
 */
async function runUpdate(req, res) {
    const GUID = req.params.GUID;
    const mod = req.params.mod;
    logger.debug('Player field update request', { GUID, mod, clientIP: req.ip });
    
    try {
        const { Element, Operation, Value } = req.body;
        // Validate required fields. Value can be 0 or an empty string so we check undefined explicitly.
        if (!Element || !Operation || Value === undefined) {
            logger.warn('Invalid update payload', { GUID, mod, body: req.body });
            return res.status(400).json({ error: 'Invalid update payload. Must include Element, Operation, and Value.' });
        }
        
        logger.debug('Updating player field', { GUID, mod, element: Element, operation: Operation });
        const result = await updatePlayerField(GUID, mod, Element, Operation, Value);
        
        logger.info('Player field updated successfully', { GUID, mod, element: Element });
        return res.json(result);
    } catch (err) {
        logger.error('Error updating player field', { 
            GUID, 
            mod, 
            element: req.body.Element, 
            operation: req.body.Operation, 
            error: err.message, 
            stack: err.stack 
        });
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
    logger.debug('Public player data load request', { GUID, mod, clientIP: req.ip });
    
    try {
        const publicMod = `Public.${mod}`;
        const data = await getPlayerModData(GUID, mod, publicMod);
        if (!data) {
            logger.info('Public player data not found', { GUID, mod });
            return res.status(404).json({ error: 'Player or public mod data not found' });
        }
        
        logger.info('Public player data loaded successfully', { GUID, mod });
        return res.json(data);
    } catch (err) {
        logger.error('Error loading public player data', { GUID, mod, error: err.message, stack: err.stack });
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
    logger.debug('Public player data save request', { GUID, mod, clientIP: req.ip });
    
    try {
        const publicMod = `Public.${mod}`;
        const modData = req.body;
        let result;
        const playerExistsFlag = await playerExists(GUID);
        
        if (playerExistsFlag) {
            logger.debug('Updating existing public player data', { GUID, mod });
            result = await updatePlayerModData(GUID, publicMod, modData);
        } else {
            logger.info('Creating new player with public data', { GUID, mod });
            const newDoc = { GUID, [publicMod]: modData };
            result = await newPlayer(GUID, newDoc);
        }
        
        logger.info('Public player data saved successfully', { GUID, mod, isNewPlayer: !playerExistsFlag });
        return res.json(result);
    } catch (err) {
        logger.error('Error saving public player data', { GUID, mod, error: err.message, stack: err.stack });
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
            logger.warn('Invalid transaction payload', { GUID, mod, body: req.body });
            return res.status(400).json({ Status: "Error", ID: GUID, Mod: mod, Error: 'Invalid transaction payload. Must include Element and Value.' });
        }
        
        logger.debug('Running player transaction', { GUID, mod, element: transactionData.Element, value: transactionData.Value });
        const result = await runPlayerTransaction(transactionData, mod, GUID);
        
        logger.info('Player transaction completed', { GUID, mod, element: transactionData.Element, result });
        return res.json(result);
    } catch (err) {
        logger.error('Error in player transaction', { 
            GUID, 
            mod, 
            element: req.body.Element, 
            error: err.message, 
            stack: err.stack 
        });
        return res.status(500).json({Status: "Error", ID: GUID, Mod: mod, Error: err.message });
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
            logger.warn('Invalid validated transaction payload', { GUID, mod, body: req.body });
            return res.status(400).json({ Status: "Error", ID: GUID, Mod: mod,
                Error: 'Invalid transaction payload. Must include Element, Value, Min, and Max.' 
            });
        }
        
        logger.debug('Running validated player transaction', { 
            GUID, mod, element: transactionData.Element, 
            value: transactionData.Value, min: transactionData.Min, max: transactionData.Max 
        });
        
        const result = await runValidatedPlayerTransaction(transactionData, mod, GUID);
        
        logger.info('Validated player transaction completed', { 
            GUID, mod, element: transactionData.Element, result 
        });
        return res.json(result);
    } catch (err) {
        logger.error('Error in validated player transaction', { 
            GUID, 
            mod, 
            element: req.body.Element, 
            error: err.message, 
            stack: err.stack 
        });
        return res.status(500).json({Status: "Error", ID: GUID, Mod: mod, Error: err.message });
    }
}

async function Transaction(req, res){
    const GUID = req.params.GUID;
    const mod = req.params.mod;
    let RawData = req.body;
    if (RawData.Min !== undefined && RawData.Max !== undefined && RawData.Min !== RawData.Max) {
        RunValidatedTransaction(req, res)
    } else {
        RunTransaction(req, res)
    }
}


module.exports = router;