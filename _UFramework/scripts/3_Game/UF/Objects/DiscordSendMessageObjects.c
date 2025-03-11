class UDiscordBasicMessage extends UFObject_Base {
	
	string Message= "";
	
	void UDiscordBasicMessage(string message){
		Message = message;
	}
	
	override string ToJson(){
		string jsonString = UJSONHandler<UDiscordBasicMessage>.ToString(this);
		return jsonString;
	}
	
}

class UDiscordObject extends UFObject_Base {
	string username = "";
	string avatar_url = "";
	string content = "";
	autoptr array<autoptr UDiscordEmbed> embeds = new array<autoptr UDiscordEmbed>;
	
	override string ToJson(){
		string jsonString = UJSONHandler<UDiscordObject>.ToString(this);
		return jsonString;
	}
}

class UDiscordMessage extends UFObject_Base {
	string id; //Message Id
	string AuthorId; // Discord ID of the player
	string AuthorGUID; // if player has discord connected in database this will be there GUID
	string Content; // Text content of the message
	string ChannelId; //Channel id for the message
	string RepliedTo; //Message id of the message if this message is a reply
	autoptr UDiscordEmbed Embed; //If the message has an Embed this will be the embed object
	int TimeStamp; //Time stamp of the message
	
	override string ToJson(){
		string jsonString = UJSONHandler<UDiscordMessage>.ToString(this);
		return jsonString;
	}
}


class UDiscordEmbed extends UFObject_Base{
	 
	autoptr UDiscordAuthor author;
	string title = "";
	string url = "";
	string description = "";
	int color = 0;
	autoptr array<autoptr UDiscordField> embeds = new array<autoptr UDiscordField>;
	autoptr UDiscordImage thumbnail; 
	autoptr UDiscordImage image; 
	autoptr UDiscordFooter footer; 
	
	override string ToJson(){
		string jsonString = UJSONHandler<UDiscordEmbed>.ToString(this);
		return jsonString;
	}
}

class UDiscordAuthor extends UFObject_Base {
	string name = "";
    string url = "";
    string icon_url= "";
	
	override string ToJson(){
		string jsonString = UJSONHandler<UDiscordAuthor>.ToString(this);
		return jsonString;
	}
}

class UDiscordField extends UFObject_Base {
	string name = "";
	string value = "";
	bool inline = false;
	
	override string ToJson(){
		string jsonString = UJSONHandler<UDiscordField>.ToString(this);
		return jsonString;
	}
}

class UDiscordImage extends UFObject_Base {
	
	string url = "";
	int height;
	int width;
	
	void UDiscordImage(string value){
		url = value;
	}
	
	override string ToJson(){
		string jsonString = UJSONHandler<UDiscordImage>.ToString(this);
		return jsonString;
	}
}

class UDiscordFooter extends UFObject_Base{
	
	string text = "";
	string icon_url = "";
	
	void UDiscordFooter(string txt, string url){
		text = txt;
		icon_url = url;
	}
	
	override string ToJson(){
		string jsonString = UJSONHandler<UDiscordFooter>.ToString(this);
		return jsonString;
	}
}
