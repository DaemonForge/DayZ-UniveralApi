class UDBCallBack : UFRestCallBackBase
{
	protected Class Instance;
	protected string Function;
	protected string OID;
	protected int CallId;

	
	protected Class GetInstance(){
		return Instance;
	}
	
	void UDBCallBack(Class instance, string function, int id, string oid){
		Instance = instance;
		Function = function;
		CallId = id;
		OID = oid;
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
		if (GetInstance() && Function != ""){
			GetGame().GameScript.CallFunctionParams(GetInstance(), Function, NULL, new Param4<int, int, string, string>(CallId, rstatus, OID, "{}"));
		}
	};
	
	override void OnTimeout() {
		if (U().IsCallCanceled(CallId)){
			Print("[UF] Call " + CallId + " not called as it was requested to be canceled - OnTimeout");
			return;
		}
		if (GetInstance() && Function != ""){
			GetGame().GameScript.CallFunctionParams(GetInstance(), Function, NULL, new Param4<int, int, string, string>(CallId, UF_TIMEOUT, OID, "{}"));
		}
	};
	
	override void OnSuccess(string data, int dataSize) {
		if (U().IsCallCanceled(CallId)){
			Print("[UF] Call " + CallId + " not called as it was requested to be canceled - OnSuccess");
			return;
		}
		int rstatus = UF_SUCCESS;
		if (data == "{}" || data == "" || data == "{ }"){
			rstatus = UF_EMPTY;
		}
		if (GetInstance() && Function != ""){
			GetGame().GameScript.CallFunctionParams(GetInstance(), Function, NULL, new Param4<int, int, string, string>(CallId, rstatus, OID, data));
		}
	};
};
