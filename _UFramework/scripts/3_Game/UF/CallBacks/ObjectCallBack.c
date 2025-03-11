//This meathod has to be used for the template class to work, you can't have a template class that exends RestCallback


class UFCallback<Class T> extends UFCallbackBase{
	
	override void OnError(int errorCode, int cid) {
		if (GetInstance() && Function != "") {
			GetGame().GameScript.CallFunctionParams(GetInstance(), Function, NULL, new Param4<int, int, string, T>(cid, errorCode, OID, NULL));
		}
	}
	
	override void OnSuccess(string jsonData, int cid) {
		if (GetInstance() && Function != ""){
			autoptr T obj;
			if (UJSONHandler<T>.FromString(jsonData, obj)){
				int rstatus = UF_SUCCESS;
				StatusObject sobj;
				if (Class.CastTo(sobj, obj)){
					switch (sobj.Status) {
						case "NotFound":
							rstatus = UF_NOTFOUND;
							break;
						case "NoResults":
							rstatus = UF_EMPTY;
							break;
						case "Error":
							rstatus = UF_ERROR;
							break;
						case "NoPerms":
							rstatus = UF_UNAUTHORIZED;
							break;
						case "NoAuth":
							rstatus = UF_UNAUTHORIZED;
							break;
						case "InvalidAuth":
							rstatus = UF_UNAUTHORIZED;
							break;
						case "NotSetup":
							rstatus = UF_NOTSETUP;
							break;
					}
				}
				GetGame().GameScript.CallFunctionParams(GetInstance(), Function, NULL, new Param4<int, int, string, T>(cid, rstatus, OID, obj));
			} else {
				GetGame().GameScript.CallFunctionParams(GetInstance(), Function, NULL, new Param4<int, int, string, T>(cid, UF_JSONERROR, OID, NULL));
			}
		}
	}
}

//Allows you to load the json to a defined object
class UFCallbackLoader<Class T> extends UFCallbackBase {
	
	autoptr T obj;
	
	void SetObject(T object){
		obj = object;
	}
	
	override void OnError(int errorCode, int cid) {
		if (GetInstance() && Function != "") {
			GetGame().GameScript.CallFunctionParams(GetInstance(), Function, NULL, new Param4<int, int, string, T>(cid, errorCode, OID, NULL));
		}
	}
	
	override void OnSuccess(string jsonData, int cid) {
		int rstatus = UF_JSONERROR;
		if (UJSONHandler<T>.FromString(jsonData, obj)){
			rstatus = UF_SUCCESS;
			StatusObject sobj;
			if (Class.CastTo(sobj, obj)){
				switch (sobj.Status) {
					case "NotFound":
						rstatus = UF_NOTFOUND;
						break;
					case "NoResults":
						rstatus = UF_EMPTY;
						break;
					case "Error":
						rstatus = UF_ERROR;
						break;
					case "NoPerms":
						rstatus = UF_UNAUTHORIZED;
						break;
					case "NoAuth":
						rstatus = UF_UNAUTHORIZED;
						break;
					case "InvalidAuth":
						rstatus = UF_UNAUTHORIZED;
						break;
					case "NotSetup":
						rstatus = UF_NOTSETUP;
						break;
				}
			}
		}
		if (GetInstance() && Function != ""){
			GetGame().GameScript.CallFunctionParams(GetInstance(), Function, NULL, new Param4<int, int, string, T>(cid, rstatus, OID, obj));
		}
	}
}

class UJSONCallback extends UFCallbackBase {
	
	override void OnError(int errorCode, int cid) {
		if (GetInstance() && Function != ""){
			GetGame().GameScript.CallFunctionParams(GetInstance(), Function, NULL, new Param4<int, int, string, string>(cid, errorCode, OID, "{}"));
		}
	}
		
	override void OnSuccess(string jsonData, int cid) {
		if (GetInstance() && Function != ""){
			GetGame().GameScript.CallFunctionParams(GetInstance(), Function, NULL, new Param4<int, int, string, string>(cid, UF_SUCCESS, OID, jsonData));
		}
	}
}



class UFCallbackBase extends Managed{

	protected Class Instance;
	protected string Function;
	protected string OID;

	
	protected Class GetInstance(){
		return Instance;
	}
	
	void UFCallbackBase(Class instance, string function, string oid = ""){
		Instance = instance;
		Function = function;
		OID = oid;
	}
	
	//So I can set it automaticly to save on coding for other devs
	void SetOID(string oid){
		if (OID == "" && oid != ""){
			OID = oid;
		}
	}
	
	void OnError(int errorCode, int cid) {
		Error2("[UF] Callback Error", "Error calling back OnError, not set up correctly CallId: " + cid);
	}
		
	void OnSuccess(string jsonData, int cid) {
		Error2("[UF] Callback Error", "Error calling back OnSuccess, not set up correctly CallId: " + cid);
	}
}

class UDBNestedCallBack : RestCallback
{
	protected int CallId;
	protected autoptr UFCallbackBase m_CB;

	
	protected UFCallbackBase GetCB(){
		return m_CB;
	}
	
	void UDBNestedCallBack(UFCallbackBase cb, int callId){
		m_CB = cb;
		CallId = callId;
	}
	
	override void OnError(int errorCode) {
		if (U().IsCallCanceled(CallId)){
			Print("[UF] Call " + CallId + " not called as it was requested to be canceled - OnError " + U().ErrorToString(errorCode));
			return;
		}
		int rstatus = UF_SERVERERROR;
		if (errorCode == ERestResultState.EREST_ERROR_CLIENTERROR){
			rstatus = UF_CLIENTERROR;
		}
		GetCB().OnError(rstatus, CallId);
	};
	
	override void OnTimeout() {
		if (U().IsCallCanceled(CallId)){
			Print("[UF] Call " + CallId + " not called as it was requested to be canceled - OnTimeout");
			return;
		}
		
		GetCB().OnError(UF_TIMEOUT, CallId);
	};
	
	override void OnSuccess(string data, int dataSize) {
		if (U().IsCallCanceled(CallId)){
			Print("[UF] Call " + CallId + " not called as it was requested to be canceled - OnSuccess");
			return;
		}
		if (dataSize <= 0 || data == "{}" || data == "" || data == "{ }"){
			GetCB().OnError(UF_EMPTY, CallId);
			return;
		}
		GetCB().OnSuccess(data, CallId);
	};
};
