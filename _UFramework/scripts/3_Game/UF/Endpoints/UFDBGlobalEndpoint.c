/**
 * UDBGlobalEndpoint class provides global database operation endpoints within the Universal Framework.
 *
 * This class extends UFBaseEndpoint and encapsulates methods for performing various RESTful operations
 * such as saving, loading, incrementing, executing transactions, and updating data related to a specified module.
 *
 * Methods:
 *
 * - EndpointBaseUrl():
 *   Returns the base URL for all endpoints by appending "Globals/" to the base URL obtained from UFConfig.
 *
 * - Save(string mod, string jsonString):
 *   Sends a POST request to save the provided JSON data for the specified module.
 *   Uses a silent callback by default. Prints an error if required parameters are missing.
 *
 * - Save(string mod, string jsonString, Class cbInstance, string cbFunction):
 *   Sends a POST request to save data with an explicit callback defined by the provided instance and function.
 *   Generates a call identifier and prints an error if parameters are invalid.
 *
 * - Save(string mod, string jsonString, UFCallbackBase cb):
 *   Similar to the previous overload, sends a POST request utilizing a callback object. Here, the module identifier
 *   is assigned to the callback before sending if possible.
 *
 * - Load(string mod, Class cbInstance, string cbFunction, string jsonString = "{}"):
 *   Initiates a POST request to load data for the given module using an explicit callback defined by the provided 
 *   class instance and function, accepting an optional JSON parameter (defaults to an empty JSON object).
 *
 * - Load(string mod, UFCallbackBase cb, string jsonString = "{}"):
 *   Loads data using a callback object that implements UFCallbackBase, ensuring the module identifier is set on the callback.
 *
 * - Increment(string mod, string element, float value = 1):
 *   A convenience wrapper for the Transaction method that increments a specified element by a given value (default is 1).
 *
 * - Transaction(string mod, string element, float value):
 *   Executes a transaction POST request by instantiating a UDBTransaction with a specified element and value.
 *   This overload uses a silent callback to process the response.
 *
 * - Transaction(string mod, string element, float value, Class cbInstance, string cbFunction):
 *   Performs the same transaction operation as above but utilizes an explicit callback defined by a class instance
 *   and callback function.
 *
 * - Transaction(string mod, string element, float value, UFCallbackBase cb):
 *   Executes a transaction using a callback object. The module identifier is set on the callback before sending.
 *
 * - Update(string mod, string element, string value, string operation = UpdateOpts.SET, Class cbInstance = NULL, string cbFunction = ""):
 *   Sends a POST request to update a specific element with a new value in the designated module.
 *   The "operation" parameter specifies the type of update (default is a 'SET' operation).
 *   If a callback instance and function are provided, they are used; otherwise, a silent callback is employed.
 *
 * Error Handling:
 *   - All methods validate the presence of required parameters (i.e., module identifier, JSON payload, element, etc.).
 *   - If any parameter is missing or invalid, an error message is printed and a call identifier (-1) is returned,
 *     indicating failure.
 *
 * Callback Registration:
 *   - Callbacks are registered via U().RegisterCall, associating a new callback instance with a call identifier.
 *   - Different callback types (silent, explicit, or nested callbacks) are created based on the method overload.
 */
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
			Post(endpoint,jsonString, U().RegisterCall(new UDBNestedCallBack(cb), cid));
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
			Post(endpoint,jsonString, U().RegisterCall(new UDBNestedCallBack(cb), cid));
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
			Post(endpoint,transaction.ToJson(),  U().RegisterCall(new UDBNestedCallBack(cb), cid));
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
