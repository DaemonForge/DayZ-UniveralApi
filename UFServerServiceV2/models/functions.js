// models/functions.js
const { MongoClient } = require('mongodb');

/**
 * Connects to the database and returns the Functions collection.
 */
async function getFunctionsCollection() {
  const client = new MongoClient(global.config.DBServer);
  await client.connect();
  const db = client.db(global.config.DB); // DB name from config
  const collection = db.collection('Functions');
  return { collection, client };
}

/**
 * Saves a new function document.
 * @param {string} mod - The mod identifier.
 * @param {object} functionData - The function data (should include functionName, code, inputSchema, permission, etc).
 * @returns {object} The result from the insert operation.
 */
async function saveFunction(mod, functionData) {
  const { collection, client } = await getFunctionsCollection();
  const doc = { mod, ...functionData, createdAt: new Date() };
  const result = await collection.insertOne(doc);
  await client.close();
  return result;
}

/**
 * Retrieves a function document based on mod and functionName.
 * @param {string} mod - The mod identifier.
 * @param {string} functionName - The name of the function.
 * @returns {object|null} The function document or null if not found.
 */
async function getFunction(mod, functionName) {
  const { collection, client } = await getFunctionsCollection();
  const functionDoc = await collection.findOne({ mod, functionName });
  await client.close();
  return functionDoc;
}

/**
 * Deletes a function document based on mod and functionName.
 * @param {string} mod - The mod identifier.
 * @param {string} functionName - The name of the function.
 * @returns {object} The result from the delete operation.
 */
async function deleteFunction(mod, functionName) {
  const { collection, client } = await getFunctionsCollection();
  const result = await collection.deleteOne({ mod, functionName });
  await client.close();
  return result;
}

module.exports = { getFunctionsCollection, saveFunction, getFunction, deleteFunction };
