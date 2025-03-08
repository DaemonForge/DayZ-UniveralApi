// models/object.js

const { MongoClient } = require("mongodb");
const config = require('../config');  // config should export DBServer and DB values
const { buildUpdateDoc, processValue } = require('../utils');

const logger = global.logger;

/**
 * Returns an object with a connected MongoClient and the "Objects" collection.
 * Caller should close the client after the operation.
 */
async function getClientAndCollection() {
    const client = new MongoClient(config.DBServer, { useUnifiedTopology: true });
    await client.connect();
    const db = client.db(config.DB);
    const collection = db.collection("Objects");
    return { client, collection };
}

/**
 * Finds an object document by ID and Mod.
 * @param {string} ObjectId - The object's ID.
 * @param {string} Mod - The object's Mod.
 * @returns {Promise<Object|null>} - The object document, or null if not found.
 */
async function getObject(ObjectId, Mod) {
    // Get database connection and Objects collection
    const { client, collection } = await getClientAndCollection();
    let object;
    try {
        // Query the database for an object with the matching ID and Mod
        object = await collection.findOne({ ObjectId, Mod });
        // If no object is found, object will be null
    } catch (err) {
        logger.error("Error in getObject", { error: err.message, stack: err.stack });
        // Note: We don't rethrow the error, allowing the function to return undefined
    } finally {
        // Always close the MongoDB connection to prevent resource leaks
        client.close();
        // Return the found object document or undefined if an error occurred
        return object.data;
    }
}

/**
 * Checks if an object with the given ObjectId and Mod exists in the database.
 * @param {string} ObjectId - The object's ID.
 * @param {string} Mod - The object's Mod.
 * @returns {Promise<boolean>} - True if the object exists, false otherwise.
 */
async function objectExist(ObjectId, Mod) {
    const { client, collection } = await getClientAndCollection();
    try {
        const count = await collection.countDocuments({ ObjectId, Mod });
        return count === 1;
    } catch (err) {
        logger.warn("Error in objectExist", { 
            error: err.message, 
            stack: err.stack,
            ObjectId: ObjectId,
            Mod: Mod
        });
        throw err;
    } finally {
        client.close();
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
    const { client, collection } = await getClientAndCollection();
    try {
        doc.ObjectId = ObjectId;
        doc.Mod = Mod;
        const result = await collection.insertOne(doc);
        return result;
    } catch (err) {
        logger.warn("Error in newObject", { 
            error: err.message, 
            stack: err.stack,
            ObjectId: ObjectId,
            Mod: Mod
        });
        throw err;
    } finally {
        client.close();
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
    const { client, collection } = await getClientAndCollection();
    try {
        const result = await collection.updateOne({ ObjectId, Mod }, updateDoc, options);
        return result;
    } catch (err) {
        logger.warn("Error in updateObject", { 
            error: err.message, 
            stack: err.stack,
            ObjectId: ObjectId,
            Mod: Mod
        });
        throw err;
    } finally {
        client.close();
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
    const { client, collection } = await getClientAndCollection();
    try {
      // Process the value
      let processedValue = processValue(value);
      let updateDoc = buildUpdateDoc("data", element, operation, processedValue);
  
      const query = { ObjectId, Mod };
      const options = { upsert: false };
      const result = await collection.updateOne(query, updateDoc, options);
      
      logger.info("Updated object field", {
        ObjectId,
        Mod,
        field,
        element,
        operation,
        matchedCount: result.matchedCount,
        modifiedCount: result.modifiedCount
      });
      
      return result;
    } catch (err) {
      logger.warn("Error in updateObjectField", { 
        error: err.message, 
        stack: err.stack,
        ObjectId,
        Mod,
        field,
        element,
        operation
      });
      throw err;
    } finally {
      await client.close();
    }
}

/**
 * Runs a transaction that increments a specified field within an object's document.
 *
 * @param {Object} data - Transaction data containing:
 *   - Element: the field name to update.
 *   - Value: the number to increment by.
 * @param {string} field - The field name.
 * @param {string} ObjectId - The object's unique identifier.
 * @param {string} Mod - The object's Mod.
 *
 * @returns {Promise<Object>} - An object with the update result.
 */
async function runObjectTransaction(data, ObjectId, Mod) {
    const { client, collection } = await getClientAndCollection();
    try {
      const query = { ObjectId, Mod };
      const fieldPath = `data.${data.Element}`;
      const field = `data`;
      const update = { $inc: { [fieldPath]: data.Value } };
  
      const result = await collection.updateOne(query, update, { upsert: false });
      if (result.matchedCount >= 1 || result.upsertedCount >= 1) {
        const doc = await collection.findOne(query, { projection: { [fieldPath]: 1 } });
        let newValue = doc && doc[field] ? doc[field][data.Element] : undefined;
        logger.info("Transaction successful", { field, ObjectId, Mod, element: fieldPath, incrementBy: data.Value, newValue });
        return { Status: "Success", ID: ObjectId, Mod, Value: newValue, Element: data.Element };
      } else {
        logger.warn("Transaction failed: Invalid ID or Mod", { field, ObjectId, Mod, element: data.Element });
        return { Status: "NotFound", ID: ObjectId, Mod, Value: 0, Element: data.Element };
      }
    } catch (err) {
      logger.error("Transaction error", { field, ObjectId, Mod, element: data.Element, error: err.message, stack: err.stack });
      return { Status: "Error", ID: ObjectId, Mod, Value: 0, Element: data.Element };
    } finally {
      await client.close();
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
    const { client, collection } = await getClientAndCollection();
    try {
      const query = { ObjectId, Mod };
      const fieldPath = `data.${data.Element}`;
      const field = `data`;
      const doc = await collection.findOne(query, { projection: { [fieldPath]: 1 } });
      let oldValue = (doc && doc[field] && typeof doc[field][data.Element] !== "undefined")
        ? doc[field][data.Element]
        : undefined;
  
      if (typeof oldValue !== "undefined") {
        const newValue = oldValue + data.Value;
        if (newValue <= data.Max && newValue >= data.Min) {
          const update = { $inc: { [fieldPath]: data.Value } };
          const result = await collection.updateOne(query, update, { upsert: false });
          if (result.matchedCount >= 1 || result.upsertedCount >= 1) {
            const updatedDoc = await collection.findOne(query, { projection: { [fieldPath]: 1 } });
            let finalValue = updatedDoc && updatedDoc[field] ? updatedDoc[field][data.Element] : undefined;
            logger.info("Validated transaction successful", { field, ObjectId, Mod, element: fieldPath, incrementBy: data.Value, newValue: finalValue });
            return { Status: "Success", Error: "", ID: ObjectId, Mod, Value: finalValue, Element: data.Element };
          } else {
            logger.warn("Validated transaction failed to update", { field, ObjectId, Mod, element: data.Element });
            return { Status: "Error", Error: "Failed to Update", ID: ObjectId, Mod, Value: 0, Element: data.Element };
          }
        } else {
          logger.info("Transaction out of range", { field, ObjectId, Mod, element: data.Element, oldValue, newValue, min: data.Min, max: data.Max });
          return { Status: "Error", Error: "Out of Range", ID: ObjectId, Mod, Value: oldValue, Element: data.Element };
        }
      } else {
        logger.warn("Validated transaction failed: Invalid ID, Mod or element", { field, ObjectId, Mod, element: data.Element });
        return { Status: "NotFound", Error: "Element or object not found", ID: ObjectId, Mod, Value: 0, Element: data.Element };
      }
    } catch (err) {
      logger.error("Validated transaction error", { field, ObjectId, Mod, element: data.Element, error: err.message, stack: err.stack });
      return { Status: "Error", Error: `Err: ${err}`, ID: ObjectId, Mod, Value: 0, Element: data.Element };
    } finally {
      await client.close();
    }
}

// Helper functions that were referenced but not defined
function isObject(value) {
    return value !== null && typeof value === 'object' && !Array.isArray(value);
}

function isArray(value) {
    return Array.isArray(value);
}

module.exports = {
    getClientAndCollection,
    getObject,
    objectExist,
    newObject,
    updateObject,
    updateObjectField,
    runObjectTransaction,
    runValidatedObjectTransaction
};
