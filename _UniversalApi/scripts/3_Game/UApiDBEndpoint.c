class UApiDBEndpoint extends UApiBaseEndpoint {
		
	protected string m_Collection = "Object";
	
	void UApiDBEndpoint(string collection){
		m_Collection = collection;
	}
	
	override protected string EndpointBaseUrl(){
		return UApiConfig().GetBaseURL() + m_Collection + "/";
	}
	
	int Save(string mod, string oid, string jsonString, Class instance = NULL, string function = "") {	
		int cid = UApi().CallId();	
		string endpoint = "/Save/" + oid + "/" + mod;
		autoptr RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDBCallBack(instance, function, cid, oid);
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
	
	int Load(string mod, string oid, Class instance, string function, string jsonString = "{}") {		
		int cid = UApi().CallId();
		string endpoint = "/Load/" + oid + "/" + mod;
		
		autoptr RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDBCallBack(instance, function, cid, oid);
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
	
	int Query(string mod, UApiQueryObject query, Class instance, string function) {
		int cid = UApi().CallId();
		string endpoint = "/Query/" + mod;
		
		autoptr RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDBCallBack(instance, function, cid, "");
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		if ( query && DBCBX){
			Post(endpoint,query.ToJson(),DBCBX);
		} else {
			Print("[UAPI] [Api] Error Querying " +  mod);
			cid = -1;
		}
		return cid;
	}
	
	int Increment(string mod, string oid, string element, float value = 1){
		return Transaction(mod, oid, element, value, NULL, "");
	}
	
	int Transaction(string mod, string oid, string element, float value = 1, Class instance = NULL, string function = "") {
		int cid = UApi().CallId();
		
		autoptr RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDBCallBack(instance, function, cid, oid);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		string endpoint = "/Transaction/" + oid   + "/"+ mod;
		
		autoptr UApiTransaction transaction = new UApiTransaction(element, value);
		
		if ( element && transaction && DBCBX){
			Post(endpoint,transaction.ToJson(),DBCBX);
		} else {
			Print("[UAPI] [Api] Error Transaction " +  mod);
			cid = -1;
		}
		return cid;
	}
	
		
	int Update(string mod, string oid, string element, string value, string operation = "set", Class instance = NULL, string function = "") {	
		int cid = UApi().CallId();
		autoptr RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDBCallBack(instance, function, cid, oid);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		string endpoint = "/Update/" + oid   + "/"+ mod;
		
		autoptr UApiUpdateData updatedata = new UApiUpdateData(element, value, operation);
		
		if ( element && updatedata && DBCBX){
			Post(endpoint, updatedata.ToJson(), DBCBX);
		} else {
			Print("[UAPI] [Api] Error Transaction " +  mod);
			cid = -1;
		}
		return cid;
	}
	
	
	
	//Only Works on Player Data	
	int PublicSave(string mod, string oid, string jsonString, Class instance = NULL, string function = "") {	
		int cid = UApi().CallId();	
		if (m_Collection != "Player") return -1;
		string endpoint = "/PublicSave/" + oid + "/" + mod;
		autoptr RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDBCallBack(instance, function, cid, oid);
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
	
	int PublicLoad(string mod, string oid, Class instance, string function, string jsonString = "{}") {		
		int cid = UApi().CallId();
		if (m_Collection != "Player") return -1;
		string endpoint = "/PublicLoad/" + oid + "/" + mod;
		
		autoptr RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDBCallBack(instance, function, cid, oid);
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

}