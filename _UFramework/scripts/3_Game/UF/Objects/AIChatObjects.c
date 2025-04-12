/**
 * File: AIChatObjects.c
 * Description: Object classes for AI Chat functionality
 */

/**
 * Object for storing a queued message
 */
class UAIChatQueuedMessage extends Managed
{
	string message;
	autoptr array<autoptr UAIChatContext> context;
	
	void UAIChatQueuedMessage(string msg, array<autoptr UAIChatContext> ctx = NULL)
	{
		message = msg;
		
		if (ctx) {
			context = new array<autoptr UAIChatContext>;
			
			// Make a deep copy of the context to avoid reference issues
			for (int i = 0; i < ctx.Count(); i++) {
				autoptr UAIChatContext originalCtx = ctx.Get(i);
				autoptr UAIChatContext newCtx = new UAIChatContext(originalCtx.Description);
				
				if (originalCtx.Context) {
					for (int j = 0; j < originalCtx.Context.Count(); j++) {
						newCtx.AddContext(originalCtx.Context.Get(j));
					}
				}
				
				context.Insert(newCtx);
			}
		}
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
	autoptr array<string> Context;
	
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
	autoptr array<autoptr UAIChatContext> Context;
	
	void UAIChatMessage(string message, array<autoptr UAIChatContext> context = NULL) {
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

/**
 * Response object for AI chat message status
 */
class UAIChatMessageResponse extends StatusObject {
	string Message;
	string MessageId;
	
	string GetMessage() {
		return Message;
	}
	
	string GetMessageId() {
		return MessageId;
	}
}

/**
 * Single chat message in history
 */
class UAIChatHistoryMessage extends Managed {
	string MessageId;
	string role;
	string Message;
	string status;
	
	string GetMessage() {
		return Message;
	}
	
	string GetRole() {
		return role;
	}
	
	string GetStatus() {
		return status;
	}
}

/**
 * Response object for chat history
 */
class UAIChatHistoryResponse extends StatusObject {
	string ChatId;
	string SystemMessage;
	int MaxHistory;
	autoptr array<autoptr UAIChatHistoryMessage> Messages;
	
	array<autoptr UAIChatHistoryMessage> GetMessages() {
		return Messages;
	}
}


/**
 * Response object for chat history
 */
class UAIChatCreateResponse extends StatusObject {
	string ChatId;
	
}

/**
 * Response object for chat summary
 */
class UAIChatSummaryResponse extends StatusObject {
	string ChatId;
	string SummaryId;
	string Summary;
	
	string GetSummary() {
		return Summary;
	}
} 