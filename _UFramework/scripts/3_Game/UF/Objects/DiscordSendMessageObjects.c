/**
 * UDiscordBasicMessage
 * ---------------------
 * Represents a basic message structure for a Discord message.
 * 
 * Properties:
 *   - Message: A string holding the content of the message.
 * 
 * Constructor:
 *   - UDiscordBasicMessage(string message): Initializes the message field.
 * 
 * Methods:
 *   - ToJson(): Serializes the UDiscordBasicMessage instance to a JSON string using UJSONHandler.
 */

/**
 * UDiscordObject
 * --------------
 * Represents a Discord message object that includes user information and message content.
 * 
 * Properties:
 *   - username: A string for the Discord username.
 *   - avatar_url: A string for the user's avatar URL.
 *   - content: A string for the message content.
 *   - embeds: An array of UDiscordEmbed objects that represent embedded content.
 * 
 * Methods:
 *   - ToJson(): Serializes the UDiscordObject instance to a JSON string using UJSONHandler.
 */

/**
 * UDiscordMessage
 * ---------------
 * Represents a complete Discord message with details such as author, content, channel, and embed.
 * 
 * Properties:
 *   - id: The unique identifier of the message.
 *   - AuthorId: The Discord ID of the author.
 *   - AuthorGUID: The GUID from the database if the player is linked.
 *   - Content: The text content of the message.
 *   - ChannelId: The channel identifier for the message.
 *   - RepliedTo: The message ID that this message replies to, if applicable.
 *   - Embed: A UDiscordEmbed object containing embed details (if present).
 *   - TimeStamp: An integer representing the timestamp when the message was created.
 * 
 * Methods:
 *   - ToJson(): Serializes the UDiscordMessage instance to a JSON string using UJSONHandler.
 */

/**
 * UDiscordEmbed
 * -------------
 * Represents an embed object in a Discord message which can include rich media content.
 * 
 * Properties:
 *   - author: A UDiscordAuthor object representing the author of the embed.
 *   - title: A string representing the title of the embed.
 *   - url: A string for the embed URL.
 *   - description: A string providing the embed description.
 *   - color: An integer representing the sidebar color of the embed.
 *   - embeds: An array of UDiscordField objects representing additional fields.
 *   - thumbnail: A UDiscordImage object representing the thumbnail image.
 *   - image: A UDiscordImage object representing the main image.
 *   - footer: A UDiscordFooter object representing the footer of the embed.
 * 
 * Methods:
 *   - ToJson(): Serializes the UDiscordEmbed instance to a JSON string using UJSONHandler.
 */

/**
 * UDiscordAuthor
 * --------------
 * Represents the author of a Discord embed.
 * 
 * Properties:
 *   - name: The name of the author.
 *   - url: A URL associated with the author.
 *   - icon_url: A URL to the author's icon.
 * 
 * Methods:
 *   - ToJson(): Serializes the UDiscordAuthor instance to a JSON string using UJSONHandler.
 */

/**
 * UDiscordField
 * -------------
 * Represents a field in a Discord embed.
 * 
 * Properties:
 *   - name: The field's name.
 *   - value: The field's value.
 *   - inline: A boolean flag indicating if the field should be displayed inline.
 * 
 * Methods:
 *   - ToJson(): Serializes the UDiscordField instance to a JSON string using UJSONHandler.
 */

/**
 * UDiscordImage
 * -------------
 * Represents an image within a Discord embed.
 * 
 * Properties:
 *   - url: A string containing the image URL.
 *   - height: The height of the image.
 *   - width: The width of the image.
 * 
 * Constructor:
 *   - UDiscordImage(string value): Initializes the UDiscordImage with the provided URL.
 * 
 * Methods:
 *   - ToJson(): Serializes the UDiscordImage instance to a JSON string using UJSONHandler.
 */

/**
 * UDiscordFooter
 * --------------
 * Represents the footer section of a Discord embed.
 * 
 * Properties:
 *   - text: A string containing the footer text.
 *   - icon_url: A string for the footer icon URL.
 * 
 * Constructor:
 *   - UDiscordFooter(string txt, string url): Initializes the UDiscordFooter with the provided text and icon URL.
 * 
 * Methods:
 *   - ToJson(): Serializes the UDiscordFooter instance to a JSON string using UJSONHandler.
 */
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
