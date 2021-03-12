class CheckIfHasDiscord extends UApiDiscordCallBack {

	override void OnDiscordUserReceived(ref UApiDiscordUser user){
		//Do Stuff Here
		Print("[UPAI] [CheckIfHasDiscord] Success: " + user.id );
		
	}
	
	override void OnDiscordUserNotFound(ref UApiDiscordUser user){
		//Do Stuff Here
		Print("[UPAI] [CheckIfHasDiscord] User not found");
		if (GetGame().IsClient()){
			GetGame().OpenURL(UApi().Discord().Link());
		}
	}

}