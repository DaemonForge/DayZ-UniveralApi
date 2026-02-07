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
	string KBId;
	
	void UAIChatCreateRequest(string systemMessage, string responseFormat, string jsonSchema = "", string model = "", int maxHistory = -1, string kbId = "") {
		SystemMessage = systemMessage;
		ResponseFormat = responseFormat;
		JsonSchema = jsonSchema;
		Model = model;
		MaxHistory = maxHistory;
		KBId = kbId;
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
 * Message to send to AI chat (with optional tools)
 */
class UAIChatMessage extends UFObject_Base {
	string Message;
	autoptr array<autoptr UAIChatContext> Context;
	autoptr array<autoptr UAIToolDef> Tools;
	
	void UAIChatMessage(string message, array<autoptr UAIChatContext> context = NULL, array<autoptr UAIToolDef> tools = NULL) {
		Message = message;
		if (context) {
			Context = context;
		}
		if (tools) {
			Tools = tools;
		}
	}
	
	override string ToJson() {
		string jsonString = JsonFileLoader<UAIChatMessage>.JsonMakeData(this);
		return jsonString;
	}
}

/**
 * Tool definition for lightweight agentic usage.
 * Sent to the server which builds proper OpenAI function schemas.
 * 
 * ParamTypes: "string", "int", "float", "bool", "vector"
 *   - string -> JSON Schema: { type: "string" }
 *   - int -> JSON Schema: { type: "integer" }
 *   - float -> JSON Schema: { type: "number" }
 *   - bool -> JSON Schema: { type: "boolean" }
 *   - vector -> JSON Schema: { type: "string" } with "x y z" format hint
 */
class UAIToolDef extends Managed {
	string Name;
	string Description;
	autoptr array<string> Parameters;
	autoptr array<string> ParamTypes;
	autoptr array<string> ParamDescs;

	void UAIToolDef(string name, string desc, array<string> parameters = NULL, array<string> paramTypes = NULL, array<string> paramDescs = NULL){
		Name = name;
		Description = desc;
		if (parameters){
			Parameters = parameters;
		}
		if (paramTypes){
			ParamTypes = paramTypes;
		}
		if (paramDescs){
			ParamDescs = paramDescs;
		}
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
 * Response object for AI chat tool call request.
 * Returned when the AI wants to call a tool - modder executes the tool and submits result.
 */
class UAIChatToolCallResponse extends StatusObject {
	string ChatId;
	string MessageId;
	string ToolCallId;
	string ToolName;
	string P1;
	string P2;
	string P3;
	string P4;
	string P5;
	
	string GetToolName() {
		return ToolName;
	}
	
	string GetP1() { return P1; }
	string GetP2() { return P2; }
	string GetP3() { return P3; }
	string GetP4() { return P4; }
	string GetP5() { return P5; }
	
	string GetMessageId() { return MessageId; }
	string GetToolCallId() { return ToolCallId; }
}

/**
 * Request object for submitting a tool result
 */
class UAIChatToolResultRequest extends UFObject_Base {
	string ToolCallId;
	string Result;
	
	void UAIChatToolResultRequest(string toolCallId, string result) {
		ToolCallId = toolCallId;
		Result = result;
	}
	
	override string ToJson() {
		string jsonString = JsonFileLoader<UAIChatToolResultRequest>.JsonMakeData(this);
		return jsonString;
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

/**
 * UAIChatHistoryEntry - stores a single message in chat history with role.
 */
class UAIChatHistoryEntry extends Managed {
    string Role;    // "user" or "assistant"
    string Message;

    void UAIChatHistoryEntry(string role, string message){
        Role = role;
        Message = message;
    }
}
