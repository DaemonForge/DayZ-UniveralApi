// models/aiAssistant.js
const { MongoClient, ObjectId } = require('mongodb');
const config = require('../config'); // Expects: { DBServer, DB }
const { createLogger } = require('../utils');
const logger = createLogger(global.logger, 'db.aiAssistant');

/**
 * Connects to the MongoDB database and returns the relevant collections.
 */
async function getCollections() {
  const client = new (require('mongodb')).MongoClient(require('../config').DBServer);
  await client.connect();
  const db = client.db(require('../config').DB);
  return {
    client,
    assistants: db.collection("Assistants"),
    threads: db.collection("Threads")
  };
}

/**
 * Creates a new assistant profile.
 */
async function createAssistant(AssistantId, AssistantApiId, Mod, Name, Description, ResponseFormat = null) {
  const { client, assistants } = await getCollections();
  try {
    logger.info(`Creating assistant with AssistantId: ${AssistantId} for Mod: ${Mod}`);
    const existing = await assistants.findOne({ AssistantId, Mod });
    if (existing) {
      logger.warn(`Assistant with AssistantId ${AssistantId} already exists under Mod ${Mod}.`);
      return { success: false, Message: "Assistant with this ID already exists under this Mod." };
    }
    const assistantData = {
      AssistantId,
      AssistantApiId,
      Mod,
      Name,
      Description,
      ResponseFormat: ResponseFormat || null,
    };
    await assistants.insertOne(assistantData);
    logger.info(`Successfully created assistant with AssistantId: ${AssistantId}`);
    return { success: true, AssistantId };
  } catch (err) {
    logger.error("Error creating assistant", { error: err });
    throw err;
  } finally {
    await client.close();
  }
}

/**
 * Registers an existing assistant profile via upsert.
 */
async function registerExistingAssistant(AssistantId, AssistantApiId, Mod, Name, Description, ResponseFormat = null) {
  const { client, assistants } = await getCollections();
  try {
    logger.info(`Registering existing assistant with AssistantId: ${AssistantId} for Mod: ${Mod}`);
    const updateFields = {
      AssistantApiId,
      Name,
      Description,
      ResponseFormat: ResponseFormat || null
    };
    await assistants.updateOne({ AssistantId, Mod }, { $set: updateFields }, { upsert: true });
    logger.info(`Successfully registered assistant with AssistantId: ${AssistantId}`);
    return { success: true, AssistantId };
  } catch (err) {
    logger.error("Error registering assistant", { error: err });
    throw err;
  } finally {
    await client.close();
  }
}

/**
 * Retrieves an assistant profile by AssistantId and Mod.
 */
async function getAssistant(AssistantId, Mod) {
  const { client, assistants } = await getCollections();
  try {
    logger.info(`Retrieving assistant with AssistantId: ${AssistantId} for Mod: ${Mod}`);
    const assistant = await assistants.findOne({ AssistantId, Mod });
    logger.info(`Assistant retrieval ${assistant ? "succeeded" : "failed"}`);
    return assistant;
  } catch (err) {
    logger.error("Error retrieving assistant", { error: err });
    throw err;
  } finally {
    await client.close();
  }
}

/**
 * Lists all assistants for a given Mod.
 */
async function listAssistants(Mod) {
  const { client, assistants } = await getCollections();
  try {
    logger.info(`Listing assistants for Mod: ${Mod}`);
    const result = await assistants.find({ Mod }, { projection: { AssistantId: 1, Name: 1, Description: 1 } }).toArray();
    logger.info(`Found ${result.length} assistants for Mod: ${Mod}`);
    return result;
  } catch (err) {
    logger.error("Error listing assistants", { error: err });
    throw err;
  } finally {
    await client.close();
  }
}

/**
 * Updates an assistant profile.
 */
async function updateAssistant(AssistantId, Mod, updates) {
  const { client, assistants } = await getCollections();
  try {
    logger.info(`Updating assistant with AssistantId: ${AssistantId} for Mod: ${Mod}`);
    const updateFields = {};
    if (updates.Name) updateFields.Name = updates.Name;
    if (updates.Description) updateFields.Description = updates.Description;
    if (updates.ResponseFormat !== undefined) updateFields.ResponseFormat = updates.ResponseFormat;
    if (updates.AssistantApiId) updateFields.AssistantApiId = updates.AssistantApiId;
    const result = await assistants.updateOne({ AssistantId, Mod }, { $set: updateFields });
    if (result.modifiedCount > 0) {
      logger.info(`Assistant with AssistantId: ${AssistantId} updated successfully`);
      return true;
    } else {
      logger.warn(`No changes were made to assistant with AssistantId: ${AssistantId}`);
      return false;
    }
  } catch (err) {
    logger.error("Error updating assistant", { error: err });
    throw err;
  } finally {
    await client.close();
  }
}

/**
 * Deletes an assistant profile.
 */
async function deleteAssistant(AssistantId, Mod) {
  const { client, assistants } = await getCollections();
  try {
    logger.info(`Deleting assistant with AssistantId: ${AssistantId} for Mod: ${Mod}`);
    const result = await assistants.deleteOne({ AssistantId, Mod });
    if (result.deletedCount > 0) {
      logger.info(`Assistant with AssistantId: ${AssistantId} deleted successfully`);
      return true;
    } else {
      logger.warn(`Assistant with AssistantId: ${AssistantId} was not found or already deleted`);
      return false;
    }
  } catch (err) {
    logger.error("Error deleting assistant", { error: err });
    throw err;
  } finally {
    await client.close();
  }
}

/**
 * Creates a new conversation thread for an assistant.
 */
async function createThread(GUID, AssistantId, Mod, ThreadId) {
  const { client, assistants, threads } = await getCollections();
  try {
    logger.info(`Creating thread with ThreadId: ${ThreadId} for assistant: ${AssistantId} under Mod: ${Mod}`);
    const assistant = await assistants.findOne({ AssistantId, Mod });
    if (!assistant) {
      logger.warn(`Assistant with AssistantId ${AssistantId} not found for Mod ${Mod}`);
      throw new Error("Assistant not found or Mod mismatch");
    }
    const threadData = {
      ThreadId,
      GUID: GUID || "",
      AssistantId,
      Mod,
      messages: [],
      ResponseFormat: assistant.ResponseFormat || {}
    };
    await threads.insertOne(threadData);
    logger.info(`Thread created with ThreadId: ${ThreadId}`);
    return { ThreadId };
  } catch (err) {
    logger.error("Error creating thread", { error: err });
    throw err;
  } finally {
    await client.close();
  }
}

/**
 * Retrieves a thread by its ThreadId.
 */
async function getThread(ThreadId) {
  const { client, threads } = await getCollections();
  try {
    logger.info(`Retrieving thread with ThreadId: ${ThreadId}`);
    const thread = await threads.findOne({ ThreadId });
    logger.info(`Thread retrieval ${thread ? "succeeded" : "failed"}`);
    return thread;
  } catch (err) {
    logger.error("Error retrieving thread", { error: err });
    throw err;
  } finally {
    await client.close();
  }
}

/**
 * Retrieves an assistant profile by ThreadId.
 */
async function getAssistantByThread(ThreadId) {
  const { client, assistants, threads } = await getCollections();
  try {
    logger.info(`Retrieving assistant for thread with ThreadId: ${ThreadId}`);
    const thread = await threads.findOne({ ThreadId });
    if (!thread) {
      logger.warn(`Thread with ThreadId ${ThreadId} not found`);
      throw new Error("Thread not found");
    }
    const assistant = await assistants.findOne({ AssistantId: thread.AssistantId, Mod: thread.Mod });
    logger.info(`Assistant retrieval by thread ${assistant ? "succeeded" : "failed"}`);
    return assistant;
  } catch (err) {
    logger.error("Error retrieving assistant by thread", { error: err });
    throw err;
  } finally {
    await client.close();
  }
}

/**
 * Adds a message to a thread and updates the lastUpdated timestamp.
 */
async function addMessageToThread(ThreadId, role, content, status = "Success") {
  const { client, threads } = await getCollections();
  try {
    logger.info(`Adding message to thread with ThreadId: ${ThreadId}`);
    const MessageId = new (require('mongodb')).ObjectId().toString();
    const message = {
      messageId: MessageId,
      role,
      content,
      status,
      timestamp: new Date()
    };
    const result = await threads.updateOne(
      { ThreadId },
      { $push: { messages: message }, $set: { lastUpdated: new Date() } }
    );
    if (result.modifiedCount > 0) {
      logger.info(`Message added with MessageId: ${MessageId} to thread ${ThreadId}`);
      return MessageId;
    } else {
      logger.warn(`Failed to add message to thread with ThreadId: ${ThreadId}`);
      throw new Error("Failed to add message to thread");
    }
  } catch (err) {
    logger.error("Error adding message to thread", { error: err });
    throw err;
  } finally {
    await client.close();
  }
}

/**
 * Updates the status and content of a message in a thread.
 */
async function updateMessageStatus(ThreadId, messageId, status, content = null) {
  const { client, threads } = await getCollections();
  try {
    logger.info(`Updating message status for MessageId: ${messageId} in thread: ${ThreadId}`);
    const updateFields = { "messages.$.status": status };
    if (content !== null) updateFields["messages.$.content"] = content;
    const result = await threads.updateOne({ ThreadId, "messages.messageId": messageId }, { $set: updateFields });
    if (result.modifiedCount > 0) {
      logger.info(`Message with MessageId ${messageId} updated successfully`);
      return true;
    } else {
      logger.warn(`No message updated for MessageId ${messageId} in thread ${ThreadId}`);
      return false;
    }
  } catch (err) {
    logger.error("Error updating message status", { error: err });
    throw err;
  } finally {
    await client.close();
  }
}

/**
 * Retrieves a message by its MessageId from any thread.
 */
async function getMessageById(MessageId) {
  const { client, threads } = await getCollections();
  try {
    logger.info(`Retrieving message with MessageId: ${MessageId}`);
    const thread = await threads.findOne(
      { "messages.messageId": MessageId },
      { projection: { ThreadId: 1, messages: { $elemMatch: { messageId: MessageId } } } }
    );
    if (!thread || !thread.messages || thread.messages.length === 0) {
      logger.warn(`Message with MessageId ${MessageId} not found`);
      return null;
    }
    logger.info(`Message with MessageId ${MessageId} retrieved successfully`);
    return { ThreadId: thread.ThreadId, message: thread.messages[0] };
  } catch (err) {
    logger.error("Error retrieving message by ID", { error: err });
    throw err;
  } finally {
    await client.close();
  }
}

/**
 * Saves a summary to an existing chat session.
 */
async function saveChatSummary(ThreadId, Summary) {
  const { client, threads } = await getCollections();
  try {
    logger.info(`Saving summary for thread with ThreadId: ${ThreadId}`);
    const date = new Date();
    const result = await threads.updateOne(
      { ThreadId },
      { $set: { Summary, lastUpdated: date, summaryUpdated: date } }
    );
    if (result.modifiedCount > 0) {
      logger.info(`Summary saved for thread with ThreadId: ${ThreadId}`);
      return true;
    } else {
      logger.warn(`Failed to save summary for thread with ThreadId: ${ThreadId}`);
      return false;
    }
  } catch (err) {
    logger.error("Error saving chat summary", { error: err });
    throw err;
  } finally {
    await client.close();
  }
}

module.exports = {
  createAssistant,
  registerExistingAssistant, // To be defined in controllers if needed.
  getAssistantByThread,
  getAssistant,
  listAssistants,
  updateAssistant,
  deleteAssistant,
  createThread,
  getThread,
  addMessageToThread,
  updateMessageStatus,
  getMessageById,
  saveChatSummary
};
