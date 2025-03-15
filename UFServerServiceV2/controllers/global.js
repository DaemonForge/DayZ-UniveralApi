const { Router } = require('express');
const router = Router();
const { requirePlayerOrServerAuth, requireServerAuth, CheckServerAuth } = require('../auth/utils');
const {getGlobal,globalExist,newGlobal,transactionGlobal, updateGlobal} = require('../models/global');
const {createLogger} = require('../utils');
const logger = createLogger(global.logger, 'DB.global');


// Apply authentication middleware before calling the controller functions.
router.post('/Load/:mod', requirePlayerOrServerAuth, runGet);
router.post('/Save/:mod', requireServerAuth, runSave);
router.post('/Transaction/:mod', requireServerAuth, runTransaction);
router.post('/Update/:mod', requireServerAuth, runUpdate);
/**
 * Retrieves the global data for a given module.
 * If no document exists and a non-empty payload is provided,
 * the model will attempt to create one.
 *
 * Expects:
 *   - req.params.mod: the module name.
 *   - req.body: default data (optional) if a new document should be created.
 *
 * @async
 * @param {object} req - The Express request object.
 * @param {object} res - The Express response object.
 */
async function runGet(req, res) {
  const { mod } = req.params;
  const defaultData = req.body;
  try {
    const data = await getGlobal(mod, defaultData, CheckServerAuth(req.headers['auth-key']));
    if (data === null) {
      // If no data is returned, document creation likely failed.
      res.status(203).json(defaultData);
    } else {
      res.json(data);
    }
  } catch (error) {
    logger.error("Error in runGet", { mod, error: error.message });
    res.status(500).json(defaultData);
  }
}

/**
 * Saves (upserts) the global data for a given module.
 * If a document does not exist, a new one is created;
 * otherwise, each property in the provided data is updated.
 *
 * Expects:
 *   - req.params.mod: the module name.
 *   - req.body: an object containing the data to save.
 *
 * @async
 * @param {object} req - The Express request object.
 * @param {object} res - The Express response object.
 */
async function runSave(req, res) {
  const { mod } = req.params;
  const rawData = req.body;
  try {
    const exists = await globalExist(mod);
    if (!exists) {
      const newData = await newGlobal(mod, rawData);
      if (newData !== null) {
        res.status(201).json(rawData);
      } else {
        res.status(203).json(rawData);
      }
    } else {
      // Update each key in the rawData object.
      let success = true;
      const keys = Object.keys(rawData);
      for (const key of keys) {
        // Use the default "set" operation to update the entire field.
        const result = await updateGlobal(mod, { Element: key, Value: rawData[key], Operation: "set" });
        if (result === null) {
          success = false;
        }
      }
      if (success) {
        res.status(200).json(rawData);
      } else {
        res.status(203).json(rawData);
      }
    }
  } catch (error) {
    logger.error("Error in runSave", { mod, error: error.message });
    res.status(500).json(rawData);
  }
}

/**
 * Performs a transaction (increment) on a specific field in the global data.
 *
 * Expects:
 *   - req.params.mod: the module name.
 *   - req.body: an object that must include:
 *       • Element: the field name (inside Data) to update.
 *       • Value: the number to increment by.
 *
 * @async
 * @param {object} req - The Express request object.
 * @param {object} res - The Express response object.
 */
async function runTransaction(req, res) {
  const { mod } = req.params;
  try {
    const newValue = await transactionGlobal(mod, req.body);
    if (newValue !== null) {
      logger.info("Transaction successful", { mod, element: req.body.Element, newValue });
      res.json({ Status: "Success", ID: mod, Value: newValue, Element: req.body.Element });
    } else {
      logger.warn("Transaction failed", { mod, element: req.body.Element });
      res.status(203).json({ Status: "Error", ID: mod, Value: 0, Element: req.body.Element });
    }
  } catch (error) {
    logger.error("Error in runTransaction", { mod, error: error.message });
    res.status(500).json({ Status: "Error", ID: mod, Value: 0, Element: req.body.Element });
  }
}

/**
 * Updates a specific field in the global data document using a provided operation.
 *
 * Expects:
 *   - req.params.mod: the module name.
 *   - req.body: an object that must include:
 *       • Element: the field name (inside Data) to update.
 *       • Value: the value to update with.
 *       • Operation: optional update operator (defaults to "set").
 *
 * @async
 * @param {object} req - The Express request object.
 * @param {object} res - The Express response object.
 */
async function runUpdate(req, res) {
  const { mod } = req.params;
  try {
    const updated = await updateGlobal(mod, req.body);
    if (updated !== null) {
      logger.info("Update successful", { mod, element: req.body.Element });
      res.status(200).json({ Status: "Success", Element: req.body.Element,  ID: mod });
    } else {
      logger.warn("Update failed", { mod, element: req.body.Element });
      res.status(203).json({ Status: "Error", Element: req.body.Element,  ID: mod });
    }
  } catch (error) {
    logger.error("Error in runUpdate", { mod, error: error.message });
    res.status(500).json({ Status: "Error", Element: req.body.Element,  ID: mod });
  }
}



module.exports = router;
