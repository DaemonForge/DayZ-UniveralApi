// controllers/objectController.js

const { Router } = require('express');
const router = Router();

const { requireServerAuth, requirePlayerOrServerAuth } = require('../auth/utils');
const { updateObject, runObjectTransaction, runValidatedObjectTransaction, getObject, newObject, updateObjectField, deleteObject } = require('../models/object');
const { makeObjectId, isEmpty, createLogger, tryConvertToObject } = require('../utils');
const logger = createLogger(global.logger, 'c.object');

// Mount the legacy query handler if needed
const queryHandler = require('./query');
router.use('/Query', queryHandler);

/**
 * POST /Load/:ObjectId/:mod
 * Loads an object. If not found and request is from a server, 
 * a new object can be created.
 * Both players and servers can load, but only servers can create.
 */
router.post('/Load/:ObjectId/:mod', requirePlayerOrServerAuth, loadObject);

/**
 * POST /Save/:ObjectId/:mod
 * Upserts an object with the provided data.
 * Only servers are allowed to save.
 */
router.post('/Save/:ObjectId/:mod', requireServerAuth, saveObject);

/**
 * POST /Update/:ObjectId/:mod
 * Updates a specific field in an object document using a designated operation.
 * Only servers are allowed to update.
 */
router.post('/Update/:ObjectId/:mod', requireServerAuth, runUpdate);

/**
 * POST /Transaction/:ObjectId/:mod
 * This function processes incoming transaction requests for objects in the DayZ universe.
 * It determines whether to run a validated transaction (when Min/Max range is specified)
 * or a standard transaction based on the provided data.
 */
router.post('/Transaction/:ObjectId/:mod', requireServerAuth, runTransaction);

/**
 * POST /Delete/:ObjectId/:mod
 * Deletes an object from the database
 * Only servers are allowed to delete.
 */
router.post('/Delete/:ObjectId/:mod', requireServerAuth, deleteObjectHandler);

/**
 * Loads an object with the specified ID and mod, creating a new one if requested
 */
async function loadObject(req, res) {
    let { ObjectId, mod } = req.params;
    const data = req.body;
    
    logger.info(`[LOAD][${mod}] Load object request`, { 
        mod, 
        ObjectId, 
        isServer: req.isServer,
        hasBodyData: !isEmpty(data),
        timestamp: new Date().toISOString()
    });
    
    try {
        const results = await getObject(ObjectId, mod);
        
        if (results === null || typeof results === 'undefined') {
            logger.info(`[LOAD][${mod}] Object NOT FOUND in database`, { mod, ObjectId });
            
            if (req.isServer && !isEmpty(data)) {
                if (ObjectId === "NewObject") {
                    ObjectId = makeObjectId();
                    data.ObjectId = ObjectId;
                    logger.info(`[LOAD][${mod}] Creating NEW object with generated ID`, { mod, ObjectId });
                } else {
                    logger.warn(`[LOAD][${mod}] Creating NEW object with PROVIDED ID (potential duplicate risk!)`, { mod, ObjectId, data: JSON.stringify(data) });
                }
                await newObject(ObjectId, mod, data);
                logger.info(`[LOAD][${mod}] NEW OBJECT CREATED via Load endpoint`, { mod, ObjectId, timestamp: new Date().toISOString() });
                return res.status(200).json(data);
            } else {
                logger.debug(`[LOAD][${mod}] Object not found, no creation (not server or no data)`, { mod, ObjectId, isServer: req.isServer });
                return res.status(200).json(data);
            }
        } else {
            logger.info(`[LOAD][${mod}] Object FOUND - returning existing data`, { mod, ObjectId, hasData: !!results });
            return res.status(200).json(results);
        }
    } catch (err) {
        logger.error(`Error in loadObject endpoint: ${err.message}`, { error: err, mod, ObjectId });
        return res.status(500).json({ error: err.message });
    }
}

/**
 * Saves an object with the provided data, creating a new one if needed
 */
async function saveObject(req, res) {
    let { ObjectId, mod } = req.params;
    const data = req.body;
    
    // Enhanced logging for territory duplication debugging
    logger.info(`[SAVE] Object save request started`, { 
        mod, 
        ObjectId, 
        dataKeys: Object.keys(data || {}),
        timestamp: new Date().toISOString()
    });
    
    // Log full data if mod is FactionTerritories to trace duplicates
    if (mod === 'FactionTerritories' || mod === 'Factions') {
        logger.info(`[SAVE][${mod}] Full save data:`, { 
            mod, 
            ObjectId, 
            fullData: JSON.stringify(data),
            stackPreview: new Error().stack.split('\n').slice(2, 5).join('\n')
        });
    }
    
    try {
        // Check if object already exists BEFORE saving
        const existingObject = await getObject(ObjectId, mod);
        if (existingObject) {
            logger.warn(`[SAVE][${mod}] DUPLICATE DETECTED - Object already exists!`, {
                mod,
                ObjectId,
                existingData: JSON.stringify(existingObject),
                newData: JSON.stringify(data)
            });
        }
        
        if (ObjectId === "NewObject") {
            ObjectId = makeObjectId();
            data.ObjectId = ObjectId;
            logger.info(`[SAVE] Generated new ObjectId: ${ObjectId} for new object`, { mod });
        }
        const options = { upsert: true };
        const updateDoc = { $set: { data: data, ObjectId, Mod: mod } };
        const result = await updateObject(ObjectId, mod, updateDoc, options);
        
        // Detailed result logging
        logger.info(`[SAVE][${mod}] Save operation completed`, {
            mod,
            ObjectId,
            matchedCount: result.matchedCount,
            upsertedCount: result.upsertedCount,
            modifiedCount: result.modifiedCount,
            wasUpdate: result.matchedCount === 1,
            wasInsert: result.upsertedCount === 1,
            timestamp: new Date().toISOString()
        });
        
        if (result.matchedCount === 1 || result.upsertedCount === 1) {
            if (result.upsertedCount === 1) {
                logger.info(`[SAVE][${mod}] NEW RECORD CREATED`, { mod, ObjectId });
            } else {
                logger.info(`[SAVE][${mod}] EXISTING RECORD UPDATED`, { mod, ObjectId });
            }
            res.status(200).json(data);
        } else {
            logger.error(`[SAVE][${mod}] SAVE FAILED - No match or upsert!`, { mod, ObjectId, result });
            res.status(200).json(data);
        }
    } catch (err) {
        logger.error(`Error in Save endpoint: ${err.message}`, { error: err, mod, ObjectId });
        res.status(500).json(data);
    }
}

/**
 * Executes a field update operation on a game object
 */
async function runUpdate(req, res) {
    let { ObjectId, mod } = req.params;
    const data = req.body;
    const element = data.Element;
    logger.info(`Object update request`, { mod, ObjectId, element });
    try {
        const operation = data.Operation || "set";
        const value = tryConvertToObject(data.Value);
        logger.debug(`Updating element: ${element} using operation: ${operation} with value: ${JSON.stringify(value)}`);
        const result = await updateObjectField(ObjectId, mod, element, operation, value);
        logger.debug(`updateObjectField result: ${JSON.stringify(result)}`);
        if (result.matchedCount >= 1 || result.upsertedCount >= 1) {
            logger.debug(`Updated ${element} for mod: ${mod}, ObjectId: ${ObjectId}`);
            res.status(200).json({ Status: "Success", Element: element, Mod: mod, ID: ObjectId });
        } else {
            logger.warn(`Error updating ${element} for mod: ${mod}, ObjectId: ${ObjectId}`);
            res.status(200).json({ Status: "NotFound", Element: element, Mod: mod, ID: ObjectId });
        }
    } catch (err) {
        logger.error(`Error in Update endpoint: ${err.message}`, { error: err, mod, ObjectId });
        res.status(200).json({ Status: "Error", Element: data.Element, Mod: mod, ID: ObjectId });
    }
}

/**
 * Executes a transaction on a game object based on request data
 */
async function runTransaction(req, res) {
    let { ObjectId, mod } = req.params;
    const data = req.body;
    logger.info(`Object transaction request`, { mod, ObjectId });
    try {
        let response;
        if (data.Min !== undefined && data.Max !== undefined) {
            logger.debug(`Running validated transaction with Min: ${data.Min} and Max: ${data.Max}`);
            response = await runValidatedObjectTransaction(data, ObjectId, mod);
        } else {
            logger.debug(`Running standard transaction`);
            response = await runObjectTransaction(data, ObjectId, mod);
        }
        logger.debug(`Transaction response: ${JSON.stringify(response)}`);
        res.json(response);
    } catch (err) {
        logger.error(`Transaction error: ${err.message}`, { error: err, mod, id: ObjectId });
        res.status(500).json({ Status: "Error", Error: err.message, ID: ObjectId, Mod: mod, Value: 0, Element: data.Element });
    }
}

/**
 * Deletes an object from the database
 */
async function deleteObjectHandler(req, res) {
    const { ObjectId, mod } = req.params;
    
    logger.info(`[DELETE][${mod}] Delete object request`, { 
        mod, 
        ObjectId,
        timestamp: new Date().toISOString()
    });
    
    try {
        const result = await deleteObject(ObjectId, mod);
        
        if (!result.success || !result.deleted) {
            logger.warn(`[DELETE][${mod}] Object not found or not deleted`, { mod, ObjectId, result });
            return res.status(404).json({ 
                error: 'Object not found',
                deleted: false,
                deletedCount: 0
            });
        }
        
        logger.info(`[DELETE][${mod}] Object deleted successfully`, { 
            mod, 
            ObjectId, 
            deletedCount: result.deletedCount,
            timestamp: new Date().toISOString()
        });
        
        return res.status(200).json({
            success: true,
            deleted: true,
            deletedCount: result.deletedCount
        });
    } catch (err) {
        logger.error(`Error in deleteObject endpoint: ${err.message}`, { error: err, mod, ObjectId });
        return res.status(500).json({ error: err.message });
    }
}

module.exports = router;
