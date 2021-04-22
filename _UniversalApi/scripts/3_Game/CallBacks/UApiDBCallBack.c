class UApiDBCallBack : RestCallback
{
	protected Class Instance;
	protected string Function;
	protected string OID;
	protected int CallId;

	void UApiDBCallBack(Class instance, string function, int id, string oid){
		Instance = instance;
		Function = function;
		CallId = id;
		OID = oid;
	}
	
	override void OnError(int errorCode) {
		int rstatus = UAPI_SERVERERROR;
		if (errorCode == ERestResultState.EREST_ERROR_CLIENTERROR){
			rstatus = UAPI_CLIENTERROR;
		}
		if (Instance && Function != ""){
			GetGame().GameScript.CallFunctionParams(Instance, Function, NULL, new Param4<int, int, string, string>(CallId, rstatus, OID, "{}"));
		}
	};
	
	override void OnTimeout() {
		if (Instance && Function != ""){
			GetGame().GameScript.CallFunctionParams(Instance, Function, NULL, new Param4<int, int, string, string>(CallId, UAPI_DBTIMEOUT, OID, "{}"));
		}
	};
	
	override void OnSuccess(string data, int dataSize) {
		int rstatus = UAPI_SUCCESS;
		if (data == "{}" || data == "" || data == "{ }"){
			rstatus = UAPI_EMPTY;
		}
		if (Instance && Function != "" ){
			GetGame().GameScript.CallFunctionParams(Instance, Function, NULL, new Param4<int, int, string, string>(CallId, rstatus, OID, data));
		}
	};
};