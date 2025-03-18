/**
 * UDiscordChannelFilter class
 *
 * Represents filter criteria for retrieving Discord channel messages.
 *
 * Fields:
 *  - Limit: (int) Specifies the maximum number of messages to retrieve. Default value is -1, indicating no limit.
 *  - Before: (string) Specifies the message identifier to fetch messages sent before. Default is an empty string.
 *  - After: (string) Specifies the message identifier to fetch messages sent after. Default is an empty string.
 *
 * Constructor:
 *  - UDiscordChannelFilter(int limit = -1, string before = "", string after = "")
 *      Initializes the filter with specified parameters.
 *
 * Methods:
 *  - ToJson()
 *      Generates a JSON representation of the UDiscordChannelFilter instance.
 */


/**
 * UDiscordMessagesResponse class
 *
 * Represents a response object that holds an array of Discord messages along with a status.
 *
 * Fields:
 *  - Messages: (array<autoptr UDiscordMessage>) Array containing the Discord message objects.
 *
 * Methods:
 *  - GetMessages()
 *      Returns the array of Discord messages contained in the response.
 */
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