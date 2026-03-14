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
	
	/**
	 * Constructor
	 * @param collection Collection name ("Object" or "Player")
	 */
	void UDBEndpoint(string collection){
		m_Collection = collection;
	}
	
	/**
	 * Returns the base URL for this database endpoint
	 * @return Base URL with collection appended
	 */
	override protected string EndpointBaseUrl(){
		return UFConfig().GetBaseURL() + m_Collection + "/";
	}
	
	/**
	 * Saves data to database (fire-and-forget, no callback).
	 * 
	 * @param mod Mod identifier namespace
	 * @param oid Object ID (unique identifier for this data)
	 * @param jsonString JSON data to save
	 * @return Call ID or -1 on error
	 * 
	 * @usage U().db().Save("MyMod", "config", configData.ToJson());
	 */
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
	
	/**
	 * Saves data with callback notification.
	 * 
	 * @param mod Mod identifier namespace
	 * @param oid Object ID
	 * @param jsonString JSON data to save
	 * @param cbInstance Object to call callback on
	 * @param cbFunction Callback method name
	 * @return Call ID or -1 on error
	 * 
	 * @usage U().db().Save("MyMod", "player_" + guid, playerData, this, "OnSaved");
	 * @note Callback signature: void OnSaved(int cid, int status, string oid, string data)
	 */
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
	
	/**
	 * Saves data with UFCallbackBase callback.
	 * 
	 * @param mod Mod identifier namespace
	 * @param oid Object ID
	 * @param jsonString JSON data to save
	 * @param cb UFCallbackBase-derived callback
	 * @return Call ID or -1 on error
	 * 
	 * @usage U().db().Save("MyMod", "stats", jsonData, new MySaveCallback());
	 */
	int Save(string mod, string oid, string jsonString, UFCallbackBase cb) {	
		if (mod == "" || oid == "" || jsonString == "" || !cb){
			Error2("[UF] Error on DB Save","OID and Mod must be valid strings");
			return -1;
		}
		int cid = -1;	
		string endpoint = "Save/" + oid + "/" + mod;
		
		cb.SetOID(oid); //Only sets if not set
		
		Post(endpoint,jsonString, U().RegisterCall(new UNestedCallBack(cb), cid));
		
		if (cid == -1){
			Error2("[UF] Error failed to register callback with UF", "Save");
		}
		return cid;
	}
	
	/**
	 * Loads data from database with UFCallbackBase callback.
	 * 
	 * @param mod Mod identifier namespace
	 * @param oid Object ID to load
	 * @param cb UFCallbackBase-derived callback
	 * @param jsonString Optional query parameters (default: "{}")
	 * @return Call ID or -1 on error
	 * 
	 * @usage U().db().Load("MyMod", "config", new MyLoadCallback());
	 * @note Callback receives parsed object as typed parameter if using UFCallback<T>
	 */
	
	int Load(string mod, string oid, UFCallbackBase cb, string jsonString = "{}") {		
		if (mod == "" || oid == "" || jsonString == "" || !cb){
			Error2("[UF] Error on DB Load","OID and Mod must be valid strings");
			return -1;
		}
		int cid = -1;
		string endpoint = "Load/" + oid + "/" + mod;
		
		cb.SetOID(oid); //Only sets if not set
		
		Post(endpoint,jsonString, U().RegisterCall(new UNestedCallBack(cb),cid));
		
		if (cid == -1){
			Error2("[UF] Error failed to register callback with UF", "Load");
		}
		return cid;
	}
	
	/**
	 * Loads data with callback notification.
	 * 
	 * @param mod Mod identifier namespace
	 * @param oid Object ID to load
	 * @param cbInstance Object to call callback on
	 * @param cbFunction Callback method name
	 * @param jsonString Optional query parameters (default: "{}")
	 * @return Call ID or -1 on error
	 * 
	 * @usage U().db().Load("MyMod", "player_" + guid, this, "OnLoaded");
	 * @note Callback signature: void OnLoaded(int cid, int status, string oid, string data)
	 */
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
	
	/**
	 * Executes a database query with UFCallbackBase callback.
	 * 
	 * @param mod Mod identifier namespace
	 * @param query Query object defining search criteria
	 * @param cb UFCallbackBase-derived callback
	 * @return Call ID or -1 on error
	 * 
	 * @usage auto query = new UDBQuery().Equals("level", "5");
	 *        U().db().Query("MyMod", query, new MyQueryCallback());
	 */
	int Query(string mod, UDBQueryBase query, UFCallbackBase cb) {
		UFLog.Debug("[UDBEndpoint::Query] mod=" + mod);
		if (mod == "" || !query || !cb){
			Error2("[UF] Error on DB Query","Mod, query and callback must be valid");
			return -1;
		}
		
		// Safety check: ensure framework is ready
		UFramework uf = U();
		if (!uf){
			UFLog.Err("[UDBEndpoint::Query] U() returned NULL - framework not ready");
			return -1;
		}
		if (!UFConfig()){
			UFLog.Err("[UDBEndpoint::Query] UFConfig() is NULL - config not loaded");
			return -1;
		}
		
		int cid = -1;
		string endpoint = "Query/" + mod;
				
		if ( query && mod && cb){
			cb.SetOID(mod); //Only sets if not set
			RestCallback regCb = uf.RegisterCall(new UNestedCallBack(cb), cid);
			if (!regCb){
				UFLog.Err("[UDBEndpoint::Query] RegisterCall returned NULL");
				return -1;
			}
			Post(endpoint, query.ToJson(), regCb);
			
			if (cid == -1){
				Error2("[UF] Error failed to register callback with UF", "Query");
			}
		} else {
			UFLog.Err("[Api] Error Querying " + mod);
			cid = -1;
		}
		return cid;
	}
	
	/**
	 * Executes a database query with callback notification.
	 * 
	 * @param mod Mod identifier namespace
	 * @param query Query object defining search criteria
	 * @param cbInstance Object to call callback on
	 * @param cbFunction Callback method name
	 * @return Call ID or -1 on error
	 * 
	 * @usage auto query = new UDBQuery().GreaterThan("score", "100");
	 *        U().db().Query("MyMod", query, this, "OnQueryResults");
	 */
	int Query(string mod, UDBQueryBase query, Class cbInstance, string cbFunction) {
		UFLog.Debug("[UDBEndpoint::Query] mod=" + mod + " cbFunction=" + cbFunction);
		if ( mod == "" || !query ){
			Error2("[UF] Error on DB Query","Mod and query must be valid");
			return -1;
		}
		
		// Safety check: ensure framework is ready
		UFramework uf = U();
		if (!uf){
			UFLog.Err("[UDBEndpoint::Query] U() returned NULL - framework not ready");
			return -1;
		}
		if (!UFConfig()){
			UFLog.Err("[UDBEndpoint::Query] UFConfig() is NULL - config not loaded");
			return -1;
		}
		
		int cid = -1;
		string endpoint = "Query/" + mod;
				
		RestCallback regCb = uf.RegisterCall(new UDBCallBack(cbInstance, cbFunction, cid, ""), cid);
		if (!regCb){
			UFLog.Err("[UDBEndpoint::Query] RegisterCall returned NULL");
			return -1;
		}
		Post(endpoint, query.ToJson(), regCb);
		
		if (cid == -1){
			Error2("[UF] Error failed to register callback with UF", "Query");
		}
		return cid;
	}
	
	/**
	 * Convenience method to increment a numeric field by a value.
	 * 
	 * @param mod Mod identifier namespace
	 * @param oid Object ID
	 * @param element Field name to increment
	 * @param value Amount to add (default: 1)
	 * @return Call ID or -1 on error
	 * 
	 * @usage U().db().Increment("MyMod", "player_123", "kills", 1);
	 */
	int Increment(string mod, string oid, string element, float value = 1){
		if (mod == "" || oid == "" || element == ""){
			Error2("[UF] Error on DB Incerment","OID and Mod must be valid strings");
			return -1;
		}
		return Transaction(mod, oid, element, value);
	}
	
	/**
	 * Atomically modifies a numeric field (fire-and-forget).
	 * 
	 * @param mod Mod identifier namespace
	 * @param oid Object ID
	 * @param element Field name
	 * @param value Amount to add/subtract
	 * @return Call ID or -1 on error
	 * 
	 * @usage U().db().Transaction("MyMod", "player", "coins", 100);
	 */
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
	
	/**
	 * Atomically modifies a numeric field with callback.
	 * 
	 * @param mod Mod identifier namespace
	 * @param oid Object ID
	 * @param element Field name
	 * @param value Amount to add/subtract
	 * @param cb UFCallbackBase-derived callback
	 * @return Call ID or -1 on error
	 * 
	 * @usage U().db().Transaction("MyMod", "player", "xp", 50, new MyCallback());
	 */
	int Transaction(string mod, string oid, string element, float value, UFCallbackBase cb) {
		if (mod == "" || oid == "" || element == "" || !cb){
			Error2("[UF] Error on DB Transaction","OID, element, callback and Mod must be valid");
			return -1;
		}
		int cid = -1;
		
		string endpoint = "Transaction/" + oid   + "/" + mod;
		
		autoptr UDBTransaction transaction = new UDBTransaction(element, value);
		
		cb.SetOID(oid); //Only sets if not set
			
		Post(endpoint,transaction.ToJson(), U().RegisterCall(new UNestedCallBack(cb), cid));
		
			if (cid == -1){
				Error2("[UF] Error failed to register callback with UF", "Transaction");
			}
	return cid;
	}
	
	/**
	 * Atomically modifies a numeric field with min/max bounds and callback.
	 * 
	 * @param mod Mod identifier namespace
	 * @param oid Object ID
	 * @param element Field name
	 * @param value Amount to add/subtract
	 * @param min Minimum allowed result value (rejects if below)
	 * @param max Maximum allowed result value (rejects if above)
	 * @param cb UFCallbackBase callback
	 * @return Call ID or -1 on error
	 * 
	 * @usage U().db().Transaction("MyMod", "player", "health", -10, 0, 100, callback);
	 */
	int Transaction(string mod, string oid, string element, float value, float min, float max, UFCallbackBase cb) {
		if (mod == "" || oid == ""  || element == "" || !cb){
			Error2("[UF] Error on DB Transaction","OID, element, callback and Mod must be valid");
			return -1;
		}
		int cid = -1;
		
		string endpoint = "Transaction/" + oid   + "/" + mod;
		
		autoptr UDBValidatedTransaction transaction = new UDBValidatedTransaction(element, value, min, max);
		
		cb.SetOID(oid); //Only sets if not set
		
		Post(endpoint,transaction.ToJson(), U().RegisterCall(new UNestedCallBack(cb), cid));
		
			if (cid == -1){
				Error2("[UF] Error failed to register callback with UF", "Transaction");
			}
		return cid;
	}
	
	/**
	 * Atomically modifies a numeric field with callback notification.
	 * 
	 * @param mod Mod identifier namespace
	 * @param oid Object ID
	 * @param element Field name
	 * @param value Amount to add/subtract
	 * @param cbInstance Object to call callback on
	 * @param cbFunction Callback method name
	 * @return Call ID or -1 on error
	 * 
	 * @usage U().db().Transaction("MyMod", "player", "coins", 10, this, "OnCoinAdded");
	 */
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
	
	/**
	 * Atomically modifies a numeric field with min/max bounds and callback notification.
	 * 
	 * @param mod Mod identifier namespace
	 * @param oid Object ID
	 * @param element Field name
	 * @param value Amount to add/subtract
	 * @param min Minimum allowed result value (rejects if below)
	 * @param max Maximum allowed result value (rejects if above)
	 * @param cbInstance Object to call callback on
	 * @param cbFunction Callback method name
	 * @return Call ID or -1 on error
	 * 
	 * @usage U().db().Transaction("MyMod", "player", "health", -25, 0, 100, this, "OnHealthChanged");
	 */
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
	
	/**
	 * Updates a single field in the database (fire-and-forget).
	 * 
	 * @param mod Mod identifier namespace
	 * @param oid Object ID
	 * @param element Field name
	 * @param value New value (JSON-encoded string)
	 * @param operation Update operation (default: UpdateOpts.SET)
	 * @return Call ID or -1 on error
	 * 
	 * @usage U().db().Update("MyMod", "config", "enabled", "true");
	 * @note For strings, must quote: Update(mod, oid, "name", "\"John\"");
	 */
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
		
	/**
	 * Updates a single field with callback.
	 * 
	 * @param mod Mod identifier namespace
	 * @param oid Object ID
	 * @param element Field name
	 * @param value New value (JSON-encoded string)
	 * @param operation Update operation
	 * @param cb UFCallbackBase-derived callback
	 * @return Call ID or -1 on error
	 * 
	 * @usage U().db().Update("MyMod", "player", "status", "\"active\"", UpdateOpts.SET, callback);
	 */
	int Update(string mod, string oid, string element, string value, string operation, UFCallbackBase cb) {	
		if (mod == "" || oid == "" || element == "" || operation == "" || !cb){
			Error2("[UF] Error on DB Update","OID, callback, operation, Element and Mod must be valid");
			return -1;
		}
		int cid = -1;

		string endpoint = "Update/" + oid   + "/"+ mod;
		
		autoptr UUpdateData updatedata = new UUpdateData(element, value, operation);
		
		cb.SetOID(oid); //Only sets if not set
		Post(endpoint, updatedata.ToJson(), U().RegisterCall(new UNestedCallBack(cb), cid));
		
			if (cid == -1){
				Error2("[UF] Error failed to register callback with UF", "Update");
			}
		return cid;
	}
	
	/**
	 * Updates a single field with callback notification.
	 * 
	 * @param mod Mod identifier namespace
	 * @param oid Object ID
	 * @param element Field name
	 * @param value New value (JSON-encoded string)
	 * @param operation Update operation
	 * @param cbInstance Object to call callback on
	 * @param cbFunction Callback method name
	 * @return Call ID or -1 on error
	 * 
	 * @usage U().db().Update("MyMod", "player", "score", "1000", UpdateOpts.SET, this, "OnScoreUpdated");
	 */
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
	
	/**
	 * Updates all records matching a query (fire-and-forget).
	 * 
	 * @param query Query object to match records
	 * @param mod Mod identifier namespace
	 * @param element Field name to update
	 * @param value New value (JSON-encoded string)
	 * @param operation Update operation (default: UpdateOpts.SET)
	 * @return Call ID or -1 on error
	 * 
	 * @usage auto query = new UDBQuery().Equals("banned", "true");
	 *        U().db().QueryUpdate(query, "MyMod", "status", "\"suspended\"");
	 */
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
	
	/**
	 * Updates all records matching a query with callback.
	 * 
	 * @param query Query object to match records
	 * @param mod Mod identifier namespace
	 * @param element Field name to update
	 * @param value New value (JSON-encoded string)
	 * @param operation Update operation
	 * @param cb UFCallbackBase-derived callback
	 * @return Call ID or -1 on error
	 * 
	 * @usage auto query = new UDBQuery().LessThan("level", "5");
	 *        U().db().QueryUpdate(query, "MyMod", "newbie", "true", UpdateOpts.SET, callback);
	 */
	int QueryUpdate(UDBQueryBase query, string mod, string element, string value, string operation, UFCallbackBase cb) {	
		if (!query || mod == "" || element == "" || operation == "" || !cb){
			Error2("[UF] Error on DB Update","OID, callback, operation, Element and Mod must be valid");
			return -1;
		}
		int cid = -1;

		string endpoint = "Query/Update/" + mod;
		
		autoptr UUpdateData updatedata = new UUpdateData(element, value, operation);
		
		cb.SetOID(mod); //Only sets if not set
		Post(endpoint, updatedata.ToJson(), U().RegisterCall(new UNestedCallBack(cb),cid));
		
			if (cid == -1){
				Error2("[UF] Error failed to register callback with UF", "QueryUpdate");
			}
		return cid;
	}
	
	/**
	 * Updates all records matching a query with callback notification.
	 * 
	 * @param query Query object to match records
	 * @param mod Mod identifier namespace
	 * @param element Field name to update
	 * @param value New value (JSON-encoded string)
	 * @param operation Update operation
	 * @param cbInstance Object to call callback on
	 * @param cbFunction Callback method name
	 * @return Call ID or -1 on error
	 * 
	 * @usage auto query = new UDBQuery().Equals("active", "false");
	 *        U().db().QueryUpdate(query, "MyMod", "archived", "true", UpdateOpts.SET, this, "OnArchived");
	 */
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
	
	
	/**
	 * Deletes an object or player mod data from the database
	 * 
	 * @param mod The mod namespace
	 * @param oid The object ID or player GUID
	 * @param cb Callback instance (UFCallbackBase)
	 * @return Callback ID or -1 on error
	 */
	int Delete(string mod, string oid, UFCallbackBase cb) {
		if (!mod || !oid || !cb) {
			UFLog.Err("[Delete] Invalid parameters - mod, oid, and callback are required");
			return -1;
		}
		
		int cid = -1;
		string endpoint = "Delete/" + oid + "/" + mod;
		
		cb.SetOID(oid);
		
		Post(endpoint, "{}", U().RegisterCall(new UNestedCallBack(cb), cid));
		if (cid == -1) {
			Error2("[UF] Error failed to register callback with UF", "Delete");
		}
		
		return cid;
	}
	
	/**
	 * Deletes an object or player mod data from the database
	 * 
	 * @param mod The mod namespace
	 * @param oid The object ID or player GUID
	 * @param cbInstance Callback instance
	 * @param cbFunction Callback function name
	 * @return Callback ID or -1 on error
	 */
	int Delete(string mod, string oid, Class cbInstance, string cbFunction) {
		if (!mod || !oid) {
			UFLog.Err("[Delete] Invalid parameters - mod and oid are required");
			return -1;
		}
		
		int cid = -1;
		string endpoint = "Delete/" + oid + "/" + mod;
		
		autoptr UFRestCallBackBase DBCBX;
		if (cbInstance && cbFunction != "") {
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, oid);
		} else {
			DBCBX = new USilentCallBack();
		}
		
		Post(endpoint, "{}", U().RegisterCall(DBCBX, cid));
		if (cid == -1) {
			Error2("[UF] Error failed to register callback with UF", "Delete");
		}
		
		return cid;
	}
	
	/**
	 * Saves publicly accessible player data (Player collection only).
	 * 
	 * @param mod Mod identifier namespace
	 * @param oid Player GUID
	 * @param jsonString JSON data to save
	 * @param cbInstance Optional callback object
	 * @param cbFunction Optional callback method name
	 * @return Call ID or -1 on error or if not Player collection
	 * 
	 * @usage U().player().PublicSave("MyMod", playerGUID, publicData.ToJson(), this, "OnSaved");
	 * @note Only works when collection is "Player"
	 */
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
			UFLog.Err("[Api] Error Saving " + endpoint + " Data for " + mod);
			cid = -1;
		}
		return cid;
	}
	
	/**
	 * Loads publicly accessible player data (Player collection only).
	 * 
	 * @param mod Mod identifier namespace
	 * @param oid Player GUID
	 * @param cbInstance Callback object
	 * @param cbFunction Callback method name
	 * @param jsonString Optional query parameters (default: "{}")
	 * @param baseUrl Optional custom base URL
	 * @return Call ID or -1 on error or if not Player collection
	 * 
	 * @usage U().player().PublicLoad("MyMod", playerGUID, this, "OnLoaded");
	 * @note Only works when collection is "Player"
	 */
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
			UFLog.Err("[Api] Error Loading Player Data for " + mod);
			cid = -1;
		}
		return cid;
	}


}
