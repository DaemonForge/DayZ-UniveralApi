//This meathod has to be used for the template class to work, you can't have a template class that exends RestCallback


class UFRestCallBackBase : RestCallback
{
	int m_UFid = -1;
	override void OnError(int errorCode) {
		//Always call super to prevent memory leaks
		string debugtrace;
		DumpStackString(debugtrace);
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).Call(U().ClearCallback,m_UFid, debugtrace);
	};
	override void OnTimeout() {
		//Always call super to prevent memory leaks
		string debugtrace;
		DumpStackString(debugtrace);
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).Call(U().ClearCallback,m_UFid, debugtrace);
	};
	override void OnSuccess(string data, int dataSize) {
		//Always call super to prevent memory leaks
		string debugtrace;
		DumpStackString(debugtrace);
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).Call(U().ClearCallback,m_UFid, debugtrace);
	};
	
	void SetId(int cid){
		m_UFid = cid;
	}
};


class UFCallback<Class T> extends UFCallbackBase {
	
	override void OnError(int errorCode, int cid) {
		Print("[UF] UFCallback<" + "> OnError  ErrorCode: " + UUtil.RestErrorToString(errorCode)+ "(" + errorCode + ")" + " cid:" + cid);
		if (GetInstance() && Function != "") {
			Param4<int, int, string, T> p = new Param4<int, int, string, T>(cid, errorCode, OID, null);
			Print(T);
			Print(GetInstance());
			Print(this);
			Print(p);
			g_Game.GameScript.CallFunctionParams(GetInstance(), Function, null, p);
		}
	}
	
	override void OnSuccess(string jsonData, int cid) {
		if (GetInstance() && Function != "") {
			autoptr T obj;
			if (UJSONHandler<T>.FromString(jsonData, obj)){
				int rstatus = UF_SUCCESS;
				StatusObject sobj;
				if (Class.CastTo(sobj, obj)){
					switch (sobj.Status) {
						case "NotFound":
							rstatus = UF_NOTFOUND;
							break;
						case "Empty":
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
				g_Game.GameScript.CallFunctionParams(GetInstance(), Function, NULL, new Param4<int, int, string, T>(cid, rstatus, OID, obj));
			} else {
				g_Game.GameScript.CallFunctionParams(GetInstance(), Function, NULL, new Param4<int, int, string, T>(cid, UF_JSONERROR, OID, NULL));
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
		Print("[UF] UFCallbackLoader<" + "> OnError  ErrorCode: " + UUtil.RestErrorToString(errorCode)+ "(" + errorCode + ")" + " cid:" + cid);
		if (GetInstance() && Function != "") {
			g_Game.GameScript.CallFunctionParams(GetInstance(), Function, NULL, new Param4<int, int, string, T>(cid, errorCode, OID, NULL));
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
					case "Empty":
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
			g_Game.GameScript.CallFunctionParams(GetInstance(), Function, NULL, new Param4<int, int, string, T>(cid, rstatus, OID, obj));
		}
	}
}

class UJSONCallback extends UFCallbackBase {
	
	override void OnError(int errorCode, int cid) {
		Print("[UF] UJSONCallback OnError  ErrorCode: " + UUtil.RestErrorToString(errorCode)+ "(" + errorCode + ")" + " cid:" + cid);
		if (GetInstance() && Function != ""){
			g_Game.GameScript.CallFunctionParams(GetInstance(), Function, NULL, new Param4<int, int, string, string>(cid, errorCode, OID, "{}"));
		}
	}
		
	override void OnSuccess(string jsonData, int cid) {
		if (GetInstance() && Function != ""){
			g_Game.GameScript.CallFunctionParams(GetInstance(), Function, NULL, new Param4<int, int, string, string>(cid, UF_SUCCESS, OID, jsonData));
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
		Class.CastTo(Instance, instance);
		Function = function;
		OID = oid;
	}
	
	//So I can set it automaticly to save on coding for other devs
	void SetOID(string oid){
		if (OID == "" && oid != ""){
			OID = oid;
		}
	}
	
	string GetOID(){
		return OID;
	}
	
	void OnError(int errorCode, int cid) {
		Error2("[UF] Callback Error", "Error calling back OnError, not set up correctly CallId: " + cid);
	}
		
	void OnSuccess(string jsonData, int cid) {
		Error2("[UF] Callback Error", "Error calling back OnSuccess, not set up correctly CallId: " + cid);
	}
}

class UNestedCallBack : UFRestCallBackBase
{
	protected autoptr UFCallbackBase m_CB;

	
	protected UFCallbackBase GetCB(){
		return m_CB;
	}
	
	void UNestedCallBack(UFCallbackBase cb){
		m_CB = cb;
		m_UFid = -1;
	}
	
	void ~UNestedCallBack(){
		if(m_CB) delete m_CB;
	}
	
	override void OnError(int errorCode) {
		if (U().IsCallCanceled(m_UFid)){
			Print("[UF] Call " + m_UFid + " not called as it was requested to be canceled - OnError " + U().ErrorToString(errorCode));
			super.OnError(errorCode);
			return;
		}
		int rstatus = UF_SERVERERROR;
		if (errorCode == ERestResultState.EREST_ERROR_CLIENTERROR){
			rstatus = UF_CLIENTERROR;
		}
		GetCB().OnError(rstatus, m_UFid);
		super.OnError(errorCode);
	};
	
	override void OnTimeout() {
		if (U().IsCallCanceled(m_UFid)){
			Print("[UF] Call " + m_UFid + " not called as it was requested to be canceled - OnTimeout");
			super.OnTimeout();
			return;
		}
		
		GetCB().OnError(UF_TIMEOUT, m_UFid);
		super.OnTimeout();
	};
	
	override void OnSuccess(string data, int dataSize) {
		if (U().IsCallCanceled(m_UFid)){
			Print("[UF] Call " + m_UFid + " not called as it was requested to be canceled - OnSuccess");
			super.OnSuccess(data, dataSize);
			return;
		}
		if (dataSize <= 0 || data == "{}" || data == "" || data == "{ }"){
			GetCB().OnError(UF_EMPTY, m_UFid);
			super.OnSuccess(data, dataSize);
			return;
		}
		GetCB().OnSuccess(data, m_UFid);
		super.OnSuccess(data, dataSize);
	};
};
