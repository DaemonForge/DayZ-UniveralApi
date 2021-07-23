class UApiDBGlobalEndpoint extends UApiBaseEndpoint {
	
	override protected string EndpointBaseUrl(){
		return UApiConfig().GetBaseURL() + "Globals/";
	}
	
	int Save(string mod, string jsonString, Class instance = NULL, string function = "") {	
		int cid = UApi().CallId();	
		string endpoint = "/Save/" + mod;
		autoptr RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDBCallBack(instance, function, cid, mod);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		if (jsonString){
			Post(endpoint,jsonString,DBCBX);
		} else {
			Print("[UAPI] [Api] Error Saving " + endpoint + " Data for " + mod);
			cid = -1;
		}
		return cid;
	}
	
	int Load(string mod, Class instance, string function, string jsonString = "{}") {		
		int cid = UApi().CallId();
		string endpoint = "/Load/" + mod;
		
		autoptr RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDBCallBack(instance, function, cid, mod);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		if (DBCBX){
			Post(endpoint,jsonString,DBCBX);
		} else {
			Print("[UAPI] [Api] Error Loading Player Data for " + mod);
			cid = -1;
		}
		return cid;
	}
	
	int Increment(string mod, string element, float value = 1){
		return Transaction(mod, element, value, NULL, "");
	}
	
	int Transaction(string mod, string element, float value = 1, Class instance = NULL, string function = "") {
		int cid = UApi().CallId();
		
		autoptr RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDBCallBack(instance, function, cid, mod);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		string endpoint = "/Transaction/" + mod;
		
		autoptr UApiTransaction transaction = new UApiTransaction(element, value);
		
		if ( element && transaction && DBCBX){
			Post(endpoint,transaction.ToJson(),DBCBX);
		} else {
			Print("[UAPI] [Api] Error Transaction " +  mod);
			cid = -1;
		}
		return cid;
	}
	
		
	int Update(string mod, string element, string value, string operation = "set", Class instance = NULL, string function = "") {	
		int cid = UApi().CallId();
		autoptr RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDBCallBack(instance, function, cid, mod);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		string endpoint = "/Update/" + mod;
		
		autoptr UApiUpdateData updatedata = new UApiUpdateData(element, value, operation);
		
		if ( element && updatedata && DBCBX){
			Post(endpoint, updatedata.ToJson(), DBCBX);
		} else {
			Print("[UAPI] [Api] Error Transaction " +  mod);
			cid = -1;
		}
		return cid;
	}

}
