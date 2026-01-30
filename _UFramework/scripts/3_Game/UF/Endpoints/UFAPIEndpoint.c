/**
 * Class: UApiEndpoint
 * -------------------
 * A subclass of UFBaseEndpoint that defines several methods for interacting with
 * a server API. This class supports various endpoints including Steam server queries,
 * random number generation, cryptocurrency pricing and conversion, as well as API status checks.
 *
 * Methods:
 *
 * SteamQuery(string ip, string queryPort, Class cbInstance, string cbFunction, string oid = "", bool ReturnString = false)
 * -----------------------------------------------------------------------------------------------
 * Executes a Steam-based server query which returns a UFServerStatus object.
 *
 * Parameters:
 *   ip          - The IP address of the server.
 *   queryPort   - The query port as a string.
 *   cbInstance  - Callback instance to be notified upon query completion.
 *   cbFunction  - Name of the callback function.
 *   oid         - (Optional) An object identifier to help track the callback.
 *   ReturnString- (Optional) If true, the callback will return a string response.
 *
 * Returns:
 *   An integer callback id (cid). Returns -1 if there is an error (e.g., invalid parameters).
 *
 *
 * ServerQuery(string ip, string queryPort, Class cbInstance, string cbFunction, string oid = "")
 * -----------------------------------------------------------------------------------------------
 * Executes a server query to return a UFServerStatus object using a direct callback mechanism.
 * (Note: This method is marked "To Be removed".)
 *
 * Parameters:
 *   ip         - The IP address of the server.
 *   queryPort  - The query port as a string.
 *   cbInstance - Callback instance to be notified upon query completion.
 *   cbFunction - Name of the callback function.
 *   oid        - (Optional) An object identifier to help track the callback.
 *
 * Returns:
 *   An integer callback id (cid). Returns -1 if there is an error.
 *
 *
 * ServerQueryObj(string ip, string queryPort, Class cbInstance, string cbFunction, string oid = "")
 * -----------------------------------------------------------------------------------------------
 * Executes a server query and utilizes a nested callback structure to return a UFServerStatus object.
 *
 * Parameters:
 *   ip         - The IP address of the server.
 *   queryPort  - The query port as a string.
 *   cbInstance - Callback instance to be notified upon query completion.
 *   cbFunction - Name of the callback function.
 *   oid        - (Optional) An object identifier to help track the callback.
 *
 * Returns:
 *   An integer callback id (cid). Returns -1 if there is an error.
 *
 *
 * RandomNumbers(int count, Class cbInstance, string cbFunction, string oid = "", bool ReturnString = false)
 * -----------------------------------------------------------------------------------------------
 * Requests an array of random numbers and returns a URandomNumberResponse.
 *
 * Parameters:
 *   count       - The number of random numbers requested. Defaults to 4096 if -1.
 *   cbInstance  - Callback instance to be notified upon completion.
 *   cbFunction  - Name of the callback function.
 *   oid         - (Optional) An object identifier for the callback.
 *   ReturnString- (Optional) If true, the response is returned as a string.
 *
 * Returns:
 *   An integer callback id (cid). Returns -1 if there is an error (e.g., count is out of bounds).
 *
 *
 * CryptoPrice(string from, string to, Class cbInstance, string cbFunction, string oid = "", bool ReturnString = false)
 * -----------------------------------------------------------------------------------------------
 * Retrieves the market price for a cryptocurrency conversion and returns a UCryptoConvertResult.
 *
 * Parameters:
 *   from        - The source cryptocurrency.
 *   to          - The target cryptocurrency.
 *   cbInstance  - Callback instance to be notified upon query completion.
 *   cbFunction  - Name of the callback function.
 *   oid         - (Optional) An object identifier for the callback.
 *   ReturnString- (Optional) If true, the response is returned as a string.
 *
 * Returns:
 *   An integer callback id (cid). Returns -1 if there is an error.
 *
 *
 * CryptoConvert(string from, string to, float value, Class cbInstance, string cbFunction, string oid = "", bool ReturnString = false)
 * -----------------------------------------------------------------------------------------------
 * Converts a specified value from one cryptocurrency to another, returning a UCryptoConvertResult.
 *
 * Parameters:
 *   from        - The source cryptocurrency.
 *   to          - The target cryptocurrency.
 *   value       - The amount to be converted (must be greater than 0).
 *   cbInstance  - Callback instance to handle the conversion result.
 *   cbFunction  - Name of the callback function.
 *   oid         - (Optional) An object identifier for the callback.
 *   ReturnString- (Optional) If true, the response is returned as a string.
 *
 * Returns:
 *   An integer callback id (cid). Returns -1 if there is an error.
 *
 *
 * Crypto(TStringArray from, string to, Class cbInstance, string cbFunction, string oid = "", bool ReturnString = false)
 * -----------------------------------------------------------------------------------------------
 * Retrieves a map of live cryptocurrency market prices, returning a UCryptoResults object.
 *
 * Parameters:
 *   from        - An array of source cryptocurrencies.
 *   to          - The target cryptocurrency.
 *   cbInstance  - Callback instance to handle the market prices response.
 *   cbFunction  - Name of the callback function.
 *   oid         - (Optional) An object identifier for the callback.
 *   ReturnString- (Optional) If true, the response is returned as a string.
 *
 * Returns:
 *   An integer callback id (cid). Returns -1 if there is an error (e.g., empty source array or invalid target).
 *
 *
 * Status(Class cbInstance, string cbFunction, string oid = "", bool ReturnString = false)
 * -----------------------------------------------------------------------------------------------
 * Retrieves the API status details, including version number and other metadata, returning a UFStatus object.
 *
 * Parameters:
 *   cbInstance  - Callback instance to be notified upon retrieval of the status.
 *   cbFunction  - Name of the callback function.
 *   oid         - (Optional) An object identifier for the callback.
 *   ReturnString- (Optional) If true, the response is returned as a string.
 *
 * Returns:
 *   An integer callback id (cid). Returns -1 if there is an error.
 */
class UApiEndpoint extends UFBaseEndpoint {
	
	/**
	 * Queries a game server via Steam query protocol.
	 * 
	 * @param ip Server IP address
	 * @param queryPort Server query port
	 * @param cbInstance Object to call callback on
	 * @param cbFunction Callback method name
	 * @param oid Optional object ID for callback context
	 * @param ReturnString If true, returns raw string instead of UFServerStatus object
	 * @return Call ID or -1 on error
	 * 
	 * @usage U().api().SteamQuery("127.0.0.1", "27016", this, "OnServerStatus");
	 * @note Callback signature: void OnServerStatus(int cid, int status, string oid, UFServerStatus data)
	 */
	int SteamQuery(string ip, string queryPort, Class cbInstance, string cbFunction, string oid = "", bool ReturnString = false){
		int cid = -1;
		string endpoint = "ServerQuery/Status/" + ip + "/" + queryPort;
		
		autoptr UFRestCallBackBase DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, oid);
		} else if (cbInstance && cbFunction != ""){
			DBCBX = new UNestedCallBack(new UFCallback<UFServerStatus>(cbInstance, cbFunction, oid));
		}
		
		if (  ip && ip != "" && queryPort && queryPort != "" && DBCBX){
			Post(endpoint,"{}", U().RegisterCall(DBCBX, cid));
		} else {
			UFLog.Err("[Api] Error ServerQuery IP:" + ip + " Port:" + queryPort);
			cid = -1;
		}
		return cid;
	}
	
	//To Be removed
	//Runs a Steam Query for a server returning a `UFServerStatus` object
	int ServerQuery(string ip, string queryPort, Class cbInstance, string cbFunction, string oid = ""){
		int cid = -1;
		string endpoint = "ServerQuery/Status/" + ip + "/" + queryPort;
		
		autoptr UFRestCallBackBase DBCBX;
		if (cbInstance && cbFunction != ""){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, oid);
		} else {
			DBCBX = new USilentCallBack();
		}
		
		
		if (  ip && ip != "" && queryPort && queryPort != "" && DBCBX){
			Post(endpoint,"{}",U().RegisterCall(DBCBX, cid));
		} else {
			UFLog.Err("[Api] Error ServerQuery IP:" + ip + " Port:" + queryPort);
			cid = -1;
		}
		return cid;
	}
	
	//Runs a Steam Query for a server returning a `UFServerStatus` object
	int ServerQueryObj(string ip, string queryPort, Class cbInstance, string cbFunction, string oid = ""){
		int cid = -1;
		string endpoint = "ServerQuery/Status/" + ip + "/" + queryPort;
		
		if (  ip && ip != "" && queryPort && queryPort != "" ){
			Post(endpoint,"{}",U().RegisterCall(new UNestedCallBack(new UFCallback<UFServerStatus>(cbInstance, cbFunction, oid)), cid));
		} else {
			UFLog.Err("[Api] Error ServerQuery IP:" + ip + " Port:" + queryPort);
			cid = -1;
		}
		return cid;
	}
	
	/**
	 * Requests quantum random numbers from the API.
	 * 
	 * @param count Number of random numbers (1-4096, or -1 for default 4096)
	 * @param cbInstance Object to call callback on
	 * @param cbFunction Callback method name
	 * @param oid Optional object ID for callback context
	 * @param ReturnString If true, returns raw string instead of URandomNumberResponse
	 * @return Call ID or -1 on error
	 * 
	 * @usage U().api().RandomNumbers(1000, this, "OnRandoms");
	 * @note Callback signature: void OnRandoms(int cid, int status, string oid, URandomNumberResponse data)
	 */
	int RandomNumbers(int count, Class cbInstance, string cbFunction, string oid = "", bool ReturnString = false){
		int cid = -1;
		string endpoint = "Random";
		if (count == -1){
			count = 4096;
		}
		autoptr UFRestCallBackBase DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, oid);
		} else if (cbInstance && cbFunction != ""){
			DBCBX = new UNestedCallBack(new UFCallback<URandomNumberResponse>(cbInstance, cbFunction, oid));
		}
		
		autoptr URandomNumberRequest randomreq = new URandomNumberRequest(count);
		
		if (  count > 0 && count <= 4096 && randomreq && DBCBX){
			Post(endpoint, randomreq.ToJson(), U().RegisterCall(DBCBX, cid));
		} else {
			Error2("[UF] [Api] Error Random", "Count: " +  count + " CID:" + cid);
			cid = -1;
		}
		return cid;
	}
	
	
	//Gets the value of the set value amount market prices for Crypto currencys `UCryptoConvertResult`
	int CryptoPrice(string from, string to, Class cbInstance, string cbFunction, string oid = "", bool ReturnString = false){
		int cid = -1;
		string endpoint = "Crypto/Price/" + from + "/" + to;
		autoptr UFRestCallBackBase DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, oid);
		} else if (cbInstance && cbFunction != "") {
			DBCBX = new UNestedCallBack(new UFCallback<UCryptoConvertResult>(cbInstance, cbFunction, oid));
		}
		
		if ( from && to && DBCBX){
			Post(endpoint, "{}", U().RegisterCall(DBCBX, cid));
		} else {
			Error2("[UF] [Api] Error Crypto Price", "From: " +  from + " To: " +  to + " CID:" + cid);
			cid = -1;
		}
		return cid;
	}
	
	//Gets the value of the set value amount market prices for Crypto currencys `UCryptoConvertResult`
	int CryptoConvert(string from, string to, float value, Class cbInstance, string cbFunction, string oid = "", bool ReturnString = false){
		int cid = -1;
		string endpoint = "Crypto/Convert/" + from + "/" + to;
		autoptr UFRestCallBackBase DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, oid);
		} else if (cbInstance && cbFunction != "") {
			DBCBX = new UNestedCallBack(new UFCallback<UCryptoConvertResult>(cbInstance, cbFunction, oid));
		}
		
		autoptr UCryptoConvertRequest req = new UCryptoConvertRequest(value);
		
		if ( from && to && value > 0 && DBCBX){
			Post(endpoint, req.ToJson(), U().RegisterCall(DBCBX, cid));
		} else {
			Error2("[UF] [Api] Error Crypto Convert", "From: " +  from + " To: " +  to + " Value: " + value + " CID:" + cid);
			cid = -1;
		}
		return cid;
	}
	
	//Gets a map of live market prices for Crypto currencys `UCryptoResults`
	int Crypto(TStringArray from, string to, Class cbInstance, string cbFunction, string oid = "", bool ReturnString = false){
		int cid = -1;
		string endpoint = "Crypto/" + to;
		autoptr UFRestCallBackBase DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, oid);
		} else if (cbInstance && cbFunction != "") {
			DBCBX = new UNestedCallBack(new UFCallback<UCryptoResults>(cbInstance, cbFunction, oid));
		}
		
		autoptr UCryptoRequest req = new UCryptoRequest(from);
		
		if ( from && from.Count() > 0 && to && DBCBX){
			Post(endpoint, req.ToJson(), U().RegisterCall(DBCBX, cid));
		} else {
			Error2("[UF] [Api] Error Crypto", "From: " +  from.Count() + " To: " +  to + " CID:" + cid);
			cid = -1;
		}
		return cid;
	}
	
	//Generates a TTS audio, taking the UTTSMessage Object and Voice ID 
	//Constants in: UTTSVoice
	//voiceID: alloy, ash, ballad, coral, echo, fable, onyx, nova, sage, shimmer, and verse
	int TTSGenerate(string voiceID, UTTSMessage msg, Class cbInstance, string cbFunction  ){
		int cid = -1;
		string endpoint = "TTS/Generate/" + voiceID;
		
		if (voiceID != "" && msg){
			Post(endpoint, msg.ToJson(), U().RegisterCall(new UNestedCallBack(new UGenTTSCallback(cbInstance, cbFunction, voiceID)), cid));
		} else {
			Error2("[UF] [Api] TTSGenerate - Play Audio", " voiceID: " +  voiceID);
			cid = -1;
		}
		return cid;
	}
	
	int TTSStatus(string ttsId, Class cbInstance, string cbFunction ){
		int cid = -1;
		string endpoint = "TTS/Status/" + ttsId;
		
		if (ttsId != ""){
			Post(endpoint, "{}", U().RegisterCall(new UNestedCallBack(new UTTSStatusCallback(cbInstance, cbFunction, ttsId)), cid));
		} else {
			Error2("[UF] [Api] TTSStatus", " ttsId: " +  ttsId);
			cid = -1;
		}
		return cid;
	}
	
	//Downloads an Audio file
	int TTSDownload(string ttsId){
		if (g_Game.IsDedicatedServer()) {
			Error2("[UF] TTSDownload Called from Server", " TTSid: " + ttsId);
			return -1;
		}
		int cid = -1;
		string endpoint = "TTS/Download/" + ttsId;
		
		if (ttsId != ""){
			Post(endpoint, "{}", U().RegisterCall(new UFDownloadTTS(ttsId), cid));
		} else {
			Error2("[UF] [Api] TTSDownload - Play Audio", " ttsId: " +  ttsId);
			cid = -1;
		}
		return cid;
	}
	
	//Downloads and Calls back when complete
	int TTSDownload(string ttsId, Class cbInstance, string cbFunction){
		if (g_Game.IsDedicatedServer()) {
			Error2("[UF] TTSDownload Called from Server", " TTSid: " + ttsId);
			return -1;
		}
		int cid = -1;
		string endpoint = "TTS/Download/" + ttsId;
		
		if (ttsId != ""){
			Post(endpoint, "{}", U().RegisterCall(new UDLTTSNestedCallback(new UDLTTSCallback(cbInstance, cbFunction, ttsId)), cid));
		} else {
			Error2("[UF] [Api] TTSDownload - Play Audio", " ttsId: " +  ttsId);
			cid = -1;
		}
		return cid;
	}
	
	int TTSPlay(string ttsId){
		if (g_Game.IsDedicatedServer()) {
			Error2("[UF] PlayTTS Called from Server", " TTSid: " + ttsId);
			return -1;
		}
		if (!GetUFVideoPlayer()){
			Error2("[UF] PlayTTS Called But GetUFVideoPlayer is null", " TTSid: " + ttsId);
			return -1;
		}
		if (FileExist("$saves:" + ttsId + ".mp4")){
			UFLog.Debug("PlayTTS ttsId already downloaded");
			GetUFVideoPlayer().LoadAndPlay(ttsId, true);
			return -1;
		}
		return TTSDownload(ttsId, GetUFVideoPlayer(), "UCBHandlePlay");
	}
	
	/**
	 * Gets API service status, version, and capabilities.
	 * 
	 * @param cbInstance Object to call callback on
	 * @param cbFunction Callback method name
	 * @param oid Optional object ID for callback context
	 * @param ReturnString If true, returns raw string instead of UFStatus object
	 * @return Call ID or -1 on error
	 * 
	 * @usage U().api().Status(this, "OnStatus");
	 * @note Callback signature: void OnStatus(int cid, int status, string oid, UFStatus data)
	 * @note UFStatus contains version, Discord/OpenAI availability, error status
	 */
	int Status(Class cbInstance, string cbFunction, string oid = "", bool ReturnString = false){
		UFLog.Debug("[UApiEndpoint::Status] Called with cbFunction=" + cbFunction);
		int cid = -1;
		
		// Pre-check: ensure we can actually make the call
		UFrameworkConfig cfg = UFConfig();
		if (!cfg){
			UFLog.Err("[UApiEndpoint::Status] UFConfig() is NULL - cannot make API call!");
			return -1;
		}
		
		UFramework uf = U();
		if (!uf){
			UFLog.Err("[UApiEndpoint::Status] U() is NULL - framework not initialized!");
			return -1;
		}
		
		UFLog.Debug("[UApiEndpoint::Status] Pre-checks passed, making POST request...");
		if (ReturnString){	
			Post("Status", "{}", new UDBCallBack(cbInstance, cbFunction, cid, oid));
		} else {
			RestCallback cb = uf.RegisterCall(new UNestedCallBack(new UFCallback<UFStatus>(cbInstance, cbFunction, oid)), cid);
			if (!cb){
				UFLog.Err("[UApiEndpoint::Status] RegisterCall returned NULL!");
				return -1;
			}
			Post("Status", "{}", cb);
		}
		UFLog.Debug("[UApiEndpoint::Status] Request sent, cid=" + cid);
		return cid;
	}
}