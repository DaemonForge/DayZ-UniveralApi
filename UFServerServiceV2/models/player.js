// models/players.js
const { MongoClient } = require("mongodb");
const { createHash } = require('crypto');
const { buildUpdateDoc, processValue, createLogger } = require('../utils');
const logger = createLogger(global.logger, 'db.player');

// Connection pool - reuse connections across requests
let _client = null;
let _db = null;
let _connectionPromise = null;

/**
 * Gets a shared MongoDB connection with automatic reconnection.
 * Uses connection pooling to avoid creating new connections for each request.
 * 
 * @async
 * @function getConnection
 * @returns {Promise<Object>} MongoDB database instance
 */
async function getConnection() {
  if (_db) {
    // Verify connection is still alive
    try {
      await _db.command({ ping: 1 });
      return _db;
    } catch (e) {
      logger.warn("MongoDB connection lost, reconnecting...", { error: e.message });
      _client = null;
      _db = null;
      _connectionPromise = null;
    }
  }

  // Prevent multiple simultaneous connection attempts
  if (_connectionPromise) {
    await _connectionPromise;
    return _db;
  }

  _connectionPromise = (async () => {
    try {
      _client = new MongoClient(global.config.DBServer, {
        maxPoolSize: 10,
        minPoolSize: 2,
        maxIdleTimeMS: 60000,
        serverSelectionTimeoutMS: 5000,
        socketTimeoutMS: 45000
      });
      await _client.connect();
      _db = _client.db(global.config.DB);
      logger.info("MongoDB connection pool established for Players");
      
      // Handle connection errors
      _client.on('error', (err) => {
        logger.error("MongoDB connection error", { error: err.message });
        _client = null;
        _db = null;
        _connectionPromise = null;
      });
      
      _client.on('close', () => {
        logger.warn("MongoDB connection closed");
        _client = null;
        _db = null;
        _connectionPromise = null;
      });
      
      return _db;
    } catch (err) {
      _connectionPromise = null;
      throw err;
    }
  })();

  await _connectionPromise;
  _connectionPromise = null;
  return _db;
}

/**
 * Returns the "Players" collection using the pooled connection.
 * 
 * @deprecated Use getCollection() instead - this is kept for backwards compatibility
 */
async function getClientAndCollection() {
    const db = await getConnection();
    const collection = db.collection("Players");
    // Return a dummy client with a no-op close for backwards compatibility
    return { client: { close: () => {} }, collection };
}

/**
 * Returns the "Players" collection using the pooled connection.
 */
async function getCollection() {
  const db = await getConnection();
  return db.collection("Players");
}

/**
 * Finds a player document by GUID.
 * @param {string} GUID - The player's GUID.
 * @returns {Promise<Object|null>} - The player document, or null if not found.
 */
async function getPlayer(GUID) {
    const collection = await getCollection();
    try {
        const player = await collection.findOne({ GUID });
        return player;
    } catch (err) {
        logger.error(`Error in getPlayer: ${err.message}. GUID: ${GUID}`, { error: err, stack: err.stack });
        return null;
    }
}

/**
 * Retrieves specific mod data for a player by GUID.
 * @param {string} GUID - The player's GUID.
 * @param {string} mod - The name of the mod data to retrieve.
 * @returns {Promise<Object|null>} - The mod-specific data for the player, or null if not found.
 */
async function getPlayerModData(GUID, mod) {
    const player = await getPlayer(GUID);
    try {
        if (player) {
            for (const [key, value] of Object.entries(player)) {
                if (key === mod) {
                    logger.info(`Retrieving mod data for mod: ${mod} and GUID: ${GUID}`);
                    return value;
                }
            }
        }
        return null;
    } catch (err) {
        logger.warn(`Error in getPlayerModData: ${err.message}. GUID: ${GUID}, mod: ${mod}`, { error: err, stack: err.stack });
        return null;
    }
}

/**
 * Checks if a player with the given GUID exists in the database.
 * @param {string} GUID - The player's GUID.
 * @returns {Promise<boolean>} - True if the player exists, false otherwise.
 */
async function playerExists(GUID) {
    const collection = await getCollection();
    try {
        const count = await collection.countDocuments({ GUID });
        return count === 1;
    } catch (err) {
        logger.warn(`Error in playerExists: ${err.message}. GUID: ${GUID}`, { error: err, stack: err.stack });
        throw err;
    }
}

/**
 * Inserts a new player document into the Players collection.
 * @param {Object} doc - The document to insert.
 * @returns {Promise<Object>} - The result of the insert operation.
 */
async function newPlayer(GUID, doc) {
    const collection = await getCollection();
    try {
        doc.GUID = GUID;
        const result = await collection.insertOne(doc);
        return result;
    } catch (err) {
        logger.warn(`Error in newPlayer/insertPlayer: ${err.message}. GUID: ${GUID}`, { error: err, stack: err.stack });
        throw err;
    }
}

/**
 * Updates a player document using the provided query and update document.
 * @param {Object} query - The query object to match the player (usually { GUID }).
 * @param {Object} updateDoc - The update document (using $set, $push, etc.).
 * @param {Object} options - Options for the update (e.g., upsert).
 * @returns {Promise<Object>} - The result of the update operation.
 */
async function updatePlayer(GUID, updateDoc, options = {}) {
    const collection = await getCollection();
    try {
        const result = await collection.updateOne({ GUID }, { $set: updateDoc }, options);
        return result;
    } catch (err) {
        logger.warn(`Error in updatePlayer: ${err.message}. GUID: ${GUID}`, { error: err, stack: err.stack });
        throw err;
    }
}

/**
 * Updates specific mod data for a player.
 * @param {string} GUID - The player's GUID.
 * @param {string} mod - The name of the mod to update.
 * @param {Object} modData - The data to store for the mod.
 * @returns {Promise<Object>} - The result of the update operation.
 */
async function updatePlayerModData(GUID, mod, modData) {
    const collection = await getCollection();
    try {
        const updateDoc = { $set: { [mod]: modData } };
        const result = await collection.updateOne({ GUID }, updateDoc);
        logger.info(`Updated player mod data for GUID: ${GUID}, mod: ${mod}. Matched count: ${result.matchedCount}`);
        return result;
    } catch (err) {
        logger.warn(`Error in updatePlayerModData: ${err.message}. GUID: ${GUID}, mod: ${mod}`, { error: err, stack: err.stack });
        throw err;
    }
}

/**
 * Updates a specific field in a player's document.
 *
 * @param {string} GUID - The player's unique identifier.
 * @param {string} mod - The module/property name in the document.
 * @param {string} element - The specific field (under mod) to update.
 * @param {string} operation - The update operation ("set", "pull", "push", "unset", "mul", "rename", "pullAll").
 * @param {*} value - The value to update with.
 *
 * @returns {Promise<Object>} - The result of the update operation.
 */
async function updatePlayerField(GUID, mod, element, operation, value) {
    const collection = await getCollection();
    try {
        const processedValue = processValue(value);
        const updateDoc = buildUpdateDoc(mod, element, operation, processedValue);
        const result = await executeUpdate(collection, GUID, updateDoc);
        logger.info(`Updated player field for GUID: ${GUID}, mod: ${mod}, element: ${element}, operation: ${operation}. Matched: ${result.matchedCount}, Modified: ${result.modifiedCount}`);
        return result;
    } catch (err) {
        logger.warn(`Error in updatePlayerField: ${err.message}. GUID: ${GUID}, mod: ${mod}, element: ${element}, operation: ${operation}`, { error: err, stack: err.stack });
        throw err;
    }
}

async function executeUpdate(collection, GUID, updateDoc) {
    const query = { GUID };
    const options = { upsert: false };
    return await collection.updateOne(query, updateDoc, options);
}

/**
 * Runs a transaction that increments a specified field within a player's document.
 *
 * @param {Object} data - Transaction data containing:
 *   - Element: the field name under the module (mod) to update.
 *   - Value: the number to increment by.
 * @param {string} mod - The module name (used as the base key for the field).
 * @param {string} GUID - The player's unique identifier.
 *
 * @returns {Promise<Object>} - An object with the update result.
 */
async function runPlayerTransaction(data, mod, GUID) {
    const collection = await getCollection();
    try {
        const query = { GUID };
        const field = `${mod}.${data.Element}`;
        const update = { $inc: { [field]: data.Value } };

        const result = await collection.updateOne(query, update, { upsert: false });
        if (result.matchedCount >= 1 || result.upsertedCount >= 1) {
            const doc = await collection.findOne(query, { projection: { [field]: 1 } });
            let newValue = doc && doc[mod] ? doc[mod][data.Element] : undefined;
            logger.info(`Transaction successful for mod: ${mod}, GUID: ${GUID}, element: ${field}, incrementBy: ${data.Value}, newValue: ${newValue}`, { mod, GUID, element: field });
            return { Status: "Success", ID: GUID, Mod: mod, Value: newValue, Element: data.Element };
        } else {
            logger.warn(`Transaction failed: Invalid ID for mod: ${mod}, GUID: ${GUID}, element: ${data.Element}`, { mod, GUID, element: data.Element });
            return { Status: "NotFound", ID: GUID, Mod: mod, Value: 0, Element: data.Element };
        }
    } catch (err) {
        logger.error(`Transaction error for mod: ${mod}, GUID: ${GUID}, element: ${data.Element}: ${err.message}`, { error: err, stack: err.stack });
        return { Status: "Error", ID: GUID, Mod: mod, Value: 0, Element: data.Element };
    }
}

/**
 * Performs a validated numeric increment operation on a player's mod element within specified limits.
 * 
 * @async
 * @function runValidatedPlayerTransaction
 * @param {Object} data - Transaction data
 * @param {string} data.Element - Name of the element to update
 * @param {number} data.Value - Increment value (can be negative)
 * @param {number} data.Max - Maximum allowed value after increment
 * @param {number} data.Min - Minimum allowed value after increment
 * @param {string} mod - Mod name that contains the element
 * @param {string} GUID - Player's unique identifier
 * @returns {Promise<Object>} Transaction result
 */
async function runValidatedPlayerTransaction(data, mod, GUID) {
    const collection = await getCollection();
    try {
        const query = { GUID };
        const field = `${mod}.${data.Element}`;
        const doc = await collection.findOne(query, { projection: { [field]: 1 } });
        let oldValue = (doc && doc[mod] && typeof doc[mod][data.Element] !== "undefined")
            ? doc[mod][data.Element]
            : undefined;

        if (typeof oldValue !== "undefined") {
            const newValue = oldValue + data.Value;
            if (newValue <= data.Max && newValue >= data.Min) {
                const update = { $inc: { [field]: data.Value } };
                const result = await collection.updateOne(query, update, { upsert: false });
                if (result.matchedCount >= 1 || result.upsertedCount >= 1) {
                    const updatedDoc = await collection.findOne(query, { projection: { [field]: 1 } });
                    let finalValue = updatedDoc && updatedDoc[mod] ? updatedDoc[mod][data.Element] : undefined;
                    logger.info(`Validated transaction successful for mod: ${mod}, GUID: ${GUID}, element: ${field}, incrementBy: ${data.Value}, newValue: ${finalValue}`, { mod, GUID, element: field });
                    return { Status: "Success", Error: "", ID: GUID, Mod: mod, Value: finalValue, Element: data.Element };
                } else {
                    logger.warn(`Validated transaction failed to update for mod: ${mod}, GUID: ${GUID}, element: ${data.Element}`, { mod, GUID, element: data.Element });
                    return { Status: "Error", Error: "Failed to Update", ID: GUID, Mod: mod, Value: 0, Element: data.Element };
                }
            } else {
                logger.info(`Transaction out of range for mod: ${mod}, GUID: ${GUID}, element: ${data.Element}. oldValue: ${oldValue}, newValue: ${newValue}, range: [${data.Min}, ${data.Max}]`, { mod, GUID, element: data.Element });
                return { Status: "Error", Error: "Out of Range", ID: GUID, Mod: mod, Value: oldValue, Element: data.Element };
            }
        } else {
            logger.warn(`Validated transaction failed: Invalid ID or element for mod: ${mod}, GUID: ${GUID}, element: ${data.Element}`, { mod, GUID, element: data.Element });
            return { Status: "NotFound", Error: "Element or object not found", ID: GUID, Mod: mod, Value: 0, Element: data.Element };
        }
    } catch (err) {
        logger.error(`Validated transaction error for mod: ${mod}, GUID: ${GUID}, element: ${data.Element}: ${err.message}`, { error: err, stack: err.stack });
        return { Status: "Error", Error: `Err: ${err}`, ID: GUID, Mod: mod, Value: 0, Element: data.Element };
    }
}

/**
 * Save AUTH token to player.
 * @param {string} GUID - Player's GUID
 * @param {string} auth - Authentication token to save
 * @returns {Promise<Object>} - The result of the operation
 */
async function saveAuthToken(GUID, auth) {    
    try {
        if (!auth) {
            logger.warn(`Missing AUTH token for GUID: ${GUID}`);
            throw new Error('Missing AUTH token');
        }
        
        let result;    
        let AUTH = await createHash('sha256').update(auth).digest('base64');    
        if ((await playerExists(GUID))) {
            logger.debug(`Updating existing player with AUTH token for GUID: ${GUID}`);
            result = await updatePlayer(GUID, { AUTH });
        } else {
            logger.info(`Creating new player with AUTH token for GUID: ${GUID}`);
            const newDoc = { GUID, AUTH };
            result = await newPlayer(GUID, newDoc);
        }
        
        logger.info(`AUTH token saved successfully for GUID: ${GUID}`);
        return true;
    } catch (err) {
        logger.error(`Error saving AUTH token for GUID: ${GUID}: ${err.message}`, { error: err, stack: err.stack });
        return false;
    }
}

/**
 * Deletes a mod's data from a player document
 * 
 * @async
 * @function deletePlayerModData
 * @param {string} GUID - The player's GUID
 * @param {string} mod - The mod namespace to delete
 * @returns {Promise<Object>} Result of deletion operation
 */
async function deletePlayerModData(GUID, mod) {
  try {
    const collection = await getCollection();
    
    logger.info('[DB][DELETE] Deleting player mod data', { GUID, mod });
    
    // Use $unset to remove the mod field from the player document
    const result = await collection.updateOne(
      { GUID },
      { $unset: { [mod]: "" } }
    );
    
    if (result.matchedCount === 0) {
      logger.warn('[DB][DELETE] Player not found for mod deletion', { GUID, mod });
      return { success: false, deleted: false, matchedCount: 0, modifiedCount: 0 };
    }
    
    if (result.modifiedCount === 0) {
      logger.warn('[DB][DELETE] Mod data did not exist on player', { GUID, mod });
      return { success: false, deleted: false, matchedCount: result.matchedCount, modifiedCount: 0 };
    }
    
    logger.info('[DB][DELETE] Player mod data deleted successfully', { 
      GUID, 
      mod, 
      modifiedCount: result.modifiedCount 
    });
    
    return { 
      success: true, 
      deleted: true, 
      matchedCount: result.matchedCount, 
      modifiedCount: result.modifiedCount 
    };
  } catch (err) {
    logger.error('[DB][DELETE] Error deleting player mod data', { 
      GUID, 
      mod, 
      error: err.message 
    });
    throw err;
  }
}

module.exports = {
    getClientAndCollection,
    getPlayer,
    getPlayerModData,
    playerExists,
    updatePlayerModData,
    newPlayer,
    updatePlayer,
    updatePlayerField,
    runPlayerTransaction,
    runValidatedPlayerTransaction,
    saveAuthToken,
    deletePlayerModData
};
