// models/functions.js
const { MongoClient } = require('mongodb');
const { createLogger } = require('../utils');
const logger = createLogger(global.logger, 'db.functions');

/**
 * Connects to the database and returns the Functions collection.
 */
async function getFunctionsCollection() {
    try {
        const client = new MongoClient(global.config.DBServer);
        await client.connect();
        logger.info(`Connected to database ${global.config.DB}`);
        const db = client.db(global.config.DB); // DB name from config
        const collection = db.collection('Functions');
        return { collection, client };
    } catch (error) {
        logger.error(`Error connecting to database: ${error.message}`, { error });
        throw error;
    }
}

/**
 * Saves a new function document.
 * @param {string} mod - The mod identifier.
 * @param {object} functionData - The function data (should include functionName, code, inputSchema, permission, etc).
 * @returns {object} The result from the insert operation.
 */
async function saveFunction(mod, functionData) {
    let client;
    try {
        const { collection, client: dbClient } = await getFunctionsCollection();
        client = dbClient;
        const doc = { mod, ...functionData, createdAt: new Date() };
        const result = await collection.insertOne(doc);
        logger.info(`Function saved for mod: ${mod}, function: ${functionData.functionName}`);
        return result;
    } catch (error) {
        logger.error(`Error saving function for mod: ${mod}, function: ${functionData.functionName} - ${error.message}`, { error });
        throw error;
    } finally {
        if (client) await client.close();
    }
}

/**
 * Retrieves a function document based on mod and functionName.
 * @param {string} mod - The mod identifier.
 * @param {string} functionName - The name of the function.
 * @returns {object|null} The function document or null if not found.
 */
async function getFunction(mod, functionName) {
    let client;
    try {
        const { collection, client: dbClient } = await getFunctionsCollection();
        client = dbClient;
        const functionDoc = await collection.findOne({ mod, functionName });
        if (functionDoc) {
            logger.info(`Function retrieved for mod: ${mod}, function: ${functionName}`);
        } else {
            logger.warn(`Function not found for mod: ${mod}, function: ${functionName}`);
        }
        return functionDoc;
    } catch (error) {
        logger.error(`Error retrieving function for mod: ${mod}, function: ${functionName} - ${error.message}`, { error });
        throw error;
    } finally {
        if (client) await client.close();
    }
}

/**
 * Deletes a function document based on mod and functionName.
 * @param {string} mod - The mod identifier.
 * @param {string} functionName - The name of the function.
 * @returns {object} The result from the delete operation.
 */
async function deleteFunction(mod, functionName) {
    let client;
    try {
        const { collection, client: dbClient } = await getFunctionsCollection();
        client = dbClient;
        const result = await collection.deleteOne({ mod, functionName });
        if (result.deletedCount === 1) {
            logger.info(`Function deleted for mod: ${mod}, function: ${functionName}`);
        } else {
            logger.warn(`No function found to delete for mod: ${mod}, function: ${functionName}`);
        }
        return result;
    } catch (error) {
        logger.error(`Error deleting function for mod: ${mod}, function: ${functionName} - ${error.message}`, { error });
        throw error;
    } finally {
        if (client) await client.close();
    }
}

module.exports = { getFunctionsCollection, saveFunction, getFunction, deleteFunction };
