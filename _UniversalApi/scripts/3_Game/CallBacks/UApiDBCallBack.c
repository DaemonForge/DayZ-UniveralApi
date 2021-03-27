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
		if (Instance && Function != ""){
			GetGame().GameScript.CallFunctionParams(Instance, Function, NULL, new Param4<int, int, string, string>(CallId, UAPI_DBSERVERERROR, OID, ""));
		}
	};
	
	override void OnTimeout() {
		if (Instance && Function != ""){
			GetGame().GameScript.CallFunctionParams(Instance, Function, NULL, new Param4<int, int, string, string>(CallId, UAPI_DBTIMEOUT, OID, ""));
		}
	};
	
	override void OnSuccess(string data, int dataSize) {
		if (Instance && Function != ""){
			GetGame().GameScript.CallFunctionParams(Instance, Function, NULL, new Param4<int, int, string, string>(CallId, UAPI_DBSUCCESS, OID, data));
		}
	};
};