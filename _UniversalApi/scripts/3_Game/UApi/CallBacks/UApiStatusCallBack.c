class UApiStatusCallBack : UApiDBCallBack
{
	
	override void OnError(int errorCode) {
		if (Instance && Function != ""){
			GetGame().GameScript.CallFunctionParams(Instance, Function, NULL, new Param4<int, int, string, autoptr StatusObject>(CallId, UAPI_ERROR, OID, NULL));
		}
	};
	
	override void OnTimeout() {
		if (Instance && Function != ""){
			GetGame().GameScript.CallFunctionParams(Instance, Function, NULL, new Param4<int, int, string, autoptr StatusObject>(CallId, UAPI_TIMEOUT, OID, NULL));
		}
	};
	
	override void OnSuccess(string data, int dataSize) {
		if (Instance && Function != ""){
			
			autoptr StatusObject obj;
			
			JsonSerializer js = new JsonSerializer();
			string error;
			js.ReadFromString(obj, data, error);
			if (error != ""){
				Print("[UPAI] [UApiStatusCallBack] Error: " + error);
			}
			if (obj && obj.Status && (obj.Status == "Success" || obj.Status == "Ok") ){ //Will eventually Phase out "Ok"			
				GetGame().GameScript.CallFunctionParams(Instance, Function, NULL, new Param4<int, int, string, autoptr StatusObject>(CallId, UAPI_SUCCESS, OID, StatusObject.Cast(obj)));
				return;
			} 
			if (obj.Status && (obj.Status == "NotFound" || obj.Status ==  "NotSetup")){
				GetGame().GameScript.CallFunctionParams(Instance, Function, NULL, new Param4<int, int, string, autoptr StatusObject>(CallId, UAPI_NOTFOUND, OID, StatusObject.Cast(obj)));
				return;
			}
			GetGame().GameScript.CallFunctionParams(Instance, Function, NULL, new Param4<int, int, string, autoptr StatusObject>(CallId, UAPI_ERROR, OID, StatusObject.Cast(obj)));
		}
	};
};