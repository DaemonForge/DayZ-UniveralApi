class UApiDSCallBack : UApiDBCallBack
{
	
	override void OnError(int errorCode) {
		if (UApi().IsCallCanceled(CallId)){
			Print("[UAPI] Call " + CallId + " not called as it was requested to be canceled - OnError " + UApi().ErrorToString(errorCode));
			return;
		}
		if (Instance && Function != ""){
			GetGame().GameScript.CallFunctionParams(Instance, Function, NULL, new Param4<int, int, string, autoptr UApiDiscordUser>(CallId, UAPI_ERROR, OID, NULL));
		}
	};
	
	override void OnTimeout() {
		if (UApi().IsCallCanceled(CallId)){
			Print("[UAPI] Call " + CallId + " not called as it was requested to be canceled - OnTimeout");
			return;
		}
		if (Instance && Function != ""){
			GetGame().GameScript.CallFunctionParams(Instance, Function, NULL, new Param4<int, int, string, autoptr UApiDiscordUser>(CallId, UAPI_TIMEOUT, OID, NULL));
		}
	};
	
	override void OnSuccess(string data, int dataSize) {
		if (UApi().IsCallCanceled(CallId)){
			Print("[UAPI] Call " + CallId + " not called as it was requested to be canceled - OnSuccess");
			return;
		}
		if (Instance && Function != ""){
			
			autoptr UApiDiscordUser user;
			
			JsonSerializer js = new JsonSerializer();
			string error;
			js.ReadFromString(user, data, error);
			if (error != ""){
				Print("[UPAI] [UApiDiscordCallBack] Error: " + error);
			}
			
			if (user && user.Status && user.Status == "Success"){			
				GetGame().GameScript.CallFunctionParams(Instance, Function, NULL, new Param4<int, int, string, autoptr UApiDiscordUser>(CallId, UAPI_SUCCESS, OID, UApiDiscordUser.Cast(user)));
				return;
			} 
			if (user.Status && (user.Status == "NotFound" || user.Status ==  "NotSetup")){
				GetGame().GameScript.CallFunctionParams(Instance, Function, NULL, new Param4<int, int, string, autoptr UApiDiscordUser>(CallId, UAPI_NOTFOUND, OID, UApiDiscordUser.Cast(user)));
				return;
			}
			GetGame().GameScript.CallFunctionParams(Instance, Function, NULL, new Param4<int, int, string, autoptr UApiDiscordUser>(CallId, UAPI_ERROR, OID, UApiDiscordUser.Cast(user)));
		}
	};
};


class UApiDiscordStatusCallBack : UApiDBCallBack
{
	
	override void OnError(int errorCode) {
		if (UApi().IsCallCanceled(CallId)){
			Print("[UAPI] Call " + CallId + " not called as it was requested to be canceled - OnError " + UApi().ErrorToString(errorCode));
			return;
		}
		if (Instance && Function != ""){
			GetGame().GameScript.CallFunctionParams(Instance, Function, NULL, new Param4<int, int, string, autoptr UApiDiscordStatusObject>(CallId, UAPI_ERROR, OID, NULL));
		}
	};
	
	override void OnTimeout() {
		if (UApi().IsCallCanceled(CallId)){
			Print("[UAPI] Call " + CallId + " not called as it was requested to be canceled - OnTimeout");
			return;
		}
		if (Instance && Function != ""){
			GetGame().GameScript.CallFunctionParams(Instance, Function, NULL, new Param4<int, int, string, autoptr UApiDiscordStatusObject>(CallId, UAPI_TIMEOUT, OID, NULL));
		}
	};
	
	override void OnSuccess(string data, int dataSize) {
		
		if (UApi().IsCallCanceled(CallId)){
			Print("[UAPI] Call " + CallId + " not called as it was requested to be canceled - OnSuccess");
			return;
		}
		if (Instance && Function != ""){
			
			autoptr UApiDiscordStatusObject obj;
			
			JsonSerializer js = new JsonSerializer();
			string error;
			js.ReadFromString(obj, data, error);
			if (error != ""){
				Print("[UPAI] [UApiStatusCallBack] Error: " + error);
			}
			if (obj && obj.Status && (obj.Status == "Success" || obj.Status == "Ok") ){ //Will eventually Phase out "Ok"			
				GetGame().GameScript.CallFunctionParams(Instance, Function, NULL, new Param4<int, int, string, autoptr UApiDiscordStatusObject>(CallId, UAPI_SUCCESS, OID, UApiDiscordStatusObject.Cast(obj)));
				return;
			} 
			if (obj.Status && (obj.Status == "NotFound" || obj.Status ==  "NotSetup")){
				GetGame().GameScript.CallFunctionParams(Instance, Function, NULL, new Param4<int, int, string, autoptr UApiDiscordStatusObject>(CallId, UAPI_NOTFOUND, OID, UApiDiscordStatusObject.Cast(obj)));
				return;
			}
			GetGame().GameScript.CallFunctionParams(Instance, Function, NULL, new Param4<int, int, string, autoptr UApiDiscordStatusObject>(CallId, UAPI_ERROR, OID, UApiDiscordStatusObject.Cast(obj)));
		}
	};
};

class UApiDiscordMessagesCallBack : UApiDBCallBack
{
	
	override void OnError(int errorCode) {
		if (UApi().IsCallCanceled(CallId)){
			Print("[UAPI] Call " + CallId + " not called as it was requested to be canceled - OnError " + UApi().ErrorToString(errorCode));
			return;
		}
		if (Instance && Function != ""){
			GetGame().GameScript.CallFunctionParams(Instance, Function, NULL, new Param4<int, int, string, ref UApiDiscordMessagesResponse>(CallId, UAPI_ERROR, OID, NULL));
		}
	};
	
	override void OnTimeout() {
		if (UApi().IsCallCanceled(CallId)){
			Print("[UAPI] Call " + CallId + " not called as it was requested to be canceled - OnTimeout");
			return;
		}
		if (Instance && Function != ""){
			GetGame().GameScript.CallFunctionParams(Instance, Function, NULL, new Param4<int, int, string, ref UApiDiscordMessagesResponse>(CallId, UAPI_TIMEOUT, OID, NULL));
		}
	};
	
	override void OnSuccess(string data, int dataSize) {
		if (UApi().IsCallCanceled(CallId)){
			Print("[UAPI] Call " + CallId + " not called as it was requested to be canceled - OnSuccess");
			return;
		}
		if (Instance && Function != ""){
			
			autoptr UApiDiscordMessagesResponse res;
			
			JsonSerializer js = new JsonSerializer();
			string error;
			js.ReadFromString(res, data, error);
			if (error != ""){
				Print("[UPAI] [UApiStatusCallBack] Error: " + error);
			}
			if (res && res.Status && (res.Status == "Success" || res.Status == "Ok") ){ //Will eventually Phase out "Ok"			
				GetGame().GameScript.CallFunctionParams(Instance, Function, NULL, new Param4<int, int, string, ref UApiDiscordMessagesResponse>(CallId, UAPI_SUCCESS, OID, UApiDiscordMessagesResponse.Cast(res)));
				return;
			} 
			if (res.Status && (res.Status == "NotFound" || res.Status ==  "NotSetup")){
				GetGame().GameScript.CallFunctionParams(Instance, Function, NULL, new Param4<int, int, string, ref UApiDiscordMessagesResponse>(CallId, UAPI_NOTFOUND, OID, UApiDiscordMessagesResponse.Cast(res)));
				return;
			}
			if (res.Status && res.Status == "NoPerms"){
				GetGame().GameScript.CallFunctionParams(Instance, Function, NULL, new Param4<int, int, string, ref UApiDiscordMessagesResponse>(CallId, UAPI_UNAUTHORIZED, OID, UApiDiscordMessagesResponse.Cast(res)));
				return;
			}
			GetGame().GameScript.CallFunctionParams(Instance, Function, NULL, new Param4<int, int, string, ref UApiDiscordMessagesResponse>(CallId, UAPI_ERROR, OID, UApiDiscordMessagesResponse.Cast(res)));
		}
	};
};