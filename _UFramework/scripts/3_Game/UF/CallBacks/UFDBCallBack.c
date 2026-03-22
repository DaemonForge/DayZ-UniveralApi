/**
 * UDBCallBack Class
 *
 * Simplified callback handler for database operations that returns raw JSON strings.
 * Used internally by database endpoints when the ReturnString parameter is true.
 *
 * This callback is simpler than UFCallback<T> as it doesn't perform JSON deserialization.
 * Instead, it passes the raw JSON string to the callback function, allowing the caller
 * to handle parsing manually if needed.
 *
 * Callback Function Signature:
 *   void MyCallback(int cid, int status, string oid, string jsonData)
 *
 * Parameters:
 *   - cid: The call identifier
 *   - status: Status code (UF_SUCCESS, UF_ERROR, UF_TIMEOUT, etc.)
 *   - oid: Object identifier (OID) provided when the call was made
 *   - jsonData: Raw JSON response string
 *
 * @see UFCallback for typed callback that auto-deserializes JSON
 */
class UDBCallBack : UFRestCallBackBase
{
	protected Class Instance;
	protected string Function;
	protected string OID;
	protected int CallId;

	
	protected Class GetInstance(){
		return Instance;
	}
	
	/**
	 * Constructor
	 *
	 * @param instance The object instance that contains the callback function
	 * @param function The name of the callback function to invoke
	 * @param id The call identifier for this request
	 * @param oid Object identifier to pass to the callback
	 */
	void UDBCallBack(Class instance, string function, int id, string oid){
		Instance = instance;
		Function = function;
		CallId = id;
		OID = oid;
	}
	
	/**
	 * OnError
	 *
	 * Called when the database request fails with an error.
	 * Checks if the call was canceled before invoking the callback.
	 *
	 * @param errorCode The REST error code (ERestResultState)
	 */
	override void OnError(int errorCode) {
		if (UF().IsCallCanceled(CallId)){
			UFLog.Debug("Call " + CallId + " not called as it was requested to be canceled - OnError " + UF().ErrorToString(errorCode));
			return;
		}
		int rstatus = UF_SERVERERROR;
		if (errorCode == ERestResultState.EREST_ERROR_CLIENTERROR){
			rstatus = UF_CLIENTERROR;
		}
		if (GetInstance() && Function != ""){
			g_Game.GameScript.CallFunctionParams(GetInstance(), Function, NULL, new Param4<int, int, string, string>(CallId, rstatus, OID, "{}"));
		}
	};
	
	/**
	 * OnTimeout
	 *
	 * Called when the database request times out.
	 * Checks if the call was canceled before invoking the callback.
	 */
	override void OnTimeout() {
		if (UF().IsCallCanceled(CallId)){
			UFLog.Debug("Call " + CallId + " not called as it was requested to be canceled - OnTimeout");
			return;
		}
		if (GetInstance() && Function != ""){
			g_Game.GameScript.CallFunctionParams(GetInstance(), Function, NULL, new Param4<int, int, string, string>(CallId, UF_TIMEOUT, OID, "{}"));
		}
	};
	
	/**
	 * OnSuccess
	 *
	 * Called when the database request succeeds.
	 * Checks if the call was canceled, then passes the raw JSON data to the callback.
	 * Returns UF_EMPTY if the response is empty (no data).
	 *
	 * @param data The raw JSON response string
	 * @param dataSize Size of the response data
	 */
	override void OnSuccess(string data, int dataSize) {
		if (UF().IsCallCanceled(CallId)){
			UFLog.Debug("Call " + CallId + " not called as it was requested to be canceled - OnSuccess");
			return;
		}
		int rstatus = UF_SUCCESS;
		if (data == "{}" || data == "" || data == "{ }"){
			rstatus = UF_EMPTY;
		}
		if (GetInstance() && Function != ""){
			g_Game.GameScript.CallFunctionParams(GetInstance(), Function, NULL, new Param4<int, int, string, string>(CallId, rstatus, OID, data));
		}
	};
};
