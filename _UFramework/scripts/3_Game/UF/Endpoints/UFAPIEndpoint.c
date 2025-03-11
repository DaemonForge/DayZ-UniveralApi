class UApiEndpoint extends UFBaseEndpoint {
		
	//Replacing ServerQuery Runs a Steam Query for a server returning a `UFServerStatus` object
	int SteamQuery(string ip, string queryPort, Class cbInstance, string cbFunction, string oid = "", bool ReturnString = false){
		int cid = U().CallId();
		string endpoint = "ServerQuery/Status/" + ip + "/" + queryPort;
		
		autoptr RestCallback DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, oid);
		} else if (cbInstance && cbFunction != ""){
			DBCBX = new UDBNestedCallBack(new UFCallback<UFServerStatus>(cbInstance, cbFunction, oid), cid);
		}
		
		if (  ip && ip != "" && queryPort && queryPort != "" && DBCBX){
			Post(endpoint,"{}",DBCBX);
		} else {
			Print("[UF] [Api] Error ServerQuery IP:" +  ip + " Port:" + queryPort);
			cid = -1;
		}
		return cid;
	}
	
	//To Be removed
	//Runs a Steam Query for a server returning a `UFServerStatus` object
	int ServerQuery(string ip, string queryPort, Class cbInstance, string cbFunction, string oid = ""){
		int cid = U().CallId();
		string endpoint = "ServerQuery/Status/" + ip + "/" + queryPort;
		
		autoptr RestCallback DBCBX;
		if (cbInstance && cbFunction != ""){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, oid);
		} else {
			DBCBX = new USilentCallBack();
		}
		
		
		if (  ip && ip != "" && queryPort && queryPort != "" && DBCBX){
			Post(endpoint,"{}",DBCBX);
		} else {
			Print("[UF] [Api] Error ServerQuery IP:" +  ip + " Port:" + queryPort);
			cid = -1;
		}
		return cid;
	}
	
	//Runs a Steam Query for a server returning a `UFServerStatus` object
	int ServerQueryObj(string ip, string queryPort, Class cbInstance, string cbFunction, string oid = ""){
		int cid = U().CallId();
		string endpoint = "ServerQuery/Status/" + ip + "/" + queryPort;
		
		if (  ip && ip != "" && queryPort && queryPort != "" ){
			Post(endpoint,"{}",new UDBNestedCallBack(new UFCallback<UFServerStatus>(cbInstance, cbFunction, oid), cid));
		} else {
			Print("[UF] [Api] Error ServerQuery IP:" +  ip + " Port:" + queryPort);
			cid = -1;
		}
		return cid;
	}
	
	//Get a array of random numbers from  returns `URandomNumberResponse`
	int RandomNumbers(int count, Class cbInstance, string cbFunction, string oid = "", bool ReturnString = false){
		int cid = U().CallId();
		string endpoint = "Random";
		if (count == -1){
			count = 4096;
		}
		autoptr RestCallback DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, oid);
		} else if (cbInstance && cbFunction != ""){
			DBCBX = new UDBNestedCallBack(new UFCallback<URandomNumberResponse>(cbInstance, cbFunction, oid), cid);
		}
		
		autoptr URandomNumberRequest randomreq = new URandomNumberRequest(count);
		
		if (  count > 0 && count <= 4096 && randomreq && DBCBX){
			Post(endpoint, randomreq.ToJson(), DBCBX);
		} else {
			Error2("[UF] [Api] Error Random", "Count: " +  count + " CID:" + cid);
			cid = -1;
		}
		return cid;
	}
	
	
	//Gets the value of the set value amount market prices for Crypto currencys `UCryptoConvertResult`
	int CryptoPrice(string from, string to, Class cbInstance, string cbFunction, string oid = "", bool ReturnString = false){
		int cid = U().CallId();
		string endpoint = "Crypto/Price/" + from + "/" + to;
		autoptr RestCallback DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, oid);
		} else if (cbInstance && cbFunction != "") {
			DBCBX = new UDBNestedCallBack(new UFCallback<UCryptoConvertResult>(cbInstance, cbFunction, oid), cid);
		}
		
		if ( from && to && DBCBX){
			Post(endpoint, "{}", DBCBX);
		} else {
			Error2("[UF] [Api] Error Crypto Price", "From: " +  from + " To: " +  to + " CID:" + cid);
			cid = -1;
		}
		return cid;
	}
	
	//Gets the value of the set value amount market prices for Crypto currencys `UCryptoConvertResult`
	int CryptoConvert(string from, string to, float value, Class cbInstance, string cbFunction, string oid = "", bool ReturnString = false){
		int cid = U().CallId();
		string endpoint = "Crypto/Convert/" + from + "/" + to;
		autoptr RestCallback DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, oid);
		} else if (cbInstance && cbFunction != "") {
			DBCBX = new UDBNestedCallBack(new UFCallback<UCryptoConvertResult>(cbInstance, cbFunction, oid), cid);
		}
		
		autoptr UCryptoConvertRequest req = new UCryptoConvertRequest(value);
		
		if ( from && to && value > 0 && DBCBX){
			Post(endpoint, req.ToJson(), DBCBX);
		} else {
			Error2("[UF] [Api] Error Crypto Convert", "From: " +  from + " To: " +  to + " Value: " + value + " CID:" + cid);
			cid = -1;
		}
		return cid;
	}
	
	//Gets a map of live market prices for Crypto currencys `UCryptoResults`
	int Crypto(TStringArray from, string to, Class cbInstance, string cbFunction, string oid = "", bool ReturnString = false){
		int cid = U().CallId();
		string endpoint = "Crypto/" + to;
		autoptr RestCallback DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, oid);
		} else if (cbInstance && cbFunction != "") {
			DBCBX = new UDBNestedCallBack(new UFCallback<UCryptoResults>(cbInstance, cbFunction, oid), cid);
		}
		
		autoptr UCryptoRequest req = new UCryptoRequest(from);
		
		if ( from && from.Count() > 0 && to && DBCBX){
			Post(endpoint, req.ToJson(), DBCBX);
		} else {
			Error2("[UF] [Api] Error Crypto", "From: " +  from.Count() + " To: " +  to + " CID:" + cid);
			cid = -1;
		}
		return cid;
	}
	
	//Request a status check from the api so you can get version number and such returns a `UFStatus` object
	int Status(Class cbInstance, string cbFunction, string oid = "", bool ReturnString = false){
		int cid = U().CallId();
		if (ReturnString){	
			Post("Status", "{}", new UDBCallBack(cbInstance, cbFunction, cid, oid));
		} else {
			Post("Status", "{}",  new UDBNestedCallBack(new UFCallback<UFStatus>(cbInstance, cbFunction, oid), cid));
		}
		return cid;
	}
}