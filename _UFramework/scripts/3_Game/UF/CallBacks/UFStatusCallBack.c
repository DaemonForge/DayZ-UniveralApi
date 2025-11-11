class UFStatusCallBack : UDBCallBack
{
	
	override void OnError(int errorCode) {
		if (Instance && Function != ""){
			g_Game.GameScript.CallFunctionParams(Instance, Function, NULL, new Param4<int, int, string, autoptr StatusObject>(CallId, UF_ERROR, OID, NULL));
		}
		super.OnError(errorCode);
	};
	
	override void OnTimeout() {
		if (Instance && Function != ""){
			g_Game.GameScript.CallFunctionParams(Instance, Function, NULL, new Param4<int, int, string, autoptr StatusObject>(CallId, UF_TIMEOUT, OID, NULL));
		}
		super.OnTimeout();
	};
	
	override void OnSuccess(string data, int dataSize) {
		if (Instance && Function != ""){
			
			autoptr StatusObject obj;
			
			JsonSerializer js = new JsonSerializer();
			string error;
			js.ReadFromString(obj, data, error);
			if (error != ""){
				Print("[UF] [UFStatusCallBack] Error: " + error);
			}
			if (obj && obj.Status && (obj.Status == "Success" || obj.Status == "Ok") ){ //Will eventually Phase out "Ok"			
				g_Game.GameScript.CallFunctionParams(Instance, Function, NULL, new Param4<int, int, string, autoptr StatusObject>(CallId, UF_SUCCESS, OID, StatusObject.Cast(obj)));
				super.OnSuccess(data,dataSize );
				return;
			} 
			if (obj.Status && (obj.Status == "NotFound" || obj.Status ==  "NotSetup")){
				g_Game.GameScript.CallFunctionParams(Instance, Function, NULL, new Param4<int, int, string, autoptr StatusObject>(CallId, UF_NOTFOUND, OID, StatusObject.Cast(obj)));
				super.OnSuccess(data,dataSize );
				return;
			}
			g_Game.GameScript.CallFunctionParams(Instance, Function, NULL, new Param4<int, int, string, autoptr StatusObject>(CallId, UF_ERROR, OID, StatusObject.Cast(obj)));
			super.OnSuccess(data,dataSize );
		}
	};
};