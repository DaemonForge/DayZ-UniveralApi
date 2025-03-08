// models/players.js
const { MongoClient } = require("mongodb");
const config = require('../config');  // config should export DBServer and DB values
const { createHash } = require('crypto');
const { buildUpdateDoc,processValue } = require('../utils');

const logger = global.logger;

/**
 * Returns an object with a connected MongoClient and the "Players" collection.
 * Caller should close the client after the operation.
 */
async function getClientAndCollection() {
    const client = new MongoClient(config.DBServer);
    await client.connect();
    const db = client.db(config.DB);
    const collection = db.collection("Players");
    return { client, collection };
}

/**
 * Finds a player document by GUID.
 * @param {string} GUID - The player's GUID.
 * @returns {Promise<Object|null>} - The player document, or null if not found.
 */
async function getPlayer(GUID) {
    // Get database connection and Players collection
    const { client, collection } = await getClientAndCollection();
    let player;
    try {
        // Query the database for a player with the matching GUID
        player = await collection.findOne({ GUID });
        // If no player is found, player will be null
    } catch (err) {
        logger.error("Error in getPlayer", { error: err.message, stack: err.stack, GUID });
        // Note: We don't rethrow the error, allowing the function to return null if an error occurs
    } finally {
        // Always close the MongoDB connection to prevent resource leaks
        await client.close();
        // Return the found player document
    }
    return player;
}

/**
 * Retrieves specific mod data for a player by GUID.
 * @param {string} GUID - The player's GUID.
 * @param {string} mod - The name of the mod data to retrieve.
 * @returns {Promise<Object|null>} - The mod-specific data for the player, or null if not found.
 */
async function getPlayerModData(GUID, mod) {
    // First retrieve the complete player document using the existing getPlayer function
    const player = await getPlayer(GUID);
    try {
        if (player) {
            // If player exists, iterate through all properties in the player document
            for (const [key, value] of Object.entries(player)) {
                // Check if the current property key matches the requested mod name
                if(key === mod){
                    logger.info("Retrieving mod data", { 
                        mod: mod, 
                        GUID: GUID 
                    });
                    // Return just the mod-specific data
                    return value;
                }
            }
            // If we reach here, the mod data wasn't found in the player document
        }
        // Return null if the player doesn't exist or the mod data wasn't found
        return null;
    } catch (err) {
        logger.warn("Error in getPlayerModData", { 
            error: err.message, 
            stack: err.stack,
            GUID: GUID,
            mod: mod
        });
        // Return null on error to avoid breaking the caller's flow
        return null;
    }
}

/**
 * Checks if a player with the given GUID exists in the database.
 * @param {string} GUID - The player's GUID.
 * @returns {Promise<boolean>} - True if the player exists, false otherwise.
 */
async function playerExists(GUID) {
        // Get database connection and Players collection
        const { client, collection } = await getClientAndCollection();
        try {
                // Use countDocuments for an efficient existence check without retrieving the full document
                const count = await collection.countDocuments({ GUID });
                // Return true only if exactly one document was found
                return count === 1;
        } catch (err) {
                logger.warn("Error in PlayerExists", { 
                        error: err.message, 
                        stack: err.stack,
                        GUID: GUID 
                });
                // Re-throw the error for the caller to handle
                throw err;
        } finally {
                // Always close the MongoDB connection to prevent resource leaks
                await client.close();
        }
}

/**
 * Inserts a new player document into the Players collection.
 * @param {Object} doc - The document to insert.
 * @returns {Promise<Object>} - The result of the insert operation.
 */
async function newPlayer(GUID, doc) {
    const { client, collection } = await getClientAndCollection();
    try {
        doc.GUID = GUID;
        const result = await collection.insertOne(doc);
        return result;
    } catch (err) {
        logger.warn("Error in insertPlayer", { 
                error: err.message, 
                stack: err.stack,
                GUID: GUID 
        });
        throw err;
    } finally {
        await client.close();
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
    const { client, collection } = await getClientAndCollection();
    try {
        const result = await collection.updateOne({ GUID }, { $set: updateDoc}, options);
        return result;
    } catch (err) {
        logger.warn("Error in updatePlayer", { 
                error: err.message, 
                stack: err.stack,
                GUID: GUID 
        });
        throw err;
    } finally {
        await client.close();
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
    const { client, collection } = await getClientAndCollection();
    try {
        // Create an update document that sets only the mod field
        const updateDoc = { $set: { [mod]: modData } };
        const result = await collection.updateOne({ GUID }, updateDoc);
        logger.info("Updated player mod data", { 
            GUID: GUID, 
            mod: mod,
            matchedCount: result.matchedCount
        });
        return result;
    } catch (err) {
        logger.warn("Error in updatePlayerModData", { 
            error: err.message, 
            stack: err.stack,
            GUID: GUID,
            mod: mod
        });
        throw err;
    } finally {
        await client.close();
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
    const { client, collection } = await getClientAndCollection();
    try {
        const processedValue = processValue(value);
        const updateDoc = buildUpdateDoc(mod, element, operation, processedValue);
        const result = await executeUpdate(collection, GUID, updateDoc);
        logger.info("Updated player field", {
            GUID,
            mod,
            element,
            operation,
            matchedCount: result.matchedCount,
            modifiedCount: result.modifiedCount
        });
        return result;
    } catch (err) {
        logger.warn("Error in updatePlayerField", {
            error: err.message,
            stack: err.stack,
            GUID,
            mod,
            element,
            operation
        });
        throw err;
    } finally {
        await client.close();
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
 *   Example:
 *     { Status: "Success", ID: GUID, Mod: mod, Value: newValue, Element: data.Element }
 */
async function runPlayerTransaction(data, mod, GUID) {
    const { client, collection } = await getClientAndCollection();
    try {
      const query = { GUID };
      // Build the field path using dot notation, e.g. "Profile.Score"
      const field = `${mod}.${data.Element}`;
      const update = { $inc: { [field]: data.Value } };
  
      const result = await collection.updateOne(query, update, { upsert: false });
      if (result.matchedCount >= 1 || result.upsertedCount >= 1) {
        // Retrieve the updated value using a projection
        const doc = await collection.findOne(query, { projection: { [field]: 1 } });
        let newValue = doc && doc[mod] ? doc[mod][data.Element] : undefined;
        logger.info("Transaction successful", { mod, GUID, element: field, incrementBy: data.Value, newValue });
        return { Status: "Success", ID: GUID, Mod: mod, Value: newValue, Element: data.Element };
      } else {
        logger.warn("Transaction failed: Invalid ID", { mod, GUID, element: data.Element });
        return { Status: "NotFound", ID: GUID, Mod: mod, Value: 0, Element: data.Element };
      }
    } catch (err) {
      logger.error("Transaction error", { mod, GUID, element: data.Element, error: err.message, stack: err.stack });
      return { Status: "Error", ID: GUID, Mod: mod, Value: 0, Element: data.Element };
    } finally {
      await client.close();
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
 * @returns {string} returns.Status - "Success", "Error", or "NotFound"
 * @returns {string} returns.Error - Error message (empty if successful)
 * @returns {string} returns.ID - Player GUID
 * @returns {string} returns.Mod - Mod name
 * @returns {number} returns.Value - Updated element value or 0 if error
 * @returns {string} returns.Element - Element name that was updated
 * @throws {Error} When database operations fail
 */
  async function runValidatedPlayerTransaction(data, mod, GUID) {
    const { client, collection } = await getClientAndCollection();
    try {
      const query = { GUID };
      const field = `${mod}.${data.Element}`;
      // Retrieve the current value using a projection
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
            // Retrieve the updated value
            const updatedDoc = await collection.findOne(query, { projection: { [field]: 1 } });
            let finalValue = updatedDoc && updatedDoc[mod] ? updatedDoc[mod][data.Element] : undefined;
            logger.info("Validated transaction successful", { mod, GUID, element: field, incrementBy: data.Value, newValue: finalValue });
            return { Status: "Success", Error: "", ID: GUID, Mod: mod, Value: finalValue, Element: data.Element };
          } else {
            logger.warn("Validated transaction failed to update", { mod, GUID, element: data.Element });
            return { Status: "Error", Error: "Failed to Update", ID: GUID, Mod: mod, Value: 0, Element: data.Element };
          }
        } else {
          logger.info("Transaction out of range", { mod, GUID, element: data.Element, oldValue, newValue, min: data.Min, max: data.Max });
          return { Status: "Error", Error: "Out of Range", ID: GUID, Mod: mod, Value: oldValue, Element: data.Element };
        }
      } else {
        logger.warn("Validated transaction failed: Invalid ID or element", { mod, GUID, element: data.Element });
        return { Status: "NotFound", Error: "Element or object not found", ID: GUID, Mod: mod, Value: 0, Element: data.Element };
      }
    } catch (err) {
      logger.error("Validated transaction error", { mod, GUID, element: data.Element, error: err.message, stack: err.stack });
      return { Status: "Error", Error: `Err: ${err}`, ID: GUID, Mod: mod, Value: 0, Element: data.Element };
    } finally {
      await client.close();
    }
  }

/**
 * Save AUTH token to player.
 * @param {string} GUID - Player's GUID
 * @param {string} AUTH - Authentication token to save
 * @returns {Promise<Object>} - The result of the operation
 */
async function saveAuthToken(GUID, auth) {    
    try {
        if (!auth) {
            logger.warn('Missing AUTH token', { GUID });
            throw new Error('Missing AUTH token');
        }
        
        let result;    
        let AUTH = await createHash('sha256').update(auth).digest('base64');    
        if ((await playerExists(GUID))) {
            logger.debug('Updating existing player with AUTH token', { GUID });
            result = await updatePlayer(GUID, { AUTH } );
        } else {
            logger.info('Creating new player with AUTH token', { GUID });
            const newDoc = { GUID, AUTH };
            result = await newPlayer(GUID, newDoc);
        }
        
        logger.info('AUTH token saved successfully', { GUID});
        return true;
    } catch (err) {
        logger.error('Error saving AUTH token', { GUID, error: err.message, stack: err.stack });
        return false;
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
    saveAuthToken
};
