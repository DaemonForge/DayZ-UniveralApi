/**
 * File: UFAIChatEndpoint.c
 * Description: The class for interacting with the AI Chat endpoints
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
	 * @return Call ID or -1 on error
	 */
	int Create(string systemMessage, string responseFormat, string jsonSchema = "", string model = "", int maxHistory = -1, UFCallbackBase cb = NULL) {
		if (systemMessage == "" || responseFormat == "") {
			Error2("[UF] AI Chat Create", "systemMessage and responseFormat must be valid strings");
			return -1;
		}
		
		int cid = -1;
		autoptr UAIChatCreateRequest req = new UAIChatCreateRequest(systemMessage, responseFormat, jsonSchema, model, maxHistory);
		
		if (cb) {
			Post("Create", req.ToJson(), U().RegisterCall(new UNestedCallBack(cb), cid));
		} else {
			Post("Create", req.ToJson(), U().RegisterCall(new USilentCallBack(), cid));
		}
		
		if (cid == -1) {
			Error2("[UF] AI Chat Create", "Error Registering Callback");
		}
		return cid;
	}
	
	/**
	 * Sends a message to an existing chat
	 * @param chatId - The ID of the chat to send to
	 * @param message - The message content to send
	 * @param context - Optional context information
	 * @param cb - Callback for response handling (required)
	 * @return Call ID or -1 on error
	 */
	int Send(string chatId, string message, autoptr array<autoptr UAIChatContext> context = NULL, UFCallbackBase cb) {
		if (chatId == "" || message == "") {
			Error2("[UF] AI Chat Send", "chatId and message must be valid strings");
			return -1;
		}
		
		if (!cb) {
			Error2("[UF] AI Chat Send", "Callback is required");
			return -1;
		}
		
		int cid = -1;
		autoptr UAIChatMessage req = new UAIChatMessage(message, context);
		
		Post("Send/" + chatId, req.ToJson(), U().RegisterCall(new UNestedCallBack(cb), cid));
		
		if (cid == -1) {
			Error2("[UF] AI Chat Send", "Error Registering Callback");
		}
		return cid;
	}
	
	/**
	 * Checks the status of a message
	 * @param messageId - The ID of the message to check
	 * @param cb - Callback for response handling
	 * @return Call ID or -1 on error
	 */
	int MessageStatus(string messageId, UFCallbackBase cb) {
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
}

/**
 * Request object for creating a new AI chat
 */
class UAIChatCreateRequest extends UFObject_Base {
	string SystemMessage;
	string ResponseFormat;
	string JsonSchema;
	string Model;
	int MaxHistory;
	
	void UAIChatCreateRequest(string systemMessage, string responseFormat, string jsonSchema = "", string model = "", int maxHistory = -1) {
		SystemMessage = systemMessage;
		ResponseFormat = responseFormat;
		JsonSchema = jsonSchema;
		Model = model;
		MaxHistory = maxHistory;
	}
	
	override string ToJson() {
		string jsonString = JsonFileLoader<UAIChatCreateRequest>.JsonMakeData(this);
		return jsonString;
	}
}

/**
 * Context information for AI chat
 */
class UAIChatContext extends Managed {
	string Description;
	ref array<string> Context;
	
	void UAIChatContext(string description) {
		Description = description;
		Context = new array<string>;
	}
	
	void AddContext(string contextItem) {
		if (!Context) {
			Context = new array<string>;
		}
		Context.Insert(contextItem);
	}
}

/**
 * Message to send to AI chat
 */
class UAIChatMessage extends UFObject_Base {
	string Message;
	ref array<ref UAIChatContext> Context;
	
	void UAIChatMessage(string message, ref array<ref UAIChatContext> context = NULL) {
		Message = message;
		if (context) {
			Context = context;
		}
	}
	
	override string ToJson() {
		string jsonString = JsonFileLoader<UAIChatMessage>.JsonMakeData(this);
		return jsonString;
	}
}
