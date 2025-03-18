/**
 * Class UDiscordRoleReq
 * -----------------------
 * Represents a request containing a Discord role.
 *
 * Fields:
 *   - string Role:
 *       Holds the role string to be processed.
 *
 * Constructor:
 *   - UDiscordRoleReq(string role):
 *       Initializes a new instance of UDiscordRoleReq with the provided role.
 *
 * Methods:
 *   - override string ToJson():
 *       Serializes the current object to a JSON string using UJSONHandler.
 */


/**
 * Class UDiscordStatusObject
 * ----------------------------
 * Represents a status object specific to Discord.
 *
 * Inheritance:
 *   - Inherits from StatusObject.
 *
 * Fields:
 *   - string oid:
 *       Holds the unique identifier associated with the Discord object.
 */


/**
 * Class UDiscordMute
 * ------------------
 * Represents a mute state for Discord interactions.
 *
 * Inheritance:
 *   - Inherits from UFObject_Base.
 *
 * Fields:
 *   - bool State (default true):
 *       Indicates whether a mute operation is enabled (true) or disabled (false).
 *
 * Constructor:
 *   - UDiscordMute(bool state):
 *       Initializes a new instance of UDiscordMute with the specified mute state.
 *
 * Methods:
 *   - override string ToJson():
 *       Serializes the current mute state object to a JSON string using UJSONHandler.
 */


/**
 * Class UDiscordNickname
 * ----------------------
 * Represents a nickname change or assignment for Discord.
 *
 * Inheritance:
 *   - Inherits from UFObject_Base.
 *
 * Fields:
 *   - string Nickname:
 *       Holds the nickname value. Initialized as an empty string.
 *
 * Constructor:
 *   - UDiscordNickname(string nickname):
 *       Initializes a new instance of UDiscordNickname with the provided nickname.
 *
 * Methods:
 *   - override string ToJson():
 *       Serializes the current nickname object to a JSON string using UJSONHandler.
 */
class UDiscordRoleReq extends UFObject_Base{
	string Role;
	void UDiscordRoleReq(string role){
		Role = role;
	}
	
	override string ToJson(){
		return UJSONHandler<UDiscordRoleReq>.ToString(this);
	}
}

class UDiscordStatusObject extends StatusObject {
	
	string oid;

}


class UDiscordMute extends UFObject_Base{
	
	bool State = true;
	
	void UDiscordMute(bool state){
		State = state;
	}
	
	override string ToJson(){
		string jsonString = UJSONHandler<UDiscordMute>.ToString(this);
		return jsonString;
	}
}

class UDiscordNickname extends UFObject_Base{
	
	string Nickname = "";
	
	void UDiscordNickname(string nickname){
		Nickname = nickname;
	}
	
	override string ToJson(){
		string jsonString = UJSONHandler<UDiscordNickname>.ToString(this);
		return jsonString;
	}
}