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