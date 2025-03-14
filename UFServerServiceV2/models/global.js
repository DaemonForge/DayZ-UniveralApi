// models/globals.js

const { MongoClient } = require("mongodb");
const config = require("../config"); // Expects config.DBServer and config.DB
const logger = global.logger;
const { isArray, isObject, isEmpty,  processValue, buildUpdateDoc } = require("../utils");

/**
 * Connects to MongoDB and returns { client, collection } for the "Globals" collection.
 */
async function getClientAndCollection() {
  const client = new MongoClient(config.DBServer);
  await client.connect();
  const db = client.db(config.DB);
  const collection = db.collection("Globals");
  return { client, collection };
}

/**
 * Retrieves the global document for a given module.
 * If no document exists and defaultData is provided, it creates a new document.
 *
 * @param {string} mod - The module name.
 * @param {Object} [defaultData={}] - Optional default data to create a new document.
 * @returns {Promise<Object|null>} - The Data object from the global document or null.
 */
async function getGlobal(mod, defaultData = {}, isServer = false) {
  const { client, collection } = await getClientAndCollection();
  try {
    const doc = await findGlobalDocument(collection, mod);
    if (!doc) {
      if (!isServer){
        return null;
      }
      return await createGlobalDocument(collection, mod, defaultData);
    } else {
      logger.info("Retrieved Global", { mod });
      return doc.Data;
    }
  } catch (err) {
    logger.error("Error in getGlobal", { mod, error: err.message });
    return null;
  } finally {
    await client.close();
  }
}

/**
 * Finds the global document for a given module.
 *
 * @param {Object} collection - The MongoDB collection.
 * @param {string} mod - The module name.
 * @returns {Promise<Object|null>} - The global document or null.
 */
async function findGlobalDocument(collection, mod) {
  const query = { Mod: mod };
  return await collection.findOne(query);
}

/**
 * Creates a new global document for a given module if defaultData is provided.
 *
 * @param {Object} collection - The MongoDB collection.
 * @param {string} mod - The module name.
 * @param {Object} defaultData - The default data to create a new document.
 * @returns {Promise<Object|null>} - The newly created Data or null.
 */
async function createGlobalDocument(collection, mod, defaultData) {
  if (!isEmpty(defaultData)) {
    const newDoc = { Mod: mod, Data: defaultData };
    const result = await collection.insertOne(newDoc);
    if (result.insertedId) {
      logger.info("Created new Global", { mod });
      return defaultData;
    }
  }
  return null;
}

/**
 * Creates a new global document for a given module.
 * Assumes the caller has already verified that no document exists for this module.
 *
 * @param {string} mod - The module name.
 * @param {Object} rawData - Data to store.
 * @returns {Promise<Object|null>} - The newly created Data or null on failure.
 */
async function newGlobal(mod, rawData) {
  const { client, collection } = await getClientAndCollection();
  try {
    const newDoc = { Mod: mod, Data: rawData };
    const result = await collection.insertOne(newDoc);
    if (result.insertedId) {
      logger.info("Created new Global", { mod });
      return rawData;
    }
    return null;
  } catch (err) {
    logger.error("Error in newGlobal", { mod, error: err.message });
    return null;
  } finally {
    await client.close();
  }
}

/**
 * Updates a specific field in the global document for a given module.
 * rawData should include:
 *   - Element: The field name (within Data) to update.
 *   - Value: The value to update with.
 *   - Operation: Optional update operator (defaults to "set").
 *
 * Returns the updated field value if retrievable, true if updated, or null on failure.
 *
 * @param {string} mod - The module name.
 * @param {Object} rawData - Update instructions.
 * @returns {Promise<any|null>}
 */
async function updateGlobal(mod, rawData) {
  const { client, collection } = await getClientAndCollection();
  try {
    const element = rawData.Element;
    const operation = rawData.Operation || "set";
    const query = { Mod: mod };
    const field = "Data." + element;
    let value = processValue(rawData.Value);
    let updateDoc = buildUpdateDoc("Data", element, operation, value);
    console.log(updateDoc);
    const result = await collection.updateOne(query, updateDoc, { upsert: false });
    if (result.matchedCount >= 1 || result.upsertedCount >= 1) {
      logger.info("Updated Global", { mod, element });
      const doc = await collection.findOne(query, { projection: { [field]: 1 } });
      return doc && doc.Data ? doc.Data[element] : true;
    } else {
      logger.warn("Failed to update Global", { mod, element });
      return null;
    }
  } catch (err) {
    console.log(err);
    logger.error("Error in updateGlobal", { mod, error: err.message });
    return null;
  } finally {
    await client.close();
  }
}

/**
 * Performs an increment transaction on a specific global field.
 * rawData should include:
 *   - Element: The field (within Data) to increment.
 *   - Value: The number to increment by.
 *
 * Returns the new value of the field, or null on failure.
 *
 * @param {string} mod - The module name.
 * @param {Object} rawData - Transaction instructions.
 * @returns {Promise<number|null>}
 */
async function transactionGlobal(mod, rawData) {
  const { client, collection } = await getClientAndCollection();
  try {
    const query = { Mod: mod };
    const field = "Data." + rawData.Element;
    const update = { $inc: { [field]: rawData.Value } };
    await collection.updateOne(query, update, { upsert: false });
    const doc = await collection.findOne(query, { projection: { [field]: 1 } });
    const newValue = doc && doc.Data ? doc.Data[rawData.Element] : null;
    logger.info("Transaction on Global", { mod, field, incrementBy: rawData.Value, newValue });
    return newValue;
  } catch (err) {
    logger.error("Error in transactionGlobal", { mod, error: err.message });
    return null;
  } finally {
    await client.close();
  }
}

/**
 * Checks if a global document exists for a given module.
 *
 * @param {string} mod - The module name.
 * @returns {Promise<boolean>} - True if a global document exists; otherwise, false.
 */
async function globalExist(mod) {
  const { client, collection } = await getClientAndCollection();
  try {
    const query = { Mod: mod };
    const doc = await collection.findOne(query);
    return !!doc;
  } catch (err) {
    logger.error("Error in globalExist", { mod, error: err.message });
    return false;
  } finally {
    await client.close();
  }
}

/**
 * Retrieves the value of a specific field from the global document for a given module.
 *
 * @param {string} mod - The module name.
 * @param {string} field - The field name (within Data) to retrieve.
 * @returns {Promise<any|null>} - The field value or null if not found.
 */
async function getGlobalField(mod, field) {
  const { client, collection } = await getClientAndCollection();
  try {
    const query = { Mod: mod };
    const projection = { [`Data.${field}`]: 1 };
    const doc = await collection.findOne(query, { projection });
    if (doc && doc.Data && Object.prototype.hasOwnProperty.call(doc.Data, field)) {
      return doc.Data[field];
    }
    return null;
  } catch (err) {
    logger.error("Error in getGlobalField", { mod, field, error: err.message });
    return null;
  } finally {
    await client.close();
  }
}

module.exports = {
  getGlobal,
  newGlobal,
  updateGlobal,
  transactionGlobal,
  globalExist,
  getGlobalField,
};
