/**
 * controllers/messages.js
 *
 * This Module defines the Express routes for the Message Queue API.
 *
 * Endpoints:
 *  - POST /Messages/Meta/:Mod/:Queue
 *      * Allows the server (server authentication only) to update meta data for the Queue.
 *      * Request body may include:
 *            { 
 *              "Order": "FIFO" | "LIFO", 
 *              "AllowPlayerWrites": 1 | 0,
 *            }
 *      * Returns:
 *            {
 *              Status: "Success" | "Error",
 *              Error: "Error message if any"
 *            }
 *
 *  - POST /Messages/Read/:Mod/:Queue
 *      * Reads messages for the authenticated user (or server).
 *      * Request body should include: { "Limit": number }
 *          - If limit is -1, return all messages.
 *          - If limit is 0, return an empty array (but still update the pointer for a player).
 *      * When the request is made by the server (no GUID available), it is identified as "Server"
 *        and no per-player pointer is updated.
 *      * Returns:
 *            {
 *              Status: "Success" | "Empty" | "Error",
 *              Error: "Error message if any",
 *              Messages: [ Array of message contents ]
 *            }
 *
 *  - POST /Messages/Write/:Mod/:Queue
 *      * Inserts a new message into the specified Mod/Queue.
 *      * Request body should include: { "Message": <message content> }
 *      * For player-authenticated requests, checks the Queue's meta to ensure that player writes are allowed.
 *      * Returns:
 *            {
 *              Status: "Success" | "NoAuth" | "Error",
 *              Error: "Error message if any"
 *            }
 *
 *  - POST /Messages/Reset/:Mod/:Queue
 *      * Resets the specified Mod/Queue by updating its global reset pointer.
 *      * Subsequent reads only return messages enQueued after this reset.
 *      * This endpoint is accessible only to the server.
 *      * Returns:
 *            {
 *              Status: "Success" | "Error",
 *              Error: "Error message if any"
 *            }
 *
 * All responses include a top-level "Status" property.
 *
 */

const express = require("express");
const router = express.Router();
const {
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
} = require("../models/messages");
const { AuthPlayerGuid, CheckServerAuth, requireServerAuth, requirePlayerOrServerAuth} = require('../auth/utils')
const { GenerateLimiter, createLogger, tryConvertToObject} = require('../utils');
const logger = createLogger(global.logger, 'messages');

// Apply rate limiting: 400 requests per 10 seconds.
router.use(GenerateLimiter(global.config.RequestLimitQuery || 400, 10));

/**
 * POST: /Messages/Read/:Mod/:Queue
 * 
 * Description: Reads messages for the authenticated user (or server).
 * For player requests, returns all messages enQueued after the later of the global reset pointer
 * or the player's last-read pointer, then updates the player's pointer. For server requests,
 * only the global reset pointer is used and no pointer update is performed.
 * 
 * Returns:
 * {
 *   Status: "Success" | "Empty" | "Error",
 *   Error: "Error message if any",
 *   messages: [ Array of Messages ]
 * }
 *
 * @async
 * @function runReadMessages
 */
router.post("/Read/:Mod/:Queue", requirePlayerOrServerAuth, runReadMessages);
async function runReadMessages(req, res) {
  try {
    const ModName = req.params.Mod;
    const QueueName = req.params.Queue;
    
    // Validate mod and queue names (prevent injection/path traversal)
    if (!isValidQueueName(ModName) || !isValidQueueName(QueueName)) {
      logger.warn(`Invalid mod or queue name: Mod="${ModName}", Queue="${QueueName}"`);
      return res.status(400).json({ Status: "Error", Error: "Invalid mod or queue name" });
    }
    
    const limitParam = req.body.Limit;
    let limit = parseInt(limitParam, 10);
    if (isNaN(limit)) {
      logger.debug("Invalid limit value provided");
      return res.status(400).json({ Status: "Error", Error: "Invalid limit value" });
    }
    
    // Check for SkipToLatest parameter - if true, skip older messages and return only the latest N
    const skipToLatest = req.body.SkipToLatest === 1 || req.body.SkipToLatest === true;
    
    // Derive caller's identifier: either the player's GUID or "Server".
    let identifier = AuthPlayerGuid(req.headers["auth-key"]);
    if (!identifier) {
      identifier = req.serverId || "Server";
    }
    logger.info(`Queue read request`, { ModName, QueueName, identifier, limit, skipToLatest });
    
    // Get the Queue meta.
    const meta = await getQueueMeta(ModName, QueueName);
    const sortOrder = meta.order === "LIFO" ? -1 : 1;
    logger.debug(`Using sortOrder: ${sortOrder}`);
    
    // For limit = 0, update pointer to current time and return an empty array.
    // This is useful for "catching up" without retrieving messages.
    if (limit === 0) {
      logger.debug("Limit is 0: updating pointer and returning empty messages array");
      await updatePlayerStatus(ModName, QueueName, identifier, new Date());
      return res.status(200).json({ Status: "Empty", Messages: [] });
    }
    
    let rawMessages, newPointer;
    
    if (skipToLatest && limit > 0) {
      // SkipToLatest mode: Get the latest N messages regardless of read pointer,
      // then update the pointer to skip all older messages
      const result = await readLatestMessagesAndUpdatePointer(
        ModName,
        QueueName,
        identifier,
        meta.resetAt,
        limit
      );
      rawMessages = result.messages;
      newPointer = result.newPointer;
    } else {
      // Normal mode: Use atomic read-and-update operation to prevent race conditions
      const result = await readMessagesAndUpdatePointer(
        ModName,
        QueueName,
        identifier,
        meta.resetAt,
        sortOrder,
        limit
      );
      rawMessages = result.messages;
      newPointer = result.newPointer;
    }
    
    logger.debug(`Retrieved ${rawMessages.length} raw messages`);
    
    // Transform messages - extract the Message field and attempt JSON parsing
    const Messages = rawMessages.map(msg => {
      try {
        return tryConvertToObject(msg.Message);
      } catch (e) {
        return msg.Message;
      }
    });
    
    if (newPointer) {
      logger.debug(`Pointer updated to: ${newPointer}`);
    }
    
    const Status = rawMessages.length > 0 ? "Success" : "Empty";
    logger.debug(`Final response Status: ${Status}`);
    return res.status(200).json({ Status, Messages });
  } catch (err) {
    logger.error(`Error reading from Mod "${req.params.Mod}" Queue "${req.params.Queue}": ${err.message}`, err);
    return res.status(500).json({ Status: "Error", Error: "Internal Server Error" });
  }
}

/**
 * Validates that a queue/mod name contains only safe characters.
 * @param {string} name - The name to validate
 * @returns {boolean} True if valid
 */
function isValidQueueName(name) {
  if (!name || typeof name !== 'string') return false;
  if (name.length > 100) return false;
  // Allow alphanumeric, dash, underscore, and period
  return /^[a-zA-Z0-9_\-\.]+$/.test(name);
}

/**
 * POST: /Messages/Write/:Mod/:Queue
 * 
 * Description: Inserts a new message into the specified Mod/Queue.
 * 
 * Returns:
 * {
 *   Status: "Success" | "NoAuth" | "Error",
 *   Error: "Error message if any"
 * }
 *
 * @async
 * @function runWriteMessage
 */
router.post("/Write/:Mod/:Queue", requirePlayerOrServerAuth, runWriteMessage);
async function runWriteMessage(req, res) {
  try {
    const ModName = req.params.Mod;
    const QueueName = req.params.Queue;
    
    // Validate mod and queue names
    if (!isValidQueueName(ModName) || !isValidQueueName(QueueName)) {
      logger.warn(`Invalid mod or queue name: Mod="${ModName}", Queue="${QueueName}"`);
      return res.status(400).json({ Status: "Error", Error: "Invalid mod or queue name" });
    }
    
    if (req.body.Message === undefined) {
      logger.warn(`Missing message field in request body for Mod "${ModName}" Queue "${QueueName}"`);
      return res.status(400).json({ Status: "Error", Error: "Missing message field" });
    }
    
    const message = req.body.Message;

    // Determine if the request is from a player.
    const isServer = req.isServer || CheckServerAuth(req.headers["auth-key"]);
    let actorId = req.serverId || "Server";
    
    if (!isServer) {
      actorId = AuthPlayerGuid(req.headers["auth-key"]);
      if (!actorId) {
        logger.warn(`Could not identify player for Mod "${ModName}" Queue "${QueueName}"`);
        return res.status(401).json({ Status: "NoAuth", Error: "Could not identify player" });
      }
      
      const meta = await getQueueMeta(ModName, QueueName);
      if (!meta.allowPlayerWrites) {
        logger.warn(`Player writes are not allowed for Mod "${ModName}" Queue "${QueueName}"`);
        return res.status(403).json({ Status: "NoAuth", Error: "Player writes are not allowed for this Queue" });
      }
    }
    
    logger.info(`Message enqueued`, { ModName, QueueName, actorId });
    await insertMessage(ModName, QueueName, actorId, message);
    return res.status(200).json({ Status: "Success" });
  } catch (err) {
    logger.error(`Error writing to Mod "${req.params.Mod}" Queue "${req.params.Queue}": ${err.message}`, err);
    return res.status(500).json({ Status: "Error", Error: "Internal Server Error" });
  }
}

/**
 * POST: /Messages/Reset/:Mod/:Queue
 * 
 * Description: Resets the specified Mod/Queue by updating its global reset pointer.
 * 
 * Returns:
 * {
 *   Status: "Success" | "Error",
 *   Error: "Error message if any"
 * }
 *
 * @async
 * @function runResetQueue
 */
router.post("/Reset/:Mod/:Queue", requireServerAuth, runResetQueue);
async function runResetQueue(req, res) {
  try {
    const ModName = req.params.Mod;
    const QueueName = req.params.Queue;
    
    // Validate mod and queue names
    if (!isValidQueueName(ModName) || !isValidQueueName(QueueName)) {
      logger.warn(`Invalid mod or queue name: Mod="${ModName}", Queue="${QueueName}"`);
      return res.status(400).json({ Status: "Error", Error: "Invalid mod or queue name" });
    }
    
    logger.info(`Queue reset request`, { ModName, QueueName });
    const resetTime = await resetQueue(ModName, QueueName);
    logger.debug(`Queue reset for Mod "${ModName}" Queue "${QueueName}" at ${resetTime.toISOString()}`);
    return res.status(200).json({ Status: "Success", Error: "", ResetAt: resetTime.toISOString() });
  } catch (err) {
    logger.error(`Error resetting Mod "${req.params.Mod}" Queue "${req.params.Queue}": ${err.message}`, err);
    return res.status(500).json({ Status: "Error", Error: err.message });
  }
}

/**
 * POST: /Messages/Purge/:Mod/:Queue
 * 
 * Description: Purges old messages from the specified Mod/Queue.
 * This is useful for cleaning up old messages to prevent unbounded database growth.
 * 
 * Request body:
 * {
 *   "OlderThanDays": number (optional, defaults to 30)
 * }
 * 
 * Returns:
 * {
 *   Status: "Success" | "Error",
 *   Error: "Error message if any",
 *   DeletedCount: number
 * }
 *
 * @async
 * @function runPurgeQueue
 */
router.post("/Purge/:Mod/:Queue", requireServerAuth, runPurgeQueue);
async function runPurgeQueue(req, res) {
  try {
    const ModName = req.params.Mod;
    const QueueName = req.params.Queue;
    
    // Validate mod and queue names
    if (!isValidQueueName(ModName) || !isValidQueueName(QueueName)) {
      logger.warn(`Invalid mod or queue name: Mod="${ModName}", Queue="${QueueName}"`);
      return res.status(400).json({ Status: "Error", Error: "Invalid mod or queue name" });
    }
    
    // Default to 30 days if not specified
    const olderThanDays = parseInt(req.body.OlderThanDays, 10) || 30;
    if (olderThanDays < 1) {
      return res.status(400).json({ Status: "Error", Error: "OlderThanDays must be at least 1" });
    }
    
    const olderThan = new Date(Date.now() - olderThanDays * 24 * 60 * 60 * 1000);
    logger.info(`Queue purge request`, { ModName, QueueName, olderThan: olderThan.toISOString() });
    
    const deletedCount = await purgeOldMessages(ModName, QueueName, olderThan);
    logger.info(`Purged ${deletedCount} messages for Mod "${ModName}" Queue "${QueueName}"`);
    
    return res.status(200).json({ Status: "Success", Error: "", DeletedCount: deletedCount });
  } catch (err) {
    logger.error(`Error purging Mod "${req.params.Mod}" Queue "${req.params.Queue}": ${err.message}`, err);
    return res.status(500).json({ Status: "Error", Error: err.message });
  }
}

/**
 * GET: /Messages/Stats/:Mod/:Queue
 * 
 * Description: Gets statistics for the specified Mod/Queue.
 * 
 * Returns:
 * {
 *   Status: "Success" | "Error",
 *   Error: "Error message if any",
 *   Stats: { count, oldestMessage, newestMessage }
 * }
 *
 * @async
 * @function runGetStats
 */
router.get("/Stats/:Mod/:Queue", requireServerAuth, runGetStats);
async function runGetStats(req, res) {
  try {
    const ModName = req.params.Mod;
    const QueueName = req.params.Queue;
    
    // Validate mod and queue names
    if (!isValidQueueName(ModName) || !isValidQueueName(QueueName)) {
      logger.warn(`Invalid mod or queue name: Mod="${ModName}", Queue="${QueueName}"`);
      return res.status(400).json({ Status: "Error", Error: "Invalid mod or queue name" });
    }
    
    const stats = await getQueueStats(ModName, QueueName);
    const meta = await getQueueMeta(ModName, QueueName);
    
    return res.status(200).json({ 
      Status: "Success", 
      Error: "", 
      Stats: stats,
      Meta: {
        order: meta.order,
        allowPlayerWrites: meta.allowPlayerWrites,
        resetAt: meta.resetAt
      }
    });
  } catch (err) {
    logger.error(`Error getting stats for Mod "${req.params.Mod}" Queue "${req.params.Queue}": ${err.message}`, err);
    return res.status(500).json({ Status: "Error", Error: err.message });
  }
}

/**
 * POST: /Messages/Meta/:Mod/:Queue
 * 
 * Description: Allows the server to update meta data for the Queue.
 * 
 * Returns:
 * {
 *   Status: "Success" | "Error",
 *   Error: "Error message if any",
 *   meta: { ...updated meta data }
 * }
 *
 * @async
 * @function runUpdateMeta
 */
router.post("/Meta/:Mod/:Queue", requireServerAuth, runUpdateMeta);
async function runUpdateMeta(req, res) {
  try {
    const ModName = req.params.Mod;
    const QueueName = req.params.Queue;
    
    // Validate mod and queue names
    if (!isValidQueueName(ModName) || !isValidQueueName(QueueName)) {
      logger.warn(`Invalid mod or queue name: Mod="${ModName}", Queue="${QueueName}"`);
      return res.status(400).json({ Status: "Error", Error: "Invalid mod or queue name" });
    }
    
    logger.debug(`Parameters: Mod="${ModName}", Queue="${QueueName}"`);
    const metaData = {};
    logger.debug(`Request body: ${JSON.stringify(req.body)}`);
    
    // Validate 'order'
    if (req.body.Order !== undefined) {
      if (req.body.Order !== "FIFO" && req.body.Order !== "LIFO") {
        logger.debug("Invalid order value provided");
        return res.status(400).json({ Status: "Error", Error: "Invalid order value. Must be 'FIFO' or 'LIFO'." });
      }
      metaData.order = req.body.Order;
    } else {
      logger.debug("Missing order field in body");
      return res.status(400).json({ Status: "Error", Error: "Missing order field" });
    }
    
    // Validate 'allowPlayerWrites'
    if (req.body.AllowPlayerWrites !== undefined) {
      const value = parseInt(req.body.AllowPlayerWrites, 10);
      if (value !== 1 && value !== 0) {
        logger.debug("Invalid allowPlayerWrites value provided");
        return res.status(400).json({ Status: "Error", Error: "Invalid allowPlayerWrites value" });
      }
      metaData.allowPlayerWrites = (value === 1);
    } else {
      logger.debug("Missing allowPlayerWrites field in body");
      return res.status(400).json({ Status: "Error", Error: "Missing allowPlayerWrites field" });
    }

    logger.debug(`Updating meta with: ${JSON.stringify(metaData)}`);
    const updatedMeta = await updateQueueMeta(ModName, QueueName, metaData);
    logger.debug(`Queue meta updated for Mod "${ModName}" Queue "${QueueName}" by Server`);
    logger.debug(`Updated meta: ${JSON.stringify(updatedMeta)}`);
    return res.status(200).json({ Status: "Success" });
  } catch (err) {
    logger.error(`Error updating meta for Mod "${req.params.Mod}" Queue "${req.params.Queue}": ${err.message}`, err);
    return res.status(500).json({ Status: "Error", Error: err.message });
  }
}

module.exports = router;
