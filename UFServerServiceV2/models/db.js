// models/db.js
// Shared MongoDB connection pool for the model modules. One client, reused
// across requests, with automatic reconnection on error/close.
const { MongoClient } = require("mongodb");
const { createLogger } = require('../utils');
const logger = createLogger(global.logger, 'db.pool');

let _client = null;
let _db = null;
let _connectionPromise = null;

/**
 * Gets the shared MongoDB database handle, connecting on first use.
 * @async
 * @returns {Promise<Object>} MongoDB database instance
 */
async function getDb() {
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
      logger.info("MongoDB connection pool established");

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
 * Closes the shared connection and resets pool state. Safe to call when
 * nothing is open (used on graceful shutdown).
 * @async
 * @returns {Promise<void>}
 */
async function closeDb() {
  if (_client) {
    try {
      await _client.close();
      logger.info("MongoDB connection pool closed");
    } catch (err) {
      logger.error("Error closing MongoDB connection", { error: err.message });
    }
  }
  _client = null;
  _db = null;
  _connectionPromise = null;
}

module.exports = { getDb, closeDb };
