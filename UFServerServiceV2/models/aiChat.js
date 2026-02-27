const { MongoClient, ObjectId } = require('mongodb');
const { createLogger } = require('../utils');
const logger = createLogger(global.logger, 'db.aiChat');

/**
 * Connects to the database and returns the Chats collection.
 * @returns {Promise<{ client: MongoClient, chats: Collection, chatSummaries: Collection }>}
 */
async function getCollections() {
  const client = new MongoClient(global.config.DBServer);
  await client.connect();
  const db = client.db(global.config.DB);
  return { client, chats: db.collection("Chats"), chatSummaries: db.collection("ChatSummaries") };
}

/**
 * Creates a new chat session.
 * @param {string} SystemMessage - The initial system prompt.
 * @param {string} ResponseFormat - "string" or "JSON"
 * @param {object|null} JsonSchema - Required if ResponseFormat is "JSON"
 * @param {string} Model - OpenAI Model to use.
 * @param {number} MaxHistory - Maximum number of Messages for context.
 * @param {string|null} KBId - Optional Knowledge Base ID for KB-enhanced chat.
 * @returns {Promise<{ ChatId: string }>}
 */
async function createChat(SystemMessage, ResponseFormat, JsonSchema, Model, MaxHistory, KBId = null) {
  const { client, chats } = await getCollections();
  try {
    logger.debug("Creating new chat session", { 
      SystemMessageLen: SystemMessage?.length, 
      ResponseFormat, 
      Model: Model || 'gpt-4o-mini',
      MaxHistory: MaxHistory || 20,
      KBId: KBId || 'none'
    });
    
    // Append KB-awareness instructions if KB is enabled
    let enhancedSystemMessage = SystemMessage;
    if (KBId) {
      const kbInstructions = `

## Knowledge Base Instructions
You have access to a knowledge base tool called "__kb_search" that contains important information specific to this context. 

**IMPORTANT:** Before answering questions, ALWAYS use the __kb_search tool to look up relevant information from the knowledge base. This ensures your responses are accurate and based on the specific information available.

The knowledge base search will return relevant documents that you should use to formulate your response. Always prefer KB information over your general knowledge when answering questions.`;
      
      enhancedSystemMessage = SystemMessage + kbInstructions;
      logger.debug("KB instructions appended to system message", { 
        KBId, 
        originalLen: SystemMessage.length, 
        enhancedLen: enhancedSystemMessage.length 
      });
    }
    
    const chatData = {
      SystemMessage: enhancedSystemMessage,
      OriginalSystemMessage: SystemMessage, // Store original for reference
      ResponseFormat,
      JsonSchema: JsonSchema || {},
      Model: Model || 'gpt-4o-mini',
      MaxHistory: MaxHistory || 20,
      KBId: KBId || null,
      Messages: [],
      createdAt: new Date(),
      lastUpdated: new Date()
    };
    const result = await chats.insertOne(chatData);
    const ChatId = result.insertedId.toString();
    await chats.updateOne({ _id: result.insertedId }, { $set: { ChatId } });
    logger.info(`Chat created successfully`, { ChatId, KBId: KBId || 'none' });
    return { ChatId };
  } catch (error) {
    logger.error(`Error creating chat: ${error.message}`, { error });
    throw error;
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
    logger.info(`Retrieving chat with ChatId ${ChatId}`);
    const chat = await chats.findOne({ ChatId });
    logger.info(`Chat retrieved`, { ChatId, found: !!chat });
    return chat;
  } catch (error) {
    logger.error(`Error getting chat: ${error.message}`, { error });
    throw error;
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
    logger.info(`Adding a new message to chat ${ChatId}`, { role });
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
    if (result.modifiedCount > 0) {
      logger.info(`Message added successfully with MessageId ${MessageId}`);
      return MessageId;
    } else {
      throw new Error("Failed to add message");
    }
  } catch (error) {
    logger.error(`Error adding message: ${error.message}`, { error });
    throw error;
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
    logger.info(`Updating message status for MessageId ${MessageId} in ChatId ${ChatId}`, { status });
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
    if (result.modifiedCount > 0) {
      logger.info(`Message status updated successfully for MessageId ${MessageId}`);
      return true;
    } else {
      logger.warn(`No message updated for MessageId ${MessageId}`);
      return false;
    }
  } catch (error) {
    logger.error(`Error updating message status: ${error.message}`, { error });
    throw error;
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
    logger.info(`Retrieving message with MessageId ${MessageId}`);
    const chat = await chats.findOne(
      { "Messages.MessageId": MessageId },
      { projection: { ChatId: 1, "Messages.$": 1 } }
    );
    if (!chat || !chat.Messages || chat.Messages.length === 0) {
      logger.warn(`Message not found for MessageId ${MessageId}`);
      return null;
    }
    logger.info(`Message retrieved for MessageId ${MessageId}`);
    return { ChatId: chat.ChatId, message: chat.Messages[0] };
  } catch (error) {
    logger.error(`Error retrieving message: ${error.message}`, { error });
    throw error;
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
    logger.info(`Retrieving chat history for ChatId ${ChatId}`);
    const chat = await chats.findOne({ ChatId });
    logger.info(`Chat history retrieved for ChatId ${ChatId}`, { found: !!chat });
    return chat;
  } catch (error) {
    logger.error(`Error retrieving chat history: ${error.message}`, { error });
    throw error;
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
    logger.info(`Resetting chat for ChatId ${ChatId}`);
    const result = await chats.updateOne(
      { ChatId },
      { $set: { Messages: [], Summary: "", lastUpdated: new Date() } }
    );
    if (result.modifiedCount > 0) {
      logger.info(`Chat reset successfully for ChatId ${ChatId}`);
      return true;
    } else {
      logger.warn(`Chat reset did not modify any document for ChatId ${ChatId}`);
      return false;
    }
  } catch (error) {
    logger.error(`Error resetting chat: ${error.message}`, { error });
    throw error;
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
    logger.info(`Deleting chat with ChatId ${ChatId}`);
    const result = await chats.deleteOne({ ChatId });
    if (result.deletedCount > 0) {
      logger.info(`Chat deleted successfully for ChatId ${ChatId}`);
      return true;
    } else {
      logger.warn(`No chat found to delete for ChatId ${ChatId}`);
      return false;
    }
  } catch (error) {
    logger.error(`Error deleting chat: ${error.message}`, { error });
    throw error;
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
    logger.info(`Saving chat summary for ChatId ${ChatId}`);
    const date = new Date();
    const result = await chats.updateOne(
      { ChatId },
      { $set: { Summary, lastUpdated: date, summaryUpdated: date } }
    );
    if (result.modifiedCount > 0) {
      logger.info(`Chat summary saved for ChatId ${ChatId}`);
      return true;
    } else {
      logger.warn(`Chat summary not saved for ChatId ${ChatId}`);
      return false;
    }
  } catch (error) {
    logger.error(`Error saving chat summary: ${error.message}`, { error });
    throw error;
  } finally {
    await client.close();
  }
}

async function updateChatSummaryStatus(SummaryId, Status, Summary) {
  const { client, chatSummaries } = await getCollections();
  try {
    logger.info(`Updating chat summary status for SummaryId ${SummaryId}`, { Status });
    const date = new Date();
    const result = await chatSummaries.updateOne(
      { SummaryId },
      { $set: { Summary, Status, lastUpdated: date, summaryUpdated: date } }
    );
    if (result.modifiedCount > 0) {
      logger.info(`Chat summary status updated for SummaryId ${SummaryId}`);
      return true;
    } else {
      logger.warn(`No update performed for chat summary with SummaryId ${SummaryId}`);
      return false;
    }
  } catch (error) {
    logger.error(`Error updating chat summary status: ${error.message}`, { error });
    throw error;
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
    logger.info(`Creating chat summary for ChatId ${ChatId}`);
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
    logger.info(`Chat summary created with SummaryId ${SummaryId}`);
    return { SummaryId };
  } catch (error) {
    logger.error(`Error creating chat summary: ${error.message}`, { error });
    throw error;
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
    logger.info(`Retrieving chat summary with SummaryId ${SummaryId}`);
    const summary = await chatSummaries.findOne({ SummaryId });
    logger.info(`Chat summary retrieved`, { SummaryId, found: !!summary });
    return summary;
  } catch (error) {
    logger.error(`Error retrieving chat summary: ${error.message}`, { error });
    throw error;
  } finally {
    await client.close();
  }
}

/**
 * Updates a message with tool call information.
 * @param {string} ChatId
 * @param {string} MessageId
 * @param {object} toolCall - The tool call info { id, name, arguments }
 * @returns {Promise<boolean>}
 */
async function updateMessageWithToolCall(ChatId, MessageId, toolCall) {
  const { client, chats } = await getCollections();
  try {
    logger.info(`Updating message with tool call for MessageId ${MessageId}`, { toolCall });
    const now = new Date();
    const result = await chats.updateOne(
      { ChatId, "Messages.MessageId": MessageId },
      { 
        $set: { 
          "Messages.$.toolCall": toolCall,
          "Messages.$.timestamp": now,
          lastUpdated: now
        } 
      }
    );
    return result.modifiedCount > 0;
  } catch (error) {
    logger.error(`Error updating message with tool call: ${error.message}`, { error });
    throw error;
  } finally {
    await client.close();
  }
}

/**
 * Gets a message with its tool call info.
 * @param {string} MessageId
 * @returns {Promise<{ ChatId: string, message: object } | null>}
 */
async function getMessageWithToolCall(MessageId) {
  const { client, chats } = await getCollections();
  try {
    logger.info(`Retrieving message with tool call for MessageId ${MessageId}`);
    const chat = await chats.findOne(
      { "Messages.MessageId": MessageId },
      { projection: { ChatId: 1, SystemMessage: 1, ResponseFormat: 1, JsonSchema: 1, Model: 1, MaxHistory: 1, Messages: 1 } }
    );
    if (!chat || !chat.Messages) {
      return null;
    }
    const message = chat.Messages.find(m => m.MessageId === MessageId);
    if (!message) {
      return null;
    }
    return { chat, message };
  } catch (error) {
    logger.error(`Error retrieving message with tool call: ${error.message}`, { error });
    throw error;
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
  updateMessageWithToolCall,
  getMessageWithToolCall,
  getMessageById,
  getChatHistory,
  resetChat,
  deleteChat
};
