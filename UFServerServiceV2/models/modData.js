/**
 * models/modData.js
 * 
 * This module provides functions to scan all MongoDB collections for installed mods
 * and delete mod-specific data across the database.
 * 
 * Collections scanned:
 * - Objects: Standard mod data storage
 * - Players (subdocuments): Nested mod data within player documents (not full player records)
 * - Messages: Message queue system
 * - MessagesMeta: Message queue metadata
 * - PlayerMessagesStatus: Player message status tracking
 * - AIChats: AI chat sessions
 * - AIMessages: AI chat message history
 * - AIAssistantThreads: OpenAI assistant threads
 * 
 * Note: Globals are managed separately via the Globals Editor and are not included here.
 */

const { MongoClient } = require("mongodb");
const { createLogger } = require('../utils');
const logger = createLogger(global.logger, 'db.modData');

// Connection pool - reuse connections across requests
let _client = null;
let _db = null;
let _connectionPromise = null;

/**
 * Gets a shared MongoDB connection with automatic reconnection.
 * @returns {Promise<Object>} MongoDB database instance
 */
async function getConnection() {
  if (_db) {
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
      logger.info("MongoDB connection pool established for ModData");
      
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
 * Scans all collections and returns a map of mod names with their data counts
 * @returns {Promise<Array>} Array of mod objects with name and collection counts
 */
async function scanInstalledMods() {
  try {
    const db = await getConnection();
    
    if (!db) {
      throw new Error('Database connection not available');
    }
    
    const modMap = new Map();

    // System/backend collections to exclude from mod management
    const SYSTEM_MODS = new Set(['AUTH', 'auth', 'Auth', 'System', 'system', 'SYSTEM', 'UniversalApiStatus', 'universalapistatus']);

    // Helper to add or update mod counts
    const addModData = (modName, collection, count) => {
      // Filter out system mods
      if (SYSTEM_MODS.has(modName)) {
        return;
      }
      
      if (!modMap.has(modName)) {
        modMap.set(modName, {
          modName: modName,
          collections: {},
          totalDocuments: 0
        });
      }
      const modData = modMap.get(modName);
      modData.collections[collection] = count;
      modData.totalDocuments += count;
    };

    // 1. Scan Objects collection
    try {
      const objectsCollection = db.collection("Objects");
      const objectsMods = await objectsCollection.aggregate([
        { $group: { _id: "$Mod", count: { $sum: 1 } } }
      ]).toArray();
      
      for (const mod of objectsMods) {
        if (mod._id) {
          addModData(mod._id, 'Objects', mod.count);
        }
      }
      logger.debug(`Scanned Objects: found ${objectsMods.length} mods`);
    } catch (err) {
      logger.warn('Failed to scan Objects collection', { error: err.message });
    }

    // 2. Scan Players collection for nested mod data
    try {
      const playersCollection = db.collection("Players");
      const players = await playersCollection.find({}, { projection: { GUID: 1 } }).toArray();
      
      for (const player of players) {
        // Each player document can have multiple mod subdocuments
        const playerDoc = await playersCollection.findOne({ GUID: player.GUID });
        if (playerDoc) {
          // Iterate over all keys in the player document
          for (const key of Object.keys(playerDoc)) {
            // Skip system fields (these are not mod subdocuments)
            if (key === '_id' || key === 'GUID' || key === 'LastUpdate') continue;
            
            // Count this as a player-mod subdocument entry
            // Note: "Players" refers to subdocuments within player records, not full player documents
            if (!modMap.has(key)) {
              addModData(key, 'Players (subdocuments)', 0);
            }
            const modData = modMap.get(key);
            modData.collections['Players (subdocuments)'] = (modData.collections['Players (subdocuments)'] || 0) + 1;
            modData.totalDocuments += 1;
          }
        }
      }
      logger.debug(`Scanned Players: found ${players.length} player documents`);
    } catch (err) {
      logger.warn('Failed to scan Players collection', { error: err.message });
    }

    // 3. Scan Messages collection
    try {
      const messagesCollection = db.collection("Messages");
      const messagesMods = await messagesCollection.aggregate([
        { $group: { _id: "$Mod", count: { $sum: 1 } } }
      ]).toArray();
      
      for (const mod of messagesMods) {
        if (mod._id) {
          addModData(mod._id, 'Messages', mod.count);
        }
      }
      logger.debug(`Scanned Messages: found ${messagesMods.length} mods`);
    } catch (err) {
      logger.warn('Failed to scan Messages collection', { error: err.message });
    }

    // 4. Scan MessagesMeta collection
    try {
      const messagesMetaCollection = db.collection("MessagesMeta");
      const messagesMetaMods = await messagesMetaCollection.aggregate([
        { $group: { _id: "$Mod", count: { $sum: 1 } } }
      ]).toArray();
      
      for (const mod of messagesMetaMods) {
        if (mod._id) {
          addModData(mod._id, 'MessagesMeta', mod.count);
        }
      }
      logger.debug(`Scanned MessagesMeta: found ${messagesMetaMods.length} mods`);
    } catch (err) {
      logger.warn('Failed to scan MessagesMeta collection', { error: err.message });
    }

    // 5. Scan PlayerMessagesStatus collection
    try {
      const playerMessagesStatusCollection = db.collection("PlayerMessagesStatus");
      const playerMessagesStatusMods = await playerMessagesStatusCollection.aggregate([
        { $group: { _id: "$Mod", count: { $sum: 1 } } }
      ]).toArray();
      
      for (const mod of playerMessagesStatusMods) {
        if (mod._id) {
          addModData(mod._id, 'PlayerMessagesStatus', mod.count);
        }
      }
      logger.debug(`Scanned PlayerMessagesStatus: found ${playerMessagesStatusMods.length} mods`);
    } catch (err) {
      logger.warn('Failed to scan PlayerMessagesStatus collection', { error: err.message });
    }

    // 6. Scan AIChats collection
    try {
      const aiChatsCollection = db.collection("AIChats");
      const aiChatsMods = await aiChatsCollection.aggregate([
        { $group: { _id: "$mod", count: { $sum: 1 } } }
      ]).toArray();
      
      for (const mod of aiChatsMods) {
        if (mod._id) {
          addModData(mod._id, 'AIChats', mod.count);
        }
      }
      logger.debug(`Scanned AIChats: found ${aiChatsMods.length} mods`);
    } catch (err) {
      logger.warn('Failed to scan AIChats collection', { error: err.message });
    }

    // 7. Scan AIMessages collection
    try {
      const aiMessagesCollection = db.collection("AIMessages");
      const aiMessagesMods = await aiMessagesCollection.aggregate([
        { $group: { _id: "$mod", count: { $sum: 1 } } }
      ]).toArray();
      
      for (const mod of aiMessagesMods) {
        if (mod._id) {
          addModData(mod._id, 'AIMessages', mod.count);
        }
      }
      logger.debug(`Scanned AIMessages: found ${aiMessagesMods.length} mods`);
    } catch (err) {
      logger.warn('Failed to scan AIMessages collection', { error: err.message });
    }

    // 8. Scan AIAssistantThreads collection
    try {
      const aiAssistantThreadsCollection = db.collection("AIAssistantThreads");
      const aiAssistantThreadsMods = await aiAssistantThreadsCollection.aggregate([
        { $group: { _id: "$mod", count: { $sum: 1 } } }
      ]).toArray();
      
      for (const mod of aiAssistantThreadsMods) {
        if (mod._id) {
          addModData(mod._id, 'AIAssistantThreads', mod.count);
        }
      }
      logger.debug(`Scanned AIAssistantThreads: found ${aiAssistantThreadsMods.length} mods`);
    } catch (err) {
      logger.warn('Failed to scan AIAssistantThreads collection', { error: err.message });
    }

    // Convert map to array and sort by mod name
    const modsArray = Array.from(modMap.values()).sort((a, b) => 
      a.modName.localeCompare(b.modName)
    );

    logger.info(`Scan complete: found ${modsArray.length} installed mods`);
    return modsArray;

  } catch (err) {
    logger.error('Failed to scan installed mods', { error: err.message, stack: err.stack });
    throw err;
  }
}

/**
 * Deletes all data for a specific mod across all collections
 * @param {string} modName - The name of the mod to delete
 * @returns {Promise<Object>} Summary of deleted documents by collection
 */
async function deleteModData(modName) {
  try {
    if (!modName || typeof modName !== 'string') {
      throw new Error('Mod name is required and must be a string');
    }

    // System/backend collections to protect from deletion
    const SYSTEM_MODS = new Set(['AUTH', 'auth', 'Auth', 'System', 'system', 'SYSTEM', 'UniversalApiStatus', 'universalapistatus']);
    
    if (SYSTEM_MODS.has(modName)) {
      throw new Error('Cannot delete system/backend data. AUTH and System collections are protected.');
    }

    const db = await getConnection();
    const deleteSummary = {
      modName: modName,
      collections: {},
      totalDeleted: 0,
      errors: []
    };

    logger.info(`Starting deletion of mod data for: ${modName}`);

    // 1. Delete from Objects collection
    try {
      const objectsCollection = db.collection("Objects");
      const objectsResult = await objectsCollection.deleteMany({ Mod: modName });
      deleteSummary.collections['Objects'] = objectsResult.deletedCount;
      deleteSummary.totalDeleted += objectsResult.deletedCount;
      logger.debug(`Deleted ${objectsResult.deletedCount} documents from Objects`);
    } catch (err) {
      logger.error('Failed to delete from Objects', { modName, error: err.message });
      deleteSummary.errors.push({ collection: 'Objects', error: err.message });
    }

    // 2. Delete from Players collection (CAREFUL - only remove the mod subdocument, not the entire player)
    try {
      const playersCollection = db.collection("Players");
      const updateSpec = { $unset: { [modName]: "" } };
      const playersResult = await playersCollection.updateMany({}, updateSpec);
      deleteSummary.collections['Players (subdocuments)'] = playersResult.modifiedCount;
      deleteSummary.totalDeleted += playersResult.modifiedCount;
      logger.debug(`Removed ${modName} subdocument from ${playersResult.modifiedCount} players`);
    } catch (err) {
      logger.error('Failed to delete from Players', { modName, error: err.message });
      deleteSummary.errors.push({ collection: 'Players (subdocuments)', error: err.message });
    }

    // 3. Delete from Messages collection
    try {
      const messagesCollection = db.collection("Messages");
      const messagesResult = await messagesCollection.deleteMany({ Mod: modName });
      deleteSummary.collections['Messages'] = messagesResult.deletedCount;
      deleteSummary.totalDeleted += messagesResult.deletedCount;
      logger.debug(`Deleted ${messagesResult.deletedCount} documents from Messages`);
    } catch (err) {
      logger.error('Failed to delete from Messages', { modName, error: err.message });
      deleteSummary.errors.push({ collection: 'Messages', error: err.message });
    }

    // 4. Delete from MessagesMeta collection
    try {
      const messagesMetaCollection = db.collection("MessagesMeta");
      const messagesMetaResult = await messagesMetaCollection.deleteMany({ Mod: modName });
      deleteSummary.collections['MessagesMeta'] = messagesMetaResult.deletedCount;
      deleteSummary.totalDeleted += messagesMetaResult.deletedCount;
      logger.debug(`Deleted ${messagesMetaResult.deletedCount} documents from MessagesMeta`);
    } catch (err) {
      logger.error('Failed to delete from MessagesMeta', { modName, error: err.message });
      deleteSummary.errors.push({ collection: 'MessagesMeta', error: err.message });
    }

    // 5. Delete from PlayerMessagesStatus collection
    try {
      const playerMessagesStatusCollection = db.collection("PlayerMessagesStatus");
      const playerMessagesStatusResult = await playerMessagesStatusCollection.deleteMany({ Mod: modName });
      deleteSummary.collections['PlayerMessagesStatus'] = playerMessagesStatusResult.deletedCount;
      deleteSummary.totalDeleted += playerMessagesStatusResult.deletedCount;
      logger.debug(`Deleted ${playerMessagesStatusResult.deletedCount} documents from PlayerMessagesStatus`);
    } catch (err) {
      logger.error('Failed to delete from PlayerMessagesStatus', { modName, error: err.message });
      deleteSummary.errors.push({ collection: 'PlayerMessagesStatus', error: err.message });
    }

    // 6. Delete from AIChats collection
    try {
      const aiChatsCollection = db.collection("AIChats");
      const aiChatsResult = await aiChatsCollection.deleteMany({ mod: modName });
      deleteSummary.collections['AIChats'] = aiChatsResult.deletedCount;
      deleteSummary.totalDeleted += aiChatsResult.deletedCount;
      logger.debug(`Deleted ${aiChatsResult.deletedCount} documents from AIChats`);
    } catch (err) {
      logger.error('Failed to delete from AIChats', { modName, error: err.message });
      deleteSummary.errors.push({ collection: 'AIChats', error: err.message });
    }

    // 7. Delete from AIMessages collection
    try {
      const aiMessagesCollection = db.collection("AIMessages");
      const aiMessagesResult = await aiMessagesCollection.deleteMany({ mod: modName });
      deleteSummary.collections['AIMessages'] = aiMessagesResult.deletedCount;
      deleteSummary.totalDeleted += aiMessagesResult.deletedCount;
      logger.debug(`Deleted ${aiMessagesResult.deletedCount} documents from AIMessages`);
    } catch (err) {
      logger.error('Failed to delete from AIMessages', { modName, error: err.message });
      deleteSummary.errors.push({ collection: 'AIMessages', error: err.message });
    }

    // 8. Delete from AIAssistantThreads collection
    try {
      const aiAssistantThreadsCollection = db.collection("AIAssistantThreads");
      const aiAssistantThreadsResult = await aiAssistantThreadsCollection.deleteMany({ mod: modName });
      deleteSummary.collections['AIAssistantThreads'] = aiAssistantThreadsResult.deletedCount;
      deleteSummary.totalDeleted += aiAssistantThreadsResult.deletedCount;
      logger.debug(`Deleted ${aiAssistantThreadsResult.deletedCount} documents from AIAssistantThreads`);
    } catch (err) {
      logger.error('Failed to delete from AIAssistantThreads', { modName, error: err.message });
      deleteSummary.errors.push({ collection: 'AIAssistantThreads', error: err.message });
    }

    // Note: Globals collection is NOT deleted - it has its own dedicated editor

    logger.info(`Deletion complete for ${modName}`, {
      totalDeleted: deleteSummary.totalDeleted,
      errors: deleteSummary.errors.length
    });

    return deleteSummary;

  } catch (err) {
    logger.error('Failed to delete mod data', { modName, error: err.message, stack: err.stack });
    throw err;
  }
}

module.exports = {
  scanInstalledMods,
  deleteModData
};
