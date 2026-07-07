// models/object.js

const { buildUpdateDoc, processValue, createLogger } = require('../utils');
const { getDb } = require('./db');
const logger = createLogger(global.logger, 'db.object');

/**
 * Returns the "Objects" collection using the shared pooled connection.
 * No need to close the connection after use - it's reused.
 *
 * @deprecated Use getCollection() instead - this is kept for backwards compatibility
 */
async function getClientAndCollection() {
  const db = await getDb();
  const collection = db.collection("Objects");
  // Return a dummy client with a no-op close for backwards compatibility
  return { client: { close: () => {} }, collection };
}

/**
 * Returns the "Objects" collection using the shared pooled connection.
 */
async function getCollection() {
  const db = await getDb();
  return db.collection("Objects");
}

/**
 * Finds an object document by ID and Mod.
 * @param {string} ObjectId - The object's ID.
 * @param {string} Mod - The object's Mod.
 * @returns {Promise<Object|null>} - The object document, or null if not found.
 */
async function getObject(ObjectId, Mod) {
  const doc = await getObjectFull(ObjectId, Mod);
  return doc ? doc.data : undefined;
}

/**
 * Finds an object document by ID and Mod, returning the FULL envelope
 * (including AllowedPlayers/AccessRules) rather than just the data payload.
 * Used by access-controlled reads; callers must not leak the envelope to clients.
 * @param {string} ObjectId - The object's ID.
 * @param {string} Mod - The object's Mod.
 * @returns {Promise<Object|undefined>} - The full document, or undefined if not found.
 */
async function getObjectFull(ObjectId, Mod) {
  const collection = await getCollection();
  try {
    const object = await collection.findOne({ ObjectId, Mod });
    logger.debug(`[DB][GETFULL][${Mod}] Object lookup`, { ObjectId, Mod, found: !!object });
    return object || undefined;
  } catch (err) {
    logger.error(`[DB][GETFULL] Error in getObjectFull: ${err.message}`, { error: err, ObjectId, Mod });
    return undefined;
  }
}

/**
 * Replaces the access control fields of an existing object.
 * @param {string} ObjectId - The object's ID.
 * @param {string} Mod - The object's Mod.
 * @param {Object} access - { AllowedPlayers: string[], AccessRules: Object[] } (already normalized).
 * @returns {Promise<boolean>} - True if the object exists.
 */
async function setObjectAccess(ObjectId, Mod, access) {
  const collection = await getCollection();
  try {
    logger.info(`[DB][ACCESS][${Mod}] Setting object access`, {
      ObjectId,
      Mod,
      allowedCount: access.AllowedPlayers.length,
      ruleCount: access.AccessRules.length
    });
    const result = await collection.updateOne(
      { ObjectId, Mod },
      { $set: { AllowedPlayers: access.AllowedPlayers, AccessRules: access.AccessRules } }
    );
    return result.matchedCount > 0;
  } catch (err) {
    logger.error(`[DB][ACCESS] Error in setObjectAccess: ${err.message}`, { error: err, ObjectId, Mod });
    throw err;
  }
}

/**
 * Checks if an object with the given ObjectId and Mod exists in the database.
 * @param {string} ObjectId - The object's ID.
 * @param {string} Mod - The object's Mod.
 * @returns {Promise<boolean>} - True if the object exists, false otherwise.
 */
async function objectExist(ObjectId, Mod) {
  const collection = await getCollection();
  try {
    const count = await collection.countDocuments({ ObjectId, Mod });
    return count === 1;
  } catch (err) {
    logger.warn(`Error in objectExist: ${err.message}`, { error: err, ObjectId, Mod });
    throw err;
  }
}

/**
 * Inserts a new object document into the Objects collection.
 * @param {string} ObjectId - The object's ID.
 * @param {string} Mod - The object's Mod.
 * @param {Object} doc - The document to insert.
 * @returns {Promise<Object>} - The result of the insert operation.
 */
async function newObject(ObjectId, Mod, doc) {
  const collection = await getCollection();
  try {
    // Check if already exists BEFORE inserting
    const existingCount = await collection.countDocuments({ ObjectId, Mod });
    if (existingCount > 0) {
      logger.error(`[DB][NEW][${Mod}] CRITICAL: Attempted to insert DUPLICATE object!`, {
        ObjectId,
        Mod,
        existingCount,
        doc: JSON.stringify(doc)
      });
      throw new Error(`Object ${ObjectId} for mod ${Mod} already exists! Refusing to create duplicate.`);
    }
    
    logger.info(`[DB][NEW][${Mod}] Inserting NEW object`, { ObjectId, Mod, timestamp: new Date().toISOString() });
    
    const result = await collection.insertOne({
      ObjectId,
      Mod,
      data: doc
    });
    
    logger.info(`[DB][NEW][${Mod}] Object INSERTED successfully`, {
      ObjectId,
      Mod,
      insertedId: result.insertedId,
      timestamp: new Date().toISOString()
    });
    
    return result;
  } catch (err) {
    logger.error(`[DB][NEW][${Mod}] Error in newObject: ${err.message}`, { error: err, ObjectId, Mod, stack: err.stack });
    throw err;
  }
}

/**
 * Updates an object document using the provided query and update document.
 * @param {string} ObjectId - The object's ID.
 * @param {string} Mod - The object's Mod.
 * @param {Object} updateDoc - The update document (using $set, $push, etc.).
 * @param {Object} options - Options for the update (e.g., upsert).
 * @returns {Promise<Object>} - The result of the update operation.
 */
async function updateObject(ObjectId, Mod, updateDoc, options = {}) {
  const collection = await getCollection();
  try {
    // Check what exists before updating
    const existingCount = await collection.countDocuments({ ObjectId, Mod });
    const isUpsert = options.upsert === true;
    
    logger.info(`[DB][UPDATE][${Mod}] Updating object`, {
      ObjectId,
      Mod,
      existingCount,
      isUpsert,
      willCreate: isUpsert && existingCount === 0,
      timestamp: new Date().toISOString()
    });
    
    if (existingCount > 1) {
      logger.error(`[DB][UPDATE][${Mod}] CRITICAL: MULTIPLE DUPLICATES DETECTED in database!`, {
        ObjectId,
        Mod,
        duplicateCount: existingCount
      });
    }
    
    const result = await collection.updateOne({ ObjectId, Mod }, updateDoc, options);
    
    logger.info(`[DB][UPDATE][${Mod}] Update completed`, {
      ObjectId,
      Mod,
      matchedCount: result.matchedCount,
      modifiedCount: result.modifiedCount,
      upsertedCount: result.upsertedCount,
      wasCreated: result.upsertedCount === 1,
      wasUpdated: result.matchedCount === 1,
      timestamp: new Date().toISOString()
    });
    
    return result;
  } catch (err) {
    logger.error(`[DB][UPDATE][${Mod}] Error in updateObject: ${err.message}`, { error: err, ObjectId, Mod, stack: err.stack });
    throw err;
  }
}

/**
 * Updates a specific field in an object's document.
 *
 * @param {string} ObjectId - The object's unique identifier.
 * @param {string} Mod - The object's Mod.
 * @param {string} element - The specific subfield to update.
 * @param {string} operation - The update operation ("set", "pull", "push", "unset", "mul", "rename", "pullAll").
 * @param {*} value - The value to update with.
 *
 * @returns {Promise<Object>} - The result of the update operation.
 */
async function updateObjectField(ObjectId, Mod, element, operation, value) {
  const collection = await getCollection();
  try {
    let processedValue = processValue(value);
    let updateDoc = buildUpdateDoc("data", element, operation, processedValue);
  
    const query = { ObjectId, Mod };
    const options = { upsert: false };
    const result = await collection.updateOne(query, updateDoc, options);
    
    logger.info(`Updated object field '${element}' with operation '${operation}'`, {
      ObjectId,
      Mod,
      operation,
      element,
      matchedCount: result.matchedCount,
      modifiedCount: result.modifiedCount
    });
    
    return result;
  } catch (err) {
    logger.warn(`Error in updateObjectField: ${err.message}`, {
      error: err,
      ObjectId,
      Mod,
      element,
      operation
    });
    throw err;
  }
}

/**
 * Runs a transaction that increments a specified field within an object's document.
 *
 * @param {Object} data - Transaction data containing:
 *   - Element: the field name to update.
 *   - Value: the number to increment by.
 * @param {string} ObjectId - The object's unique identifier.
 * @param {string} Mod - The object's Mod.
 *
 * @returns {Promise<Object>} - An object with the update result.
 */
async function runObjectTransaction(data, ObjectId, Mod) {
  const collection = await getCollection();
  try {
    const query = { ObjectId, Mod };
    const fieldPath = `data.${data.Element}`;
    const update = { $inc: { [fieldPath]: data.Value } };
  
    const result = await collection.updateOne(query, update, { upsert: false });
    if (result.matchedCount >= 1 || result.upsertedCount >= 1) {
      const doc = await collection.findOne(query, { projection: { [fieldPath]: 1 } });
      let newValue = doc && doc.data ? doc.data[data.Element] : undefined;
      logger.info(`Transaction successful for field '${data.Element}'`, {
        ObjectId,
        Mod,
        element: fieldPath,
        incrementBy: data.Value,
        newValue
      });
      return { Status: "Success", ID: ObjectId, Mod, Value: newValue, Element: data.Element };
    } else {
      logger.warn(`Transaction failed: Invalid ObjectId or Mod for field '${data.Element}'`, { ObjectId, Mod });
      return { Status: "NotFound", ID: ObjectId, Mod, Value: 0, Element: data.Element };
    }
  } catch (err) {
    logger.error(`Transaction error for field '${data.Element}': ${err.message}`, { error: err, ObjectId, Mod });
    return { Status: "Error", ID: ObjectId, Mod, Value: 0, Element: data.Element };
  }
}

/**
 * Runs a validated transaction that increments a specified field only if the new value
 * will be within the allowed range.
 *
 * @param {Object} data - Transaction data containing:
 *   - Element: the field name to update.
 *   - Value: the number to increment by.
 *   - Min: minimum allowed new value.
 *   - Max: maximum allowed new value.
 * @param {string} ObjectId - The object's unique identifier.
 * @param {string} Mod - The object's Mod.
 *
 * @returns {Promise<Object>} - An object with the update result.
 */
async function runValidatedObjectTransaction(data, ObjectId, Mod) {
  const collection = await getCollection();
  try {
    const query = { ObjectId, Mod };
    const fieldPath = `data.${data.Element}`;

    // Atomic bounds-checked update: the filter ensures the current value
    // is in the range that would keep the result within [Min, Max] after $inc.
    const minCurrent = data.Min - data.Value;
    const maxCurrent = data.Max - data.Value;
    const filter = { ...query, [fieldPath]: { $exists: true, $gte: minCurrent, $lte: maxCurrent } };
    const result = await collection.findOneAndUpdate(
        filter,
        { $inc: { [fieldPath]: data.Value } },
        { returnDocument: 'after', projection: { [fieldPath]: 1 } }
    );

    if (result) {
      const finalValue = result.data ? result.data[data.Element] : undefined;
      logger.info(`Validated transaction successful for field '${data.Element}'`, {
        ObjectId,
        Mod,
        element: fieldPath,
        incrementBy: data.Value,
        newValue: finalValue
      });
      return { Status: "Success", Error: "", ID: ObjectId, Mod, Value: finalValue, Element: data.Element };
    }

    // The atomic update didn't match — determine why.
    const doc = await collection.findOne(query, { projection: { [fieldPath]: 1 } });
    const oldValue = (doc && doc.data && typeof doc.data[data.Element] !== "undefined")
      ? doc.data[data.Element]
      : undefined;

    if (typeof oldValue === "undefined") {
      logger.warn(`Validated transaction failed: Object or element '${data.Element}' not found`, { ObjectId, Mod });
      return { Status: "NotFound", Error: "Element or object not found", ID: ObjectId, Mod, Value: 0, Element: data.Element };
    }

    logger.info(`Transaction out of range for field '${data.Element}'`, {
      ObjectId,
      Mod,
      element: data.Element,
      currentValue: oldValue,
      attempted: data.Value,
      min: data.Min,
      max: data.Max
    });
    return { Status: "Error", Error: "Out of Range", ID: ObjectId, Mod, Value: oldValue, Element: data.Element };
  } catch (err) {
    logger.error(`Validated transaction error for field '${data.Element}': ${err.message}`, { error: err, ObjectId, Mod });
    return { Status: "Error", Error: `Err: ${err.message}`, ID: ObjectId, Mod, Value: 0, Element: data.Element };
  }
}

// Helper functions that were referenced but not defined
function isObject(value) {
  return value !== null && typeof value === 'object' && !Array.isArray(value);
}

function isArray(value) {
  return Array.isArray(value);
}

/**
 * Deletes an object from the Objects collection
 * 
 * @async
 * @function deleteObject
 * @param {string} ObjectId - The object's unique identifier
 * @param {string} mod - The mod namespace
 * @returns {Promise<Object>} Result of deletion operation
 */
async function deleteObject(ObjectId, mod) {
  try {
    const collection = await getCollection();
    
    logger.info('[DB][DELETE] Deleting object', { ObjectId, mod });
    
    const result = await collection.deleteOne({ ObjectId, Mod: mod });
    
    if (result.deletedCount === 0) {
      logger.warn('[DB][DELETE] Object not found for deletion', { ObjectId, mod });
      return { success: false, deleted: false, deletedCount: 0 };
    }
    
    logger.info('[DB][DELETE] Object deleted successfully', { 
      ObjectId, 
      mod, 
      deletedCount: result.deletedCount 
    });
    
    return { success: true, deleted: true, deletedCount: result.deletedCount };
  } catch (err) {
    logger.error('[DB][DELETE] Error deleting object', { 
      ObjectId, 
      mod, 
      error: err.message 
    });
    throw err;
  }
}

module.exports = {
  getClientAndCollection,
  getObject,
  getObjectFull,
  setObjectAccess,
  objectExist,
  newObject,
  updateObject,
  updateObjectField,
  runObjectTransaction,
  runValidatedObjectTransaction,
  deleteObject
};
