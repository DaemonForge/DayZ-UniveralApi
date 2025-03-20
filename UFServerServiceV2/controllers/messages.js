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
  readMessages
} = require("../models/messages");
const { AuthPlayerGuid, CheckServerAuth, requireServerAuth, requirePlayerOrServerAuth} = require('../auth/utils')
const { GenerateLimiter, createLogger} = require('../utils');
const logger = createLogger(global.logger, 'DB.global');

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
 * @param {Object} req - Express request object.
 * @param {Object} req.params - Request parameters.
 * @param {string} req.params.Mod - Mod identifier.
 * @param {string} req.params.Queue - Queue identifier.
 * @param {Object} req.body - Request body.
 * @param {number|string} req.body.limit - Number of messages to return (-1 for all, 0 for empty).
 * @param {Object} res - Express response object.
 * @returns {Promise<void>} Returns JSON response with status and messages.
 */
router.post("/Read/:Mod/:Queue", requirePlayerOrServerAuth, runReadMessages);
async function runReadMessages(req, res) {
    try {
      const ModName = req.params.Mod;
      const QueueName = req.params.Queue;
      const limitParam = req.body.Limit;
      let limit = parseInt(limitParam, 10);
      if (isNaN(limit)) {
        return res.status(400).json({ Status: "Error", Error: "Invalid limit value" });
      }
      
      // Derive caller's identifier: either the player's GUID or "Server".
      let identifier = AuthPlayerGuid(req.headers["auth-key"]);
      if (!identifier) {
        identifier = "Server";
      }
      logger.info(`"${identifier}" reading from Mod "${ModName}" Queue "${QueueName}" with limit ${limit}`);
      
      // Get the Queue meta and determine sort order.
      const meta = await getQueueMeta(ModName, QueueName);
      const sortOrder = meta.order === "LIFO" ? -1 : 1;
      
      // Always use the caller's pointer (player or server) to determine effectiveTime.
      const lastRead = await getPlayerStatus(ModName, QueueName, identifier);
      const effectiveTime = meta.resetAt > lastRead ? meta.resetAt : lastRead;
      
      // For limit = 0, update pointer and return an empty array.
      if (limit === 0) {
        await updatePlayerStatus(ModName, QueueName, identifier, new Date());
        return res.status(200).json({ Status: "Empty", Messages: [] });
      }
      
      // Retrieve messages (this returns an array of message contents).
      const rawMessages = await readMessages(ModName, QueueName, effectiveTime, sortOrder, limit);
        const Messages = rawMessages.map(msg => msg.Message);
      // Update the caller's pointer with the createdAt of the last returned message (or current time if none).
      const newLastRead = rawMessages.length > 0 ? rawMessages[rawMessages.length - 1].createdAt : new Date();
      await updatePlayerStatus(ModName, QueueName, identifier, newLastRead);
      const Status = rawMessages.length > 0 ? "Success" : "Empty";
      return res.status(200).json({ Status, Messages });
    } catch (err) {
      logger.error(`Error reading from Mod "${req.params.Mod}" Queue "${req.params.Queue}": ${err.message}`, err);
      return res.status(500).json({ Status: "Error", Error: "Internal Server Error" });
    }
  }

/**
 * POST: /Messages/Write/:Mod/:Queue
 * 
 * Description: Inserts a new message into the specified Mod/Queue. For player-authenticated requests,
 * checks the Queue meta to ensure that player writes are allowed.
 * 
 * Returns:
 * {
 *   Status: "Success" | "NoAuth" | "Error",
 *   Error: "Error message if any"
 * }
 *
 * @async
 * @function runWriteMessage
 * @param {Object} req - Express request object.
 * @param {Object} req.params - Request parameters.
 * @param {string} req.params.Mod - Mod identifier.
 * @param {string} req.params.Queue - Queue identifier.
 * @param {Object} req.body - Request body.
 * @param {any} req.body.Message - The message content.
 * @param {Object} res - Express response object.
 * @returns {Promise<void>} Returns JSON response with status and message.
 */
router.post("/Write/:Mod/:Queue", requirePlayerOrServerAuth, runWriteMessage);
async function runWriteMessage(req, res) {
  try {
    if (req.body.Message === undefined) {
        logger.warn(`Missing message field in request body for Mod "${req.params.Mod}" Queue "${req.params.Queue}"`);
      return res.status(400).json({ Status: "Error", Error: "Missing message field" });
    }
    const ModName = req.params.Mod;
    const QueueName = req.params.Queue;
    const message = req.body.Message;


    // Determine if the request is from a player.
    const isServer = (CheckServerAuth(req.headers["auth-key"]));
    let actorId = "Server";
    if (!isServer) {
        actorId = AuthPlayerGuid(req.headers["auth-key"]);
        const meta = await getQueueMeta(ModName, QueueName);
        if (!meta.allowPlayerWrites) {
            logger.warn(`Player writes are not allowed for Mod "${ModName}" Queue "${QueueName}"`);
            return res.status(403).json({ Status: "NoAuth", Error: "Player writes are not allowed for this Queue" });
        }
    }
    logger.info(`Message enQueued to Mod "${ModName}" Queue "${QueueName}" by "${actorId}"`);
    await insertMessage(ModName, QueueName, actorId, message);
    return res.status(201).json({ Status: "Success" });
  } catch (err) {
    logger.error(`Error writing to Mod "${req.params.Mod}" Queue "${req.params.Queue}": ${err.message}`, err);
    return res.status(500).json({ Status: "Error", Error: "Internal Server Error" });
  }
}

/**
 * POST: /Messages/Reset/:Mod/:Queue
 * 
 * Description: Resets the specified Mod/Queue by updating its global reset pointer.
 * Subsequent reads will only return messages enQueued after this reset.
 * This endpoint is accessible only to the server and updates the meta data.
 * 
 * Returns:
 * {
 *   Status: "Success" | "Error",
 *   Error: "Error message if any"
 * }
 *
 * @async
 * @function runResetQueue
 * @param {Object} req - Express request object.
 * @param {Object} req.params - Request parameters.
 * @param {string} req.params.Mod - Mod identifier.
 * @param {string} req.params.Queue - Queue identifier.
 * @param {Object} res - Express response object.
 * @returns {Promise<void>} Returns JSON response with status, confirmation, resetAt timestamp, and updated meta data.
 */
router.post("/Reset/:Mod/:Queue", requireServerAuth, runResetQueue);
async function runResetQueue(req, res) {
  try {
    const ModName = req.params.Mod;
    const QueueName = req.params.Queue;
    const resetTime = await resetQueue(ModName, QueueName);
    logger.info(`Queue reset for Mod "${ModName}" Queue "${QueueName}" at ${resetTime.toISOString()}`);
    return res.status(200).json({ Status: "Success" });
  } catch (err) {
    logger.error(`Error resetting Mod "${req.params.Mod}" Queue "${req.params.Queue}": ${err.message}`, err);
    return res.status(500).json({ Status: "Error", Error: `${err.message}` });
  }
}

/**
 * POST: /Messages/Meta/:Mod/:Queue
 * 
 * Description: Allows the server (server authentication only) to update meta data for the Queue.
 * Acceptable meta fields in the request body include:
 *   - order: "FIFO" or "LIFO"
 *   - allowPlayerWrites: number (1 for true, 0 for false)
 *   - resetAt: (optional) date string to manually set the reset pointer.
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
 * @param {Object} req - Express request object.
 * @param {Object} req.params - Request parameters.
 * @param {string} req.params.Mod - Mod identifier.
 * @param {string} req.params.Queue - Queue identifier.
 * @param {Object} req.body - Request body containing meta data to update.
 * @param {Object} res - Express response object.
 * @returns {Promise<void>} Returns JSON response with status and updated meta data.
 */
router.post("/Meta/:Mod/:Queue", requireServerAuth, runUpdateMeta);
async function runUpdateMeta(req, res) {
  try {
    const ModName = req.params.Mod;
    const QueueName = req.params.Queue;
    const metaData = {};
    console.log(req.body);
    // Validate 'order'
    if (req.body.Order !== undefined) {
      if (req.body.Order !== "FIFO" && req.body.Order !== "LIFO") {
        return res.status(400).json({ Status: "Error", Error: "Invalid order value. Must be 'FIFO' or 'LIFO'." });
      }
      metaData.order = req.body.Order;
    } else {
      return res.status(400).json({ Status: "Error", Error: "Missing order field" });
    }
    
    // Validate 'allowPlayerWrites' - should be 1 (true) or 0 (false).
    if (req.body.AllowPlayerWrites !== undefined) {
      const value = parseInt(req.body.AllowPlayerWrites, 10);
      if (value !== 1 && value !== 0) {
        return res.status(400).json({ Status: "Error", Error: "Invalid allowPlayerWrites value" });
      }
      metaData.allowPlayerWrites = (value === 1);
    } else {
      return res.status(400).json({ Status: "Error", Error: "Missing allowPlayerWrites field" }); 
    }

    const updatedMeta = await updateQueueMeta(ModName, QueueName, metaData);
    logger.info(`Queue meta updated for Mod "${ModName}" Queue "${QueueName}" by Server`);
    return res.status(200).json({ Status: "Success" });
  } catch (err) {
    logger.error(`Error updating meta for Mod "${req.params.Mod}" Queue "${req.params.Queue}": ${err.message}`, err);
    return res.status(500).json({ Status: "Error", Error: err.message });
  }
}


module.exports = router;
