const { MongoClient, ObjectId } = require('mongodb');
const config = require('../config'); // Exports { DBServer, DB }

/**
 * Connects to the database and returns the Chats collection.
 * @returns {Promise<{ client: MongoClient, chats: Collection }>}
 */
async function getCollections() {
  const client = new MongoClient(config.DBServer);
  await client.connect();
  const db = client.db(config.DB);
  return { client, chats: db.collection("Chats"), chatSummaries: db.collection("ChatSummaries")};
}

/**
 * Creates a new chat session.
 * @param {string} SystemMessage - The initial system prompt.
 * @param {string} ResponseFormat - "string" or "JSON"
 * @param {object|null} JsonSchema - Required if ResponseFormat is "JSON"
 * @param {string} Model - OpenAI Model to use.
 * @param {number} MaxHistory - Maximum number of Messages for context.
 * @returns {Promise<{ ChatId: string }>}
 */
async function createChat(SystemMessage, ResponseFormat, JsonSchema, Model, MaxHistory) {
  const { client, chats } = await getCollections();
  try {
    const chatData = {
      SystemMessage,
      ResponseFormat,
      JsonSchema: JsonSchema || {},
      Model: Model || 'gpt-4o-mini',
      MaxHistory: MaxHistory || 20,
      Messages: [],
      createdAt: new Date(),
      lastUpdated: new Date()
    };
    const result = await chats.insertOne(chatData);
    const ChatId = result.insertedId.toString();
    await chats.updateOne({ _id: result.insertedId }, { $set: { ChatId } });
    return { ChatId };
  } finally {
    await client.close();
  }
}

/**
 * Retrieves a chat session by ChatId.
 * @param {string} ChatId
 * @returns {Promise<object|null>}
 */
async function getChat(ChatId) {
  const { client, chats } = await getCollections();
  try {
    return await chats.findOne({ ChatId });
  } finally {
    await client.close();
  }
}

/**
 * Appends a new message to a chat session.
 * @param {string} ChatId
 * @param {string} role - "user" or "assistant"
 * @param {string} content - The message content.
 * @param {string} status - Initial status.
 * @returns {Promise<string>} - The generated MessageId.
 */
async function addMessageToChat(ChatId, role, content, status = "Success") {
  const { client, chats } = await getCollections();
  try {
    const MessageId = new ObjectId().toString();
    const message = {
      MessageId,
      role,
      content,
      status,
      timestamp: new Date()
    };
    const result = await chats.updateOne(
      { ChatId },
      { $push: { Messages: message }, $set: { lastUpdated: new Date() } }
    );
    if (result.modifiedCount > 0) return MessageId;
    else throw new Error("Failed to add message");
  } finally {
    await client.close();
  }
}

/**
 * Updates the status (and optionally the content) of a specific message.
 * Also updates the lastUpdated field of the parent chat.
 * @param {string} ChatId
 * @param {string} MessageId
 * @param {string} status - New status.
 * @param {string|null} content - Optional new content.
 * @returns {Promise<boolean>}
 */
async function updateMessageStatus(ChatId, MessageId, status, content = null) {
    const { client, chats } = await getCollections();
    try {
        const now = new Date();
        const updateFields = { 
            "Messages.$.status": status, 
            "Messages.$.timestamp": now,
            lastUpdated: now 
        };
        if (content !== null) updateFields["Messages.$.content"] = content;
        const result = await chats.updateOne(
            { ChatId, "Messages.MessageId": MessageId }, 
            { $set: updateFields }
        );
        return result.modifiedCount > 0;
    } finally {
        await client.close();
    }
}

/**
 * Retrieves a specific message by its MessageId from any chat.
 * @param {string} MessageId
 * @returns {Promise<{ ChatId: string, message: object } | null>}
 */
async function getMessageById(MessageId) {
  const { client, chats } = await getCollections();
  try {
    // Query without ChatId, search across all chats.
    const chat = await chats.findOne({ "Messages.MessageId": MessageId }, { projection: { ChatId: 1, "Messages.$": 1 } });
    if (!chat || !chat.Messages || chat.Messages.length === 0) return null;
    return { ChatId: chat.ChatId, message: chat.Messages[0] };
  } finally {
    await client.close();
  }
}

/**
 * Retrieves the full chat history.
 * @param {string} ChatId
 * @returns {Promise<object|null>}
 */
async function getChatHistory(ChatId) {
  const { client, chats } = await getCollections();
  try {
    return await chats.findOne({ ChatId });
  } finally {
    await client.close();
  }
}

/**
 * Resets a chat session by clearing its Messages.
 * @param {string} ChatId
 * @returns {Promise<boolean>}
 */
async function resetChat(ChatId) {
  const { client, chats } = await getCollections();
  try {
    const result = await chats.updateOne({ ChatId }, { $set: { Messages: [], Summary: "", lastUpdated: new Date() } });
    return result.modifiedCount > 0;
  } finally {
    await client.close();
  }
}

/**
 * Deletes a chat session.
 * @param {string} ChatId
 * @returns {Promise<boolean>}
 */
async function deleteChat(ChatId) {
  const { client, chats } = await getCollections();
  try {
    const result = await chats.deleteOne({ ChatId });
    return result.deletedCount > 0;
  } finally {
    await client.close();
  }
}

/**
 * Saves a summary to an existing chat session.
 * This function updates the chat document by adding or updating the 'summary' field.
 *
 * @param {string} ChatId - The unique identifier of the chat session.
 * @param {string} summary - The summary text to be saved.
 * @returns {Promise<boolean>} - Returns true if the update was successful.
 */
async function saveChatSummary(ChatId, Summary) {
    const { client, chats } = await getCollections();
    try {
        const date = new Date()
        const result = await chats.updateOne(
            { ChatId },
            { $set: { Summary, lastUpdated: date, summaryUpdated: date } }
        );
        return result.modifiedCount > 0;
    } finally {
        await client.close();
    }
}

async function updateChatSummaryStatus(SummaryId, Status, Summary) {
    const { client, chatSummaries } = await getCollections();
    try {
        const date = new Date()
        const result = await chatSummaries.updateOne(
            { SummaryId },
            { $set: { Summary, Status, lastUpdated: date, summaryUpdated: date } }
        );
        return result.modifiedCount > 0;
    } finally {
        await client.close();
    }
}

/**
 * Creates a new chat summary record with status "Pending".
 * @param {string} ChatId - The chat session identifier.
 * @returns {Promise<{ SummaryId: string }>}
 */
async function createChatSummary(ChatId) {
    const { client, chatSummaries } = await getCollections();
    try {
        const summaryData = {
            ChatId,
            Status: "Pending",
            Summary: "",
            createdAt: new Date(),
            lastUpdated: new Date()
        };
        const result = await chatSummaries.insertOne(summaryData);
        const SummaryId = result.insertedId.toString();
        await chatSummaries.updateOne({ _id: result.insertedId }, { $set: { SummaryId } });
        return { SummaryId };
    } finally {
        await client.close();
    }
}
      
    /**
     * Retrieves a chat summary by its SummaryId.
     * @param {string} SummaryId
     * @returns {Promise<object|null>} - Returns the summary document or null if not found.
     */
    async function getSummaryById(SummaryId) {
        const { client, chatSummaries } = await getCollections();
        try {
            return await chatSummaries.findOne({ SummaryId });
        } finally {
            await client.close();
        }
    }


module.exports = {
    createChatSummary,
    getSummaryById,
    updateChatSummaryStatus,
    saveChatSummary,
    createChat,
    getChat,
    addMessageToChat,
    updateMessageStatus,
    getMessageById,
    getChatHistory,
    resetChat,
    deleteChat
};
