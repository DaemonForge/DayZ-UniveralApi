// controllers/aiAssistant.js
const express = require('express');
const router = express.Router();
const aiAssistantModel = require('../models/aiAssistant');
const { getSummaryById, updateChatSummaryStatus, createChatSummary } = require('../models/aiChat');
// Note: the Assistants API is OpenAI-proprietary. With a custom
// OpenAIApi.BaseURL (compat provider) these endpoints will not work.
const { createClient } = require('../aiClient');

const Ajv = require('ajv');
const { createLogger } = require('../utils');
const logger = createLogger(global.logger, 'aiAssistant'); // Winston-based logger

// Initialize Ajv.
const ajv = new Ajv({ allErrors: true });

// Initialize OpenAI API client.
let openai;
if (global.config.OpenAIApi?.ApiKey && global.config.OpenAIApi.ApiKey !== "") {
  openai = createClient();
  logger.debug(`OpenAI client initialized with provided API key.`);
} else {
  logger.debug(`No OpenAI API key found.`);
}

// Import authentication middlewares (adjust as needed)
const { requirePlayerOrServerAuth, requireServerAuth } = require('../auth/utils');

router.use((req, res, next) => {
  logger.debug('Global middleware checking OpenAI status...', { OPENAISTATUS: global.OPENAISTATUS });
  if (global.OPENAISTATUS === "Disabled") {
    logger.warn('OpenAI is disabled, AI Chat will not work', { status: global.OPENAISTATUS });
    return res.status(501).json({ Status: "Error", Error: "OpenAI is disabled" });
  }
  if (global.OPENAISTATUS === "Error") {
    logger.warn('OpenAI is in an error state, AI Chat will not work', { status: global.OPENAISTATUS });
    return res.status(501).json({ Status: "Error", Error: "OpenAI is in an error state" });
  }
  if (global.OPENAISTATUS !== "Online") {
    logger.warn('OpenAI is not online yet, AI Assistant will not work', { status: global.OPENAISTATUS });
    return res.status(503).json({ Status: "Error", Error: "OpenAI is not online" });
  }
  next();
});

/*
 * POST /AI/Assistant/Create
 * Body: { AssistantId, Name, Description, Tools (optional), Model (optional), ResponseFormat (optional), Functions (optional) }
 * Returns: { Status, AssistantId }
 */
router.post('/:Mod/Create', requireServerAuth, createAssistant);

/*
 * POST /AI/Assistant/RegisterExisting
 * Body: { AssistantId, AssistantApiId, Mod, Name, Description, ResponseFormat (optional), Functions (optional) }
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
 * Flow:
 *  1. Store the user message.
 *  2. Insert an assistant placeholder message with status "Pending" and get its MessageId.
 *  3. Immediately return { Status: "Pending", MessageId }.
 *  4. Asynchronously, call OpenAI and update the placeholder.
 * Returns: { Status, MessageId }
 */
router.post('/Send/:ThreadId', requirePlayerOrServerAuth, sendMessageInThread);

/*
 * POST /AI/Assistant/MsgStatus/:MessageId
 * URL Param: MessageId
 * Returns:
 * - If processed, a JSON object with { Message, FunctionCall, FunctionArguments, FunctionReturn }.
 * - Otherwise, plain text message.
 */
router.post('/MsgStatus/:MessageId', requirePlayerOrServerAuth, checkMessageStatus);

/*
 * POST /AI/Assistant/Summarize/:ThreadId
 * URL Param: ThreadId
 * Returns: { Status, SummaryId, Error }
 * Initiates summary generation.
 */
router.post('/Summarize/:ThreadId', requireServerAuth, runSummarizeChat);

/*
 * POST /AI/Assistant/SummaryStatus/:SummaryId
 * URL Param: SummaryId
 * Returns: { Status, Summary, Error }
 * Checks the status of summary generation.
 */
router.post('/SummaryStatus/:SummaryId', requireServerAuth, getSummaryStatus);

/*
 * NEW: POST /AI/Assistant/FunctionReturn/:MessageId
 * URL Param: MessageId
 * Expected Body: { FunctionReturn: <string or JSON> }
 * This endpoint is called by a remote server after processing a function call.
 * It updates the original message with the returned value and creates a new assistant message.
 */
router.post('/FunctionReturn/:MessageId', requireServerAuth, handleFunctionReturn);

/* ------------------- Endpoint Implementations --------------------- */

async function createAssistant(req, res) {
  try {
    const { Mod } = req.params;
    const { AssistantId, Name, Description, Tools, Model, ResponseFormat, Functions } = req.body;
    logger.info(`[createAssistant] Request received`, { AssistantId, Mod, Name });
    if (!AssistantId || !Mod || !Name || !Description) {
      logger.warn('[createAssistant] Missing required fields', { AssistantId, Mod, Name, Description });
      return res.status(400).json({ Status: "Error", Error: "AssistantId, Mod, Name, and Description are required" });
    }
    const assistantResp = await openai.beta.assistants.create({
      name: Name,
      instructions: Description,
      tools: Tools || [],
      model: Model || "gpt-4o",
      functions: Functions || []
    });
    logger.debug('[createAssistant] OpenAI assistant created', { AssistantApiId: assistantResp.id });
    const AssistantApiId = assistantResp.id;
    const result = await aiAssistantModel.createAssistant(AssistantId, AssistantApiId, Mod, Name, Description, ResponseFormat, Functions);
    if (!result.success) {
      logger.warn('[createAssistant] Failed to create assistant in DB', { AssistantId, error: result.Message });
      return res.status(400).json({ Status: "Error", Error: result.Message });
    }
    logger.info(`[createAssistant] Assistant created with ID ${AssistantId}`, { AssistantApiId, Mod });
    return res.status(200).json({ Status: "Success", AssistantId: result.AssistantId });
  } catch (err) {
    logger.error(`[createAssistant] Error: ${err.message}`, { error: err });
    return res.status(500).json({ Status: "Error", Error: "Failed to create assistant" });
  }
}

async function registerExistingAssistant(req, res) {
  try {
    const { Mod } = req.params;
    const { AssistantId, AssistantApiId, Name, Description, ResponseFormat, Functions } = req.body;
    logger.info(`[registerExistingAssistant] Request received`, { AssistantId, Mod });
    if (!AssistantId || !AssistantApiId || !Mod || !Name || !Description) {
      logger.warn('[registerExistingAssistant] Missing required fields', { AssistantId, AssistantApiId, Mod, Name });
      return res.status(400).json({ Status: "Error", Error: "AssistantId, AssistantApiId, Mod, Name, and Description are required" });
    }
    const result = await aiAssistantModel.registerExistingAssistant(AssistantId, AssistantApiId, Mod, Name, Description, ResponseFormat, Functions);
    logger.info(`[registerExistingAssistant] Existing assistant registered with ID ${AssistantId}`, { AssistantApiId, Mod });
    return res.status(200).json({ Status: "Success", AssistantId: result.AssistantId });
  } catch (err) {
    logger.error(`[registerExistingAssistant] Error: ${err.message}`, { error: err });
    return res.status(500).json({ Status: "Error", Error: "Failed to register assistant" });
  }
}

async function getAssistants(req, res) {
  try {
    const { Mod } = req.params;
    logger.info(`[getAssistants] Listing assistants`, { Mod });
    if (!Mod) {
      logger.warn('[getAssistants] Missing Mod parameter');
      return res.status(400).json({ Status: "Error", Error: "Mod is required" });
    }
    const assistants = await aiAssistantModel.listAssistants(Mod);
    logger.debug(`[getAssistants] Retrieved ${assistants.length} assistants`);
    if (!assistants || assistants.length === 0) {
      logger.info(`[getAssistants] No assistants found for Mod ${Mod}`);
      return res.status(200).json({ Status: "Empty", Assistants: [] });
    }
    return res.status(200).json({ Status: "Success", Assistants: assistants });
  } catch (err) {
    logger.error(`[getAssistants] Error: ${err.message}`, { error: err });
    return res.status(500).json({ Status: "Error", Error: "Failed to list assistants" });
  }
}

async function updateAssistant(req, res) {
  try {
    const { AssistantId, Mod } = req.params;
    const Updates = req.body;
    logger.info(`[updateAssistant] Request received`, { AssistantId, Mod });
    if (!AssistantId || !Mod || !Updates) {
      logger.warn('[updateAssistant] Missing required fields', { AssistantId, Mod, Updates });
      return res.status(400).json({ Status: "Error", Error: "AssistantId, Mod, and Updates are required" });
    }
    const success = await aiAssistantModel.updateAssistant(AssistantId, Mod, Updates);
    if (!success) {
      logger.warn(`[updateAssistant] Failed to update assistant ${AssistantId}`);
      return res.status(400).json({ Status: "Error", Error: "Failed to update assistant" });
    }
    logger.info(`[updateAssistant] Assistant ${AssistantId} updated successfully`);
    return res.status(200).json({ Status: "Success", Message: "Assistant updated successfully" });
  } catch (err) {
    logger.error(`[updateAssistant] Error: ${err.message}`, { error: err });
    return res.status(500).json({ Status: "Error", Error: "Failed to update assistant" });
  }
}

async function deleteAssistant(req, res) {
  try {
    const { AssistantId, Mod } = req.params;
    logger.info(`[deleteAssistant] Request received`, { AssistantId, Mod });
    if (!AssistantId || !Mod) {
      logger.warn('[deleteAssistant] Missing AssistantId or Mod', { AssistantId, Mod });
      return res.status(400).json({ Status: "Error", Error: "AssistantId and Mod are required" });
    }
    const success = await aiAssistantModel.deleteAssistant(AssistantId, Mod);
    if (!success) {
      logger.warn(`[deleteAssistant] Assistant ${AssistantId} not found or already deleted`);
      return res.status(404).json({ Status: "Error", Error: "Assistant not found or already deleted" });
    }
    logger.info(`[deleteAssistant] Assistant ${AssistantId} deleted successfully`);
    return res.status(200).json({ Status: "Success", Error: "" });
  } catch (err) {
    logger.error(`[deleteAssistant] Error: ${err.message}`, { error: err });
    return res.status(500).json({ Status: "Error", Error: "Failed to delete assistant" });
  }
}

async function createThread(req, res) {
  try {
    const { Mod, AssistantId } = req.params;
    const { GUID } = req.body;
    logger.info(`[createThread] Request received`, { AssistantId, Mod, GUID });
    if (!AssistantId || !Mod) {
      logger.warn('[createThread] Missing AssistantId or Mod', { AssistantId, Mod });
      return res.status(400).json({ Status: "Error", Error: "AssistantId and Mod are required" });
    }
    const openaiThread = await openai.beta.threads.create();
    logger.debug('[createThread] OpenAI thread created', { openaiThreadId: openaiThread.id });
    const result = await aiAssistantModel.createThread(GUID, AssistantId, Mod, openaiThread.id);
    logger.info(`[createThread] Thread created with ThreadId: ${openaiThread.id}`);
    return res.status(200).json({ Status: "Success", ThreadId: openaiThread.id });
  } catch (err) {
    logger.error(`[createThread] Error: ${err.message}`, { error: err });
    return res.status(500).json({ Status: "Error", Error: "Failed to create thread" });
  }
}

async function getThreadHistory(req, res) {
  try {
    const { ThreadId } = req.params;
    logger.debug(`[getThreadHistory] Retrieving history for ThreadId=${ThreadId}`);
    if (!ThreadId) {
      logger.warn('[getThreadHistory] Missing ThreadId');
      return res.status(400).json({ Status: "Error", Error: "ThreadId is required" });
    }
    const thread = await aiAssistantModel.getThread(ThreadId);
    if (!thread) {
      logger.warn(`[getThreadHistory] Thread ${ThreadId} not found`);
      return res.status(404).json({ Status: "Empty", Error: "Thread not found" });
    }
    logger.info(`[getThreadHistory] Retrieved thread history for ThreadId=${ThreadId}`);
    const { GUID, AssistantId, Mod, messages } = thread;
    return res.status(200).json({ Status: "Success", Thread: { GUID, AssistantId, Mod, Messages: messages } });
  } catch (err) {
    logger.error(`[getThreadHistory] Error: ${err.message}`, { error: err });
    return res.status(500).json({ Status: "Error", Error: "Failed to retrieve thread history" });
  }
}

/* ======================================================================
   sendMessageInThread
   POST /AI/Assistant/Send/:ThreadId
   Body: { "Message": "user query", "Context": [...] }
   Response Example: { "Status": "Pending", "MessageId": "<id>" }
===================================================================== */
async function sendMessageInThread(req, res) {
  try {
    const { ThreadId } = req.params;
    const { Message, Context } = req.body;
    logger.info('[sendMessageInThread] Request received', { ThreadId, MessageLength: Message?.length });
    if (!ThreadId || !Message) {
      logger.warn('[sendMessageInThread] Missing required fields', { ThreadId, Message });
      return res.status(400).json({ Status: "Error", Error: "ThreadId and Message are required" });
    }
    
    // 1. Save the user message.
    await aiAssistantModel.addMessageToThread(ThreadId, "user", Message, "Success");
    logger.debug('[sendMessageInThread] User message stored.');

    // 2. Create an assistant placeholder message with status "Pending".
    const MessageId = await aiAssistantModel.addMessageToThread(ThreadId, "assistant", "", "Pending");
    logger.debug('[sendMessageInThread] Assistant placeholder inserted', { MessageId });

    // 3. Return the placeholder MessageId immediately.
    res.status(200).json({ Status: "Pending", MessageId });
    logger.debug('[sendMessageInThread] Returned placeholder MessageId to client', { MessageId });
    
    // Build additional instructions (if context is provided).
    let instructions = "";
    if (Context && Array.isArray(Context) && Context.length > 0) {
      instructions = Context.map(ctx => {
        let str = "";
        if (ctx.Description) { str += `${ctx.Description}: `; }
        if (Array.isArray(ctx.Context)) { str += ctx.Context.join(", "); }
        return str;
      }).join("\n");
    }
    logger.debug('[sendMessageInThread] Built additional instructions', { instructions });
    
    // 4. Asynchronously call OpenAI.
    try {
      logger.debug('[sendMessageInThread] Sending user message to OpenAI...');
      await openai.beta.threads.messages.create(ThreadId, { role: "user", content: Message });
      
      const assistantInfo = await aiAssistantModel.getAssistantByThread(ThreadId);
      logger.debug('[sendMessageInThread] Retrieved assistant info', { assistantInfo });
      
      const runResult = await openai.beta.threads.runs.createAndPoll(ThreadId, {
        assistant_id: (assistantInfo && assistantInfo.AssistantApiId) || "",
        additional_instructions: instructions
      });
      logger.debug('[sendMessageInThread] OpenAI run result received', { run: runResult });
      
      let assistantResponse = "";
      let newStatus = "Success"; // default
      
      if (runResult.status === "completed") {
        // Plain text final answer.
        const messagesRes = await openai.beta.threads.messages.list(ThreadId);
        logger.debug('[sendMessageInThread] Retrieved messages from OpenAI (completed)', { messages: messagesRes.data });
        const latestMessage = messagesRes.data[0];
        assistantResponse = (latestMessage.content && latestMessage.content[0] && latestMessage.content[0].text.value) || "";
        logger.debug('[sendMessageInThread] Final text response obtained', { assistantResponse });
      } else if (runResult.status === "requires_action") {
        // Extract function call details from run.required_action.
        logger.debug('[sendMessageInThread] Run status "requires_action" detected.');
        if (
          runResult.required_action &&
          runResult.required_action.submit_tool_outputs &&
          Array.isArray(runResult.required_action.submit_tool_outputs.tool_calls) &&
          runResult.required_action.submit_tool_outputs.tool_calls.length > 0
        ) {
          const toolCall = runResult.required_action.submit_tool_outputs.tool_calls[0];
          const functionCall = toolCall.function?.name || "";
          const functionArguments = toolCall.function?.arguments || "";
          const activeRunId = runResult.id; // Save active run id.
          const toolCallId = toolCall.id;  // Save this tool call id.
          const responseObj = {
            Message: "",
            FunctionCall: functionCall,
            FunctionArguments: functionArguments,
            FunctionReturn: "",
            ActiveRunId: activeRunId,
            ToolCallId: toolCallId
          };
          assistantResponse = JSON.stringify(responseObj);
          newStatus = "AwaitingFunctionReturn";
          logger.debug('[sendMessageInThread] Extracted function call details from required_action.', { responseObj });
        } else {
          logger.warn('[sendMessageInThread] requires_action but no tool_calls found.');
          assistantResponse = JSON.stringify({
            Message: "",
            FunctionCall: "unknown_function",
            FunctionArguments: "{}",
            FunctionReturn: "",
            ActiveRunId: runResult.id,
            ToolCallId: ""
          });
          newStatus = "AwaitingFunctionReturn";
        }
      } else {
        logger.error('[sendMessageInThread] Unexpected run status', { runStatus: runResult.status });
        assistantResponse = runResult.status || "unknown error";
        newStatus = "Error";
      }
      
      // 5. Update the placeholder message with the new status and content.
      await aiAssistantModel.updateMessageStatus(ThreadId, MessageId, newStatus, assistantResponse);
      logger.info('[sendMessageInThread] Updated placeholder message with new status', { MessageId, newStatus });
      
    } catch (err) {
      logger.error('[sendMessageInThread] Error during OpenAI processing', { error: err, ThreadId, MessageId });
      await aiAssistantModel.updateMessageStatus(ThreadId, MessageId, "Error", err.message || "unknown error");
    }
  } catch (err) {
    logger.error('[sendMessageInThread] General error', { error: err });
    return res.status(500).json({ Status: "Error", Error: "Failed to send message in thread" });
  }
}

/* ======================================================================
   checkMessageStatus
   POST /AI/Assistant/MsgStatus/:MessageId
   Returns: Structured JSON if message content is JSON, otherwise plain text.
===================================================================== */
async function checkMessageStatus(req, res) {
  try {
    const { MessageId } = req.params;
    logger.info('[checkMessageStatus] Request received', { MessageId });
    if (!MessageId) {
      logger.warn('[checkMessageStatus] Missing MessageId.');
      return res.status(400).json({ Status: "Error", Error: "MessageId is required" });
    }
    const result = await aiAssistantModel.getMessageById(MessageId);
    logger.debug('[checkMessageStatus] Retrieved message from DB', { result });
    if (!result || !result.message) {
      logger.info('[checkMessageStatus] Message not found', { MessageId });
      return res.status(200).json({ Status: "Empty", Message: "" });
    }
    const { message } = result;
    let responsePayload = {
      Status: message.status,
      Message: "",
      FunctionCall: "",
      FunctionArguments: ""
    };
    try {
      logger.debug('[checkMessageStatus] Attempting to parse message.content as JSON', { content: message.content });
      const parsedContent = JSON.parse(message.content);
      if (parsedContent && typeof parsedContent === "object" && parsedContent.FunctionCall !== undefined) {
        responsePayload = {
          Status: message.status,
          Message: parsedContent.Message || "",
          FunctionCall: parsedContent.FunctionCall || "",
          FunctionArguments: parsedContent.FunctionArguments || ""
        };
        logger.debug('[checkMessageStatus] Parsed structured content successfully', { responsePayload });
      } else {
        responsePayload.Message = message.content;
        logger.debug('[checkMessageStatus] No structured content found; using plain text.');
      }
    } catch (e) {
      logger.debug('[checkMessageStatus] Failed to parse JSON; returning plain text', { content: message.content });
      responsePayload.Message = message.content;
    }
    
    // Optionally, you can add logic here to monitor the ActiveRunId if needed.
    return res.status(200).json(responsePayload);
  } catch (err) {
    logger.error('[checkMessageStatus] Error occurred', { error: err });
    return res.status(500).json({ Status: "Error", Error: "Failed to check message status" });
  }
}

/* ======================================================================
   handleFunctionReturn
   POST /AI/Assistant/FunctionReturn/:MessageId
   Body: { "FunctionReturn": "<string or JSON>" }
   Workflow:
    1. Retrieve the pending placeholder message (status "AwaitingFunctionReturn").
    2. Parse its JSON and update the FunctionReturn field.
    3. Use submitToolOutputsAndPoll to feed the function output back into the active run.
    4. Wait for the run to complete and then retrieve the final assistant message.
    5. Return the final assistant message's MessageId.
===================================================================== */
async function handleFunctionReturn(req, res) {
  try {
    const { MessageId } = req.params;
    const { FunctionReturn } = req.body;
    logger.info('[handleFunctionReturn] Function return received', { MessageId });
    if (!MessageId) {
      logger.warn('[handleFunctionReturn] Missing MessageId.');
      return res.status(400).json({ Status: "Error", Error: "MessageId is required" });
    }
    if (FunctionReturn === undefined || FunctionReturn === null) {
      logger.warn('[handleFunctionReturn] Missing FunctionReturn.');
      return res.status(400).json({ Status: "Error", Error: "FunctionReturn is required" });
    }
    
    // 1. Retrieve the original placeholder message.
    const result = await aiAssistantModel.getMessageById(MessageId);
    logger.debug('[handleFunctionReturn] Retrieved original message from DB', { result });
    if (!result || !result.message) {
      logger.warn(`[handleFunctionReturn] Message ${MessageId} not found.`);
      return res.status(404).json({ Status: "Error", Error: "Message not found" });
    }
    const { ThreadId, message } = result;
    
    // 2. Parse the stored JSON.
    let contentObj = {};
    try {
      logger.debug('[handleFunctionReturn] Parsing stored content', { content: message.content });
      contentObj = JSON.parse(message.content);
    } catch (e) {
      logger.debug('[handleFunctionReturn] Failed to parse stored content; using default structure', { error: e.message });
      contentObj = { Message: "", FunctionCall: "", FunctionArguments: "", FunctionReturn: "", ActiveRunId: "", ToolCallId: "" };
    }
    
    // 3. Update the FunctionReturn field.
    contentObj.FunctionReturn = FunctionReturn;
    const updatedContent = JSON.stringify(contentObj);
    logger.debug('[handleFunctionReturn] Updated content prepared', { updatedContent });
    
    // 4. Update the original placeholder message status to "FunctionComplete".
    const updateSuccess = await aiAssistantModel.updateMessageStatus(ThreadId, MessageId, "FunctionComplete", updatedContent);
    if (!updateSuccess) {
      logger.warn(`[handleFunctionReturn] Failed to update message ${MessageId}`);
      return res.status(500).json({ Status: "Error", Error: "Failed to update message with function return" });
    }
    logger.info(`[handleFunctionReturn] Message ${MessageId} updated with function return.`);
    
    // 5. Create a new assistant placeholder message for the final answer.
    const newMessageId = await aiAssistantModel.addMessageToThread(ThreadId, "assistant", "", "Pending");
    logger.debug('[handleFunctionReturn] New placeholder for final answer created', { newMessageId });
    
    // 6. Asynchronously, submit the tool outputs into the active run using submitToolOutputsAndPoll.
    // We'll use the stored ActiveRunId and ToolCallId from contentObj.
    if (!contentObj.ActiveRunId || !contentObj.ToolCallId) {
      logger.error('[handleFunctionReturn] ActiveRunId or ToolCallId missing in stored content.');
    } else {
      const toolOutputs = [{
        tool_call_id: contentObj.ToolCallId,
        output: FunctionReturn
      }];
      logger.debug('[handleFunctionReturn] Submitting tool outputs', { toolOutputs });
      // Submit function outputs to resume the run.
      openai.beta.threads.runs.submitToolOutputsAndPoll(ThreadId, contentObj.ActiveRunId, { tool_outputs: toolOutputs })
        .then((runAfter) => {
          logger.debug('[handleFunctionReturn] Run resumed after tool output submission', { runAfter });
          if (runAfter.status === "completed") {
            openai.beta.threads.messages.list(ThreadId)
              .then((messagesRes) => {
                logger.debug('[handleFunctionReturn] Retrieved messages after run completion', { messages: messagesRes.data });
                const finalAssistantMsg = messagesRes.data.find(msg => msg.role === "assistant" && msg.messageId !== message.messageId);
                if (finalAssistantMsg && finalAssistantMsg.content && finalAssistantMsg.content[0] && finalAssistantMsg.content[0].text) {
                  const finalText = finalAssistantMsg.content[0].text.value;
                  aiAssistantModel.updateMessageStatus(ThreadId, newMessageId, "Success", finalText)
                    .then(() => {
                      logger.info('[handleFunctionReturn] Updated new placeholder with final assistant output.');
                    })
                    .catch((updateErr) => {
                      logger.error('[handleFunctionReturn] Error updating new placeholder message.', { updateErr });
                    });
                } else {
                  logger.error('[handleFunctionReturn] Final assistant message not found.');
                }
              })
              .catch((listErr) => {
                logger.error('[handleFunctionReturn] Error retrieving messages after re-run.', { listErr });
              });
          } else {
            logger.error('[handleFunctionReturn] Run did not complete after submitting tool outputs.', { runStatus: runAfter.status });
          }
        })
        .catch((submitErr) => {
          logger.error('[handleFunctionReturn] Error submitting tool outputs.', { submitErr });
        });
    }
    
    // 7. Immediately return the new placeholder MessageId so that the client can poll.
    return res.status(200).json({ Status: "Pending", MessageId: newMessageId });
  } catch (err) {
    logger.error('[handleFunctionReturn] Error occurred', { error: err });
    return res.status(500).json({ Status: "Error", Error: err.message });
  }
}

/**
 * POST /AI/Assistant/Summarize/:ThreadId
 * Initiates summary generation for a thread.
 */
async function runSummarizeChat(req, res) {
  try {
    const { ThreadId } = req.params;
    logger.info(`[runSummarizeChat] Request received`, { ThreadId });
    if (!ThreadId) {
      logger.warn('[runSummarizeChat] Missing ThreadId');
      return res.status(400).json({ Status: "Error", SummaryId: "", Summary: "", Error: "ThreadId is required" });
    }
    const thread = await aiAssistantModel.getThread(ThreadId);
    if (!thread) {
      logger.warn(`[runSummarizeChat] Thread ${ThreadId} not found`);
      return res.status(200).json({ Status: "NotFound", SummaryId: "", Summary: "", Error: "Thread not found" });
    }
    if (thread.Summary !== undefined && thread.Summary !== "") {
      let timeDif = thread.lastUpdated - thread.summaryUpdated;
      if (timeDif <= 1) {
        logger.info(`[runSummarizeChat] Existing summary is up-to-date for ThreadId=${ThreadId}`);
        return res.status(200).json({ Status: "Success", SummaryId: "", Summary: thread.Summary, Error: "" });
      }
    }
    
    const summaryRecord = await createChatSummary(ThreadId);
    const { SummaryId } = summaryRecord;
    logger.info(`[runSummarizeChat] Created summary record ${SummaryId} for ThreadId=${ThreadId}`);
    res.status(200).json({ Status: "Pending", SummaryId, Summary: "", Error: "" });
    
    // Asynchronous summary processing.
    (async () => {
      try {
        logger.debug(`[runSummarizeChat] Starting summary generation for SummaryId=${SummaryId}`);
        let conversationText = thread.SystemMessage ? thread.SystemMessage + "\n" : "";
        thread.messages.forEach(msg => {
          conversationText += `${msg.role}: ${msg.content}\n`;
        });
        logger.debug(`[runSummarizeChat] Built conversation text: ${conversationText.substring(0,100)}...`);
        const summaryResponse = await openai.responses.create({
          model: 'o3-mini',
          reasoning: { effort: "medium" },
          instructions: 'You are tasked with summarizing a conversation between an NPC (an AI in DayZ Standalone) and a player to create a concise, historically accurate record for internal memory management. This summary will replace storing the full conversation, so it must capture essential details while preserving the unique tone and immersion of the interaction. Follow these guidelines: ...',
          input: `Summarize the following conversation:\n\n"${conversationText}"`
        });
        const summaryText = (summaryResponse.output_text || '').trim();
        logger.info(`[runSummarizeChat] Summary generated: ${summaryText.substring(0,50)}...`, { SummaryId });
        await updateChatSummaryStatus(SummaryId, "Success", summaryText);
        logger.info(`[runSummarizeChat] Updated summary record ${SummaryId} with Success status`);
        await aiAssistantModel.saveChatSummary(ThreadId, summaryText);
        logger.info(`[runSummarizeChat] Saved summary for ThreadId=${ThreadId}`);
      } catch (err) {
        logger.error(`[runSummarizeChat] Error during summary generation: ${err.message}`, { error: err, ThreadId, SummaryId });
        await updateChatSummaryStatus(SummaryId, "Error", err.message);
      }
    })();
  } catch (err) {
    logger.error(`[runSummarizeChat] Error: ${err.message}`, { error: err });
    return res.status(500).json({ Status: "Error", Error: "Failed to initiate summary generation", Summary: "" });
  }
}

async function getSummaryStatus(req, res) {
  try {
    const { SummaryId } = req.params;
    logger.debug(`[getSummaryStatus] Received request for SummaryId=${SummaryId}`);
    if (!SummaryId) {
      logger.warn('[getSummaryStatus] Missing SummaryId');
      return res.status(400).json({ Status: "Error", Error: "SummaryId is required", Summary: "" });
    }
    const summaryRecord = await getSummaryById(SummaryId);
    if (!summaryRecord || !summaryRecord.Status) {
      logger.warn(`[getSummaryStatus] Summary record not found for SummaryId=${SummaryId}`);
      return res.status(404).json({ Status: "NotFound", Error: "Summary not found", Summary: "" });
    }
    const { Status, Summary } = summaryRecord;
    logger.info(`[getSummaryStatus] Returning summary status for SummaryId=${SummaryId}: ${Status}`);
    if (Status === "Success" || Status === "Error") {
      return res.status(200).json({ Status, Summary, Error: "" });
    }
    return res.status(200).json({ Status });
  } catch (err) {
    logger.error(`[getSummaryStatus] Error: ${err.message}`, { error: err });
    return res.status(500).json({ Status: "Error", Error: "Failed to retrieve summary status", Summary: "" });
  }
}


/**
 * Simple sleep helper.
 * @param {number} ms - Milliseconds to wait.
 */
function sleep(ms) {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

/* ------------------- End of Updated Endpoints --------------------- */

module.exports = router;