/**
 * UFDBGlobalEndpoint - Global database operations for mod-wide shared data.
 * 
 * Unlike UDBEndpoint which uses objectIDs, globals are per-mod only (no OID).
 * Useful for server-wide stats, leaderboards, global configuration.
 * 
 * @usage UF().globals().Save("MyMod", jsonData);
 * @usage UF().globals().Load("MyMod", this, "OnLoaded");
 */
class UDBGlobalEndpoint extends UFBaseEndpoint {
	
	/**
	 * Returns the base URL for global database endpoint
	 * @return Base URL with "Globals/" appended
	 */
	override protected string EndpointBaseUrl(){
		UFrameworkConfig ucfg = UFrameworkConfig.Cast(UFConfig());
		if (!ucfg){
			UFLog.Err("[UFDBGlobalEndpoint] EndpointBaseUrl called but UFConfig() is null - RPC not received yet?");
			return "";
		}
		return ucfg.GetBaseURL() + "Globals/";
	}
	
	/**
	 * Saves global data for a mod (fire-and-forget).
	 * 
	 * @param mod Mod identifier
	 * @param jsonString JSON data to save
	 * @return Call ID or -1 on error
	 * 
	 * @usage UF().globals().Save("MyMod", statsData.ToJson());
	 */
	int Save(string mod, string jsonString) {	
		int cid = -1;	
		string endpoint = "Save/" + mod;
		if (mod && jsonString){
			Post(endpoint,jsonString, UF().RegisterCall(new USilentCallBack(), cid));
		} else {
			UFLog.Err("[Api] Error Saving " + endpoint + " Data for " + mod);
			cid = -1;
		}
		return cid;
	}
	
	/**
	 * Saves global data with callback notification.
	 * 
	 * @param mod Mod identifier
	 * @param jsonString JSON data to save
	 * @param cbInstance Object to call callback on
	 * @param cbFunction Callback method name
	 * @return Call ID or -1 on error
	 * 
	 * @usage UF().globals().Save("MyMod", data.ToJson(), this, "OnSaved");
	 * @note Callback signature: void OnSaved(int cid, int status, string oid, string data)
	 */
	int Save(string mod, string jsonString, Class cbInstance, string cbFunction) {	
		int cid = UF().CallId();	
		string endpoint = "Save/" + mod;		
		if (mod && jsonString){
			Post(endpoint,jsonString, new UDBCallBack(cbInstance, cbFunction, cid, mod));
		} else {
			UFLog.Err("[Api] Error Saving " + endpoint + " Data for " + mod);
			cid = -1;
		}
		return cid;
	}
	/**
	 * Saves global data with UFCallbackBase callback.
	 * 
	 * @param mod Mod identifier
	 * @param jsonString JSON data to save
	 * @param cb UFCallbackBase-derived callback
	 * @return Call ID or -1 on error
	 */
	int Save(string mod, string jsonString, UFCallbackBase cb) {	
		int cid = -1;	
		string endpoint = "Save/" + mod;

		if (mod && jsonString && cb){
			cb.SetOID(mod); //Only sets if not set
			Post(endpoint,jsonString, UF().RegisterCall(new UNestedCallBack(cb), cid));
		} else {
			UFLog.Err("[Api] Error Saving " + endpoint + " Data for " + mod);
			cid = -1;
		}
		return cid;
	}
	
	/**
	 * Loads global data with callback notification.
	 * 
	 * @param mod Mod identifier
	 * @param cbInstance Object to call callback on
	 * @param cbFunction Callback method name
	 * @param jsonString Optional query parameters (default: "{}")
	 * @return Call ID or -1 on error
	 * 
	 * @usage UF().globals().Load("MyMod", this, "OnLoaded");
	 * @note Callback signature: void OnLoaded(int cid, int status, string oid, string data)
	 */
	int Load(string mod, Class cbInstance, string cbFunction, string jsonString = "{}") {
		UFLog.Debug("[UDBGlobalEndpoint::Load] mod=" + mod + " cbFunction=" + cbFunction);
		int cid = -1;
		string endpoint = "Load/" + mod;

		// Safety check: ensure framework is ready
		UFramework uf = UF();
		if (!uf){
			UFLog.Err("[UDBGlobalEndpoint::Load] UF() returned NULL - framework not ready");
			return -1;
		}
		if (!UFConfig()){
			UFLog.Err("[UDBGlobalEndpoint::Load] UFConfig() is NULL - config not loaded");
			return -1;
		}

		if (mod && jsonString){
			RestCallback regCb = uf.RegisterCall(new UDBCallBack(cbInstance, cbFunction, cid, mod), cid);
			if (!regCb){
				UFLog.Err("[UDBGlobalEndpoint::Load] RegisterCall returned NULL");
				return -1;
			}
			Post(endpoint, jsonString, regCb);
		} else {
			UFLog.Err("[Api] Error Loading Player Data for " + mod);
			cid = -1;
		}
		return cid;
	}
	
	/**
	 * Loads global data with UFCallbackBase callback.
	 * 
	 * @param mod Mod identifier
	 * @param cb UFCallbackBase-derived callback
	 * @param jsonString Optional query parameters (default: "{}")
	 * @return Call ID or -1 on error
	 * 
	 * @usage UF().globals().Load("MyMod", new MyLoadCallback());
	 * @note Callback receives parsed object as typed parameter if using UFCallback<T>
	 */
	int Load(string mod, UFCallbackBase cb, string jsonString = "{}") {
		UFLog.Debug("[UDBGlobalEndpoint::Load] mod=" + mod + " with UFCallbackBase");
		int cid = -1;
		string endpoint = "Load/" + mod;
		
		// Safety check: ensure framework is ready
		UFramework uf = UF();
		if (!uf){
			UFLog.Err("[UDBGlobalEndpoint::Load] UF() returned NULL - framework not ready");
			return -1;
		}
		if (!UFConfig()){
			UFLog.Err("[UDBGlobalEndpoint::Load] UFConfig() is NULL - config not loaded");
			return -1;
		}
		
		if (mod && cb && jsonString){
			cb.SetOID(mod); //Only sets if not set
			RestCallback regCb = uf.RegisterCall(new UNestedCallBack(cb), cid);
			if (!regCb){
				UFLog.Err("[UDBGlobalEndpoint::Load] RegisterCall returned NULL");
				return -1;
			}
			Post(endpoint, jsonString, regCb);
		} else {
			UFLog.Err("[Api] Error Loading Player Data for " + mod);
			cid = -1;
		}
		return cid;
	}
	
	/**
	 * Atomically increments a global numeric field (convenience for Transaction).
	 * 
	 * @param mod Mod identifier
	 * @param element Field name
	 * @param value Amount to add (default: 1)
	 * @return Call ID or -1 on error
	 * 
	 * @usage UF().globals().Increment("MyMod", "totalKills", 1);
	 * @note Thread-safe atomic operation
	 */
	int Increment(string mod, string element, float value = 1){
		return Transaction(mod, element, value);
	}
	
	/**
	 * Atomically modifies a global numeric field (fire-and-forget).
	 * 
	 * @param mod Mod identifier
	 * @param element Field name
	 * @param value Amount to add/subtract
	 * @return Call ID or -1 on error
	 * 
	 * @usage UF().globals().Transaction("MyMod", "activePlayers", 1); // Increment
	 * @note Atomic operation - safe for concurrent modifications
	 */
	int Transaction(string mod, string element, float value) {
		int cid = -1;
		string endpoint = "Transaction/" + mod;
		
		autoptr UDBTransaction transaction = new UDBTransaction(element, value);
		
		if ( element && transaction && mod){
			Post(endpoint,transaction.ToJson(), UF().RegisterCall(new USilentCallBack(), cid));
		} else {
			UFLog.Err("[Api] Error Transaction " + mod);
			cid = -1;
		}
		return cid;
	}
	
	/**
	 * Atomically modifies a global numeric field with callback notification.
	 * 
	 * @param mod Mod identifier
	 * @param element Field name
	 * @param value Amount to add/subtract
	 * @param cbInstance Object to call callback on
	 * @param cbFunction Callback method name
	 * @return Call ID or -1 on error
	 * 
	 * @usage UF().globals().Transaction("MyMod", "totalKills", 1, this, "OnUpdated");
	 * @note Atomic operation - thread-safe
	 */
	int Transaction(string mod, string element, float value, Class cbInstance, string cbFunction) {
		int cid = UF().CallId();
		string endpoint = "Transaction/" + mod;
		
		autoptr UDBTransaction transaction = new UDBTransaction(element, value);
		
		if ( element && transaction && mod){
			Post(endpoint,transaction.ToJson(), new UDBCallBack(cbInstance, cbFunction, cid, mod));
		} else {
			UFLog.Err("[Api] Error Transaction " + mod);
			cid = -1;
		}
		return cid;
	}
	
	/**
	 * Atomically modifies a global numeric field with UFCallbackBase callback.
	 * 
	 * @param mod Mod identifier
	 * @param element Field name
	 * @param value Amount to add/subtract
	 * @param cb UFCallbackBase-derived callback
	 * @return Call ID or -1 on error
	 * 
	 * @usage UF().globals().Transaction("MyMod", "serverScore", 100, new MyCallback());
	 * @note Atomic operation - thread-safe
	 */
	int Transaction(string mod, string element, float value, UFCallbackBase cb) {
		int cid = -1;
		string endpoint = "Transaction/" + mod;
		
		autoptr UDBTransaction transaction = new UDBTransaction(element, value);
		
		if ( element && transaction && mod){
			cb.SetOID(mod); //Only sets if not set
			Post(endpoint,transaction.ToJson(),  UF().RegisterCall(new UNestedCallBack(cb), cid));
		} else {
			UFLog.Err("[Api] Error Transaction " + mod);
			cid = -1;
		}
		return cid;
	}
	
	/**
	 * Updates a single global field.
	 * 
	 * @param mod Mod identifier
	 * @param element Field name
	 * @param value New value (JSON-encoded string)
	 * @param operation Update operation (default: UpdateOpts.SET)
	 * @param cbInstance Optional callback object
	 * @param cbFunction Optional callback method name
	 * @return Call ID or -1 on error
	 * 
	 * @usage UF().globals().Update("MyMod", "status", "\"active\"", UpdateOpts.SET, this, "OnUpdated");
	 * @note For strings, must quote: Update(mod, "name", "\"ServerName\"");
	 */	
	int Update(string mod, string element, string value, string operation = UpdateOpts.SET, Class cbInstance = NULL, string cbFunction = "") {	
		int cid = UF().CallId();
		autoptr UFRestCallBackBase DBCBX;
		if (cbInstance && cbFunction != ""){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, mod);
		} else {
			DBCBX = new USilentCallBack();
		}
		
		string endpoint = "Update/" + mod;
		
		autoptr UUpdateData updatedata = new UUpdateData(element, value, operation);
		
		if ( element && updatedata && DBCBX){
			Post(endpoint, updatedata.ToJson(), UF().RegisterCall(DBCBX, cid));
		} else {
			UFLog.Err("[Api] Error Transaction " + mod);
			cid = -1;
		}
		return cid;
	}

}
