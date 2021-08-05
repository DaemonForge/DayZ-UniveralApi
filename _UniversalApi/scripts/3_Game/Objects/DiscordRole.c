class UApiDiscordUser extends StatusObject{
		
	string id;
	string Username;
	
	string Discriminator;
	string Avatar;
	
	TStringArray Roles;
	
	string VoiceChannel;
	
	bool HasRole(string roleid){
		if (!Roles) return false;
		return (Roles.Find(roleid) != -1);
	}
	
}

class UApiDiscordRoleReq extends UApiObject_Base{
	string Role;
	void UApiDiscordRoleReq(string role){
		Role = role;
	}
	
	override string ToJson(){
		return JsonFileLoader<UApiDiscordRoleReq>.JsonMakeData(this);
	}
	
}
