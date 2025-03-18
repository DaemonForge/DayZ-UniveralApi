/**
 * UDBEndpoint Class
 * 
 * This class extends UFBaseEndpoint and provides methods to interact with a database through RESTful endpoints.
 * It formats URLs using a base URL obtained from UFConfig and a specified collection, then issues POST requests
 * to perform various operations such as Save, Load, Query, Increment, Transaction, Update, QueryUpdate, PublicSave,
 * and PublicLoad.
 *
 * Constructors:
 *   - UDBEndpoint(string collection)
 *       Initializes the endpoint using the provided collection name.
 *
 * Methods:
 *   - EndpointBaseUrl()
 *       Constructs and returns the base URL for the endpoint by appending the collection to the configuration's base URL.
 *
 *   - Save(string mod, string oid, string jsonString)
 *       Validates input strings, then performs a POST request to save data using a silent callback.
 *       Returns the callback ID or -1 if an error occurs.
 *
 *   - Save(string mod, string oid, string jsonString, Class cbInstance, string cbFunction)
 *       Performs a save operation similar to the previous overload but registers a callback using the provided instance and function.
 *       Returns the callback ID or -1 if an error occurs.
 *
 *   - Save(string mod, string oid, string jsonString, UFCallbackBase cb)
 *       Executes a save request and uses the given callback instance (after setting its OID) in a nested callback structure.
 *       Returns the callback ID or -1 if an error occurs.
 *
 *   - Load(string mod, string oid, UFCallbackBase cb, string jsonString = "{}")
 *       Loads data via a POST request with the provided parameters and a default empty JSON string.
 *       The callback is set with the OID and used in a nested callback registration.
 *       Returns the callback ID or -1 if an error occurs.
 *
 *   - Load(string mod, string oid, Class cbInstance, string cbFunction, string jsonString = "{}")
 *       Similar to the previous Load method, this version registers a callback using the provided instance and function.
 *       Returns the callback ID or -1 if an error occurs.
 *
 *   - Query(string mod, UDBQueryBase query, UFCallbackBase cb)
 *       Executes a database query using a query object and a nested callback. The callback’s OID is set to the mod.
 *       Returns the callback ID or -1 if an error occurs.
 *
 *   - Query(string mod, UDBQueryBase query, Class cbInstance, string cbFunction)
 *       Performs a query and registers a callback using the provided callback instance and function.
 *       Returns the callback ID or -1 if an error occurs.
 *
 *   - Increment(string mod, string oid, string element, float value = 1)
 *       A convenience method that calls Transaction to increment a specified element by the given value.
 *       Returns the callback ID or -1 if an error occurs.
 *
 *   - Transaction(...) Overloads:
 *       Executes a transaction operation by preparing a JSON payload representing the change.
 *       Overloads include:
 *         • Transaction(string mod, string oid, string element, float value)
 *         • Transaction(string mod, string oid, string element, float value, UFCallbackBase cb)
 *         • Transaction(string mod, string oid, string element, float value, float min, float max, UFCallbackBase cb)
 *         • Transaction(string mod, string oid, string element, float value, Class cbInstance, string cbFunction)
 *         • Transaction(string mod, string oid, string element, float value, float min, float max, Class cbInstance, string cbFunction)
 *       Each overload validates parameters, registers a callback accordingly, and returns the callback ID or -1 if an error occurs.
 *
 *   - Update(string mod, string oid, string element, string value, string operation = UpdateOpts.SET)
 *       Performs an update operation by sending update data as a JSON payload through a POST request using a silent callback.
 *       Returns the callback ID or -1 if an error occurs.
 *
 *   - Update(string mod, string oid, string element, string value, string operation, UFCallbackBase cb)
 *       Executes an update operation similar to the previous method, but registers a callback provided by the caller.
 *       Returns the callback ID or -1 if an error occurs.
 *
 *   - Update(string mod, string oid, string element, string value, string operation, Class cbInstance, string cbFunction)
 *       Updates data and registers a callback based on provided instance and function; if not provided, falls back to a silent callback.
 *       Returns the callback ID or -1 if an error occurs.
 *
 *   - QueryUpdate(...) Overloads:
 *       These methods update records that match a given query using a JSON-encoded query-update payload.
 *       Overloads include:
 *         • QueryUpdate(UDBQueryBase query, string mod, string element, string value, string operation = UpdateOpts.SET)
 *         • QueryUpdate(UDBQueryBase query, string mod, string element, string value, string operation, UFCallbackBase cb)
 *         • QueryUpdate(UDBQueryBase query, string mod, string element, string value, string operation, Class cbInstance, string cbFunction)
 *       Returns the callback ID or -1 if an error occurs.
 *
 *   - PublicSave(string mod, string oid, string jsonString, Class cbInstance = NULL, string cbFunction = "")
 *       Specifically for player data (when the collection is "Player"), this method saves public data and registers a callback.
 *       Returns the callback ID or -1 if the operation is invalid or an error occurs.
 *
 *   - PublicLoad(string mod, string oid, Class cbInstance, string cbFunction, string jsonString = "{}", string baseUrl = "")
 *       Specifically for player data (when the collection is "Player"), this method loads public data.
 *       It optionally supports a custom base URL and registers the callback provided.
 *       Returns the callback ID or -1 if the operation is invalid or an error occurs.
 *
 * General Notes:
 *   - All methods perform input validation and log errors using Error2 when validations fail.
 *   - The methods utilize a global instance (returned by U()) to register callbacks and execute POST requests.
 *   - Callback mechanisms vary between silent, nested, and instance/function-based callbacks to support different usage scenarios.
 */
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
		int cid = -1;	
		string endpoint = "Save/" + oid + "/" + mod;
		
		Post(endpoint,jsonString, U().RegisterCall(new USilentCallBack(), cid));
		
		if (cid == -1){
			Error2("[UF] Error failed to register callback with UF", "Save");
		}
		return cid;
	}
	
	int Save(string mod, string oid, string jsonString, Class cbInstance, string cbFunction) {	
		if (mod == "" || oid == "" || jsonString == ""){
			Error2("[UF] Error on DB Save","OID, jsonString and Mod must be valid strings");
			return -1;
		}
		int cid = -1;	
		string endpoint = "Save/" + oid + "/" + mod;

		Post(endpoint,jsonString, U().RegisterCall(new UDBCallBack(cbInstance, cbFunction, cid, oid), cid));
		
		if (cid == -1){
			Error2("[UF] Error failed to register callback with UF", "Save");
		}
		return cid;
	}
	
	int Save(string mod, string oid, string jsonString, UFCallbackBase cb) {	
		if (mod == "" || oid == "" || jsonString == "" || !cb){
			Error2("[UF] Error on DB Save","OID and Mod must be valid strings");
			return -1;
		}
		int cid = -1;	
		string endpoint = "Save/" + oid + "/" + mod;
		
		cb.SetOID(oid); //Only sets if not set
		
		Post(endpoint,jsonString, U().RegisterCall(new UDBNestedCallBack(cb, cid), cid));
		
		if (cid == -1){
			Error2("[UF] Error failed to register callback with UF", "Save");
		}
		return cid;
	}
	
	int Load(string mod, string oid, UFCallbackBase cb, string jsonString = "{}") {		
		if (mod == "" || oid == "" || jsonString == "" || !cb){
			Error2("[UF] Error on DB Load","OID and Mod must be valid strings");
			return -1;
		}
		int cid = -1;
		string endpoint = "Load/" + oid + "/" + mod;
		
		cb.SetOID(oid); //Only sets if not set
		
		Post(endpoint,jsonString, U().RegisterCall(new UDBNestedCallBack(cb, cid),cid));
		
		if (cid == -1){
			Error2("[UF] Error failed to register callback with UF", "Load");
		}
		return cid;
	}
	
	int Load(string mod, string oid, Class cbInstance, string cbFunction, string jsonString = "{}") {		
		if (mod == "" || oid == "" || jsonString == ""){
			Error2("[UF] Error on DB Load","OID, jsonString and Mod must be valid strings");
			return -1;
		}
		int cid = -1;
		string endpoint = "Load/" + oid + "/" + mod;
		
		Post(endpoint,jsonString,  U().RegisterCall(new UDBCallBack(cbInstance, cbFunction, cid, oid), cid));
		
		if (cid == -1){
			Error2("[UF] Error failed to register callback with UF", "Load");
		}
		return cid;
	}
	
	
	int Query(string mod, UDBQueryBase query, UFCallbackBase cb) {
		if (mod == "" || !query || !cb){
			Error2("[UF] Error on DB Query","Mod, query and callback must be valid");
			return -1;
		}
		int cid = -1;
		string endpoint = "Query/" + mod;
				
		if ( query && mod && cb){
			cb.SetOID(mod); //Only sets if not set
			Post(endpoint,query.ToJson(), U().RegisterCall(new UDBNestedCallBack(cb, cid), cid));
			
			if (cid == -1){
				Error2("[UF] Error failed to register callback with UF", "Query");
			}
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
		int cid = -1;
		string endpoint = "Query/" + mod;
				
		Post(endpoint,query.ToJson(),U().RegisterCall(new UDBCallBack(cbInstance, cbFunction, cid, ""),cid));
		
			if (cid == -1){
				Error2("[UF] Error failed to register callback with UF", "Query");
			}
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
		int cid = -1;
				
		string endpoint = "Transaction/" + oid   + "/" + mod;
		
		autoptr UDBTransaction transaction = new UDBTransaction(element, value);
		
		Post(endpoint,transaction.ToJson(), U().RegisterCall(new USilentCallBack(), cid));
		
			if (cid == -1){
				Error2("[UF] Error failed to register callback with UF", "Transaction");
			}
		return cid;
	}
	
	int Transaction(string mod, string oid, string element, float value, UFCallbackBase cb) {
		if (mod == "" || oid == "" || element == "" || !cb){
			Error2("[UF] Error on DB Transaction","OID, element, callback and Mod must be valid");
			return -1;
		}
		int cid = -1;
		
		string endpoint = "Transaction/" + oid   + "/" + mod;
		
		autoptr UDBTransaction transaction = new UDBTransaction(element, value);
		
		cb.SetOID(oid); //Only sets if not set
			
		Post(endpoint,transaction.ToJson(), U().RegisterCall(new UDBNestedCallBack(cb, cid), cid));
		
			if (cid == -1){
				Error2("[UF] Error failed to register callback with UF", "Transaction");
			}
		return cid;
	}
	
	int Transaction(string mod, string oid, string element, float value, float min, float max, UFCallbackBase cb) {
		if (mod == "" || oid == ""  || element == "" || !cb){
			Error2("[UF] Error on DB Transaction","OID, element, callback and Mod must be valid");
			return -1;
		}
		int cid = -1;
		
		string endpoint = "Transaction/" + oid   + "/" + mod;
		
		autoptr UDBValidatedTransaction transaction = new UDBValidatedTransaction(element, value, min, max);
		
		cb.SetOID(oid); //Only sets if not set
		
		Post(endpoint,transaction.ToJson(), U().RegisterCall(new UDBNestedCallBack(cb, cid), cid));
		
			if (cid == -1){
				Error2("[UF] Error failed to register callback with UF", "Transaction");
			}
		return cid;
	}
	
	int Transaction(string mod, string oid, string element, float value, Class cbInstance, string cbFunction) {
		if (mod == "" || element == "" || oid == ""){
			Error2("[UF] Error on DB Transaction","OID, element and Mod must be valid strings");
			return -1;
		}
		int cid = -1;
		
		autoptr RestCallback DBCBX = ;
		
		string endpoint = "Transaction/" + oid   + "/"+ mod;
		
		autoptr UDBTransaction transaction = new UDBTransaction(element, value);
		
		Post(endpoint,transaction.ToJson(), U().RegisterCall(new UDBCallBack(cbInstance, cbFunction, cid, oid), cid));
		
			if (cid == -1){
				Error2("[UF] Error failed to register callback with UF", "Transaction");
			}
		return cid;
	}
	
	int Transaction(string mod, string oid, string element, float value, float min, float max, Class cbInstance, string cbFunction) {
		if (mod == "" || oid == "" || element == ""){
			Error2("[UF] Error on DB Transaction","OID, element, and Mod must be valid strings");
			return -1;
		}
		int cid = -1;
		
		autoptr UFRestCallBackBase DBCBX = ;
		
		string endpoint = "Transaction/" + oid   + "/"+ mod;
		
		autoptr UDBValidatedTransaction transaction = new UDBValidatedTransaction(element, value, min, max);
		
		Post(endpoint, transaction.ToJson(), U().RegisterCall(new UDBCallBack(cbInstance, cbFunction, cid, oid), cid));
		if (cid == -1){
			Error2("[UF] Error failed to register callback with UF", "Transaction");
		}
		return cid;
	}
	
	int Update(string mod, string oid, string element, string value, string operation = UpdateOpts.SET) {	
		if (mod == "" || oid == "" || element == "" || operation == ""){
			Error2("[UF] Error on DB Update","OID, Element, operation, and Mod must be valid strings");
			return -1;
		}
		int cid = -1;
		
		string endpoint = "Update/" + oid   + "/"+ mod;
		
		autoptr UUpdateData updatedata = new UUpdateData(element, value, operation);
		
		Post(endpoint, updatedata.ToJson(), U().RegisterCall(new USilentCallBack(), cid));
		
			if (cid == -1){
				Error2("[UF] Error failed to register callback with UF", "Update");
			}
		return cid;
	}
		
	int Update(string mod, string oid, string element, string value, string operation, UFCallbackBase cb) {	
		if (mod == "" || oid == "" || element == "" || operation == "" || !cb){
			Error2("[UF] Error on DB Update","OID, callback, operation, Element and Mod must be valid");
			return -1;
		}
		int cid = -1;

		string endpoint = "Update/" + oid   + "/"+ mod;
		
		autoptr UUpdateData updatedata = new UUpdateData(element, value, operation);
		
		cb.SetOID(oid); //Only sets if not set
		Post(endpoint, updatedata.ToJson(), U().RegisterCall(new UDBNestedCallBack(cb, cid), cid));
		
			if (cid == -1){
				Error2("[UF] Error failed to register callback with UF", "Update");
			}
		return cid;
	}
	
	int Update(string mod, string oid, string element, string value, string operation, Class cbInstance, string cbFunction) {	
		if (mod == "" || oid == "" || element == "" || operation == ""){
			Error2("[UF] Error on DB Update","OID, Element, operation, and Mod must be valid strings");
			return -1;
		}
		int cid = -1;
		autoptr UFRestCallBackBase DBCBX;
		if (cbInstance && cbFunction != ""){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, oid);
		} else {
			DBCBX = new USilentCallBack();
		}
		
		string endpoint = "Update/" + oid   + "/"+ mod;
		
		autoptr UUpdateData updatedata = new UUpdateData(element, value, operation);
		
		Post(endpoint, updatedata.ToJson(), U().RegisterCall(DBCBX, cid));
		
			if (cid == -1){
				Error2("[UF] Error failed to register callback with UF", "Update");
			}
		return cid;
	}
	
	
	int QueryUpdate(UDBQueryBase query, string mod, string element, string value, string operation = UpdateOpts.SET) {	
		if (!query || mod == "" || element == "" || operation == ""){
			Error2("[UF] Error on DB Update","OID, Element, operation, and Mod must be valid strings");
			return -1;
		}
		int cid = -1;
		
		string endpoint = "Query/Update/" + mod;
		
		autoptr UDBQueryUpdate updatedata = new UDBQueryUpdate(query, element, value, operation);
		
		Post(endpoint, updatedata.ToJson(), U().RegisterCall(new USilentCallBack(), cid));
		
			if (cid == -1){
				Error2("[UF] Error failed to register callback with UF", "QueryUpdate");
			}
		return cid;
	}
	
	int QueryUpdate(UDBQueryBase query, string mod, string element, string value, string operation, UFCallbackBase cb) {	
		if (!query || mod == "" || element == "" || operation == "" || !cb){
			Error2("[UF] Error on DB Update","OID, callback, operation, Element and Mod must be valid");
			return -1;
		}
		int cid = -1;

		string endpoint = "Query/Update/" + mod;
		
		autoptr UUpdateData updatedata = new UUpdateData(element, value, operation);
		
		cb.SetOID(mod); //Only sets if not set
		Post(endpoint, updatedata.ToJson(), U().RegisterCall(new UDBNestedCallBack(cb, cid),cid));
		
			if (cid == -1){
				Error2("[UF] Error failed to register callback with UF", "QueryUpdate");
			}
		return cid;
	}
	
	int QueryUpdate(UDBQueryBase query, string mod, string element, string value, string operation, Class cbInstance, string cbFunction) {
		if (!query || mod == "" || element == "" || operation == ""){
			Error2("[UF] Error on DB Update","OID, Element, operation, and Mod must be valid strings");
			return -1;
		}
		int cid = U().CallId();
		autoptr UFRestCallBackBase DBCBX;
		if (cbInstance && cbFunction != ""){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, mod);
		} else {
			DBCBX = new USilentCallBack();
		}
		
		string endpoint = "Query/Update/" + mod;
		
		autoptr UDBQueryUpdate updatedata = new UDBQueryUpdate(query,element, value, operation);
		
		Post(endpoint, updatedata.ToJson(), U().RegisterCall(DBCBX, cid));
		
			if (cid == -1){
				Error2("[UF] Error failed to register callback with UF", "QueryUpdate");
			}
		return cid;
		
	}
	
	
	//Only Works on Player Data	
	int PublicSave(string mod, string oid, string jsonString, Class cbInstance = NULL, string cbFunction = "") {	
		if (m_Collection != "Player") return -1;
		int cid = -1;	
		string endpoint = "PublicSave/" + oid + "/" + mod;
		autoptr UFRestCallBackBase DBCBX;
		if (cbInstance && cbFunction != ""){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, oid);
		} else {
			DBCBX = new USilentCallBack();
		}
		
		if (jsonString){
			Post(endpoint,jsonString,U().RegisterCall(DBCBX, cid));
			if (cid == -1){
				Error2("[UF] Error failed to register callback with UF", "PublicSave");
			}
		} else {
			Print("[UF] [Api] Error Saving " + endpoint + " Data for " + mod);
			cid = -1;
		}
		return cid;
	}
	
	int PublicLoad(string mod, string oid, Class cbInstance, string cbFunction, string jsonString = "{}", string baseUrl = "") {		
		if (m_Collection != "Player") return -1;
		int cid = -1;
		string endpoint = "PublicLoad/" + oid + "/" + mod;
		
		autoptr UFRestCallBackBase DBCBX;
		if (cbInstance && cbFunction != ""){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, oid);
		} else {
			DBCBX = new USilentCallBack();
		}
		if ( baseUrl != "" && DBCBX ){
			string url = baseUrl + m_Collection + "/" + endpoint;
			//Print("[UF] Public Load with custom Base: " + url);
			U().Post(url,jsonString,U().RegisterCall(DBCBX, cid));
			if (cid == -1){
				Error2("[UF] Error failed to register callback with UF", "PublicLoad");
			}
		} else if (DBCBX){
			Post(endpoint,jsonString,U().RegisterCall(DBCBX, cid));
			if (cid == -1){
				Error2("[UF] Error failed to register callback with UF", "PublicLoad");
			}
		} else {
			Print("[UF] [Api] Error Loading Player Data for " + mod);
			cid = -1;
		}
		return cid;
	}


}
