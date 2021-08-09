class UApiDiscordChannelFilter extends UApiObject_Base {
	
	int Limit = -1;
	string Before = "";
	string After = "";
	
	void UApiDiscordChannelFilter(int limit = -1, string before = "", string after = ""){
		Limit = limit;
		Before = before;
		After = after;
	}
	
	override string ToJson(){
		string jsonString = JsonFileLoader<UApiDiscordChannelFilter>.JsonMakeData(this);
		return jsonString;
	}
	
}



class UApiDiscordMessagesResponse extends StatusObject {
	
	autoptr array<autoptr UApiDiscordMessage> Messages;
	
	
	array<autoptr UApiDiscordMessage> GetMessages(){
		return Messages;
	}
}