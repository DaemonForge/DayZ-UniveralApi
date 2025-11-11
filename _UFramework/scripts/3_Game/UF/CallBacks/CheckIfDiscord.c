class CheckIfHasDiscord extends UDiscordCallBack {

	override void OnDiscordUserReceived(UDiscordUser user){
		//Do Stuff Here
		Print("[UF] [CheckIfHasDiscord] Success: " + user.id );
		
	}
	
	override void OnDiscordUserNotFound(UDiscordUser user){
		//Do Stuff Here
		Print("[UF] [CheckIfHasDiscord] User not found");
		if (g_Game.IsClient()){
			g_Game.OpenURL(U().ds().Link());
		}
	}

}