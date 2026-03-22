class CheckIfHasDiscord extends UDiscordCallBack {

	override void OnDiscordUserReceived(UDiscordUser user){
		//Do Stuff Here
		UFLog.Info("[CheckIfHasDiscord] Success: " + user.id);
		
	}
	
	override void OnDiscordUserNotFound(UDiscordUser user){
		//Do Stuff Here
		UFLog.Info("[CheckIfHasDiscord] User not found");
		if (g_Game.IsClient()){
			g_Game.OpenURL(UF().ds().Link());
		}
	}

}