class UDiscordCallBack: RestCallback
{
	
	override void OnError(int errorCode) {
		UFLog.Err("[UDiscordCallBack] Failed errorCode: " + errorCode);
	};
	
	override void OnTimeout() {
		UFLog.Err("[UDiscordCallBack] Failed errorCode: Timeout");
	};
	
	override void OnSuccess(string data, int dataSize) {
		UDiscordUser user;
		
		JsonSerializer js = new JsonSerializer();
		string error;
		js.ReadFromString(user, data, error);
		if (error != ""){
			UFLog.Err("[UDiscordCallBack] Error: " + error);
		}
		if (user.Status && user.Status == "Success" && user.id && user.id != "0"){
			OnDiscordUserReceived(UDiscordUser.Cast(user));
		} else if (user.Status && (user.Status == "NotFound" || user.Status ==  "NotSetup")){
			OnDiscordUserNotFound(UDiscordUser.Cast(user));
			
		} else {
			OnDiscordUserError(UDiscordUser.Cast(user));
		}
	};
	
	void OnDiscordUserReceived(UDiscordUser user){
		//Do Stuff Here
		UFLog.Info("[UDiscordCallBack] Success: " + user.id);
		
	}
	
	void OnDiscordUserNotFound(UDiscordUser user){
		//Do Stuff Here
		UFLog.Info("[UDiscordCallBack] User not found");
	}
	
	void OnDiscordUserError(UDiscordUser user){
		//Do Stuff Here
		UFLog.Err("[UDiscordCallBack] Error: " + user.Error);
		
	}
}
