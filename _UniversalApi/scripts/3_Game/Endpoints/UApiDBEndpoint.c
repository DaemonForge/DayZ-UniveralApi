class UApiDBEndpoint extends UApiBaseEndpoint {
		
	protected string m_Collection = "Object";
	
	void UApiDBEndpoint(string collection){
		m_Collection = collection;
	}
	
	override protected string EndpointBaseUrl(){
		return UApiConfig().GetBaseURL() + m_Collection + "/";
	}
	
	int Save(string mod, string oid, string jsonString) {	
		int cid = UApi().CallId();	
		string endpoint = "/Save/" + oid + "/" + mod;
		if (mod && oid && jsonString){
			Post(endpoint,jsonString,new UApiSilentCallBack());
		} else {
			Print("[UAPI] [Api] Error Saving " + endpoint + " Data for " + mod);
			cid = -1;
		}
		return cid;
	}
	
	int Save(string mod, string oid, string jsonString, Class instance, string function) {	
		int cid = UApi().CallId();	
		string endpoint = "/Save/" + oid + "/" + mod;

		if (mod && oid && jsonString){
			Post(endpoint,jsonString, new UApiDBCallBack(instance, function, cid, oid));
		} else {
			Print("[UAPI] [Api] Error Saving " + endpoint + " Data for " + mod);
			cid = -1;
		}
		return cid;
	}
	
	int Save(string mod, string oid, string jsonString, UApiCallbackBase cb) {	
		int cid = UApi().CallId();	
		string endpoint = "/Save/" + oid + "/" + mod;
		if (mod && oid && jsonString && cb){
			cb.SetOID(oid); //Only sets if not set
			Post(endpoint,jsonString, new UApiDBNestedCallBack(cb, cid));
		} else {
			Print("[UAPI] [Api] Error Saving " + endpoint + " Data for " + mod);
			cid = -1;
		}
		return cid;
	}
	
	int Load(string mod, string oid, UApiCallbackBase cb, string jsonString = "{}") {		
		int cid = UApi().CallId();
		string endpoint = "/Load/" + oid + "/" + mod;
		
		
		if (mod && oid && cb && jsonString ){
			cb.SetOID(oid); //Only sets if not set
			Post(endpoint,jsonString,new UApiDBNestedCallBack(cb, cid));
		} else {
			Print("[UAPI] [Api] Error Loading Player Data for " + mod);
			cid = -1;
		}
		return cid;
	}
	
	int Load(string mod, string oid, Class instance, string function, string jsonString = "{}") {		
		int cid = UApi().CallId();
		string endpoint = "/Load/" + oid + "/" + mod;
		
		if (mod && oid && jsonString){
			Post(endpoint,jsonString, new UApiDBCallBack(instance, function, cid, oid));
		} else {
			Print("[UAPI] [Api] Error Loading Player Data for " + mod);
			cid = -1;
		}
		return cid;
	}
	
	
	int Query(string mod, UApiQueryBase query, UApiCallbackBase cb) {
		int cid = UApi().CallId();
		string endpoint = "/Query/" + mod;
				
		if ( query && mod && cb){
			cb.SetOID(mod); //Only sets if not set
			Post(endpoint,query.ToJson(), new UApiDBNestedCallBack(cb, cid));
		} else {
			Print("[UAPI] [Api] Error Querying " +  mod);
			cid = -1;
		}
		return cid;
	}
	
	int Query(string mod, UApiQueryBase query, Class instance, string function) {
		int cid = UApi().CallId();
		string endpoint = "/Query/" + mod;
				
		if (mod && query){
			Post(endpoint,query.ToJson(),new UApiDBCallBack(instance, function, cid, ""));
		} else {
			Print("[UAPI] [Api] Error Querying " +  mod);
			cid = -1;
		}
		return cid;
	}
	
	int Increment(string mod, string oid, string element, float value = 1){
		return Transaction(mod, oid, element, value);
	}
	
	int Transaction(string mod, string oid, string element, float value) {
		int cid = UApi().CallId();
				
		string endpoint = "/Transaction/" + oid   + "/"+ mod;
		
		autoptr UApiTransaction transaction = new UApiTransaction(element, value);
		
		if (mod && oid && element){
			Post(endpoint,transaction.ToJson(), new UApiSilentCallBack());
		} else {
			Print("[UAPI] [Api] Error Transaction " +  mod);
			cid = -1;
		}
		return cid;
	}
	
	int Transaction(string mod, string oid, string element, float value, UApiCallbackBase cb) {
		int cid = UApi().CallId();
		
		string endpoint = "/Transaction/" + oid   + "/"+ mod;
		
		autoptr UApiTransaction transaction = new UApiTransaction(element, value);
		
		if ( mod && cb && oid && element && transaction){
			cb.SetOID(oid); //Only sets if not set
			Post(endpoint,transaction.ToJson(), new UApiDBNestedCallBack(cb, cid));
		} else {
			Print("[UAPI] [Api] Error Transaction " +  mod);
			cid = -1;
		}
		return cid;
	}
	
	int Transaction(string mod, string oid, string element, float value, Class instance, string function) {
		int cid = UApi().CallId();
		
		autoptr RestCallback DBCBX = ;
		
		string endpoint = "/Transaction/" + oid   + "/"+ mod;
		
		autoptr UApiTransaction transaction = new UApiTransaction(element, value);
		
		if (mod && oid && element && transaction){
			Post(endpoint,transaction.ToJson(), new UApiDBCallBack(instance, function, cid, oid));
		} else {
			Print("[UAPI] [Api] Error Transaction " +  mod);
			cid = -1;
		}
		return cid;
	}
	
	int Update(string mod, string oid, string element, string value, string operation = UpdateOpts.SET) {	
		int cid = UApi().CallId();
		
		string endpoint = "/Update/" + oid   + "/"+ mod;
		
		autoptr UApiUpdateData updatedata = new UApiUpdateData(element, value, operation);
		
		if ( mod && element && operation && updatedata){
			Post(endpoint, updatedata.ToJson(), new UApiSilentCallBack());
		} else {
			Print("[UAPI] [Api] Error Transaction " +  mod);
			cid = -1;
		}
		return cid;
	}
		
	int Update(string mod, string oid, string element, string value, string operation, UApiCallbackBase cb) {	
		int cid = UApi().CallId();

		string endpoint = "/Update/" + oid   + "/"+ mod;
		
		autoptr UApiUpdateData updatedata = new UApiUpdateData(element, value, operation);
		
		if ( mod && element && updatedata && cb){
			Post(endpoint, updatedata.ToJson(), new UApiDBNestedCallBack(cb, cid));
		} else {
			Print("[UAPI] [Api] Error Transaction " +  mod);
			cid = -1;
		}
		return cid;
	}
	
	int Update(string mod, string oid, string element, string value, string operation, Class instance, string function) {	
		int cid = UApi().CallId();
		autoptr RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDBCallBack(instance, function, cid, oid);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		string endpoint = "/Update/" + oid   + "/"+ mod;
		
		autoptr UApiUpdateData updatedata = new UApiUpdateData(element, value, operation);
		
		if (mod && element && updatedata && DBCBX){
			Post(endpoint, updatedata.ToJson(), DBCBX);
		} else {
			Print("[UAPI] [Api] Error Transaction " +  mod);
			cid = -1;
		}
		return cid;
	}
	
	
	
	//Only Works on Player Data	
	int PublicSave(string mod, string oid, string jsonString, Class instance = NULL, string function = "") {	
		if (m_Collection != "Player") return -1;
		int cid = UApi().CallId();	
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
		if (m_Collection != "Player") return -1;
		int cid = UApi().CallId();
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