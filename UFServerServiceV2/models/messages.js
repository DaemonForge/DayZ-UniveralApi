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
 * Uses the application's MongoDB connection style.
 */

const { MongoClient } = require("mongodb");
const config = require("../config");
const logger = global.logger;

/**
 * Connects to MongoDB and returns the necessary collections.
 * 
 * @async
 * @function getCollections
 * @returns {Promise<Object>} An object containing:
 *    - client: The MongoClient instance.
 *    - messages: The "Messages" collection.
 *    - messagesMeta: The "MessagesMeta" collection.
 *    - playerStatus: The "PlayerMessagesStatus" collection.
 */
async function getCollections() {
  const client = new MongoClient(config.DBServer);
  await client.connect();
  const db = client.db(config.DB);
  return {
    client,
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
  const { client, messagesMeta } = await getCollections();
  try {
    const meta = await messagesMeta.findOne({ Mod, Queue });
    if (meta) {
      meta.resetAt = meta.resetAt ? new Date(meta.resetAt) : new Date(0);
      return meta;
    }
    return { Mod, Queue, ...defaultMeta };
  } finally {
    client.close();
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
  const { client, messagesMeta } = await getCollections();
  try {
    await messagesMeta.updateOne(
      { Mod, Queue },
      { $set: metaData },
      { upsert: true }
    );
    return await getQueueMeta(Mod, Queue);
  } finally {
    client.close();
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
  const { client, playerStatus } = await getCollections();
  try {
    const status = await playerStatus.findOne({ Mod, Queue, playerGuid });
    return status && status.lastRead ? new Date(status.lastRead) : new Date(0);
  } finally {
    client.close();
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
  const { client, playerStatus } = await getCollections();
  try {
    await playerStatus.updateOne(
      { Mod, Queue, playerGuid },
      { $set: { lastRead } },
      { upsert: true }
    );
  } finally {
    client.close();
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
 * @param {any} content - The Message payload.
 * @returns {Promise<ObjectId>} The inserted Message's ID.
 */
async function insertMessage(Mod, Queue, Actor, Message) {
  const { client, messages } = await getCollections();
  try {
    const doc = {
      Mod,
      Actor,
      Queue,
      Message,
      createdAt: new Date()
    };
    const result = await messages.insertOne(doc);
    return result.insertedId;
  } finally {
    client.close();
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
  const { client, messages } = await getCollections();
  try {
    const query = {
      Mod,
      Queue,
      createdAt: { $gt: effectiveTime }
    };
    let cursor = messages.find(query).sort({ createdAt: sortOrder });
    if (limit !== -1) {
      cursor = cursor.limit(limit);
    }
    const msgs = await cursor.toArray();
    return msgs;
  } finally {
    client.close();
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
    await updateQueueMeta(Mod, Queue, { resetAt: now });
    return now;
  }
  
module.exports = {
  getQueueMeta,
  updateQueueMeta,
  resetQueue,
  getPlayerStatus,
  updatePlayerStatus,
  insertMessage,
  readMessages
};
