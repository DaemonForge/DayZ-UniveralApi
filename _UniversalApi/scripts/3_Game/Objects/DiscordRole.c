class UApiDiscordUser extends StatusObject{
		
	string id;
	string Username;
	
	string Discriminator;
	string Avatar;
	
	TStringArray Roles;
	
	bool HasRole(string roleid){
		return (Roles.Find(roleid) != -1);
	}
	
	void ~UApiDiscordUser(){
		Print("[UAPI] ~UApiDiscordUser");
	}
}

class UApiDiscordRoleReq extends UApiDiscordObject_Base{
	string Role;
	void UApiDiscordRoleReq(string role){
		Role = role;
	}
	
	override string ToJson(){
		return JsonFileLoader<UApiDiscordRoleReq>.JsonMakeData(this);;
	}
	
}
