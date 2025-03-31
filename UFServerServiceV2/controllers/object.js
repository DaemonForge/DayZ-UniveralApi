// controllers/objectController.js

const { Router } = require('express');
const router = Router();

const { requireServerAuth, requirePlayerOrServerAuth } = require('../auth/utils');
const { updateObject, runObjectTransaction, runValidatedObjectTransaction, getObject, newObject, updateObjectField } = require('../models/object');
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
 * Loads an object with the specified ID and mod, creating a new one if requested
 */
async function loadObject(req, res) {
    let { ObjectId, mod } = req.params;
    const data = req.body;
    logger.debug(`loadObject called with ObjectId: ${ObjectId}, mod: ${mod}, data: ${JSON.stringify(data)}`);
    logger.debug(`Received load request. from: ${ req.isServer ? "Server": "Client"} mod: ${mod}, ObjectId: ${ObjectId}, data: ${JSON.stringify(data)}`);
    try {
        const results = await getObject(ObjectId, mod);
        logger.debug(`getObject returned: ${JSON.stringify(results)}`);
        logger.debug(`getObject returned: ${JSON.stringify(results)}`);
        if (results === null || typeof results === 'undefined') {
            if (req.isServer && !isEmpty(data)) {
                if (ObjectId === "NewObject") {
                    ObjectId = makeObjectId();
                    data.ObjectId = ObjectId;
                    logger.debug(`Generated new ObjectId: ${ObjectId}`);
                    logger.debug(`Creating new object with generated id. mod: ${mod}, ObjectId: ${ObjectId}`);
                } else {
                    logger.debug(`Using provided ObjectId: ${ObjectId}`);
                    logger.debug(`Creating new object with provided id. mod: ${mod}, ObjectId: ${ObjectId}`);
                }
                await newObject(ObjectId, mod, data);
                logger.debug(`New object created, data: ${JSON.stringify(data)}`);
                logger.debug(`New object created successfully. mod: ${mod}, ObjectId: ${ObjectId}`);
                return res.status(201).json(data);
            } else {
                logger.debug(`No object found and creation criteria not met, isServer: ${req.isServer}, data: ${JSON.stringify(data)}`);
                logger.debug(`No object found and creation criteria not met. mod: ${mod}, ObjectId: ${ObjectId}`, {isServer: req.isServer, data});
                return res.status(204).json(data);
            }
        } else {
            logger.debug(`Object found, returning existing object.`);
            logger.debug(`Existing object loaded. mod: ${mod}, ObjectId: ${ObjectId}`);
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
    logger.debug(`saveObject called with ObjectId: ${ObjectId}, mod: ${mod}, data: ${JSON.stringify(data)}`);
    try {
        if (ObjectId === "NewObject") {
            ObjectId = makeObjectId();
            data.ObjectId = ObjectId;
            logger.debug(`Generated new ObjectId: ${ObjectId} for new object`);
        }
        const options = { upsert: true };
        const updateDoc = { $set: { data: data, ObjectId, Mod: mod } };
        logger.debug(`Update document prepared: ${JSON.stringify(updateDoc)}`);
        const result = await updateObject(ObjectId, mod, updateDoc, options);
        logger.debug(`updateObject result: ${JSON.stringify(result)}`);
        if (result.matchedCount === 1 || result.upsertedCount === 1) {
            logger.debug(`Updated object data. mod: ${mod}, ObjectId: ${ObjectId}`);
            res.status(201).json(data);
        } else {
            logger.warn(`Error updating object data for mod: ${mod}, ObjectId: ${ObjectId}`);
            res.status(203).json(data);
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
    logger.debug(`runUpdate called with ObjectId: ${ObjectId}, mod: ${mod}, data: ${JSON.stringify(data)}`);
    try {
        const element = data.Element;
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
            res.status(203).json({ Status: "NotFound", Element: element, Mod: mod, ID: ObjectId });
        }
    } catch (err) {
        logger.error(`Error in Update endpoint: ${err.message}`, { error: err, mod, ObjectId });
        res.status(203).json({ Status: "Error", Element: data.Element, Mod: mod, ID: ObjectId });
    }
}

/**
 * Executes a transaction on a game object based on request data
 */
async function runTransaction(req, res) {
    let { ObjectId, mod } = req.params;
    const data = req.body;
    logger.debug(`runTransaction called with ObjectId: ${ObjectId}, mod: ${mod}, data: ${JSON.stringify(data)}`);
    try {
        let response;
        if (data.Min !== undefined && data.Max !== undefined && data.Min !== data.Max) {
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

module.exports = router;
