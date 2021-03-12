class UApiDiscordUser{
	
	string Status;
	string Error;
	
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

class UApiDiscordRoleReq{
	string Role;
	void UApiDiscordRoleReq(string role){
		Role = role;
	}
	
	string ToJson(){
		return JsonFileLoader<UApiDiscordRoleReq>.JsonMakeData(this);;
	}
	
}
