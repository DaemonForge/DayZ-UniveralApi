class UDBGlobalEndpoint extends UFBaseEndpoint {
	
	override protected string EndpointBaseUrl(){
		return UFConfig().GetBaseURL() + "Globals/";
	}
	
	int Save(string mod, string jsonString) {	
		int cid = -1;	
		string endpoint = "/Save/" + mod;
		if (mod && jsonString){
			Post(endpoint,jsonString, U().RegisterCall(new USilentCallBack(), cid));
		} else {
			Print("[UF] [Api] Error Saving " + endpoint + " Data for " + mod);
			cid = -1;
		}
		return cid;
	}
	
	int Save(string mod, string jsonString, Class cbInstance, string cbFunction) {	
		int cid = U().CallId();	
		string endpoint = "/Save/" + mod;		
		if (mod && jsonString){
			Post(endpoint,jsonString, new UDBCallBack(cbInstance, cbFunction, cid, mod));
		} else {
			Print("[UF] [Api] Error Saving " + endpoint + " Data for " + mod);
			cid = -1;
		}
		return cid;
	}
	int Save(string mod, string jsonString, UFCallbackBase cb) {	
		int cid = -1;	
		string endpoint = "/Save/" + mod;

		if (mod && jsonString && cb){
			cb.SetOID(mod); //Only sets if not set
			Post(endpoint,jsonString, U().RegisterCall(new UDBNestedCallBack(cb, cid), cid));
		} else {
			Print("[UF] [Api] Error Saving " + endpoint + " Data for " + mod);
			cid = -1;
		}
		return cid;
	}
	
	int Load(string mod, Class cbInstance, string cbFunction, string jsonString = "{}") {		
		int cid = -1;
		string endpoint = "/Load/" + mod;

		if (mod && jsonString){
			Post(endpoint,jsonString,U().RegisterCall(new UDBCallBack(cbInstance, cbFunction, cid, mod), cid));
		} else {
			Print("[UF] [Api] Error Loading Player Data for " + mod);
			cid = -1;
		}
		return cid;
	}
	
	int Load(string mod, UFCallbackBase cb, string jsonString = "{}") {		
		int cid = -1;
		string endpoint = "/Load/" + mod;
		if (mod && cb && jsonString){
			cb.SetOID(mod); //Only sets if not set
			Post(endpoint,jsonString, U().RegisterCall(new UDBNestedCallBack(cb, cid), cid));
		} else {
			Print("[UF] [Api] Error Loading Player Data for " + mod);
			cid = -1;
		}
		return cid;
	}
	
	int Increment(string mod, string element, float value = 1){
		return Transaction(mod, element, value);
	}
	
	int Transaction(string mod, string element, float value) {
		int cid = -1;
		string endpoint = "/Transaction/" + mod;
		
		autoptr UDBTransaction transaction = new UDBTransaction(element, value);
		
		if ( element && transaction && mod){
			Post(endpoint,transaction.ToJson(), U().RegisterCall(new USilentCallBack(), cid));
		} else {
			Print("[UF] [Api] Error Transaction " +  mod);
			cid = -1;
		}
		return cid;
	}
	
	int Transaction(string mod, string element, float value, Class cbInstance, string cbFunction) {
		int cid = U().CallId();
		string endpoint = "/Transaction/" + mod;
		
		autoptr UDBTransaction transaction = new UDBTransaction(element, value);
		
		if ( element && transaction && mod){
			Post(endpoint,transaction.ToJson(), new UDBCallBack(cbInstance, cbFunction, cid, mod));
		} else {
			Print("[UF] [Api] Error Transaction " +  mod);
			cid = -1;
		}
		return cid;
	}
	
	int Transaction(string mod, string element, float value, UFCallbackBase cb) {
		int cid = -1;
		string endpoint = "/Transaction/" + mod;
		
		autoptr UDBTransaction transaction = new UDBTransaction(element, value);
		
		if ( element && transaction && mod){
			cb.SetOID(mod); //Only sets if not set
			Post(endpoint,transaction.ToJson(),  U().RegisterCall(new UDBNestedCallBack(cb, cid), cid));
		} else {
			Print("[UF] [Api] Error Transaction " +  mod);
			cid = -1;
		}
		return cid;
	}
	
		
	int Update(string mod, string element, string value, string operation = UpdateOpts.SET, Class cbInstance = NULL, string cbFunction = "") {	
		int cid = U().CallId();
		autoptr UFRestCallBackBase DBCBX;
		if (cbInstance && cbFunction != ""){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, mod);
		} else {
			DBCBX = new USilentCallBack();
		}
		
		string endpoint = "/Update/" + mod;
		
		autoptr UUpdateData updatedata = new UUpdateData(element, value, operation);
		
		if ( element && updatedata && DBCBX){
			Post(endpoint, updatedata.ToJson(), U().RegisterCall(DBCBX, cid));
		} else {
			Print("[UF] [Api] Error Transaction " +  mod);
			cid = -1;
		}
		return cid;
	}

}
