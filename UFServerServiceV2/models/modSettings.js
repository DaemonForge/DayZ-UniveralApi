// models/modSettings.js
// MongoDB data access layer for Mod Settings (custom HTML setting pages uploaded by mods)

const { MongoClient } = require("mongodb");
const config = require("../config");
const { createLogger } = require("../utils");

const logger = createLogger(global.logger, "db.modSettings");

const COLLECTION = "ModSettings";

/**
 * Connect to MongoDB and return { client, collection } for the ModSettings collection.
 */
async function getClientAndCollection() {
  const client = new MongoClient(config.DBServer);
  await client.connect();
  const db = client.db(config.DB);
  const collection = db.collection(COLLECTION);
  return { client, collection };
}

/**
 * List all registered mod settings pages.
 * Returns an array of { modId, modName, author, updatedAt }.
 */
async function listModSettings() {
  const { client, collection } = await getClientAndCollection();
  try {
    const docs = await collection
      .find({}, { projection: { modId: 1, modName: 1, author: 1, updatedAt: 1, _id: 0 } })
      .sort({ modName: 1 })
      .toArray();
    logger.info("[ModSettings] Listed mod settings", { count: docs.length });
    return docs;
  } finally {
    await client.close();
  }
}

/**
 * Get a single mod settings document by modId.
 * Returns the full document including the HTML template.
 */
async function getModSettings(modId) {
  const { client, collection } = await getClientAndCollection();
  try {
    const doc = await collection.findOne({ modId }, { projection: { _id: 0 } });
    if (doc) {
      logger.info("[ModSettings] Retrieved settings page", { modId });
    } else {
      logger.warn("[ModSettings] Settings page not found", { modId });
    }
    return doc;
  } finally {
    await client.close();
  }
}

/**
 * Register or update a mod settings page (upsert).
 *
 * @param {string} modId - Unique identifier for the mod (alphanumeric + hyphens/underscores)
 * @param {object} payload - { modName, author, template, globals }
 *   - modName: Human-readable mod name
 *   - author: Author name (optional)
 *   - template: The full HTML template string
 *   - globals: Array of global names (informational) e.g. ["MyMod_Config"]
 * @returns {object} The upserted document (without _id)
 */
async function upsertModSettings(modId, payload) {
  const { client, collection } = await getClientAndCollection();
  try {
    const now = new Date();
    const doc = {
      modId,
      modName: payload.modName || modId,
      author: payload.author || "",
      template: payload.template || "",
      globals: Array.isArray(payload.globals) ? payload.globals : [],
      updatedAt: now,
    };

    const result = await collection.updateOne(
      { modId },
      { $set: doc, $setOnInsert: { createdAt: now } },
      { upsert: true }
    );

    const action = result.upsertedCount > 0 ? "created" : "updated";
    logger.info(`[ModSettings] ${action} settings page`, { modId, action });
    return doc;
  } finally {
    await client.close();
  }
}

/**
 * Delete a mod settings page.
 * @param {string} modId
 * @returns {boolean} true if deleted, false if not found
 */
async function deleteModSettings(modId) {
  const { client, collection } = await getClientAndCollection();
  try {
    const result = await collection.deleteOne({ modId });
    if (result.deletedCount > 0) {
      logger.info("[ModSettings] Deleted settings page", { modId });
      return true;
    }
    logger.warn("[ModSettings] Delete target not found", { modId });
    return false;
  } finally {
    await client.close();
  }
}

/**
 * Ensure indexes exist on the ModSettings collection.
 */
async function ensureModSettingsIndexes() {
  const { client, collection } = await getClientAndCollection();
  try {
    await collection.createIndex({ modId: 1 }, { unique: true });
    logger.info("[ModSettings] Indexes ensured");
  } catch (err) {
    logger.warn("[ModSettings] Index creation warning", { error: err.message });
  } finally {
    await client.close();
  }
}

module.exports = {
  listModSettings,
  getModSettings,
  upsertModSettings,
  deleteModSettings,
  ensureModSettingsIndexes,
};
