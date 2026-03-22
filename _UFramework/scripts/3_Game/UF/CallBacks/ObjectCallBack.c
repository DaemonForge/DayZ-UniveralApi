//This meathod has to be used for the template class to work, you can't have a template class that exends RestCallback

/**
 * UFRestCallBackBase
 * Base callback class for all REST API calls in Universal Framework.
 * Automatically clears callbacks via UF().ClearCallback() to prevent memory leaks.
 * All child callbacks inherit automatic cleanup behavior.
 */
class UFRestCallBackBase : RestCallback
{
	int m_UFid = -1;
	
	/**
	 * Called when REST API call encounters an error
	 * @param errorCode The error code from the REST API
	 */
	override void OnError(int errorCode) {
		//Always call super to prevent memory leaks
		string debugtrace;
		DumpStackString(debugtrace);
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).Call(UF().ClearCallback,m_UFid, debugtrace);
	};
	
	/**
	 * Called when REST API call times out
	 */
	override void OnTimeout() {
		//Always call super to prevent memory leaks
		string debugtrace;
		DumpStackString(debugtrace);
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).Call(UF().ClearCallback,m_UFid, debugtrace);
	};
	
	/**
	 * Called when REST API call succeeds
	 * @param data The response data as JSON string
	 * @param dataSize Size of the response data
	 */
	override void OnSuccess(string data, int dataSize) {
		//Always call super to prevent memory leaks
		string debugtrace;
		DumpStackString(debugtrace);
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).Call(UF().ClearCallback, m_UFid, debugtrace);
	};
	
	/**
	 * Sets the call ID for this callback
	 * @param cid The call ID assigned by the framework
	 */
	void SetId(int cid){
		m_UFid = cid;
	}
};

/**
 * UFCallback<T>
 * Template-based callback that automatically deserializes JSON responses into typed objects.
 * Handles status code parsing and authentication failures.
 * 
 * @tparam T The type to deserialize the response into
 * 
 * Usage:
 * @code
 * UF().db().Load("MyMod", "player123", new UFCallback<PlayerData>(this, "OnPlayerLoaded"));
 * 
 * void OnPlayerLoaded(int cid, int status, string oid, PlayerData data) {
 *     if (status == UF_SUCCESS) {
 *         // Use data
 *     }
 * }
 * @endcode
 */
class UFCallback<Class T> extends UFCallbackBase {
	
	/**
	 * Called when REST request fails
	 * @param errorCode The error code from REST API
	 * @param cid The call ID
	 */
	override void OnError(int errorCode, int cid) {
		UFLog.Err("UFCallback<" + "> OnError  ErrorCode: " + UUtil.RestErrorToString(errorCode)+ "(" + errorCode + ")" + " cid:" + cid);
		if (GetInstance() && Function != "") {
			Param4<int, int, string, T> p = new Param4<int, int, string, T>(cid, errorCode, OID, null);
			UFLog.Debug("" + GetInstance());
			UFLog.Debug("" + this);
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
							UF().OnAuthFailure(); // Trigger token renewal
							break;
						case "InvalidAuth":
							rstatus = UF_UNAUTHORIZED;
							UF().OnAuthFailure(); // Trigger token renewal
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

/**
 * UFCallbackLoader<T>
 * Template callback that loads JSON data directly into a pre-existing object instance.
 * Useful when you want to update an existing object rather than create a new one.
 * 
 * @tparam T The type of object to load data into
 * 
 * Usage:
 * @code
 * autoptr PlayerData myData = new PlayerData();
 * autoptr UFCallbackLoader<PlayerData> cb = new UFCallbackLoader<PlayerData>(this, "OnLoaded");
 * cb.SetObject(myData);
 * UF().db().Load("MyMod", "player123", cb);
 * @endcode
 */
class UFCallbackLoader<Class T> extends UFCallbackBase {
	
	autoptr T obj;
	
	/**
	 * Sets the object instance that will receive the loaded JSON data
	 * @param object The object to populate with JSON data
	 */
	void SetObject(T object){
		obj = object;
	}
	
	/**
	 * Called when REST request fails
	 * @param errorCode The error code from REST API
	 * @param cid The call ID
	 */
	override void OnError(int errorCode, int cid) {
		UFLog.Err("UFCallbackLoader<" + "> OnError  ErrorCode: " + UUtil.RestErrorToString(errorCode)+ "(" + errorCode + ")" + " cid:" + cid);
		if (GetInstance() && Function != "") {
			g_Game.GameScript.CallFunctionParams(GetInstance(), Function, NULL, new Param4<int, int, string, T>(cid, errorCode, OID, NULL));
		}
	}
	
	/**
	 * Called when REST request succeeds
	 * Deserializes JSON into the pre-set object instance
	 * @param jsonData The JSON response string
	 * @param cid The call ID
	 */
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
						UF().OnAuthFailure(); // Trigger token renewal
						break;
					case "InvalidAuth":
						rstatus = UF_UNAUTHORIZED;
						UF().OnAuthFailure(); // Trigger token renewal
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

/**
 * UJSONCallback
 * Callback that returns raw JSON string instead of deserialized objects.
 * Use when you need direct access to JSON without automatic type conversion.
 * 
 * Callback signature: void OnCallback(int cid, int status, string oid, string jsonData)
 */
class UJSONCallback extends UFCallbackBase {
	
	/**
	 * Called when REST request fails
	 * @param errorCode The error code from REST API
	 * @param cid The call ID
	 */
	override void OnError(int errorCode, int cid) {
		UFLog.Err("UJSONCallback OnError  ErrorCode: " + UUtil.RestErrorToString(errorCode)+ "(" + errorCode + ")" + " cid:" + cid);
		if (GetInstance() && Function != ""){
			g_Game.GameScript.CallFunctionParams(GetInstance(), Function, NULL, new Param4<int, int, string, string>(cid, errorCode, OID, "{}"));
		}
	}
	
	/**
	 * Called when REST request succeeds
	 * Returns raw JSON string without deserialization
	 * @param jsonData The JSON response string
	 * @param cid The call ID
	 */
	override void OnSuccess(string jsonData, int cid) {
		if (GetInstance() && Function != ""){
			g_Game.GameScript.CallFunctionParams(GetInstance(), Function, NULL, new Param4<int, int, string, string>(cid, UF_SUCCESS, OID, jsonData));
		}
	}
}


/**
 * UFCallbackBase
 * Base class for all Universal Framework callbacks.
 * Stores callback instance, function name, and optional object ID (OID).
 * All template callbacks inherit from this class.
 */
class UFCallbackBase extends Managed{

	protected Class Instance;
	protected string Function;
	protected string OID;
	
	/**
	 * Returns the callback instance
	 * @return Class The instance that will receive the callback
	 */
	protected Class GetInstance(){
		return Instance;
	}
	
	/**
	 * Constructor
	 * @param instance The object instance to call the callback function on
	 * @param function The name of the callback function
	 * @param oid Optional object identifier (e.g., player GUID, mod name)
	 */
	void UFCallbackBase(Class instance, string function, string oid = ""){
		Class.CastTo(Instance, instance);
		Function = function;
		OID = oid;
	}
	
	/**
	 * Sets the object identifier if not already set
	 * @param oid The object identifier to set
	 */
	void SetOID(string oid){
		if (OID == "" && oid != ""){
			OID = oid;
		}
	}
	
	/**
	 * Returns the stored object identifier
	 * @return string The object ID (e.g., GUID, mod name)
	 */
	string GetOID(){
		return OID;
	}
	
	/**
	 * Called when REST request fails
	 * Must be overridden by child classes
	 * @param errorCode The error code from REST API
	 * @param cid The call ID
	 */
	void OnError(int errorCode, int cid) {
		Error2("[UF] Callback Error", "Error calling back OnError, not set up correctly CallId: " + cid);
	}
	
	/**
	 * Called when REST request succeeds
	 * Must be overridden by child classes
	 * @param jsonData The JSON response string
	 * @param cid The call ID
	 */
	void OnSuccess(string jsonData, int cid) {
		Error2("[UF] Callback Error", "Error calling back OnSuccess, not set up correctly CallId: " + cid);
	}
}

/**
 * UNestedCallBack
 * Wrapper callback that converts REST callback into UFCallbackBase callback.
 * Handles error code translation, timeout detection, and cancellation checks.
 * Automatically forwards results to the wrapped callback.
 */
class UNestedCallBack : UFRestCallBackBase
{
	protected autoptr UFCallbackBase m_CB;

	
	/**
	 * Returns the wrapped callback
	 * @return UFCallbackBase The nested callback instance
	 */
	protected UFCallbackBase GetCB(){
		return m_CB;
	}
	
	/**
	 * Constructor
	 * @param cb The UFCallbackBase to wrap and forward results to
	 */
	void UNestedCallBack(UFCallbackBase cb){
		m_CB = cb;
		m_UFid = -1;
		UFLog.Debug("[UNestedCallBack] Created with callback: " + cb.ToString());
	}
	
	void ~UNestedCallBack(){
		if(m_CB) delete m_CB;
	}
	
	/**
	 * Called when REST API call encounters an error
	 * Translates REST error codes to UF status codes and forwards to nested callback
	 * @param errorCode The REST error code
	 */
	override void OnError(int errorCode) {
		UFLog.Debug("[UNestedCallBack] OnError - CID: " + m_UFid + ", ErrorCode: " + errorCode);
		if (UF().IsCallCanceled(m_UFid)){
			UFLog.Debug("Call " + m_UFid + " not called as it was requested to be canceled - OnError " + UF().ErrorToString(errorCode));
			super.OnError(errorCode);
			return;
		}
		int rstatus = UF_SERVERERROR;
		if (errorCode == ERestResultState.EREST_ERROR_CLIENTERROR){
			rstatus = UF_CLIENTERROR;
		}
		UFLog.Debug("[UNestedCallBack] Forwarding OnError to callback, status: " + rstatus);
		GetCB().OnError(rstatus, m_UFid);
		super.OnError(errorCode);
	};
	
	/**
	 * Called when REST API call times out
	 * Forwards UF_TIMEOUT status to nested callback
	 */
	override void OnTimeout() {
		UFLog.Debug("[UNestedCallBack] OnTimeout - CID: " + m_UFid);
		if (UF().IsCallCanceled(m_UFid)){
			UFLog.Debug("Call " + m_UFid + " not called as it was requested to be canceled - OnTimeout");
			super.OnTimeout();
			return;
		}
		
		UFLog.Debug("[UNestedCallBack] Forwarding OnTimeout to callback");
		GetCB().OnError(UF_TIMEOUT, m_UFid);
		super.OnTimeout();
	};
	
	/**
	 * Called when REST API call succeeds
	 * Validates data and forwards to nested callback
	 * @param data The response data
	 * @param dataSize Size of the response data
	 */
	override void OnSuccess(string data, int dataSize) {
		UFLog.Debug("[UNestedCallBack] OnSuccess - CID: " + m_UFid + ", DataSize: " + dataSize);
		if (UF().IsCallCanceled(m_UFid)){
			UFLog.Debug("Call " + m_UFid + " not called as it was requested to be canceled - OnSuccess");
			super.OnSuccess(data, dataSize);
			return;
		}
		if (dataSize <= 0 || data == "{}" || data == "" || data == "{ }"){
			UFLog.Debug("[UNestedCallBack] Empty data, forwarding as UF_EMPTY");
			GetCB().OnError(UF_EMPTY, m_UFid);
			super.OnSuccess(data, dataSize);
			return;
		}
		UFLog.Debug("[UNestedCallBack] Forwarding OnSuccess to callback, data: " + data.Substring(0, Math.Min(200, data.Length())));
		GetCB().OnSuccess(data, m_UFid);
		super.OnSuccess(data, dataSize);
	};
};
