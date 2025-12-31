/**
 * File: UFAIChatEndpoint.c
 * Description: The class for interacting with the AI Chat endpoints
 * 
 * Tool calling is now handled natively by OpenAI's function calling feature.
 * Tools are passed in the request body and OpenAI returns structured tool call responses.
 */

class UFAIChatEndpoint extends UFBaseEndpoint {

	override protected string EndpointBaseUrl(){
		return UFConfig().GetBaseURL() + "AI/Chat/";
	}
    
	/**
	 * Creates a new AI chat session
	 * @param systemMessage - Initial system prompt for the AI
	 * @param responseFormat - Format of responses ("string" or "JSON")
	 * @param jsonSchema - Optional schema for JSON responses
	 * @param model - Optional AI model to use
	 * @param maxHistory - Optional maximum history to keep
	 * @param cb - Callback for response handling
	 * @param kbId - Optional Knowledge Base ID for enhanced context retrieval
	 * @return Call ID or -1 on error
	 */
	int Create(string systemMessage, string responseFormat, string jsonSchema = "", string model = "", int maxHistory = -1, UFCallbackBase cb = NULL, string kbId = "") {
		if (!U().IsOpenAIEnabled()){
			Error2("[UF] AI Chat Create", "OpenAI service is not online");
			return -1;
		}
		if (systemMessage == "" || responseFormat == "") {
			Error2("[UF] AI Chat Create", "systemMessage and responseFormat must be valid strings");
			return -1;
		}
		
		// Debug logging for KB integration
		if (kbId != "") {
			UFLog.Debug("[AI Chat] Creating session with KB: " + kbId);
		} else {
			UFLog.Debug("[AI Chat] Creating session without KB");
		}
		
		int cid = -1;
		autoptr UAIChatCreateRequest req = new UAIChatCreateRequest(systemMessage, responseFormat, jsonSchema, model, maxHistory, kbId);
		
		UFLog.Debug("[AI Chat] Create request body: " + req.ToJson());
		
		if (cb) {
			Post("Create", req.ToJson(), U().RegisterCall(new UNestedCallBack(cb), cid));
		} else {
			Post("Create", req.ToJson(), U().RegisterCall(new USilentCallBack(), cid));
		}
		
		if (cid == -1) {
			Error2("[UF] AI Chat Create", "Error Registering Callback");
		} else {
			UFLog.Debug("[AI Chat] Create registered with CID: " + cid);
		}
		return cid;
	}
	
	/**
	 * Sends a message to an existing chat
	 * @param chatId - The ID of the chat to send to
	 * @param message - The message content to send
	 * @param cb - Callback for response handling (required)
	 * @param context - Optional context information
	 * @param tools - Optional tool definitions to expose to the AI (uses native OpenAI function calling)
	 * @return Call ID or -1 on error
	 */
	int Send(string chatId, string message, UFCallbackBase cb, array<autoptr UAIChatContext> context = NULL, array<autoptr UAIToolDef> tools = NULL) {
		if (!U().IsOpenAIEnabled()){
			Error2("[UF] AI Chat Send", "OpenAI service is not online");
			return -1;
		}
		if (chatId == "" || message == "") {
			Error2("[UF] AI Chat Send", "chatId and message must be valid strings");
			return -1;
		}
		
		if (!cb) {
			Error2("[UF] AI Chat Send", "Callback is required");
			return -1;
		}
		
		// Debug logging
		UFLog.Debug("[AI Chat] Sending message to chat: " + chatId);
		UFLog.Debug("[AI Chat] Message length: " + message.Length().ToString());
		if (context) {
			UFLog.Debug("[AI Chat] Context items: " + context.Count().ToString());
		}
		if (tools) {
			UFLog.Debug("[AI Chat] Tools count: " + tools.Count().ToString());
		}
		
		int cid = -1;
		// Pass tools directly in the message - service handles OpenAI native function calling
		autoptr UAIChatMessage req = new UAIChatMessage(message, context, tools);
		
		Post("Send/" + chatId, req.ToJson(), U().RegisterCall(new UNestedCallBack(cb), cid));
		
		if (cid == -1) {
			Error2("[UF] AI Chat Send", "Error Registering Callback");
		} else {
			UFLog.Debug("[AI Chat] Send registered with CID: " + cid);
		}
		return cid;
	}

	/**
	 * Convenience wrapper to send a message with tools without specifying context first.
	 */
	int SendWithTools(string chatId, string message, array<autoptr UAIToolDef> tools, UFCallbackBase cb, array<autoptr UAIChatContext> context = NULL) {
		return Send(chatId, message, cb, context, tools);
	}
	
	/**
	 * Checks the status of a message
	 * @param messageId - The ID of the message to check
	 * @param cb - Callback for response handling
	 * @return Call ID or -1 on error
	 */
	int MessageStatus(string messageId, UFCallbackBase cb) {
		if (!U().IsOpenAIEnabled()){
			Error2("[UF] AI Chat MessageStatus", "OpenAI service is not online");
			return -1;
		}
		if (messageId == "") {
			Error2("[UF] AI Chat MessageStatus", "messageId must be a valid string");
			return -1;
		}
		
		if (!cb) {
			Error2("[UF] AI Chat MessageStatus", "Callback is required");
			return -1;
		}
		
		int cid = -1;
		Post("MessageStatus/" + messageId, "{}", U().RegisterCall(new UNestedCallBack(cb), cid));
		
		if (cid == -1) {
			Error2("[UF] AI Chat MessageStatus", "Error Registering Callback");
		}
		return cid;
	}
	
	/**
	 * Retrieves chat history
	 * @param chatId - The ID of the chat to retrieve
	 * @param cb - Callback for response handling
	 * @return Call ID or -1 on error
	 */
	int Read(string chatId, UFCallbackBase cb) {
		if (!U().IsOpenAIEnabled()){
			Error2("[UF] AI Chat Read", "OpenAI service is not online");
			return -1;
		}
		if (chatId == "") {
			Error2("[UF] AI Chat Read", "chatId must be a valid string");
			return -1;
		}
		
		if (!cb) {
			Error2("[UF] AI Chat Read", "Callback is required");
			return -1;
		}
		
		int cid = -1;
		Post("Read/" + chatId, "{}", U().RegisterCall(new UNestedCallBack(cb), cid));
		
		if (cid == -1) {
			Error2("[UF] AI Chat Read", "Error Registering Callback");
		}
		return cid;
	}
	
	/**
	 * Resets a chat session (clears history)
	 * @param chatId - The ID of the chat to reset
	 * @param cb - Optional callback for response handling
	 * @return Call ID or -1 on error
	 */
	int Reset(string chatId, UFCallbackBase cb = NULL) {
		if (!U().IsOpenAIEnabled()){
			Error2("[UF] AI Chat Reset", "OpenAI service is not online");
			return -1;
		}
		if (chatId == "") {
			Error2("[UF] AI Chat Reset", "chatId must be a valid string");
			return -1;
		}
		
		int cid = -1;
		
		if (cb) {
			Post("Reset/" + chatId, "{}", U().RegisterCall(new UNestedCallBack(cb), cid));
		} else {
			Post("Reset/" + chatId, "{}", U().RegisterCall(new USilentCallBack(), cid));
		}
		
		if (cid == -1) {
			Error2("[UF] AI Chat Reset", "Error Registering Callback");
		}
		return cid;
	}
	
	/**
	 * Deletes a chat session
	 * @param chatId - The ID of the chat to delete
	 * @param cb - Optional callback for response handling
	 * @return Call ID or -1 on error
	 */
	int Delete(string chatId, UFCallbackBase cb = NULL) {
		if (!U().IsOpenAIEnabled()){
			Error2("[UF] AI Chat Delete", "OpenAI service is not online");
			return -1;
		}
		if (chatId == "") {
			Error2("[UF] AI Chat Delete", "chatId must be a valid string");
			return -1;
		}
		
		int cid = -1;
		
		if (cb) {
			Post("Delete/" + chatId, "{}", U().RegisterCall(new UNestedCallBack(cb), cid));
		} else {
			Post("Delete/" + chatId, "{}", U().RegisterCall(new USilentCallBack(), cid));
		}
		
		if (cid == -1) {
			Error2("[UF] AI Chat Delete", "Error Registering Callback");
		}
		return cid;
	}
	
	/**
	 * Generates a summary of a chat
	 * @param chatId - The ID of the chat to summarize
	 * @param cb - Callback for response handling
	 * @return Call ID or -1 on error
	 */
	int Summarize(string chatId, UFCallbackBase cb) {
		if (!U().IsOpenAIEnabled()){
			Error2("[UF] AI Chat Summarize", "OpenAI service is not online");
			return -1;
		}
		if (chatId == "") {
			Error2("[UF] AI Chat Summarize", "chatId must be a valid string");
			return -1;
		}
		
		if (!cb) {
			Error2("[UF] AI Chat Summarize", "Callback is required");
			return -1;
		}
		
		int cid = -1;
		Post("Summarize/" + chatId, "{}", U().RegisterCall(new UNestedCallBack(cb), cid));
		
		if (cid == -1) {
			Error2("[UF] AI Chat Summarize", "Error Registering Callback");
		}
		return cid;
	}
	
	/**
	 * Checks the status of a summary generation
	 * @param summaryId - The ID of the summary to check
	 * @param cb - Callback for response handling
	 * @return Call ID or -1 on error
	 */
	int SummaryStatus(string summaryId, UFCallbackBase cb) {
		if (!U().IsOpenAIEnabled()){
			Error2("[UF] AI Chat SummaryStatus", "OpenAI service is not online");
			return -1;
		}
		if (summaryId == "") {
			Error2("[UF] AI Chat SummaryStatus", "summaryId must be a valid string");
			return -1;
		}
		
		if (!cb) {
			Error2("[UF] AI Chat SummaryStatus", "Callback is required");
			return -1;
		}
		
		int cid = -1;
		Post("SummaryStatus/" + summaryId, "{}", U().RegisterCall(new UNestedCallBack(cb), cid));
		
		if (cid == -1) {
			Error2("[UF] AI Chat SummaryStatus", "Error Registering Callback");
		}
		return cid;
	}
	
	/**
	 * Submits a tool execution result back to the AI to continue the conversation.
	 * Use this after receiving a "ToolCall" status from MessageStatus.
	 * @param messageId - The message ID that requested the tool call
	 * @param toolCallId - The tool call ID from the ToolCall response
	 * @param result - The result of executing the tool (as a string)
	 * @param cb - Callback for response handling (will return new MessageId to poll)
	 * @return Call ID or -1 on error
	 */
	int SubmitToolResult(string messageId, string toolCallId, string result, UFCallbackBase cb) {
		if (!U().IsOpenAIEnabled()){
			Error2("[UF] AI Chat SubmitToolResult", "OpenAI service is not online");
			return -1;
		}
		if (messageId == "" || toolCallId == "") {
			Error2("[UF] AI Chat SubmitToolResult", "messageId and toolCallId must be valid strings");
			return -1;
		}
		
		if (!cb) {
			Error2("[UF] AI Chat SubmitToolResult", "Callback is required");
			return -1;
		}
		
		int cid = -1;
		autoptr UAIChatToolResultRequest req = new UAIChatToolResultRequest(toolCallId, result);
		
		Post("ToolResult/" + messageId, req.ToJson(), U().RegisterCall(new UNestedCallBack(cb), cid));
		
		if (cid == -1) {
			Error2("[UF] AI Chat SubmitToolResult", "Error Registering Callback");
		}
		return cid;
	}
}
