class UApiDiscordCallBack: RestCallback
{
	
	override void OnError(int errorCode) {
		Print("[UPAI] [UApiDiscordCallBack] Failed errorCode: " + errorCode);
	};
	
	override void OnTimeout() {
		Print("[UPAI] [UApiDiscordCallBack] Failed errorCode: Timeout");
	};
	
	override void OnSuccess(string data, int dataSize) {
		ref UApiDiscordUser user;
		
		JsonSerializer js = new JsonSerializer();
		string error;
		js.ReadFromString(user, data, error);
		if (error != ""){
			Print("[UPAI] [UApiDiscordCallBack] Error: " + error);
		}
		if (user.Status && user.Status == "Success" && user.id && user.id != "0"){
			OnDiscordUserReceived(UApiDiscordUser.Cast(user));
		} else if (user.Status && user.Status == "UserNotFound"){
			OnDiscordUserNotFound(UApiDiscordUser.Cast(user));
			
		} else {
			OnDiscordUserError(UApiDiscordUser.Cast(user));
		}
	};
	
	void OnDiscordUserReceived(ref UApiDiscordUser user){
		//Do Stuff Here
		Print("[UPAI] [UApiDiscordCallBack] Success: " + user.id );
		
	}
	
	void OnDiscordUserNotFound(ref UApiDiscordUser user){
		//Do Stuff Here
		Print("[UPAI] [UApiDiscordCallBack] User not found");
	}
	
	void OnDiscordUserError(ref UApiDiscordUser user){
		//Do Stuff Here
		Print("[UPAI] [UApiDiscordCallBack] Error: " + user.Error);
		
	}
}
