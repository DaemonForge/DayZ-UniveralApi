// controllers/objectController.js

const { Router } = require('express');
const router = Router();

const { requireServerAuth, requirePlayerOrServerAuth } = require('../auth/utils');
const { updateObject, runObjectTransaction, runValidatedObjectTransaction, getObject, newObject, updateObjectField } = require('../models/object');
const { makeObjectId, isEmpty, createLogger } = require('../utils');
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
 * 
 * @async
 * @param {Object} req - Express request object
 * @param {Object} req.params - Request parameters
 * @param {string} req.params.ObjectId - ID of the target object or "NewObject" to generate new ID
 * @param {string} req.params.mod - Mod identifier
 * @param {Object} req.body - Request body containing initial data for new objects
 * @param {Object} res - Express response object
 * @returns {Promise<void>} - Resolves when load operation is complete
 */
async function loadObject(req, res) {
    let { ObjectId, mod } = req.params;
    const data = req.body;
    logger.info(`Received load request. from: ${ req.isServer ? "Server": "Client"} mod: ${mod}, ObjectId: ${ObjectId}, data: ${JSON.stringify(data)}`);
    try {
        const results = await getObject(ObjectId, mod);
        logger.info(`getObject returned: ${JSON.stringify(results)}`);
        if (results === null || typeof results === 'undefined') {
            if (req.isServer && !isEmpty(data)) {
                if (ObjectId === "NewObject") {
                    ObjectId = makeObjectId();
                    data.ObjectId = ObjectId;
                    logger.info(`Creating new object with generated id. mod: ${mod}, ObjectId: ${ObjectId}`);
                } else {
                    logger.info(`Creating new object with provided id. mod: ${mod}, ObjectId: ${ObjectId}`);
                }
                await newObject(ObjectId, mod, data);
                logger.info(`New object created successfully. mod: ${mod}, ObjectId: ${ObjectId}`);
                return res.status(201).json(data);
            } else {
                logger.info(`No object found and creation criteria not met. mod: ${mod}, ObjectId: ${ObjectId}`, {isServer: req.isServer, data});
                return res.status(204).json(data);
            }
        } else {
            logger.info(`Existing object loaded. mod: ${mod}, ObjectId: ${ObjectId}`);
            return res.status(200).json(results);
        }
    } catch (err) {
        logger.error(`Error in loadObject endpoint: ${err.message}`, { error: err, mod, ObjectId });
        return res.status(500).json({ error: err.message });
    }
}

/**
 * Saves an object with the provided data, creating a new one if needed
 * 
 * @async
 * @param {Object} req - Express request object
 * @param {Object} req.params - Request parameters
 * @param {string} req.params.ObjectId - ID of the target object or "NewObject" to generate new ID
 * @param {string} req.params.mod - Mod identifier
 * @param {Object} req.body - Request body containing the object data to save
 * @param {Object} res - Express response object
 * @returns {Promise<void>} - Resolves when save operation is complete
 */
async function saveObject(req, res) {
    let { ObjectId, mod } = req.params;
    const data = req.body;
    try {
        if (ObjectId === "NewObject") {
            ObjectId = makeObjectId();
            data.ObjectId = ObjectId;
        }
        const options = { upsert: true };
        const updateDoc = { $set: { data: data, ObjectId, Mod: mod } };
        const result = await updateObject(ObjectId, mod, updateDoc, options);
        if (result.matchedCount === 1 || result.upsertedCount === 1) {
            logger.info(`Updated object data. mod: ${mod}, ObjectId: ${ObjectId}`);
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
 * 
 * @async
 * @param {Object} req - Express request object
 * @param {Object} req.params - Request parameters
 * @param {string} req.params.ObjectId - ID of the target object
 * @param {string} req.params.mod - Mod identifier
 * @param {Object} req.body - Request body containing update data
 * @param {string} req.body.Element - Field to update
 * @param {string} [req.body.Operation="set"] - Update operation type
 * @param {any} req.body.Value - New value for the field
 * @param {Object} res - Express response object
 * @returns {Promise<void>} - Resolves when update is complete
 */
async function runUpdate(req, res) {
    let { ObjectId, mod } = req.params;
    const data = req.body;
    try {
        const element = data.Element;
        const operation = data.Operation || "set";
        const value = tryConvertToObject(data.Value);
        const result = await updateObjectField(ObjectId, mod, element, operation, value);
        if (result.matchedCount >= 1 || result.upsertedCount >= 1) {
            logger.info(`Updated ${element} for mod: ${mod}, ObjectId: ${ObjectId}`);
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
 * 
 * @async
 * @param {Object} req - Express request object
 * @param {Object} req.params - Request parameters
 * @param {string} req.params.ObjectId - ID of the target object
 * @param {string} req.params.mod - Mod identifier
 * @param {Object} req.body - Request body containing transaction data
 * @param {number} [req.body.Min] - (Optional) Minimum value for range-based transactions
 * @param {number} [req.body.Max] - (Optional) Maximum value for range-based transactions
 * @param {string} [req.body.Element] - Element identifier
 * @param {Object} res - Express response object
 * @returns {Promise<void>} - Resolves when transaction is complete
 * @throws {Error} - If transaction processing fails
 */
async function runTransaction(req, res) {
    let { ObjectId, mod } = req.params;
    const data = req.body;

    try {
        let response;
        if (data.Min !== undefined && data.Max !== undefined && data.Min !== data.Max) {
            response = await runValidatedObjectTransaction(data, ObjectId, mod);
        } else {
            response = await runObjectTransaction(data, ObjectId, mod);
        }
        res.json(response);
    } catch (err) {
        logger.error(`Transaction error: ${err.message}`, { error: err, mod, id: ObjectId });
        res.status(500).json({ Status: "Error", Error: err.message, ID: ObjectId, Mod: mod, Value: 0, Element: data.Element });
    }
}

module.exports = router;
