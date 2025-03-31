class CheckIfHasDiscord extends UDiscordCallBack {

	override void OnDiscordUserReceived(UDiscordUser user){
		//Do Stuff Here
		Print("[UF] [CheckIfHasDiscord] Success: " + user.id );
		
	}
	
	override void OnDiscordUserNotFound(UDiscordUser user){
		//Do Stuff Here
		Print("[UF] [CheckIfHasDiscord] User not found");
		if (GetGame().IsClient()){
			GetGame().OpenURL(U().ds().Link());
		}
	}

}