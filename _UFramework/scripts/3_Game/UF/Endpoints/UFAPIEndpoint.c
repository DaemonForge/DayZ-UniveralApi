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
		
	//Replacing ServerQuery Runs a Steam Query for a server returning a `UFServerStatus` object
	int SteamQuery(string ip, string queryPort, Class cbInstance, string cbFunction, string oid = "", bool ReturnString = false){
		int cid = -1;
		string endpoint = "ServerQuery/Status/" + ip + "/" + queryPort;
		
		autoptr UFRestCallBackBase DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, oid);
		} else if (cbInstance && cbFunction != ""){
			DBCBX = new UDBNestedCallBack(new UFCallback<UFServerStatus>(cbInstance, cbFunction, oid));
		}
		
		if (  ip && ip != "" && queryPort && queryPort != "" && DBCBX){
			Post(endpoint,"{}", U().RegisterCall(DBCBX, cid));
		} else {
			Print("[UF] [Api] Error ServerQuery IP:" +  ip + " Port:" + queryPort);
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
			Print("[UF] [Api] Error ServerQuery IP:" +  ip + " Port:" + queryPort);
			cid = -1;
		}
		return cid;
	}
	
	//Runs a Steam Query for a server returning a `UFServerStatus` object
	int ServerQueryObj(string ip, string queryPort, Class cbInstance, string cbFunction, string oid = ""){
		int cid = -1;
		string endpoint = "ServerQuery/Status/" + ip + "/" + queryPort;
		
		if (  ip && ip != "" && queryPort && queryPort != "" ){
			Post(endpoint,"{}",U().RegisterCall(new UDBNestedCallBack(new UFCallback<UFServerStatus>(cbInstance, cbFunction, oid)), cid));
		} else {
			Print("[UF] [Api] Error ServerQuery IP:" +  ip + " Port:" + queryPort);
			cid = -1;
		}
		return cid;
	}
	
	//Get a array of random numbers from  returns `URandomNumberResponse`
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
			DBCBX = new UDBNestedCallBack(new UFCallback<URandomNumberResponse>(cbInstance, cbFunction, oid));
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
			DBCBX = new UDBNestedCallBack(new UFCallback<UCryptoConvertResult>(cbInstance, cbFunction, oid));
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
			DBCBX = new UDBNestedCallBack(new UFCallback<UCryptoConvertResult>(cbInstance, cbFunction, oid));
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
			DBCBX = new UDBNestedCallBack(new UFCallback<UCryptoResults>(cbInstance, cbFunction, oid));
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
	
	//Request a status check from the api so you can get version number and such returns a `UFStatus` object
	int Status(Class cbInstance, string cbFunction, string oid = "", bool ReturnString = false){
		int cid = -1;
		if (ReturnString){	
			Post("Status", "{}", new UDBCallBack(cbInstance, cbFunction, cid, oid));
		} else {
			Post("Status", "{}",  U().RegisterCall(new UDBNestedCallBack(new UFCallback<UFStatus>(cbInstance, cbFunction, oid)), cid));
		}
		return cid;
	}
}