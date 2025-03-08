// controllers/objectController.js

const { Router } = require('express');
const router = Router();

const { requireServerAuth, requirePlayerOrServerAuth } = require('../auth/utils');
const {updateObject,runObjectTransaction,runValidatedObjectTransaction, getObject, newObject, updateObjectField} = require('../models/object');
const { makeObjectId, isEmpty, NormalizeToGUID } = require('../utils');

const logger = global.logger;

// Mount the legacy query handler if needed
const queryHandler = require('./query.js');
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
 * This function handles loading object data from the database. If the object is not found
 * and the request is from a server, it can optionally create a new object.
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
    try {
        // Try to get the object using the model
        const results = await getObject(ObjectId, mod);
        if (results === null || typeof results === 'undefined') {
            // Object does not exist. Only allow server to create new objects
            if (req.isServer && !isEmpty(data)) {
                if (ObjectId === "NewObject") {
                    ObjectId = makeObjectId();
                    data.ObjectId = ObjectId;
                    logger.info('Creating new object with new id', { mod, ObjectId });
                } else {
                    logger.info('Creating new object', { mod, ObjectId });
                }
                await newObject(ObjectId, mod, data);
                return res.status(201).json(data);
            } else {
                return res.status(204).json(data);
            }
        } else {
            return res.status(200).json(results);
        }
    } catch (err) {
        logger.error('Error in Load endpoint', { error: err.message, mod, ObjectId });
        return res.status(500).json({ error: err.message });
    }
}


/**
 * Saves an object with the provided data, creating a new one if needed
 * 
 * This function handles saving object data, generating a new ID if requested,
 * and performing an upsert operation to either update an existing object or
 * create a new one if it doesn't exist.
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
        // Use the model's updateObject method with upsert enabled.
        const options = { upsert: true };
        const updateDoc = { $set: { data: data, ObjectId, Mod: mod } };
        const result = await updateObject(ObjectId, mod, updateDoc, options);
        if (result.matchedCount === 1 || result.upsertedCount === 1) {
            logger.info('Updated object data', { mod, ObjectId });
            res.status(201).json(data);
        } else {
            logger.warn(`Error updating ${mod} data for Object: ${ObjectId}`);
            res.status(203).json(data);
        }
    } catch (err) {
        logger.error('Error in Save endpoint', { error: err.message, mod, ObjectId });
        res.status(500).json(data);
    }
}

/**
 * Executes a field update operation on a game object
 * 
 * This function processes a request to update a specific field in an object
 * using the operation specified in the request body. It supports various update
 * operations like set, increment, push, etc.
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
        const value = data.Value;
        // Update the specific field using the model helper.
        const result = await updateObjectField(ObjectId, mod, element, operation, value);
        if (result.matchedCount >= 1 || result.upsertedCount >= 1) {
            logger.info(`Updated ${element} for ${mod} data for ObjectId: ${ObjectId}`);
            res.status(200).json({ Status: "Success", Element: element, Mod: mod, ID: ObjectId });
        } else {
            logger.warn(`Error updating ${element} for ${mod} data for ObjectId: ${ObjectId}`);
            res.status(203).json({ Status: "NotFound", Element: element, Mod: mod, ID: ObjectId });
        }
    } catch (err) {
        logger.error(`Error in Update endpoint: ${err.message}`, { mod, ObjectId });
        res.status(203).json({ Status: "Error", Element: data.Element, Mod: mod, ID: ObjectId });
    }
}



/**
 * Executes a transaction on a game object based on request data
 * 
 * This function processes incoming transaction requests for objects in the DayZ universe.
 * It determines whether to run a validated transaction (when Min/Max range is specified)
 * or a standard transaction based on the provided data.
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
        logger.error('Transaction error', { error: err.message, mod, id: ObjectId });
        res.status(500).json({ Status: "Error", Error: err.message, ID: ObjectId, Mod: mod, Value: 0, Element: data.Element });
    }
}


module.exports = router;
