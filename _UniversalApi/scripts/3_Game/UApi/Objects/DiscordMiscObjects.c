class UApiDiscordRoleReq extends UApiObject_Base{
	string Role;
	void UApiDiscordRoleReq(string role){
		Role = role;
	}
	
	override string ToJson(){
		return JsonFileLoader<UApiDiscordRoleReq>.JsonMakeData(this);
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
		string jsonString = JsonFileLoader<UApiDiscordMute>.JsonMakeData(this);
		return jsonString;
	}
}