class UDBEndpoint extends UFBaseEndpoint {
		
	protected string m_Collection = "Object";
	
	void UDBEndpoint(string collection){
		m_Collection = collection;
	}
	
	override protected string EndpointBaseUrl(){
		return UFConfig().GetBaseURL() + m_Collection + "/";
	}
	
	int Save(string mod, string oid, string jsonString) {	
		if (mod == "" || oid == "" || jsonString == ""){
			Error2("[UF] Error on DB Save","OID, jsonString and Mod must be valid strings");
			return -1;
		}
		int cid = U().CallId();	
		string endpoint = "Save/" + oid + "/" + mod;
		
		Post(endpoint,jsonString,new USilentCallBack());
		
		return cid;
	}
	
	int Save(string mod, string oid, string jsonString, Class cbInstance, string cbFunction) {	
		if (mod == "" || oid == "" || jsonString == ""){
			Error2("[UF] Error on DB Save","OID, jsonString and Mod must be valid strings");
			return -1;
		}
		int cid = U().CallId();	
		string endpoint = "Save/" + oid + "/" + mod;

		Post(endpoint,jsonString, new UDBCallBack(cbInstance, cbFunction, cid, oid));
		
		return cid;
	}
	
	int Save(string mod, string oid, string jsonString, UFCallbackBase cb) {	
		if (mod == "" || oid == "" || jsonString == "" || !cb){
			Error2("[UF] Error on DB Save","OID and Mod must be valid strings");
			return -1;
		}
		int cid = U().CallId();	
		string endpoint = "Save/" + oid + "/" + mod;
		
		cb.SetOID(oid); //Only sets if not set
		
		Post(endpoint,jsonString, new UDBNestedCallBack(cb, cid));
		
		return cid;
	}
	
	int Load(string mod, string oid, UFCallbackBase cb, string jsonString = "{}") {		
		if (mod == "" || oid == "" || jsonString == "" || !cb){
			Error2("[UF] Error on DB Load","OID and Mod must be valid strings");
			return -1;
		}
		int cid = U().CallId();
		string endpoint = "Load/" + oid + "/" + mod;
		
		cb.SetOID(oid); //Only sets if not set
		
		Post(endpoint,jsonString,new UDBNestedCallBack(cb, cid));
		
		return cid;
	}
	
	int Load(string mod, string oid, Class cbInstance, string cbFunction, string jsonString = "{}") {		
		if (mod == "" || oid == "" || jsonString == ""){
			Error2("[UF] Error on DB Load","OID, jsonString and Mod must be valid strings");
			return -1;
		}
		int cid = U().CallId();
		string endpoint = "Load/" + oid + "/" + mod;
		
		Post(endpoint,jsonString, new UDBCallBack(cbInstance, cbFunction, cid, oid));
		
		return cid;
	}
	
	
	int Query(string mod, UDBQueryBase query, UFCallbackBase cb) {
		if (mod == "" || !query || !cb){
			Error2("[UF] Error on DB Query","Mod, query and callback must be valid");
			return -1;
		}
		int cid = U().CallId();
		string endpoint = "Query/" + mod;
				
		if ( query && mod && cb){
			cb.SetOID(mod); //Only sets if not set
			Post(endpoint,query.ToJson(), new UDBNestedCallBack(cb, cid));
		} else {
			Print("[UF] [Api] Error Querying " +  mod);
			cid = -1;
		}
		return cid;
	}
	
	int Query(string mod, UDBQueryBase query, Class cbInstance, string cbFunction) {
		if ( mod == "" || !query ){
			Error2("[UF] Error on DB Query","Mod and query must be valid");
			return -1;
		}
		int cid = U().CallId();
		string endpoint = "Query/" + mod;
				
		Post(endpoint,query.ToJson(),new UDBCallBack(cbInstance, cbFunction, cid, ""));
		
		return cid;
	}
	
	int Increment(string mod, string oid, string element, float value = 1){
		if (mod == "" || oid == "" || element == ""){
			Error2("[UF] Error on DB Incerment","OID and Mod must be valid strings");
			return -1;
		}
		return Transaction(mod, oid, element, value);
	}
	
	int Transaction(string mod, string oid, string element, float value) {
		if (mod == "" || oid == "" || element == ""){
			Error2("[UF] Error on DB Transaction","OID, element and Mod must be valid strings");
			return -1;
		}
		int cid = U().CallId();
				
		string endpoint = "Transaction/" + oid   + "/" + mod;
		
		autoptr UDBTransaction transaction = new UDBTransaction(element, value);
		
		Post(endpoint,transaction.ToJson(), new USilentCallBack());
		
		return cid;
	}
	
	int Transaction(string mod, string oid, string element, float value, UFCallbackBase cb) {
		if (mod == "" || oid == "" || element == "" || !cb){
			Error2("[UF] Error on DB Transaction","OID, element, callback and Mod must be valid");
			return -1;
		}
		int cid = U().CallId();
		
		string endpoint = "Transaction/" + oid   + "/" + mod;
		
		autoptr UDBTransaction transaction = new UDBTransaction(element, value);
		
		cb.SetOID(oid); //Only sets if not set
			
		Post(endpoint,transaction.ToJson(), new UDBNestedCallBack(cb, cid));
		
		return cid;
	}
	
	int Transaction(string mod, string oid, string element, float value, float min, float max, UFCallbackBase cb) {
		if (mod == "" || oid == ""  || element == "" || !cb){
			Error2("[UF] Error on DB Transaction","OID, element, callback and Mod must be valid");
			return -1;
		}
		int cid = U().CallId();
		
		string endpoint = "Transaction/" + oid   + "/" + mod;
		
		autoptr UDBValidatedTransaction transaction = new UDBValidatedTransaction(element, value, min, max);
		
		cb.SetOID(oid); //Only sets if not set
		
		Post(endpoint,transaction.ToJson(), new UDBNestedCallBack(cb, cid));
		
		return cid;
	}
	
	int Transaction(string mod, string oid, string element, float value, Class cbInstance, string cbFunction) {
		if (mod == "" || element == "" || oid == ""){
			Error2("[UF] Error on DB Transaction","OID, element and Mod must be valid strings");
			return -1;
		}
		int cid = U().CallId();
		
		autoptr RestCallback DBCBX = ;
		
		string endpoint = "Transaction/" + oid   + "/"+ mod;
		
		autoptr UDBTransaction transaction = new UDBTransaction(element, value);
		
		Post(endpoint,transaction.ToJson(), new UDBCallBack(cbInstance, cbFunction, cid, oid));
		
		return cid;
	}
	
	int Transaction(string mod, string oid, string element, float value, float min, float max, Class cbInstance, string cbFunction) {
		if (mod == "" || oid == "" || element == ""){
			Error2("[UF] Error on DB Transaction","OID, element, and Mod must be valid strings");
			return -1;
		}
		int cid = U().CallId();
		
		autoptr RestCallback DBCBX = ;
		
		string endpoint = "Transaction/" + oid   + "/"+ mod;
		
		autoptr UDBValidatedTransaction transaction = new UDBValidatedTransaction(element, value, min, max);
		
		Post(endpoint, transaction.ToJson(), new UDBCallBack(cbInstance, cbFunction, cid, oid));
		
		return cid;
	}
	
	int Update(string mod, string oid, string element, string value, string operation = UpdateOpts.SET) {	
		if (mod == "" || oid == "" || element == "" || operation == ""){
			Error2("[UF] Error on DB Update","OID, Element, operation, and Mod must be valid strings");
			return -1;
		}
		int cid = U().CallId();
		
		string endpoint = "Update/" + oid   + "/"+ mod;
		
		autoptr UUpdateData updatedata = new UUpdateData(element, value, operation);
		
		Post(endpoint, updatedata.ToJson(), new USilentCallBack());
		
		return cid;
	}
		
	int Update(string mod, string oid, string element, string value, string operation, UFCallbackBase cb) {	
		if (mod == "" || oid == "" || element == "" || operation == "" || !cb){
			Error2("[UF] Error on DB Update","OID, callback, operation, Element and Mod must be valid");
			return -1;
		}
		int cid = U().CallId();

		string endpoint = "Update/" + oid   + "/"+ mod;
		
		autoptr UUpdateData updatedata = new UUpdateData(element, value, operation);
		
		cb.SetOID(oid); //Only sets if not set
		Post(endpoint, updatedata.ToJson(), new UDBNestedCallBack(cb, cid));
		
		return cid;
	}
	
	int Update(string mod, string oid, string element, string value, string operation, Class cbInstance, string cbFunction) {	
		if (mod == "" || oid == "" || element == "" || operation == ""){
			Error2("[UF] Error on DB Update","OID, Element, operation, and Mod must be valid strings");
			return -1;
		}
		int cid = U().CallId();
		autoptr RestCallback DBCBX;
		if (cbInstance && cbFunction != ""){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, oid);
		} else {
			DBCBX = new USilentCallBack();
		}
		
		string endpoint = "Update/" + oid   + "/"+ mod;
		
		autoptr UUpdateData updatedata = new UUpdateData(element, value, operation);
		
		Post(endpoint, updatedata.ToJson(), DBCBX);
		
		return cid;
	}
	
	
	int QueryUpdate(UDBQueryBase query, string mod, string element, string value, string operation = UpdateOpts.SET) {	
		if (!query || mod == "" || element == "" || operation == ""){
			Error2("[UF] Error on DB Update","OID, Element, operation, and Mod must be valid strings");
			return -1;
		}
		int cid = U().CallId();
		
		string endpoint = "Query/Update/" + mod;
		
		autoptr UDBQueryUpdate updatedata = new UDBQueryUpdate(query, element, value, operation);
		
		Post(endpoint, updatedata.ToJson(), new USilentCallBack());
		
		return cid;
	}
	
	int QueryUpdate(UDBQueryBase query, string mod, string element, string value, string operation, UFCallbackBase cb) {	
		if (!query || mod == "" || element == "" || operation == "" || !cb){
			Error2("[UF] Error on DB Update","OID, callback, operation, Element and Mod must be valid");
			return -1;
		}
		int cid = U().CallId();

		string endpoint = "Query/Update/" + mod;
		
		autoptr UUpdateData updatedata = new UUpdateData(element, value, operation);
		
		cb.SetOID(mod); //Only sets if not set
		Post(endpoint, updatedata.ToJson(), new UDBNestedCallBack(cb, cid));
		
		return cid;
	}
	
	int QueryUpdate(UDBQueryBase query, string mod, string element, string value, string operation, Class cbInstance, string cbFunction) {
		if (!query || mod == "" || element == "" || operation == ""){
			Error2("[UF] Error on DB Update","OID, Element, operation, and Mod must be valid strings");
			return -1;
		}
		int cid = U().CallId();
		autoptr RestCallback DBCBX;
		if (cbInstance && cbFunction != ""){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, mod);
		} else {
			DBCBX = new USilentCallBack();
		}
		
		string endpoint = "Query/Update/" + mod;
		
		autoptr UDBQueryUpdate updatedata = new UDBQueryUpdate(query,element, value, operation);
		
		Post(endpoint, updatedata.ToJson(), DBCBX);
		
		return cid;
		
	}
	
	
	//Only Works on Player Data	
	int PublicSave(string mod, string oid, string jsonString, Class cbInstance = NULL, string cbFunction = "") {	
		if (m_Collection != "Player") return -1;
		int cid = U().CallId();	
		string endpoint = "PublicSave/" + oid + "/" + mod;
		autoptr RestCallback DBCBX;
		if (cbInstance && cbFunction != ""){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, oid);
		} else {
			DBCBX = new USilentCallBack();
		}
		
		if (jsonString){
			Post(endpoint,jsonString,DBCBX);
		} else {
			Print("[UF] [Api] Error Saving " + endpoint + " Data for " + mod);
			cid = -1;
		}
		return cid;
	}
	
	int PublicLoad(string mod, string oid, Class cbInstance, string cbFunction, string jsonString = "{}", string baseUrl = "") {		
		if (m_Collection != "Player") return -1;
		int cid = U().CallId();
		string endpoint = "PublicLoad/" + oid + "/" + mod;
		
		autoptr RestCallback DBCBX;
		if (cbInstance && cbFunction != ""){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, oid);
		} else {
			DBCBX = new USilentCallBack();
		}
		if ( baseUrl != "" && DBCBX ){
			string url = baseUrl + m_Collection + "/" + endpoint;
			//Print("[UF] Public Load with custom Base: " + url);
			U().Post(url,jsonString,DBCBX);
		} else if (DBCBX){
			Post(endpoint,jsonString,DBCBX);
		} else {
			Print("[UF] [Api] Error Loading Player Data for " + mod);
			cid = -1;
		}
		return cid;
	}


}
