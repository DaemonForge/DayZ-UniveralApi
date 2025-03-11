class UDiscordCallBack: RestCallback
{
	
	override void OnError(int errorCode) {
		Print("[UF] [UDiscordCallBack] Failed errorCode: " + errorCode);
	};
	
	override void OnTimeout() {
		Print("[UF] [UDiscordCallBack] Failed errorCode: Timeout");
	};
	
	override void OnSuccess(string data, int dataSize) {
		UDiscordUser user;
		
		JsonSerializer js = new JsonSerializer();
		string error;
		js.ReadFromString(user, data, error);
		if (error != ""){
			Print("[UF] [UDiscordCallBack] Error: " + error);
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
		Print("[UF] [UDiscordCallBack] Success: " + user.id );
		
	}
	
	void OnDiscordUserNotFound(UDiscordUser user){
		//Do Stuff Here
		Print("[UF] [UDiscordCallBack] User not found");
	}
	
	void OnDiscordUserError(UDiscordUser user){
		//Do Stuff Here
		Print("[UF] [UDiscordCallBack] Error: " + user.Error);
		
	}
}
