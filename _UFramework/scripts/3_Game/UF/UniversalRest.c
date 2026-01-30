/**
 * UniversalRest class provides static methods for REST API operations.
 * It abstracts HTTP requests (GET/POST) and handles the creation/configuration of the REST API.
 *
 * Methods:
 *
 * Api()
 *   - Returns a configured instance of RestApi.
 *   - If no instance exists, it creates a new one and configures it with a read operation timeout.
 *
 * Post(string url, string jsonString = "{}", RestCallback UCBX = NULL)
 *   - Sends a POST request to the specified URL with the provided JSON payload.
 *   - Uses a default silent callback (USilentCallBack) if none is provided.
 *   - Sets the authorization header on the REST context before sending the request.
 *
 * Get(string url, RestCallback UCBX = NULL)
 *   - Sends a GET request to the given URL.
 *   - Uses a default silent callback (USilentCallBack) if a callback is not provided.
 *
 * BaseUrl()
 *   - Retrieves the base URL of the API from the application configuration (UFConfig().ServerURL).
 *
 * GetAuth(string guid)
 *   - Constructs a GET authentication request URL by appending the GUID to the base URL.
 *   - Sends a POST request with an empty JSON payload to obtain authentication.
 *   - Registers a callback to handle the response and logs the callback ID.
 *
 * GlobalsSave(string mod, string jsonString, UFRestCallBackBase UCBX = NULL)
 *   - Saves global state data for a specified module.
 *   - Validates input and dispatches a POST request with the provided JSON payload.
 *   - Uses a default silent callback if no callback is specified.
 *
 * GlobalsLoad(string mod, UFRestCallBackBase UCBX, string jsonString = "{}")
 *   - Loads global state data for a specific module.
 *   - Dispatches a POST request and allows an optional JSON payload.
 *   - Uses a default silent callback (USilentCallBack) if one is not provided.
 *
 * GlobalsIncrement(string mod, string element, float value = 1)
 *   - Increments a numeric global element for a module by a specified value.
 *   - Internally calls GlobalsTransaction.
 *
 * GlobalsTransaction(string mod, string element, float value = 1, UFRestCallBackBase UCBX = NULL)
 *   - Processes a transactional update for a global value.
 *   - Creates a transaction payload via a UDBTransaction object.
 *   - Registers a callback and validates the registration.
 *
 * GlobalsUpdate(string mod, string element, string value, UFRestCallBackBase UCBX = NULL)
 *   - Updates a global string value for a module.
 *   - Requires that string values be wrapped in quotes.
 *   - Sends a POST request with a JSON payload created from a UUpdateData object.
 *
 * GlobalsUpdateAdv(string mod, string element, string value, string operation, UFRestCallBackBase UCBX = NULL)
 *   - Executes an advanced update for a global value, allowing a specific operation.
 *   - Builds the JSON payload using the UUpdateData object.
 *
 * Request(UApiForwarder data, UFRestCallBackBase UCBX = NULL)
 *   - Forwards a custom API request using data encapsulated in a UApiForwarder object.
 *   - Registers the callback and sends the request.
 *
 * Log(string jsonString, UFRestCallBackBase UCBX = NULL)
 *   - Sends a single log entry to the logging endpoint.
 *   - Constructs the URL with the server ID and dispatches a POST request.
 *
 * LogBulk(string jsonString, UFRestCallBackBase UCBX = NULL)
 *   - Submits multiple log entries in a bulk logging operation.
 *   - Constructs the proper endpoint URL using the server ID.
 */
class UniversalRest extends Managed
{		
	protected static RestApi Api()
	{
		RestApi clCore = GetRestApi();
		if (!clCore)
		{
			clCore = CreateRestApi();
			clCore.SetOption(ERestOption.ERESTOPTION_READOPERATION, 15);
		}
		return clCore;
	}
	
	protected static void Post(string url, string jsonString = "{}", RestCallback UCBX = NULL)
	{
		autoptr RestCallback vUCBX = UCBX;
		if (!vUCBX){
			vUCBX = new USilentCallBack;
		}
		RestContext ctx =  Api().GetRestContext(url);
		string token = U().GetAuthToken();
		if (token == "" || token == "null"){
			// Use Print() NOT UFLog to avoid infinite loop (UFLog.SendToApi -> Post -> UFLog -> SendToApi...)
			Print("[UF] [Api] UniversalRest.Post called with invalid token: '" + token + "'");
		}
		ctx.SetHeader(token);
		// Use Print() NOT UFLog to avoid infinite loop
		// Print("[UF] [Api] POST " +  url);
		ctx.POST(vUCBX , "", jsonString);
	}
	
	protected static void Get(string url, RestCallback UCBX = NULL)
	{
		autoptr RestCallback vUCBX = UCBX;
		if (!vUCBX){
			vUCBX = new USilentCallBack;
		}
		RestContext ctx =  Api().GetRestContext(url);
		ctx.GET(vUCBX , "");
	}
	
	/**
	 * Gets base API URL from configuration.
	 * 
	 * @return Base URL string from UFConfig().ServerURL
	 */
	protected static string BaseUrl(){
		return UFConfig().ServerURL;
	}
	
	/**
	 * Requests authentication token for a player (server-side only).
	 * 
	 * @param guid Player GUID to request auth for
	 * 
	 * @note Internal use - called by UFramework.PreparePlayerAuth()
	 * @note Uses UAuthCallBack to handle response
	 */
	static void GetAuth( string guid ){
		string url = BaseUrl() + "GetAuth/" + guid;
		
		int cid = -1;
		Post(url, "{}", U().RegisterCall(new UAuthCallBack(guid), cid));
		Print("Get Auth Called got CID: " + cid);
		if (cid == -1){
			Error2("[UF] Error failed to register callback with UF", "GetAuth");
		}
	}
	
	/**
	 * Saves global data (legacy method - prefer U().globals().Save()).
	 * 
	 * @param mod Mod identifier
	 * @param jsonString JSON data to save
	 * @param UCBX Optional RestCallback
	 * 
	 * @deprecated Use U().globals().Save() instead for better callback support
	 */
	static void GlobalsSave(string mod, string jsonString, UFRestCallBackBase UCBX = NULL) {

		string url = BaseUrl() + "Globals/Save/" + mod;
		autoptr UFRestCallBackBase vUCBX = UCBX;
		if (!vUCBX){
			vUCBX = new USilentCallBack;
		}
				
		if (jsonString){
			Post(url,jsonString,vUCBX);
		} else {
			UFLog.Err("[Api] Error Saving Globals Data for " + mod);
		}
	}
	
	/**
	 * Loads global data (legacy method - prefer U().globals().Load()).
	 * 
	 * @param mod Mod identifier
	 * @param UCBX RestCallback for response
	 * @param jsonString Optional query parameters
	 * 
	 * @deprecated Use U().globals().Load() instead for better callback support
	 */
	static void GlobalsLoad(string mod, UFRestCallBackBase UCBX, string jsonString = "{}") {

		string url = BaseUrl() + "Globals/Load/" + mod;
		autoptr UFRestCallBackBase vUCBX = UCBX;
		if (!vUCBX){
			vUCBX = new USilentCallBack;
		}

		if (vUCBX){
			Post(url,jsonString,vUCBX);
		} else {
			UFLog.Err("[Api] Error Loading Globals Data for " + mod);
		}
	}
	
	/**
	 * Increments global field (legacy - prefer U().globals().Increment()).
	 * 
	 * @param mod Mod identifier
	 * @param element Field name
	 * @param value Amount to add
	 * 
	 * @deprecated Use U().globals().Increment() instead
	 */
	static void GlobalsIncrement(string mod, string element, float value = 1){
		GlobalsTransaction(mod, element, value, NULL);
	}
	
	/**
	 * Global transaction (legacy - prefer U().globals().Transaction()).
	 * 
	 * @param mod Mod identifier
	 * @param element Field name
	 * @param value Amount to add/subtract
	 * @param UCBX Optional RestCallback
	 * 
	 * @deprecated Use U().globals().Transaction() instead
	 */
	static void GlobalsTransaction(string mod, string element, float value = 1, UFRestCallBackBase UCBX = NULL) {
		
		autoptr UFRestCallBackBase vUCBX = UCBX;
		if (!vUCBX){
			vUCBX = new UDBTransactionCallBack;
		}
		string url = BaseUrl() + "Globals/Transaction/" + mod;

		
		autoptr UDBTransaction transaction = new UDBTransaction(element, value);
		int cid = -1;
		if ( element && transaction && vUCBX){
			Post(url,transaction.ToJson(), U().RegisterCall(vUCBX, cid));
		} else {
			UFLog.Err("[Api] Error Transaction " +  mod);
		}
		if (cid == -1){
			Error2("[UF] Error failed to register callback with UF", "GetAuth");
		}
	}
	
	//String Values must be wrapped with Quotes example string newValue = "\"NewValue\""
	static void GlobalsUpdate(string mod, string element, string value, UFRestCallBackBase UCBX = NULL) {
		
		autoptr UFRestCallBackBase vUCBX = UCBX;
		if (!vUCBX){
			vUCBX = new USilentCallBack;
		}

		string url = BaseUrl() + "Globals/Update/" + mod;
		int cid = -1;
		
		autoptr UUpdateData updatedata = new UUpdateData(element, value);
		
		if ( element && updatedata && vUCBX){
			Post(url,updatedata.ToJson(),U().RegisterCall(vUCBX, cid));
		} else {
			UFLog.Err("[Api] Error Transaction " +  mod);
		}
	}
	
	//String Values must be wrapped with Quotes example string newValue = "\"NewValue\""
	static void GlobalsUpdateAdv(string mod, string element, string value, string operation, UFRestCallBackBase UCBX = NULL) {
		
		autoptr UFRestCallBackBase vUCBX = UCBX;
		if (!vUCBX){
			vUCBX = new USilentCallBack;
		}
		int cid = -1;

		string url = BaseUrl() + "Globals/Update/" + mod;
		
		autoptr UUpdateData updatedata = new UUpdateData(element, value, operation);
		
		if ( element && updatedata && vUCBX){
			Post(url,updatedata.ToJson(),U().RegisterCall(vUCBX, cid));
		} else {
			UFLog.Err("[Api] Error Transaction " +  mod);
		}
	}
	
	static void Request(UApiForwarder data, UFRestCallBackBase UCBX = NULL){
				
		autoptr UFRestCallBackBase vUCBX = UCBX;
		if (!vUCBX){
			vUCBX = new USilentCallBack;
		}
		
		int cid = -1;
		
		string url = BaseUrl() + "Forward";
		
		if ( data && vUCBX){
			Post(url,data.ToJson(),U().RegisterCall(vUCBX, cid));
		} else {
			UFLog.Err("[Api] Error Forwarding");
		}
	}

	static void Log(string jsonString, UFRestCallBackBase UCBX = NULL){
		
		autoptr UFRestCallBackBase vUCBX = UCBX;
		if (!vUCBX){
			vUCBX = new USilentCallBack;
		}
		
		int cid = -1;
		string url = BaseUrl() + "Logger/One/" + UFConfig().ServerID;
		
		if ( jsonString && vUCBX){
			Post(url,jsonString,U().RegisterCall(vUCBX, cid));
		} else {
			// Use Print() NOT UFLog to avoid infinite loop (UFLog -> SendToApi -> Log -> UFLog...)
			Print("[UF] [Api] Error Forwarding Log");
		}
	}
	
	//JsonFileLoader<array<autoptr LogObject>>.JsonMakeData(AnArrayOfYourObjects);
	static void LogBulk(string jsonString, UFRestCallBackBase UCBX = NULL){
		
		autoptr UFRestCallBackBase vUCBX = UCBX;
		if (!vUCBX){
			vUCBX = new USilentCallBack;
		}
		int cid = -1;
		
		string url = BaseUrl() + "Logger/Many/" + UFConfig().ServerID;
		if (jsonString && vUCBX){
			Post(url,jsonString,U().RegisterCall(vUCBX, cid));
		} else {
			// Use Print() NOT UFLog to avoid infinite loop
			Print("[UF] [Api] Error Forwarding LogBulk");
		}
	}	
	
};