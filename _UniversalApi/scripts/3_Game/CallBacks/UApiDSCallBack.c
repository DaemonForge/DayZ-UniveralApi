class UApiDSCallBack : UApiDBCallBack
{
	
	override void OnError(int errorCode) {
		if (Instance && Function != ""){
			GetGame().GameScript.CallFunctionParams(Instance, Function, NULL, new Param4<int, int, string, ref UApiDiscordUser>(CallId, UAPI_DBSERVERERROR, OID, NULL));
		}
	};
	
	override void OnTimeout() {
		if (Instance && Function != ""){
			GetGame().GameScript.CallFunctionParams(Instance, Function, NULL, new Param4<int, int, string, ref UApiDiscordUser>(CallId, UAPI_DBTIMEOUT, OID, NULL));
		}
	};
	
	override void OnSuccess(string data, int dataSize) {
		if (Instance && Function != ""){
			
			ref UApiDiscordUser user;
			
			JsonSerializer js = new JsonSerializer();
			string error;
			js.ReadFromString(user, data, error);
			if (error != ""){
				Print("[UPAI] [UApiDiscordCallBack] Error: " + error);
			}
			if (user && user.Status && user.Status == "Success"){			
				GetGame().GameScript.CallFunctionParams(Instance, Function, NULL, new Param4<int, int, string, ref UApiDiscordUser>(CallId, UAPI_DBSUCCESS, OID, UApiDiscordUser.Cast(user)));
				return;
			} 
			if (user.Status && (user.Status == "NotFound" || user.Status ==  "NotSetup")){
				GetGame().GameScript.CallFunctionParams(Instance, Function, NULL, new Param4<int, int, string, ref UApiDiscordUser>(CallId, UAPI_NOTFOUND, OID, UApiDiscordUser.Cast(user)));
				return;
			}
			GetGame().GameScript.CallFunctionParams(Instance, Function, NULL, new Param4<int, int, string, ref UApiDiscordUser>(CallId, UAPI_DBERROR, OID, UApiDiscordUser.Cast(user)));
		}
	};
};