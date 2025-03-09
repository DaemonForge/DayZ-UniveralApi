// models/aiAssistant.js
const { MongoClient, ObjectId } = require('mongodb');
const config = require('../config'); // Expects: { DBServer, DB }

/**
 * Connects to the MongoDB database and returns the relevant collections.
 */
async function getCollections() {
  const client = new MongoClient(config.DBServer);
  await client.connect();
  const db = client.db(config.DB);
  return {
    client,
    assistants: db.collection("Assistants"),
    threads: db.collection("Threads")
  };
}

/**
 * Creates a new assistant profile.
 * Required parameters:
 *   - AssistantId: a user-defined "easy" id (string)
 *   - AssistantApiId: the ID returned by OpenAI's Assistants API (string)
 *   - Mod: moderator identifier (string)
 *   - Name: assistant's name (string)
 *   - Description: description (string)
 *   - ResponseFormat: optional JSON Schema (object)
 */
async function createAssistant(AssistantId, AssistantApiId, Mod, Name, Description, ResponseFormat = null) {
  const { client, assistants } = await getCollections();
  try {
    const existing = await assistants.findOne({ AssistantId, Mod });
    if (existing) {
      return { success: false, Message: "Assistant with this ID already exists under this Mod." };
    }
    const assistantData = {
      AssistantId,
      AssistantApiId,
      Mod,
      Name,
      Description,
      ResponseFormat: ResponseFormat || null,
      // We omit date fields from external returns.
    };
    await assistants.insertOne(assistantData);
    return { success: true, AssistantId };
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
    const updateFields = {
      AssistantApiId,
      Name,
      Description,
      ResponseFormat: ResponseFormat || null
    };
    await assistants.updateOne({ AssistantId, Mod }, { $set: updateFields }, { upsert: true });
    return { success: true, AssistantId };
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
    return await assistants.findOne({ AssistantId, Mod });
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
    return await assistants.find({ Mod }, { projection: { AssistantId: 1, Name: 1, Description: 1 } }).toArray();
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
    const updateFields = {};
    if (updates.Name) updateFields.Name = updates.Name;
    if (updates.Description) updateFields.Description = updates.Description;
    if (updates.ResponseFormat !== undefined) updateFields.ResponseFormat = updates.ResponseFormat;
    if (updates.AssistantApiId) updateFields.AssistantApiId = updates.AssistantApiId;
    const result = await assistants.updateOne({ AssistantId, Mod }, { $set: updateFields });
    return result.modifiedCount > 0;
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
    const result = await assistants.deleteOne({ AssistantId, Mod });
    return result.deletedCount > 0;
  } finally {
    await client.close();
  }
}

/**
 * Creates a new conversation thread for an assistant.
 * The initiating user's identifier is called GUID.
 */
async function createThread(GUID, AssistantId, Mod, ThreadId) {
  const { client, assistants, threads } = await getCollections();
  try {
    const assistant = await assistants.findOne({ AssistantId, Mod });
    if (!assistant) {
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
    return { ThreadId };
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
    return await threads.findOne({ ThreadId });
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
    const thread = await threads.findOne({ ThreadId });
    if (!thread) {
      throw new Error("Thread not found");
    }
    return await assistants.findOne({ AssistantId: thread.AssistantId, Mod: thread.Mod });
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
    const MessageId = new ObjectId().toString();
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
    if (result.modifiedCount > 0) return MessageId;
    else throw new Error("Failed to add message to thread");
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
    const updateFields = { "messages.$.status": status };
    if (content !== null) updateFields["messages.$.content"] = content;
    const result = await threads.updateOne({ ThreadId, "messages.messageId": messageId }, { $set: updateFields });
    return result.modifiedCount > 0;
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
    const thread = await threads.findOne(
      { "messages.messageId": MessageId },
      { projection: { ThreadId: 1, messages: { $elemMatch: { messageId: MessageId } } } }
    );
    if (!thread || !thread.messages || thread.messages.length === 0) return null;
    return { ThreadId: thread.ThreadId, message: thread.messages[0] };
  } finally {
    await client.close();
  }
}


/**
 * Saves a summary to an existing chat session.
 * This function updates the chat document by adding or updating the 'summary' field.
 *
 * @param {string} ThreadId - The unique identifier of the chat session.
 * @param {string} summary - The summary text to be saved.
 * @returns {Promise<boolean>} - Returns true if the update was successful.
 */
async function saveChatSummary(ThreadId, Summary) {
    const { client, threads } = await getCollections();
    try {
        const date = new Date()
        const result = await threads.updateOne(
            { ThreadId },
            { $set: { Summary, lastUpdated: date, summaryUpdated: date } }
        );
        return result.modifiedCount > 0;
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
