/*
  models/tts.js

  This module uses the native mongodb driver (without Mongoose) to manage our tts job store.
  It follows the provided example pattern:
    - getCollections() connects to the database and returns the collection.
    - createJob() accepts key details (mod, voiceId, message, instructions, staticLevel, visualMode),
      computes a SHA512 hash of (message+instructions) for privacy, and inserts a minimal job document.
    - updateJob() and getJob() update and retrieve only key fields.
  
  Each function ensures the MongoDB client is closed after use.
  
  Make sure config.js exports:
    - MongoUrl, e.g., "mongodb://localhost:27017"
    - MongoDbName, e.g., "YourDatabase"
*/

const { MongoClient, ObjectId } = require('mongodb');
const config = require('../config'); // Expects { MongoUrl, MongoDbName }
const crypto = require('crypto');
const { createLogger } = require('../utils');
const logger = createLogger(global.logger, 'db.TTS');

/**
 * Connects to the database and returns the audioJobs collection.
 * @returns {Promise<{ client: MongoClient, audioJobs: Collection }>}
 */
async function getCollections() {
  const client = new MongoClient(config.DBServer);
  await client.connect();
  const db = client.db(config.DB);
  return { client, audioJobs: db.collection("ttsJobs") };
}

/**
 * Creates a new audio job.
 * Accepts key details and handles building the job document, computing a SHA512 hash
 * of (message + instructions) for privacy, then inserts the document.
 * Returns the inserted job ID.
 * @param {string} voiceId - The voice identifier.
 * @param {string} message - The text to convert to speech.
 * @param {string} instructions - Additional instructions for TTS.
 * @param {number} staticLevel - Float between 0.0 and 1.0 controlling static intensity.
 * @param {string} visualMode - One of "line", "bars", "spectrogram", "cqt", or "none".
 * @returns {Promise<string>} - The inserted job ID.
 */
async function createJob(voiceId, message, instructions, staticLevel, visualMode) {
  const { client, audioJobs } = await getCollections();
  try {
    const messageHash = crypto.createHash('sha512').update(voiceId + message + instructions).digest('hex');
    const jobData = {
      voiceId,
      messageHash, // stored hashed for privacy
      staticLevel,
      visualMode,
      status: 'Pending',
      createdAt: new Date()
    };
    logger.info("Creating audio job", { jobData });
    const result = await audioJobs.insertOne(jobData);
    const jobId = result.insertedId.toString();
    await audioJobs.updateOne({ _id: result.insertedId }, { $set: { jobId } });
    logger.info("Audio job created", { jobId });
    return jobId;
  } catch (error) {
    logger.error("Error creating audio job", { error: error.message });
    throw error;
  } finally {
    await client.close();
  }
}

/**
 * Updates an existing audio job.
 * @param {string} jobId - The job ID.
 * @param {object} updateObj - The update object.
 * @returns {Promise<void>}
 */
async function updateJob(jobId, updateObj) {
  const { client, audioJobs } = await getCollections();
  try {
    await audioJobs.updateOne({ _id: new ObjectId(jobId) }, { $set: updateObj });
  } finally {
    await client.close();
  }
}

/**
 * Retrieves key details of an audio job by its ID.
 * Returns an object with key fields: jobId, status, file, error, and createdAt.
 * @param {string} jobId - The job ID.
 * @returns {Promise<object|null>}
 */
async function getJob(jobId) {
  const { client, audioJobs } = await getCollections();
  try {
    const projection = { jobId: 1, status: 1, file: 1, error: 1, createdAt: 1 };
    return await audioJobs.findOne({ _id: new ObjectId(jobId) }, { projection });
  } finally {
    await client.close();
  }
}

module.exports = { createJob, updateJob, getJob };