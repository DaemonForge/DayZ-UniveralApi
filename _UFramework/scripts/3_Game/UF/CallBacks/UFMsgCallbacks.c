/**
 * Callback for reading string messages from a queue
 * Returns: (cid, status, queueName, TStringArray messages)
 */
class UFMsgStringCallback extends UFCallbackBase{
	
	/**
	 * Called when message read request fails
	 * @param errorCode The error code from REST API
	 * @param cid The call ID
	 */
	override void OnError(int errorCode, int cid) {
		if (GetInstance() && Function != "") {
			g_Game.GameScript.CallFunctionParams(GetInstance(), Function, NULL, new Param4<int, int, string, TStringArray>(cid, errorCode, OID, NULL));
		}
	}
	
	override void OnSuccess(string jsonData, int cid) {
		if (!GetInstance() || Function == ""){
			return;
		}
		
		autoptr UReadMsgString obj;
		if (!UJSONHandler<UReadMsgString>.FromString(jsonData, obj)){
			g_Game.GameScript.CallFunctionParams(GetInstance(), Function, NULL, new Param4<int, int, string, TStringArray>(cid, UF_JSONERROR, OID, NULL));
			return;
		}
		
		int rstatus = ParseStatusCode(obj);
		TStringArray messages = NULL;
		
		// Only return messages if status is success and we have messages
		if (rstatus == UF_SUCCESS && obj.HasMessages()){
			messages = obj.GetMessages();
		} else if (rstatus == UF_SUCCESS) {
			// Status is success but no messages - treat as empty
			rstatus = UF_EMPTY;
			messages = new TStringArray();
		}
		
		g_Game.GameScript.CallFunctionParams(GetInstance(), Function, NULL, new Param4<int, int, string, TStringArray>(cid, rstatus, OID, messages));
	}
}

/**
 * Callback for reading typed messages from a queue
 * Returns: (cid, status, queueName, array<T> messages)
 * @tparam T The type of messages expected
 */
class UFMsgCallback<Class T> extends UFCallbackBase{
	
	/**
	 * Called when message read request fails
	 * @param errorCode The error code from REST API
	 * @param cid The call ID
	 */
	override void OnError(int errorCode, int cid) {
		if (GetInstance() && Function != "") {
			g_Game.GameScript.CallFunctionParams(GetInstance(), Function, NULL, new Param4<int, int, string, array<autoptr T>>(cid, errorCode, OID, NULL));
		}
	}
	
	/**
	 * Called when message read request succeeds
	 * Deserializes JSON and extracts typed messages array
	 * @param jsonData The JSON response string
	 * @param cid The call ID
	 */
	override void OnSuccess(string jsonData, int cid) {
		if (!GetInstance() || Function == ""){
			return;
		}
		
		autoptr UReadMsg<T> obj;
		if (!UJSONHandler<UReadMsg<T>>.FromString(jsonData, obj)){
			g_Game.GameScript.CallFunctionParams(GetInstance(), Function, NULL, new Param4<int, int, string, array<autoptr T>>(cid, UF_JSONERROR, OID, NULL));
			return;
		}
		
		int rstatus = ParseStatusCode(obj);
		array<autoptr T> messages = NULL;
		
		// Only return messages if status is success and we have messages
		if (rstatus == UF_SUCCESS && obj.HasMessages()){
			messages = obj.GetMessages();
		} else if (rstatus == UF_SUCCESS) {
			// Status is success but no messages - treat as empty
			rstatus = UF_EMPTY;
			messages = new array<autoptr T>();
		}
		
		g_Game.GameScript.CallFunctionParams(GetInstance(), Function, NULL, new Param4<int, int, string, array<autoptr T>>(cid, rstatus, OID, messages));
	}
}

/**
 * ParseStatusCode
 * Helper function to parse status code from response object.
 * Centralized to avoid code duplication between callback types.
 * @param sobj The status object from the API response
 * @return int The UF status code (UF_SUCCESS, UF_ERROR, etc.)
 */
static int ParseStatusCode(StatusObject sobj){
	if (!sobj){
		return UF_ERROR;
	}
	
	switch (sobj.Status) {
		case "Success":
			return UF_SUCCESS;
		case "NotFound":
			return UF_NOTFOUND;
		case "Empty":
			return UF_EMPTY;
		case "Error":
			return UF_ERROR;
		case "NoPerms":
			return UF_UNAUTHORIZED;
		case "NoAuth":
			return UF_UNAUTHORIZED;
		case "InvalidAuth":
			return UF_UNAUTHORIZED;
		case "NotSetup":
			return UF_NOTSETUP;
	}
	
	// Default to success if no Status field
	return UF_SUCCESS;
}
