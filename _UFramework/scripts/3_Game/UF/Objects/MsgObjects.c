/**
 * Queue metadata configuration
 * Used to set queue behavior: ordering and player write permissions
 */
class UQueueMeta extends UFObject_Base {
	
	string Order = UF_QUEUE_FIFO;
	int AllowPlayerWrites = 0;
	
	void UQueueMeta(string order = UF_QUEUE_FIFO, bool allowPlayerWrites = false){
		Order = order;
		if (allowPlayerWrites){
			AllowPlayerWrites = 1;
		} else {
			AllowPlayerWrites = 0;
		}
	}	
	
	override string ToJson(){
		return JsonFileLoader<UQueueMeta>.JsonMakeData(this);
	}
}

/**
 * Request object for reading messages with a limit
 */
class UMsgReadObj extends UFObject_Base {
	int Limit = -1;
	
	void UMsgReadObj(int limit = -1){
		Limit = limit;
	}
	
	override string ToJson(){
		return JsonFileLoader<UMsgReadObj>.JsonMakeData(this);
	}
}

/**
 * Simple string message wrapper
 */
class UStringMessage extends UMessageBase {
	
	string Message = "";
	
	void UStringMessage(string message = ""){
		Message = message;
	}	
	
	override string ToJson(){
		return JsonFileLoader<UStringMessage>.JsonMakeData(this);
	}
}

/**
 * Typed message wrapper for sending structured data
 * @tparam T The type of the message content
 */
class UMessage<Class T> extends UMessageBase {
	
	autoptr T Message;
	
	void UMessage(T message = NULL){
		Message = message;
	}	
	
	void ~UMessage(){
		Message = NULL;
	}
	
	override string ToJson(){
		// Serialize the wrapper which contains the Message field
		return JsonFileLoader<UMessage<T>>.JsonMakeData(this);
	}
}

/**
 * Base class for all message types
 */
class UMessageBase extends UFObject_Base {
	
}


// ============================================================================
// Message Response Objects
// ============================================================================

/**
 * Base class for message read responses
 */
class UReadMsgBase extends StatusObject {
}

/**
 * Typed message array response
 * @tparam T The type of messages in the array
 */
class UReadMsg<Class T> extends UReadMsgBase {
	autoptr array<autoptr T> Messages;

	void UReadMsg(){
		Messages = new array<autoptr T>();
	}
	
	void ~UReadMsg(){
		if (Messages){
			Messages.Clear();
		}
	}

	array<autoptr T> GetMessages(){
		return Messages;
	}
	
	/**
	 * Check if there are any messages
	 */
	bool HasMessages(){
		return Messages && Messages.Count() > 0;
	}
	
	/**
	 * Get the count of messages
	 */
	int Count(){
		if (!Messages) return 0;
		return Messages.Count();
	}
}

/**
 * String message array response
 */
class UReadMsgString extends UReadMsgBase {
	autoptr TStringArray Messages;
	
	void UReadMsgString(){
		Messages = new TStringArray();
	}
	
	void ~UReadMsgString(){
		if (Messages){
			Messages.Clear();
		}
	}
	
	TStringArray GetMessages(){
		return Messages;
	}
	
	/**
	 * Check if there are any messages
	 */
	bool HasMessages(){
		return Messages && Messages.Count() > 0;
	}
	
	/**
	 * Get the count of messages
	 */
	int Count(){
		if (!Messages) return 0;
		return Messages.Count();
	}
}

