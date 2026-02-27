/**
 * Models/Messages.js
 *
 * This Module provides database operations for the Message Queue system.
 *
 * Collections:
 *   - "Messages": Stores individual Messages.
 *   - "MessagesMeta": Stores global metadata for each Mod/Queue.
 *       * Fields include:
 *           - resetAt: Global reset timestamp.
 *           - order: "FIFO" or "LIFO" (determines sort order on read).
 *           - allowPlayerWrites: Boolean indicating if players may submit Messages.
 *   - "PlayerMessagesStatus": Stores each player's last-read pointer per Mod/Queue.
 *
 * Uses the application's MongoDB connection style with connection pooling.
 */

const { MongoClient } = require("mongodb");
const { createLogger } = require('../utils');
const logger = createLogger(global.logger, 'db.messages');

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
      logger.warn("MongoDB connection lost, reconnecting...");
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
      logger.info("MongoDB connection pool established for Messages");
      
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
 * Gets the collections for message operations.
 * Uses pooled connection instead of creating new connections.
 * 
 * @async
 * @function getCollections
 * @returns {Promise<Object>} An object containing:
 *    - messages: The "Messages" collection.
 *    - messagesMeta: The "MessagesMeta" collection.
 *    - playerStatus: The "PlayerMessagesStatus" collection.
 */
async function getCollections() {
  const db = await getConnection();
  return {
    messages: db.collection("Messages"),
    messagesMeta: db.collection("MessagesMeta"),
    playerStatus: db.collection("PlayerMessagesStatus")
  };
}

// Default meta settings for a new Queue.
const defaultMeta = {
  resetAt: new Date(0),
  order: "FIFO",           // Default ordering is FIFO.
  allowPlayerWrites: true    // By default, players can write.
};

/**
 * Retrieves the meta document for a given Mod and Queue.
 * 
 * @async
 * @function getQueueMeta
 * @param {string} Mod - The Mod identifier.
 * @param {string} Queue - The Queue identifier.
 * @returns {Promise<Object>} The meta document with properties such as resetAt, order, and allowPlayerWrites.
 */
async function getQueueMeta(Mod, Queue) {
  const { messagesMeta } = await getCollections();
  try {
    const meta = await messagesMeta.findOne({ Mod, Queue });
    if (meta) {
      meta.resetAt = meta.resetAt ? new Date(meta.resetAt) : new Date(0);
      logger.debug(`getQueueMeta: Retrieved meta for Mod: ${Mod} Queue: ${Queue}`);
      return meta;
    }
    logger.debug(`getQueueMeta: No meta found for Mod: ${Mod} Queue: ${Queue}, returning defaults`);
    return { Mod, Queue, ...defaultMeta };
  } catch (error) {
    logger.error(`getQueueMeta: Error retrieving meta for Mod: ${Mod} Queue: ${Queue}: ${error.message}`, { error });
    throw error;
  }
}

/**
 * Updates the meta document for a given Mod and Queue.
 * 
 * @async
 * @function updateQueueMeta
 * @param {string} Mod - The Mod identifier.
 * @param {string} Queue - The Queue identifier.
 * @param {Object} metaData - An object that may include 'order', 'allowPlayerWrites', and/or 'resetAt'.
 * @returns {Promise<Object>} The updated meta document.
 */
async function updateQueueMeta(Mod, Queue, metaData) {
  const { messagesMeta } = await getCollections();
  try {
    await messagesMeta.updateOne(
      { Mod, Queue },
      { $set: metaData },
      { upsert: true }
    );
    logger.debug(`updateQueueMeta: Updated meta for Mod: ${Mod} Queue: ${Queue}`, metaData);
    return await getQueueMeta(Mod, Queue);
  } catch (error) {
    logger.error(`updateQueueMeta: Error updating meta for Mod: ${Mod} Queue: ${Queue}: ${error.message}`, { error });
    throw error;
  }
}

/**
 * Retrieves a player's last-read pointer for a given Mod and Queue.
 * 
 * @async
 * @function getPlayerStatus
 * @param {string} Mod - The Mod identifier.
 * @param {string} Queue - The Queue identifier.
 * @param {string} playerGuid - The player's GUID.
 * @returns {Promise<Date>} The last-read timestamp or epoch (new Date(0)) if not found.
 */
async function getPlayerStatus(Mod, Queue, playerGuid) {
  const { playerStatus } = await getCollections();
  try {
    const status = await playerStatus.findOne({ Mod, Queue, playerGuid });
    logger.debug(`getPlayerStatus: Retrieved status for player ${playerGuid} in Mod: ${Mod} Queue: ${Queue}`);
    return status && status.lastRead ? new Date(status.lastRead) : new Date(0);
  } catch (error) {
    logger.error(`getPlayerStatus: Error retrieving status for player ${playerGuid} in Mod: ${Mod} Queue: ${Queue}: ${error.message}`, { error });
    throw error;
  }
}

/**
 * Updates (or inserts) a player's last-read pointer for a given Mod and Queue.
 * 
 * @async
 * @function updatePlayerStatus
 * @param {string} Mod - The Mod identifier.
 * @param {string} Queue - The Queue identifier.
 * @param {string} playerGuid - The player's GUID.
 * @param {Date} lastRead - The new last-read timestamp.
 * @returns {Promise<void>}
 */
async function updatePlayerStatus(Mod, Queue, playerGuid, lastRead) {
  const { playerStatus } = await getCollections();
  try {
    await playerStatus.updateOne(
      { Mod, Queue, playerGuid },
      { $set: { lastRead } },
      { upsert: true }
    );
    logger.debug(`updatePlayerStatus: Updated status for player ${playerGuid} in Mod: ${Mod} Queue: ${Queue} to ${lastRead}`);
  } catch (error) {
    logger.error(`updatePlayerStatus: Error updating status for player ${playerGuid} in Mod: ${Mod} Queue: ${Queue}: ${error.message}`, { error });
    throw error;
  }
}

/**
 * Inserts a new Message into the "Messages" collection.
 * 
 * @async
 * @function insertMessage
 * @param {string} Mod - The Mod identifier.
 * @param {string} Queue - The Queue identifier.
 * @param {string} Actor - The author of the Message, if Queue is only writeable by server this should only be "Server".
 * @param {any} Message - The Message payload.
 * @returns {Promise<ObjectId>} The inserted Message's ID.
 */
async function insertMessage(Mod, Queue, Actor, Message) {
  const { messages } = await getCollections();
  try {
    const doc = {
      Mod,
      Actor,
      Queue,
      Message,
      createdAt: new Date()
    };
    const result = await messages.insertOne(doc);
    logger.info(`insertMessage: Message inserted`, { Mod, Queue, Actor, insertedId: result.insertedId });
    return result.insertedId;
  } catch (error) {
    logger.error(`insertMessage: Error inserting message for Mod: ${Mod} Queue: ${Queue}: ${error.message}`, { error });
    throw error;
  }
}

/**
 * Reads Messages for a given Mod and Queue that have been created after a specified effective time.
 * 
 * @async
 * @function readMessages
 * @param {string} Mod - The Mod identifier.
 * @param {string} Queue - The Queue identifier.
 * @param {Date} effectiveTime - Only Messages with createdAt > effectiveTime are returned.
 * @param {number} sortOrder - 1 for ascending (FIFO) or -1 for descending (LIFO).
 * @param {number} limit - Maximum number of Messages to return (-1 for no limit).
 * @returns {Promise<Array>} An array of Message objects.
 */
async function readMessages(Mod, Queue, effectiveTime, sortOrder, limit) {
  const { messages } = await getCollections();
  try {
    const query = {
      Mod,
      Queue,
      createdAt: { $gt: effectiveTime }
    };
    let cursor = messages.find(query).sort({ createdAt: sortOrder });
    // Apply a reasonable maximum limit to prevent memory issues
    const effectiveLimit = limit === -1 ? 1000 : Math.min(limit, 1000);
    cursor = cursor.limit(effectiveLimit);
    const msgs = await cursor.toArray();
    logger.debug(`readMessages: Retrieved ${msgs.length} messages for Mod: ${Mod} Queue: ${Queue}`);
    return msgs;
  } catch (error) {
    logger.error(`readMessages: Error reading messages for Mod: ${Mod} Queue: ${Queue}: ${error.message}`, { error });
    throw error;
  }
}

/**
 * Reads messages and atomically updates the player's pointer in a single operation.
 * This prevents race conditions where multiple reads could return the same messages.
 * 
 * @async
 * @function readMessagesAndUpdatePointer
 * @param {string} Mod - The Mod identifier.
 * @param {string} Queue - The Queue identifier.
 * @param {string} playerGuid - The player's GUID (or "Server" for server reads).
 * @param {Date} resetAt - The queue's reset timestamp.
 * @param {number} sortOrder - 1 for ascending (FIFO) or -1 for descending (LIFO).
 * @param {number} limit - Maximum number of Messages to return (-1 for no limit).
 * @returns {Promise<Object>} An object containing { messages: Array, newPointer: Date }
 */
async function readMessagesAndUpdatePointer(Mod, Queue, playerGuid, resetAt, sortOrder, limit) {
  const { messages, playerStatus } = await getCollections();
  
  try {
    // Get the current player status
    const status = await playerStatus.findOne({ Mod, Queue, playerGuid });
    const lastRead = status && status.lastRead ? new Date(status.lastRead) : new Date(0);
    
    // Use the later of resetAt or lastRead as the effective time
    const effectiveTime = resetAt > lastRead ? resetAt : lastRead;
    
    // Query for messages
    const query = {
      Mod,
      Queue,
      createdAt: { $gt: effectiveTime }
    };
    
    let cursor = messages.find(query).sort({ createdAt: sortOrder });
    const effectiveLimit = limit === -1 ? 1000 : Math.min(limit, 1000);
    cursor = cursor.limit(effectiveLimit);
    const msgs = await cursor.toArray();
    
    if (msgs.length === 0) {
      return { messages: [], newPointer: null };
    }
    
    // Determine the new pointer based on the NEWEST message timestamp
    // For FIFO (sortOrder=1), messages are oldest-first, so newest is at the end
    // For LIFO (sortOrder=-1), messages are newest-first, so newest is at the beginning
    let newestMessageTime;
    if (sortOrder === 1) {
      // FIFO: last message in array is the newest
      newestMessageTime = msgs[msgs.length - 1].createdAt;
    } else {
      // LIFO: first message in array is the newest
      newestMessageTime = msgs[0].createdAt;
    }
    
    // Update the player's pointer to the newest message time
    await playerStatus.updateOne(
      { Mod, Queue, playerGuid },
      { $set: { lastRead: newestMessageTime } },
      { upsert: true }
    );
    
    logger.debug(`readMessagesAndUpdatePointer: Read ${msgs.length} messages and updated pointer to ${newestMessageTime} for ${playerGuid}`);
    
    return { messages: msgs, newPointer: newestMessageTime };
  } catch (error) {
    logger.error(`readMessagesAndUpdatePointer: Error for Mod: ${Mod} Queue: ${Queue} player: ${playerGuid}: ${error.message}`, { error });
    throw error;
  }
}

/**
 * Reads the latest N messages from a queue, skipping older unread messages.
 * This is useful when a client wants to "catch up" to the latest messages without
 * processing older ones. The player's pointer is updated to skip all older messages.
 * 
 * @async
 * @function readLatestMessagesAndUpdatePointer
 * @param {string} Mod - The Mod identifier.
 * @param {string} Queue - The Queue identifier.
 * @param {string} playerGuid - The player's GUID (or "Server" for server reads).
 * @param {Date} resetAt - The queue's reset timestamp.
 * @param {number} limit - Maximum number of latest messages to return.
 * @returns {Promise<Object>} An object containing { messages: Array, newPointer: Date }
 */
async function readLatestMessagesAndUpdatePointer(Mod, Queue, playerGuid, resetAt, limit) {
  const { messages, playerStatus } = await getCollections();
  
  try {
    // Get the current player status
    const status = await playerStatus.findOne({ Mod, Queue, playerGuid });
    const lastRead = status && status.lastRead ? new Date(status.lastRead) : new Date(0);
    
    // Use the later of resetAt or lastRead as the effective time
    const effectiveTime = resetAt > lastRead ? resetAt : lastRead;
    
    // Query for unread messages after the effective time, sorted newest first
    const query = {
      Mod,
      Queue,
      createdAt: { $gt: effectiveTime }
    };
    
    // Get the latest N messages (sorted by createdAt descending = newest first)
    const effectiveLimit = Math.min(limit, 1000);
    const latestMsgs = await messages.find(query)
      .sort({ createdAt: -1 })
      .limit(effectiveLimit)
      .toArray();
    
    if (latestMsgs.length === 0) {
      // No messages, but update pointer to current time to skip any future old messages
      await playerStatus.updateOne(
        { Mod, Queue, playerGuid },
        { $set: { lastRead: new Date() } },
        { upsert: true }
      );
      return { messages: [], newPointer: new Date() };
    }
    
    // The newest message is at index 0 (since sorted descending)
    const newestMessageTime = latestMsgs[0].createdAt;
    
    // Update the player's pointer to the newest message time
    // This effectively marks all older messages as "read"
    await playerStatus.updateOne(
      { Mod, Queue, playerGuid },
      { $set: { lastRead: newestMessageTime } },
      { upsert: true }
    );
    
    // Reverse the array so messages are returned in chronological order (oldest of the N first)
    latestMsgs.reverse();
    
    logger.debug(`readLatestMessagesAndUpdatePointer: Read ${latestMsgs.length} latest messages and updated pointer to ${newestMessageTime} for ${playerGuid}`);
    
    return { messages: latestMsgs, newPointer: newestMessageTime };
  } catch (error) {
    logger.error(`readLatestMessagesAndUpdatePointer: Error for Mod: ${Mod} Queue: ${Queue} player: ${playerGuid}: ${error.message}`, { error });
    throw error;
  }
}

/**
 * Resets the Queue by updating its global reset pointer to the current time.
 * 
 * @async
 * @function resetQueue
 * @param {string} Mod - The Mod identifier.
 * @param {string} Queue - The Queue identifier.
 * @returns {Promise<Date>} The new reset timestamp.
 */
async function resetQueue(Mod, Queue) {
  const now = new Date();
  try {
    await updateQueueMeta(Mod, Queue, { resetAt: now });
    logger.info(`resetQueue: Queue reset`, { Mod, Queue, resetTime: now });
    return now;
  } catch (error) {
    logger.error(`resetQueue: Error resetting queue for Mod: ${Mod} Queue: ${Queue}: ${error.message}`, { error });
    throw error;
  }
}

/**
 * Deletes old messages from a queue to prevent unbounded growth.
 * 
 * @async
 * @function purgeOldMessages
 * @param {string} Mod - The Mod identifier.
 * @param {string} Queue - The Queue identifier.
 * @param {Date} olderThan - Delete messages older than this date.
 * @returns {Promise<number>} The number of messages deleted.
 */
async function purgeOldMessages(Mod, Queue, olderThan) {
  const { messages } = await getCollections();
  try {
    const result = await messages.deleteMany({
      Mod,
      Queue,
      createdAt: { $lt: olderThan }
    });
    logger.info(`purgeOldMessages: Deleted ${result.deletedCount} messages for Mod: ${Mod} Queue: ${Queue}`);
    return result.deletedCount;
  } catch (error) {
    logger.error(`purgeOldMessages: Error purging messages for Mod: ${Mod} Queue: ${Queue}: ${error.message}`, { error });
    throw error;
  }
}

/**
 * Gets message count for a queue (useful for monitoring).
 * 
 * @async
 * @function getQueueStats
 * @param {string} Mod - The Mod identifier.
 * @param {string} Queue - The Queue identifier.
 * @returns {Promise<Object>} Queue statistics.
 */
async function getQueueStats(Mod, Queue) {
  const { messages } = await getCollections();
  try {
    const count = await messages.countDocuments({ Mod, Queue });
    const oldest = await messages.findOne({ Mod, Queue }, { sort: { createdAt: 1 } });
    const newest = await messages.findOne({ Mod, Queue }, { sort: { createdAt: -1 } });
    return {
      count,
      oldestMessage: oldest ? oldest.createdAt : null,
      newestMessage: newest ? newest.createdAt : null
    };
  } catch (error) {
    logger.error(`getQueueStats: Error getting stats for Mod: ${Mod} Queue: ${Queue}: ${error.message}`, { error });
    throw error;
  }
}

module.exports = {
  getQueueMeta,
  updateQueueMeta,
  resetQueue,
  getPlayerStatus,
  updatePlayerStatus,
  insertMessage,
  readMessages,
  readMessagesAndUpdatePointer,
  readLatestMessagesAndUpdatePointer,
  purgeOldMessages,
  getQueueStats
};
