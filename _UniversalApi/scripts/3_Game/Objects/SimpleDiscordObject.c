class UApiDiscordObject{
	string username = "";
	string avatar_url = "";
	string content = "";
	ref array<ref UApiDiscordEmbed> embeds = new ref array<ref UApiDiscordEmbed>;
	
	string ToJson(){
		string jsonString = JsonFileLoader<UApiDiscordObject>.JsonMakeData(this);
		return jsonString;
	}
}

class UApiDiscordEmbed{
	 
	ref UApiDiscordAuthor author;
	string title = "";
	string url = "";
	string description = "";
	int color = 0;
	ref array<ref UApiDiscordField> embeds = new ref array<ref UApiDiscordField>;
	ref UApiDiscordImage thumbnail; 
	ref UApiDiscordImage image; 
	ref UApiDiscordFooter footer; 
	
}

class UApiDiscordAuthor{
	string name = "";
    string url = "";
    string icon_url= "";
}

class UApiDiscordField{
	string name = "";
	string value = "";
	bool inline = false;
}

class UApiDiscordImage{
	
	string url = "";
	int height;
	int width;
	
	void UApiDiscordImage(string value){
		url = value;
	}
}

class UApiDiscordFooter{
	
	string text = "";
	string icon_url = "";
	
	void UApiDiscordImage(string txt, string url){
		text = txt;
		icon_url = url;
	}
}
