class UDiscordChannelFilter extends UFObject_Base {
	
	int Limit = -1;
	string Before = "";
	string After = "";
	
	void UDiscordChannelFilter(int limit = -1, string before = "", string after = ""){
		Limit = limit;
		Before = before;
		After = after;
	}
	
	override string ToJson(){
		string jsonString = JsonFileLoader<UDiscordChannelFilter>.JsonMakeData(this);
		return jsonString;
	}
	
}



class UDiscordMessagesResponse extends StatusObject {
	
	autoptr array<autoptr UDiscordMessage> Messages;
	
	
	array<autoptr UDiscordMessage> GetMessages(){
		return Messages;
	}
}