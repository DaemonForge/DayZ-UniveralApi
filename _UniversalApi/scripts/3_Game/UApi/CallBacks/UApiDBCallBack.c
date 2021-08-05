class UApiDBCallBack : RestCallback
{
	protected Class Instance;
	protected string Function;
	protected string OID;
	protected int CallId;

	
	protected Class GetInstance(){
		return Instance;
	}
	
	void UApiDBCallBack(Class instance, string function, int id, string oid){
		Instance = instance;
		Function = function;
		CallId = id;
		OID = oid;
	}
	
	override void OnError(int errorCode) {
		if (UApi().IsCallCanceled(CallId)){
			Print("[UAPI] Call " + CallId + " not called as it was requested to be canceled - OnError " + UApi().ErrorToString(errorCode));
			return;
		}
		int rstatus = UAPI_SERVERERROR;
		if (errorCode == ERestResultState.EREST_ERROR_CLIENTERROR){
			rstatus = UAPI_CLIENTERROR;
		}
		if (GetInstance() && Function != ""){
			GetGame().GameScript.CallFunctionParams(GetInstance(), Function, NULL, new Param4<int, int, string, string>(CallId, rstatus, OID, "{}"));
		}
	};
	
	override void OnTimeout() {
		if (UApi().IsCallCanceled(CallId)){
			Print("[UAPI] Call " + CallId + " not called as it was requested to be canceled - OnTimeout");
			return;
		}
		if (GetInstance() && Function != ""){
			GetGame().GameScript.CallFunctionParams(GetInstance(), Function, NULL, new Param4<int, int, string, string>(CallId, UAPI_TIMEOUT, OID, "{}"));
		}
	};
	
	override void OnSuccess(string data, int dataSize) {
		if (UApi().IsCallCanceled(CallId)){
			Print("[UAPI] Call " + CallId + " not called as it was requested to be canceled - OnSuccess");
			return;
		}
		int rstatus = UAPI_SUCCESS;
		if (data == "{}" || data == "" || data == "{ }"){
			rstatus = UAPI_EMPTY;
		}
		if (GetInstance() && Function != ""){
			GetGame().GameScript.CallFunctionParams(GetInstance(), Function, NULL, new Param4<int, int, string, string>(CallId, rstatus, OID, data));
		}
	};
};
