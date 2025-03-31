class UQueueMeta extends UFObject_Base {
	
	string Order = UF_QUEUE_FIFO;
	bool AllowPlayerWrites = false;
	
	void UQueueMeta(string order, bool allowPlayerWrites = false){
		Order = order;
		AllowPlayerWrites = allowPlayerWrites;
	}	
	
	override string ToJson(){
		return JsonFileLoader<UQueueMeta>.JsonMakeData(this);
	}
}

class UMsgReadObj extends UFObject_Base {
	int Limit = -1;
	
	void UMsgReadObj(int limit){
		Limit = limit;
	}
	
	override string ToJson(){
		return JsonFileLoader<UMsgReadObj>.JsonMakeData(this);
	}
}

class UStringMessage extends UMessageBase {
	
	string Message = "";
	
	void UStringMessage(string message){
		Message = message;
	}	
	
	override string ToJson(){
		return JsonFileLoader<UStringMessage>.JsonMakeData(this);
	}
}


class UMessage<Class T> extends UMessageBase {
	
	T Message;
	
	void UMessage(T message){
		Message = message;
	}	
	
	override string ToJson(){
		return JsonFileLoader<T>.JsonMakeData(this);
	}
}

class UMessageBase extends UFObject_Base {
	

}


//MessageReturn Objects
class UReadMsgBase extends StatusObject {
}


class UReadMsg<Class T> extends UReadMsgBase {
	autoptr array<T> Messages;

	array<T> GetMessages(){
		return Messages;
	}
}


class UReadMsgString extends UReadMsgBase {
	autoptr TStringArray Messages;
	
	TStringArray GetMessages();
}

