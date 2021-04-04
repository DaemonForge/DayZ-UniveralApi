class UApiDBEndpoint extends Managed {
	
	static protected string m_BaseUrl = "";
	
	protected RestContext m_Context;
	
	protected string m_Collection = "Object";
	
	
	void UApiDBEndpoint(string collection){
		m_Collection = collection;
	}
	
	static void SetBaseUrl(string new_BaseUrl){
		m_BaseUrl = new_BaseUrl;
	}
	
	static string BaseUrl(){
		if (m_BaseUrl != ""){
			return m_BaseUrl;
		}
		return UApiConfig().ServerURL;
	}

	RestContext Api()
	{
		if (!m_Context){
			RestApi clCore = GetRestApi();
			if (!clCore)
			{
				clCore = CreateRestApi();
				clCore.SetOption(ERestOption.ERESTOPTION_READOPERATION, 15);
			}
			string url = BaseUrl() + m_Collection;
			m_Context =  clCore.GetRestContext(url);
			m_Context.SetHeader("application/json");
		}
		return m_Context;
	}
	
	void Post(string endpoint, string jsonString, RestCallback UCBX)
	{
		string route = endpoint + "/" + UApi().GetAuthToken();
		Api().POST(UCBX, route, jsonString);
	}
		
	int Save(string mod, string oid, string jsonString, Class instance = NULL, string function = "") {	
		int cid = UApi().CallId();	
		string endpoint = "/Save/" + oid + "/" + mod;
		ref RestCallback DBCBX;
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
		
		ref RestCallback DBCBX;
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
		
		ref RestCallback DBCBX;
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
		
		ref RestCallback DBCBX;
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
		ref RestCallback DBCBX;
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

}