// models/globals.js

const { MongoClient } = require("mongodb");
const config = require("../config"); // Expects config.DBServer and config.DB

const { isArray, isObject, isEmpty, processValue, buildUpdateDoc, createLogger } = require("../utils");

const logger = createLogger(global.logger, 'db.global');

function normalizeId(id) {
  if (id && typeof id === 'object' && typeof id.toHexString === 'function') {
    return id.toHexString();
  }
  if (id === undefined || id === null) {
    return '';
  }
  return String(id);
}

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
      if (!isServer) {
        logger.warn(`Global for module "${mod}" not found. Returning null.`, { mod });
        return null;
      }
      logger.info(`Global for module "${mod}" not found. Creating new document.`, { mod, defaultData });
      return await createGlobalDocument(collection, mod, defaultData);
    } else {
      logger.info(`Retrieved Global for module "${mod}".`, { mod });
      return doc.Data;
    }
  } catch (err) {
    logger.error(`Error in getGlobal: ${err.message}`, { mod, error: err });
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
      logger.info(`Created new Global for module "${mod}".`, { mod, newDoc });
      return defaultData;
    }
  }
  logger.warn(`Default data is empty. Global for module "${mod}" was not created.`, { mod });
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
      logger.info(`Created new Global for module "${mod}".`, { mod, newDoc });
      return rawData;
    }
    logger.warn(`Failed to create new Global for module "${mod}".`, { mod });
    return null;
  } catch (err) {
    logger.error(`Error in newGlobal: ${err.message}`, { mod, error: err });
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
      logger.info(`Updated Global field "${element}" for module "${mod}".`, { mod, element, updateDoc });
      const doc = await collection.findOne(query, { projection: { [field]: 1 } });
      return doc && doc.Data ? doc.Data[element] : true;
    } else {
      logger.warn(`Failed to update Global field "${element}" for module "${mod}".`, { mod, element });
      return null;
    }
  } catch (err) {
    logger.error(`Error in updateGlobal: ${err.message}`, { mod, element: rawData?.Element, error: err });
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
    logger.info(
      `Transaction on Global for module "${mod}": incremented "${rawData.Element}" by ${rawData.Value}. New value: ${newValue}`,
      { mod, field, incrementBy: rawData.Value, newValue }
    );
    return newValue;
  } catch (err) {
    logger.error(`Error in transactionGlobal: ${err.message}`, { mod, error: err });
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
    logger.error(`Error in globalExist: ${err.message}`, { mod, error: err });
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
      logger.info(`Retrieved field "${field}" from Global for module "${mod}".`, { mod, field });
      return doc.Data[field];
    }
    logger.warn(`Field "${field}" not found in Global for module "${mod}".`, { mod, field });
    return null;
  } catch (err) {
    logger.error(`Error in getGlobalField for module "${mod}" and field "${field}": ${err.message}`, { mod, field, error: err });
    return null;
  } finally {
    await client.close();
  }
}

/**
 * Retrieves a lightweight list of all global documents, including the module name and id.
 * Used by the desktop editor to populate the module list.
 *
 * @returns {Promise<Array<{id: string, mod: string}>>}
 */
async function listGlobals() {
  const { client, collection } = await getClientAndCollection();
  try {
    const docs = await collection
      .find({}, { projection: { Mod: 1, mod: 1, ID: 1, id: 1, Module: 1, module: 1 } })
      .sort({ Mod: 1, mod: 1, ID: 1 })
      .toArray();
    return docs.map((doc) => {
      const modName = doc.Mod
        || doc.mod
        || doc.Module
        || doc.module
        || doc.ID
        || doc.Id
        || doc.id
        || '';
      return {
        id: normalizeId(doc._id),
        mod: String(modName)
      };
    });
  } catch (err) {
    logger.error(`Error in listGlobals: ${err.message}`, { error: err });
    return [];
  } finally {
    await client.close();
  }
}

/**
 * Retrieves the full global document for a given module.
 *
 * @param {string} mod
 * @returns {Promise<{id: string, mod: string, data: Object}|null>}
 */
async function getGlobalDocument(mod) {
  const { client, collection } = await getClientAndCollection();
  try {
    const doc = await collection.findOne({ Mod: mod });
    if (!doc) {
      logger.warn(`getGlobalDocument: Global for module "${mod}" not found.`, { mod });
      return null;
    }
    return {
  id: normalizeId(doc._id),
      mod: doc.Mod,
      data: doc.Data ?? {}
    };
  } catch (err) {
    logger.error(`Error in getGlobalDocument for module "${mod}": ${err.message}`, { mod, error: err });
    return null;
  } finally {
    await client.close();
  }
}

/**
 * Replaces the Data payload for a specific module with the provided document.
 * Upserts the record if it does not exist.
 *
 * @param {string} mod
 * @param {Object} data
 * @returns {Promise<boolean>} true when the document was stored successfully.
 */
async function saveGlobalDocument(mod, data) {
  if (!mod || typeof mod !== 'string') {
    throw new Error('Module name is required to save global data.');
  }
  if ( data === null || (!isObject(data) && !isArray(data)) ) {
    throw new Error('Global data must be a JSON object or array.');
  }

  const { client, collection } = await getClientAndCollection();
  try {
    const update = { $set: { Mod: mod, Data: data } };
    const result = await collection.updateOne({ Mod: mod }, update, { upsert: true });
    const success = (result.matchedCount === 1 || result.upsertedCount === 1);
    if (success) {
      logger.info(`Saved Global document for module "${mod}".`, { mod, upserted: result.upsertedCount === 1 });
    } else {
      logger.warn(`saveGlobalDocument: No document updated for module "${mod}".`, { mod });
    }
    return success;
  } catch (err) {
    logger.error(`Error in saveGlobalDocument for module "${mod}": ${err.message}`, { mod, error: err });
    throw err;
  } finally {
    await client.close();
  }
}

/**
 * Deletes the Global document associated with the provided module name.
 *
 * @param {string} mod
 * @returns {Promise<boolean>} true when a document was removed.
 */
async function deleteGlobal(mod) {
  const { client, collection } = await getClientAndCollection();
  try {
    const result = await collection.deleteOne({ Mod: mod });
    if (result.deletedCount === 1) {
      logger.info(`Deleted Global document for module "${mod}".`, { mod });
      return true;
    }
    logger.warn(`deleteGlobal: No document deleted for module "${mod}".`, { mod });
    return false;
  } catch (err) {
    logger.error(`Error in deleteGlobal for module "${mod}": ${err.message}`, { mod, error: err });
    throw err;
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
  listGlobals,
  getGlobalDocument,
  saveGlobalDocument,
  deleteGlobal,
};
