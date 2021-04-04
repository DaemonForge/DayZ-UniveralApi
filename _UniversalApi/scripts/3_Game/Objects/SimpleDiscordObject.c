class UApiDiscordObject_Base extends Managed{

	string ToJson(){
		return "{}";
	}
	
}

class UApiDiscordObject extends UApiDiscordObject_Base {
	string username = "";
	string avatar_url = "";
	string content = "";
	ref array<ref UApiDiscordEmbed> embeds = new array<ref UApiDiscordEmbed>;
	
	override string ToJson(){
		string jsonString = JsonFileLoader<UApiDiscordObject>.JsonMakeData(this);
		return jsonString;
	}
}

class UApiDiscordMessage extends UApiDiscordObject_Base {
	string id;
	string AuthorId;
	string Content;
	string ChannelId;
	ref UApiDiscordEmbed Embed;
	int TimeStamp;
	
	override string ToJson(){
		string jsonString = JsonFileLoader<UApiDiscordMessage>.JsonMakeData(this);
		return jsonString;
	}
}

class UApiDiscordBasicMessage extends UApiDiscordObject_Base {
	
	string Message= "";
	
	void UApiDiscordBasicMessage(string message){
		Message = message;
	}
	
	override string ToJson(){
		string jsonString = JsonFileLoader<UApiDiscordBasicMessage>.JsonMakeData(this);
		return jsonString;
	}
	
}

class UApiDiscordEmbed extends UApiDiscordObject_Base{
	 
	ref UApiDiscordAuthor author;
	string title = "";
	string url = "";
	string description = "";
	int color = 0;
	ref array<ref UApiDiscordField> embeds = new array<ref UApiDiscordField>;
	ref UApiDiscordImage thumbnail; 
	ref UApiDiscordImage image; 
	ref UApiDiscordFooter footer; 
	
	override string ToJson(){
		string jsonString = JsonFileLoader<UApiDiscordEmbed>.JsonMakeData(this);
		return jsonString;
	}
}

class UApiDiscordAuthor extends UApiDiscordObject_Base {
	string name = "";
    string url = "";
    string icon_url= "";
	
	override string ToJson(){
		string jsonString = JsonFileLoader<UApiDiscordAuthor>.JsonMakeData(this);
		return jsonString;
	}
}

class UApiDiscordField extends UApiDiscordObject_Base {
	string name = "";
	string value = "";
	bool inline = false;
	
	override string ToJson(){
		string jsonString = JsonFileLoader<UApiDiscordField>.JsonMakeData(this);
		return jsonString;
	}
}

class UApiDiscordImage extends UApiDiscordObject_Base {
	
	string url = "";
	int height;
	int width;
	
	void UApiDiscordImage(string value){
		url = value;
	}
	
	override string ToJson(){
		string jsonString = JsonFileLoader<UApiDiscordImage>.JsonMakeData(this);
		return jsonString;
	}
}

class UApiDiscordFooter extends UApiDiscordObject_Base{
	
	string text = "";
	string icon_url = "";
	
	void UApiDiscordFooter(string txt, string url){
		text = txt;
		icon_url = url;
	}
	
	override string ToJson(){
		string jsonString = JsonFileLoader<UApiDiscordFooter>.JsonMakeData(this);
		return jsonString;
	}
}
