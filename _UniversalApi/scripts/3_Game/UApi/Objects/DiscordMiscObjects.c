class UApiDiscordRoleReq extends UApiObject_Base{
	string Role;
	void UApiDiscordRoleReq(string role){
		Role = role;
	}
	
	override string ToJson(){
		return UApiJSONHandler<UApiDiscordRoleReq>.ToString(this);
	}
}

class UApiDiscordStatusObject extends StatusObject {
	
	string oid;

}


class UApiDiscordMute extends UApiObject_Base{
	
	bool State = true;
	
	void UApiDiscordMute(bool state){
		State = state;
	}
	
	override string ToJson(){
		string jsonString = UApiJSONHandler<UApiDiscordMute>.ToString(this);
		return jsonString;
	}
}

class UApiDiscordNickname extends UApiObject_Base{
	
	string Nickname = "";
	
	void UApiDiscordNickname(string nickname){
		Nickname = nickname;
	}
	
	override string ToJson(){
		string jsonString = UApiJSONHandler<UApiDiscordNickname>.ToString(this);
		return jsonString;
	}
}