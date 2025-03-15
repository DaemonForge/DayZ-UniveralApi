// controllers/aiAssistant.js
const express = require('express');
const router = express.Router();
const aiAssistantModel = require('../models/aiAssistant');
const { getSummaryById, updateChatSummaryStatus, createChatSummary } = require('../models/aiChat');
const { OpenAI } = require('openai').default;

const Ajv = require('ajv');
const { createLogger } = require('../utils');
const logger = createLogger(global.logger, 'aiAssistant'); // Winston-based logger

// Initialize Ajv.
const ajv = new Ajv({ allErrors: true });

// Initialize OpenAI API client.
let openai;
if (global.config.OpenAIApi?.ApiKey != undefined && global.config.OpenAIApi.ApiKey != "") {
  openai = new OpenAI({ apiKey: global.config.OpenAIApi.ApiKey });
}

// Import authentication middlewares (adjust as needed)
const { requirePlayerOrServerAuth, requireServerAuth } = require('../auth/utils');

router.use((req, res, next) => {
  if (global.OPENAISTATUS === "Disabled") {
    logger.warn(`OpenAI is disabled, AI Chat will not work`, { status: global.OPENAISTATUS });
    return res.status(501).json({ Status: "Error", Error: "OpenAI is disabled" });
  }
  if (global.OPENAISTATUS === "Error") {
    logger.warn(`OpenAI is in an error state, AI Chat will not work`, { status: global.OPENAISTATUS });
    return res.status(501).json({ Status: "Error", Error: "OpenAI is in an error state" });
  }
  next();
});

/*
 * POST /AI/Assistant/Create
 * Body: { AssistantId, Name, Description, Tools (optional), Model (optional), ResponseFormat (optional) }
 * Returns: { Status, AssistantId }
 */
router.post('/:Mod/Create', requireServerAuth, createAssistant);

/*
 * POST /AI/Assistant/RegisterExisting
 * Body: { AssistantId, AssistantApiId, Mod, Name, Description, ResponseFormat (optional) }
 * Returns: { Status, AssistantId }
 */
router.post('/:Mod/RegisterExisting', requireServerAuth, registerExistingAssistant);

/*
 * POST /AI/Assistant/:Mod/Get
 * URL Param: Mod
 * Returns: { Status, Assistants: [ { AssistantId, Name, Description }, ... ] }
 */
router.post('/:Mod/Get', requireServerAuth, getAssistants);

/*
 * POST /AI/Assistant/:Mod/Update/:AssistantId
 * Body: { Updates: { ... } }
 * Returns: { Status, Message }
 */
router.post('/:Mod/Update/:AssistantId', requireServerAuth, updateAssistant);

/*
 * POST /AI/Assistant/:Mod/Delete/:AssistantId
 * Body: { }
 * Returns: { Status, Message }
 */
router.post('/:Mod/Delete/:AssistantId', requireServerAuth, deleteAssistant);

/*
 * POST /AI/Assistant/:Mod/CreateThread/:AssistantId
 * URL Param: AssistantId
 * Body: { GUID } Optional
 * Returns: { Status, ThreadId }
 */
router.post('/:Mod/CreateThread/:AssistantId', requireServerAuth, createThread);

/*
 * POST /AI/Assistant/GetThreadHistory/:ThreadId
 * URL Param: ThreadId
 * Returns: { Status, Thread }
 */
router.post('/GetThreadHistory/:ThreadId', requirePlayerOrServerAuth, getThreadHistory);

/*
 * POST /AI/Assistant/Send/:ThreadId
 * URL Param: ThreadId
 * Body: { Message, Context }  
 * (Context: Optional array of context objects as described)
 * Returns: { Status, MessageId } immediately, then asynchronously updates the message in DB.
 */
router.post('/Send/:ThreadId', requirePlayerOrServerAuth, sendMessageInThread);

/*
 * POST /AI/Assistant/MsgStatus/:MessageId
 * URL Param: MessageId
 * Returns: { Status, Message (if available) }
 */
router.post('/MsgStatus/:MessageId', requirePlayerOrServerAuth, checkMessageStatus);

/*
 * POST /AI/Assistant/Summarize/:ThreadId
  * URL Param: ThreadId
  * Returns: { Status, SummaryId, Error }
  * Initiates summary generation for a thread.
  * Expected: URL parameter ThreadId.
*/
router.post('/Summarize/:ThreadId', requireServerAuth, runSummarizeChat);

/*
 * POST /AI/Assistant/SummaryStatus/:SummaryId
  * URL Param: SummaryId
  * Returns: { Status, Summary, Error }
  * Checks the status of a summary generation.
*/
router.post('/SummaryStatus/:SummaryId', requireServerAuth, getSummaryStatus);

module.exports = router;

/**
 * Generates the JSON response format enforcement message.
 * @param {object} jsonSchema - The JSON schema to be enforced.
 * @returns {string} - The enforcement message.
 */
function getJsonResponseFormatMessage(jsonSchema) {
  return `!IMPORTANT: Respond ONLY with a valid JSON object matching this JSON schema: ${JSON.stringify(jsonSchema)}. DO NOT include extra text.`;
}

/**
 * Validates an object against a provided JSON schema.
 * @param {object} data - The data to validate.
 * @param {object} schema - The JSON Schema.
 * @returns {boolean} - True if valid, false otherwise.
 */
function validateJSON(data, schema) {
  const validate = ajv.compile(schema);
  return { Valid: validate(data), Errors: validate.errors };
}

/**
 * POST /AI/Assistant/:Mod/Create
 * 
 * Expected Request Body:
 * {
 *   "AssistantId": "easyId",
 *   "Name": "Assistant Name",
 *   "Description": "Custom instructions for the assistant",
 *   "Tools": [ { "type": "code_interpreter" } ], // Optional
 *   "Model": "gpt-4o",                          // Optional
 *   "ResponseFormat": { ... }           // Optional JSON Schema
 * }
 * 
 * Expected Response:
 * { "Status": "Success", "AssistantId": "easyId" }
 */
async function createAssistant(req, res) {
  try {
    const { Mod } = req.params;
    const { AssistantId, Name, Description, Tools, Model, ResponseFormat } = req.body;
    console.log(req.body);
    if (!AssistantId || !Mod || !Name || !Description) {
      logger.warn(`Missing required fields for creating assistant`, { AssistantId, Mod, Name, Description });
      return res.status(400).json({ Status: "Error", Error: "AssistantId, Mod, Name, and Description are required" });
    }
    const assistantResp = await openai.beta.assistants.create({
      name: Name,
      instructions: Description,
      tools: Tools || [],
      model: Model || "gpt-4o"
    });
    const AssistantApiId = assistantResp.id;
    const result = await aiAssistantModel.createAssistant(AssistantId, AssistantApiId, Mod, Name, Description, ResponseFormat);
    if (!result.success) {
      logger.warn(`Failed to create assistant in DB`, { AssistantId, error: result.Message });
      return res.status(400).json({ Status: "Error", Error: result.Message });
    }
    logger.info(`Assistant created with ID ${AssistantId}`, { AssistantId, AssistantApiId, Mod });
    return res.status(201).json({ Status: "Success", AssistantId: result.AssistantId });
  } catch (err) {
    logger.error(`Error creating assistant: ${err.message}`, { error: err });
    return res.status(500).json({ Status: "Error", Error: "Failed to create assistant" });
  }
}

/**
 * POST /AI/Assistant/:Mod/RegisterExisting
 * 
 * Expected Request Body:
 * {
 *   "AssistantId": "easyId",
 *   "AssistantApiId": "openai_assistant_id",
 *   "Name": "Assistant Name",
 *   "Description": "Assistant description",
 *   "ResponseFormat": { ... } // Optional
 * }
 * 
 * Expected Response:
 * { "Status": "Success", "AssistantId": "easyId" }
 */
async function registerExistingAssistant(req, res) {
  try {
    const { Mod } = req.params;
    const { AssistantId, AssistantApiId, Name, Description, ResponseFormat } = req.body;
    if (!AssistantId || !AssistantApiId || !Mod || !Name || !Description) {
      logger.warn(`Missing required fields for registering existing assistant`, { AssistantId, AssistantApiId, Mod, Name });
      return res.status(400).json({ Status: "Error", Error: "AssistantId, AssistantApiId, Mod, Name, and Description are required" });
    }
    const result = await aiAssistantModel.registerExistingAssistant(AssistantId, AssistantApiId, Mod, Name, Description, ResponseFormat);
    logger.info(`Existing assistant registered with ID ${AssistantId}`, { AssistantId, AssistantApiId, Mod });
    return res.status(200).json({ Status: "Success", AssistantId: result.AssistantId });
  } catch (err) {
    logger.error(`Error registering assistant: ${err.message}`, { error: err });
    return res.status(500).json({ Status: "Error", Error: "Failed to register assistant" });
  }
}

/**
 * GET /AI/Assistant/:Mod/Get
 * 
 * Expected URL Parameter: Mod
 * 
 * Expected Response:
 * { "Status": "Success", "Assistants": [ { AssistantId, Name, Description }, ... ] }
 */
async function getAssistants(req, res) {
  try {
    const { Mod } = req.params;
    if (!Mod) {
      logger.warn(`Mod parameter is missing for listing assistants`);
      return res.status(400).json({ Status: "Error", Error: "Mod is required" });
    }
    const assistants = await aiAssistantModel.listAssistants(Mod);
    if (!assistants || assistants.length === 0) {
      logger.info(`No assistants found for Mod ${Mod}`, { Mod });
      return res.status(200).json({ Status: "Empty", Assistants: [] });
    }
    logger.info(`Retrieved ${assistants.length} assistants for Mod ${Mod}`, { Mod, count: assistants.length });
    return res.status(200).json({ Status: "Success", Assistants: assistants });
  } catch (err) {
    logger.error(`Error listing assistants: ${err.message}`, { error: err });
    return res.status(500).json({ Status: "Error", Error: "Failed to list assistants" });
  }
}

/**
 * PUT /AI/Assistant/:Mod/Update/:AssistantId
 * 
 * Expected Request Body:
 * {
 *   "Name": "Updated Name", "Description": "Updated description", ... 
 * }
 * 
 * Expected Response:
 * { "Status": "Success", "Message": "Assistant updated successfully" }
 */
async function updateAssistant(req, res) {
  try {
    const { AssistantId, Mod } = req.params;
    const Updates = req.body;
    if (!AssistantId || !Mod || !Updates) {
      logger.warn(`Missing required fields for updating assistant`, { AssistantId, Mod, Updates });
      return res.status(400).json({ Status: "Error", Error: "AssistantId, Mod, and Updates are required" });
    }
    const success = await aiAssistantModel.updateAssistant(AssistantId, Mod, Updates);
    if (!success) {
      logger.warn(`Failed to update assistant ${AssistantId}`, { AssistantId, Mod });
      return res.status(400).json({ Status: "Error", Error: "Failed to update assistant" });
    }
    logger.info(`Assistant updated successfully: ${AssistantId}`, { AssistantId, Mod, updates: Updates });
    return res.status(200).json({ Status: "Success", Message: "Assistant updated successfully" });
  } catch (err) {
    logger.error(`Error updating assistant: ${err.message}`, { error: err });
    return res.status(500).json({ Status: "Error", Error: "Failed to update assistant" });
  }
}

/**
 * DELETE /AI/Assistant/:Mod/Delete/:AssistantId
 * 
 * Expected Request Body:
 * { }
 * 
 * Expected Response:
 * { "Status": "Success" }
 */
async function deleteAssistant(req, res) {
  try {
    const { AssistantId, Mod } = req.params;
    if (!AssistantId || !Mod) {
      logger.warn(`Missing AssistantId or Mod for deletion`, { AssistantId, Mod });
      return res.status(400).json({ Status: "Error", Error: "AssistantId and Mod are required" });
    }
    const success = await aiAssistantModel.deleteAssistant(AssistantId, Mod);
    if (!success) {
      logger.warn(`Assistant not found or already deleted: ${AssistantId}`, { AssistantId, Mod });
      return res.status(404).json({ Status: "Error", Error: "Assistant not found or already deleted" });
    }
    logger.info(`Assistant deleted successfully: ${AssistantId}`, { AssistantId, Mod });
    return res.status(200).json({ Status: "Success", Error: "" });
  } catch (err) {
    logger.error(`Error deleting assistant: ${err.message}`, { error: err });
    return res.status(500).json({ Status: "Error", Error: "Failed to delete assistant" });
  }
}

/**
 * POST /AI/Assistant/:Mod/CreateThread/:AssistantId
 * 
 * Expected URL Parameter: AssistantId
 * Expected Request Body:
 * { "GUID": "user-guid" }
 * 
 * Expected Response:
 * { "Status": "Success", "ThreadId": "thread_id" }
 */
async function createThread(req, res) {
  try {
    const { Mod, AssistantId } = req.params;
    const { GUID } = req.body;
    if (!AssistantId || !Mod) {
      logger.warn(`Missing AssistantId or Mod for thread creation`, { AssistantId, Mod });
      return res.status(400).json({ Status: "Error", Error: "AssistantId and Mod are required" });
    }
    const openaiThread = await openai.beta.threads.create();
    const result = await aiAssistantModel.createThread(GUID, AssistantId, Mod, openaiThread.id);
    logger.info(`New thread created: ${openaiThread.id}`, { threadId: openaiThread.id, AssistantId, Mod, GUID });
    return res.status(201).json({ Status: "Success", ThreadId: openaiThread.id });
  } catch (err) {
    logger.error(`Error creating thread: ${err.message}`, { error: err });
    return res.status(500).json({ Status: "Error", Error: "Failed to create thread" });
  }
}

/**
 * GET /AI/Assistant/GetThreadHistory/:ThreadId
 * 
 * Expected URL Parameter: ThreadId
 * 
 * Expected Response:
 * { "Status": "Success", "Thread": { ThreadId, AssistantId, Mod, messages: [ ... ] } }
 */
async function getThreadHistory(req, res) {
  try {
    const { ThreadId } = req.params;
    if (!ThreadId) {
      logger.warn(`ThreadId missing when retrieving thread history`);
      return res.status(400).json({ Status: "Error", Error: "ThreadId is required" });
    }
    const thread = await aiAssistantModel.getThread(ThreadId);
    if (!thread) {
      logger.warn(`Thread not found: ${ThreadId}`);
      return res.status(404).json({ Status: "Empty", Error: "Thread not found" });
    }
    const { GUID, AssistantId, Mod, messages } = thread;
    logger.info(`Thread history retrieved for ${ThreadId}`, { threadId: ThreadId, AssistantId, Mod });
    return res.status(200).json({ Status: "Success", Thread: { GUID, AssistantId, Mod, Messages: messages } });
  } catch (err) {
    logger.error(`Error retrieving thread history: ${err.message}`, { error: err });
    return res.status(500).json({ Status: "Error", Error: "Failed to retrieve thread history" });
  }
}

/**
 * POST /AI/Assistant/Send/:ThreadId
 * 
 * Expected URL Parameter: ThreadId
 * Expected Request Body:
 * {
 *   "Message": "User message text",
 *   "Context": [                    // Optional array of context objects
 *     {
 *       "Description": "Context description",
 *       "Context": ["string1", "string2", ...]
 *     },
 *     ...
 *   ]
 * }
 * 
 * Flow:
 * 1. Store the user message in the thread DB.
 * 2. Insert an assistant placeholder message with status "Pending" and get its MessageId.
 * 3. Immediately return { "Status": "Pending", "MessageId": "..." }.
 * 4. Asynchronously, call OpenAI's beta.threads.runs.createAndPoll() with the new message (and transformed Context) and update the placeholder.
 * 
 * Expected Response:
 * { "Status": "Pending", "MessageId": "generated_message_id" }
 */
async function sendMessageInThread(req, res) {
  try {
    const { ThreadId, GUID, isServer } = req.params;
    const { Message, Context } = req.body;
    if (!ThreadId || !Message) {
      logger.warn(`Missing ThreadId or Message in sendMessageInThread`, { ThreadId, Message });
      return res.status(400).json({ Status: "Error", Error: "ThreadId and Message are required" });
    }
    const assistant = await aiAssistantModel.getAssistantByThread(ThreadId);
    const thread = await aiAssistantModel.getThread(ThreadId);
    if (GUID !== undefined && GUID !== null && GUID !== "" && (thread.GUID !== GUID) && !isServer) {
      logger.warn(`Unauthorized message send attempt in thread ${ThreadId}`, { threadGUID: thread.GUID, providedGUID: GUID });
      return res.status(403).json({ Status: "NoAuth", Error: "Unauthorized" });
    }
    // Store the user message in our DB.
    await aiAssistantModel.addMessageToThread(ThreadId, "user", Message, "Success");
    // Insert an assistant placeholder with status "Pending" and obtain its MessageId.
    const MessageId = await aiAssistantModel.addMessageToThread(ThreadId, "assistant", "", "Pending");
    logger.info(`User message received in thread ${ThreadId}`, { threadId: ThreadId, Message });
    // Return the MessageId so the client can poll for status.
    res.status(202).json({ Status: "Pending", MessageId });

    // Process the assistant's response asynchronously.
    let instructions = "";
    if (Context && Array.isArray(Context) && Context.length > 0) {
      instructions = Context.map(ctx => {
        let str = "";
        if (ctx.Description) {
          str += `${ctx.Description}: `;
        }
        if (Array.isArray(ctx.Context)) {
          str += ctx.Context.join(", ");
        }
        return str;
      }).join("\n");
    }
    try {
      await openai.beta.threads.messages.create(ThreadId, {
        role: "user",
        content: Message
      });
      const run = await openai.beta.threads.runs.createAndPoll(
        ThreadId,
        {
          assistant_id: assistant.AssistantApiId || "",
          additional_instructions: instructions
        }
      );
      if (run.status === 'completed') {
        const messages = await openai.beta.threads.messages.list(ThreadId);
        const assistantResponse = messages.data[0].content[0].text.value || "";
        await aiAssistantModel.updateMessageStatus(ThreadId, MessageId, "Success", assistantResponse);
        logger.info(`Assistant response successfully updated for message ${MessageId}`, { threadId: ThreadId, MessageId });
      } else {
        logger.error(`Error sending message in thread, run status: ${run.status}`, { threadId: ThreadId, runStatus: run.status });
        await aiAssistantModel.updateMessageStatus(ThreadId, MessageId, "Error", run.status || "");
      }
    } catch (err) {
      logger.error(`Error during assistant processing: ${err.message}`, { error: err, threadId: ThreadId, MessageId });
      await aiAssistantModel.updateMessageStatus(ThreadId, MessageId, "Error", err.message || "");
    }
  } catch (err) {
    logger.error(`Error sending message in thread: ${err.message}`, { error: err });
    return res.status(500).json({ Status: "Error", Error: "Failed to send message in thread" });
  }
}

/**
 * GET /AI/Assistant/MsgStatus/:MessageId
 * 
 * Expected URL Parameter: MessageId
 * 
 * Expected Response:
 * If the message with the given MessageId has status "Success":
 *   { "Status": "Success", "Message": "assistant message" }
 * Otherwise:
 *   { "Status": "Pending" or "Error" }
 */
async function checkMessageStatus(req, res) {
  try {
    const { MessageId } = req.params;
    if (!MessageId) {
      logger.warn(`Missing MessageId when checking message status`);
      return res.status(400).json({ Status: "Error", Error: "MessageId is required" });
    }
    const { message } = await aiAssistantModel.getMessageById(MessageId);
    if (!message) {
      logger.info(`Message not found for MessageId ${MessageId}`, { MessageId });
      return res.status(200).json({ Status: "Empty", Message: "" });
    }
    if (message.status === "Success") {
      logger.info(`Message ${MessageId} has been successfully processed`, { MessageId });
      return res.status(200).json({ Status: "Success", Message: message.content });
    } else {
      logger.info(`Message ${MessageId} in status ${message.status}`, { MessageId, status: message.status });
      return res.status(200).json({ Status: message.status || "", Message: "" });
    }
  } catch (err) {
    logger.error(`Error checking message status: ${err.message}`, { error: err });
    return res.status(500).json({ Status: "Error", Error: "Failed to check message status" });
  }
}

/**
 * POST /AI/Assistant/Summarize/:ThreadId
 * Initiates summary generation for a thread.
 * Expected: URL parameter ThreadId.
 * Returns immediately: { Status: "Success", Summary: "..." } if a valid summary exists,
 * or { Status: "Pending", SummaryId: "..." } if summarization is in progress,
 * or { Status: "Pending", SummaryId: "..." } when starting summarization.
 *
 * Note: You need to implement a model function createChatSummary(ThreadId)
 * which should create a new summary record with initial status "Pending"
 * and return an object, e.g., { SummaryId: "newSummaryUniqueIdentifier" }.
 */
async function runSummarizeChat(req, res) {
  try {
    const { ThreadId } = req.params;
    logger.info(`Received request to summarize thread ${ThreadId}`, { threadId: ThreadId });
    if (!ThreadId) {
      logger.warn(`Missing ThreadId for summarization`);
      return res.status(400).json({ Status: "Error", SummaryId: "", Summary: "", Error: "ThreadId is required" });
    }
    const thread = await aiAssistantModel.getThread(ThreadId);
    if (!thread) {
      logger.warn(`Thread not found for summarization`, { threadId: ThreadId });
      return res.status(200).json({ Status: "NotFound", SummaryId: "", Summary: "", Error: "Thread not found" });
    }
    if (thread.Summary !== undefined && thread.Summary !== "") {
      let timeDif = thread.lastUpdated - thread.summaryUpdated;
      if (timeDif <= 1) {
        logger.info(`Existing summary is up-to-date for thread ${ThreadId}`, { threadId: ThreadId });
        return res.status(200).json({ Status: "Success", SummaryId: "", Summary: thread.Summary, Error: "" });
      }
    }
    
    const summaryRecord = await createChatSummary(ThreadId);
    const { SummaryId } = summaryRecord;
    logger.info(`Created summary record ${SummaryId} for thread ${ThreadId}`, { threadId: ThreadId, SummaryId });
    res.status(202).json({ Status: "Pending", SummaryId, Summary: "", Error: "" });

    // Asynchronous summary processing.
    (async () => {
      try {
        logger.info(`Starting asynchronous summary generation for SummaryId ${SummaryId}`, { threadId: ThreadId, SummaryId });
        let conversationText = thread.SystemMessage ? thread.SystemMessage + "\n" : "";
        thread.messages.forEach(msg => {
          conversationText += `${msg.role}: ${msg.content}\n`;
        });
        logger.info(`Built conversation text for summarization for thread ${ThreadId}`, { threadId: ThreadId, SummaryId });
        const summaryResponse = await openai.chat.completions.create({
          model: 'o3-mini',
          messages: [
            {
              role: 'system',
              content: 'You are tasked with summarizing a conversation between an NPC (an AI in DayZ Standalone) and a player to create a concise, historically accurate record for internal memory management. This summary will replace storing the full conversation, so it must capture essential details while preserving the unique tone and immersion of the interaction. Follow these guidelines:\n\n- Focus on Interaction History:\n  Capture key decisions, significant moments, and notable dialogue from both the AI and the player.\n\n- Player-Centric Detailing:\n  Emphasize the player\'s contributions, including specific statements and nuances of their demeanor, while also noting any relevant information provided by the AI. If previous key details (such as mentions of specific items like an M4A1) are available, include a note for continuity.\n\n- Concise and Fact-Based:\n  Deliver a succinct summary that is factual and strictly based on the conversation transcript. Avoid extraneous details or interpretations beyond what is explicitly stated.\n\n- Immersion and Tone Preservation:\n  Retain elements that enhance immersion, such as game-specific terms, jargon, and categories (for example, gear suggestions, safety warnings). Also, briefly describe the overall mood of the conversation (for example, friendly, formal, tense) as evident from the transcript.\n\n- Structured Format:\n  Organize the summary into bullet points or short paragraphs to clearly separate topics. Do not include any headers, titles, or introductory labels.\n\n- Use Safe Characters:\n  Ensure the summary uses only safe characters; avoid emojis and any special characters that DayZ cannot handle.\n\n- Memory Consistency and Updates:\n  If this conversation references or updates previous interactions, integrate these details to maintain a consistent historical record. Record any changes in the player\'s state (such as inventory or gear updates) and flag new information that modifies or adds to previous memory entries.\nIMPORTANT use basic ASCII Chaters, for example don\'t use • use -'
            },
            { role: 'user', content: `Summarize the following conversation:\n\n"${conversationText}"` }
          ]
        });
        const summaryText = summaryResponse.choices[0].message.content.trim();
        logger.info(`AI generated summary for SummaryId ${SummaryId}`, { threadId: ThreadId, SummaryId, summaryText });
        await updateChatSummaryStatus(SummaryId, "Success", summaryText);
        logger.info(`Updated summary record ${SummaryId} with Success status`, { threadId: ThreadId, SummaryId });
        await aiAssistantModel.saveChatSummary(ThreadId, summaryText);
        logger.info(`Saved thread summary for thread ${ThreadId}`, { threadId: ThreadId, SummaryId });
      } catch (err) {
        logger.error(`Error during asynchronous summary generation: ${err.message}`, { error: err, threadId: ThreadId, SummaryId });
        await updateChatSummaryStatus(SummaryId, "Error", err.message);
      }
    })();
  } catch (err) {
    logger.error(`Error initiating summary generation: ${err.message}`, { error: err });
    return res.status(500).json({ Status: "Error", Error: "Failed to initiate summary generation", Summary: "" });
  }
}

/**
 * Checks the status of a summary generation.
 * Expected: URL parameter SummaryId.
 * Returns:
 *   If status is "Success": { Status: "Success", Summary: "<summary text>" }
 *   If still pending: { Status: "Pending" }
 *   If error: { Status: "Error", Error: "<error message>" }
 *
 * Note: Expected model function: getSummaryById(SummaryId) -> returns an object:
 * { SummaryId, ThreadId, Status, summary } or null if not found.
 */
async function getSummaryStatus(req, res) {
  try {
    const { SummaryId } = req.params;
    logger.info(`Received request to check summary status for SummaryId ${SummaryId}`, { SummaryId });
    if (!SummaryId) {
      logger.warn(`Missing SummaryId when checking summary status`);
      return res.status(400).json({ Status: "Error", Error: "SummaryId is required", Summary: "" });
    }
    const summaryRecord = await getSummaryById(SummaryId);
    if (!summaryRecord || !summaryRecord.Status) {
      logger.warn(`Summary record not found for SummaryId ${SummaryId}`, { SummaryId });
      return res.status(404).json({ Status: "NotFound", Error: "Summary not found", Summary: "" });
    }
    const { Status, Summary } = summaryRecord;
    logger.info(`Returning summary status for SummaryId ${SummaryId}: ${Status}`, { SummaryId, Status });
    if (Status === "Success") {
      return res.status(200).json({ Status, Summary, Error: "" });
    } else if (Status === "Error") {
      return res.status(200).json({ Status, Summary, Error: "" });
    }
    return res.status(200).json({ Status });
  } catch (err) {
    logger.error(`Error retrieving summary status: ${err.message}`, { error: err });
    return res.status(500).json({ Status: "Error", Error: "Failed to retrieve summary status", Summary: "" });
  }
}
