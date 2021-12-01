class UApiDiscordBasicMessage extends UApiObject_Base {
	
	string Message= "";
	
	void UApiDiscordBasicMessage(string message){
		Message = message;
	}
	
	override string ToJson(){
		string jsonString = UApiJSONHandler<UApiDiscordBasicMessage>.ToString(this);
		return jsonString;
	}
	
}

class UApiDiscordObject extends UApiObject_Base {
	string username = "";
	string avatar_url = "";
	string content = "";
	autoptr array<autoptr UApiDiscordEmbed> embeds = new array<autoptr UApiDiscordEmbed>;
	
	override string ToJson(){
		string jsonString = UApiJSONHandler<UApiDiscordObject>.ToString(this);
		return jsonString;
	}
}

class UApiDiscordMessage extends UApiObject_Base {
	string id; //Message Id
	string AuthorId; // Discord ID of the player
	string AuthorGUID; // if player has discord connected in database this will be there GUID
	string Content; // Text content of the message
	string ChannelId; //Channel id for the message
	string RepliedTo; //Message id of the message if this message is a reply
	autoptr UApiDiscordEmbed Embed; //If the message has an Embed this will be the embed object
	int TimeStamp; //Time stamp of the message
	
	override string ToJson(){
		string jsonString = UApiJSONHandler<UApiDiscordMessage>.ToString(this);
		return jsonString;
	}
}


class UApiDiscordEmbed extends UApiObject_Base{
	 
	autoptr UApiDiscordAuthor author;
	string title = "";
	string url = "";
	string description = "";
	int color = 0;
	autoptr array<autoptr UApiDiscordField> embeds = new array<autoptr UApiDiscordField>;
	autoptr UApiDiscordImage thumbnail; 
	autoptr UApiDiscordImage image; 
	autoptr UApiDiscordFooter footer; 
	
	override string ToJson(){
		string jsonString = UApiJSONHandler<UApiDiscordEmbed>.ToString(this);
		return jsonString;
	}
}

class UApiDiscordAuthor extends UApiObject_Base {
	string name = "";
    string url = "";
    string icon_url= "";
	
	override string ToJson(){
		string jsonString = UApiJSONHandler<UApiDiscordAuthor>.ToString(this);
		return jsonString;
	}
}

class UApiDiscordField extends UApiObject_Base {
	string name = "";
	string value = "";
	bool inline = false;
	
	override string ToJson(){
		string jsonString = UApiJSONHandler<UApiDiscordField>.ToString(this);
		return jsonString;
	}
}

class UApiDiscordImage extends UApiObject_Base {
	
	string url = "";
	int height;
	int width;
	
	void UApiDiscordImage(string value){
		url = value;
	}
	
	override string ToJson(){
		string jsonString = UApiJSONHandler<UApiDiscordImage>.ToString(this);
		return jsonString;
	}
}

class UApiDiscordFooter extends UApiObject_Base{
	
	string text = "";
	string icon_url = "";
	
	void UApiDiscordFooter(string txt, string url){
		text = txt;
		icon_url = url;
	}
	
	override string ToJson(){
		string jsonString = UApiJSONHandler<UApiDiscordFooter>.ToString(this);
		return jsonString;
	}
}
