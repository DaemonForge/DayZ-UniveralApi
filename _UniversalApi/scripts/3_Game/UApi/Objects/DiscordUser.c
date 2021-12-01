class UApiDiscordUser extends StatusObject{
		
	string id;
	string Username;
	
	string Discriminator;
	string Avatar;
	
	autoptr TStringArray Roles;
	
	string VoiceChannel;
	
	bool HasRole(string roleid){
		if (!Roles) return false;
		return (Roles.Find(roleid) != -1);
	}
	
}