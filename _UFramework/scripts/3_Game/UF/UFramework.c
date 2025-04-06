/**
 * UFramework Class Documentation
 *
 * The UFramework class acts as the central management hub for the Universal Framework within the DayZ mod environment.
 * It consolidates various functionalities such as endpoint management (for database, Discord, globals, and API operations),
 * REST API call handling, RPC communication, authentication token management, and periodic tasks scheduling.
 *
 * Main Functionalities:
 * 
 * 1. Endpoint Getters:
 *    - db(int collection = OBJECT_DB): Returns the proper database endpoint instance depending on whether player data (PLAYER_DB) or object data (OBJECT_DB) is required.
 *    - ds(): Retrieves the Discord endpoint, responsible for Discord-related functionality.
 *    - globals(): Provides access to the global endpoint used in handling global data.
 *    - api(): Returns the API endpoint that processes various application-specific REST API calls.
 *    - Cron(): Provides the UCronManager for scheduling recurring tasks.
 *
 * 2. REST API Utilities:
 *    - Post(string url): Simplified interface for sending POST requests with default JSON headers.
 *    - Overloaded Post(string url, string jsonString, ...) methods: Several variants support providing JSON payloads, specifying content types, and handling callbacks.
 *    - Get(string url): Simplified interface for sending GET requests.
 *    - Overloaded Get(string url, ...) methods: Variants that support providing callbacks, both as RestCallback or UFCallbackBase.
 *
 * 3. Authentication and Player Management:
 *    - GetAuthToken() & HasValidAuth(): Manage and validate the current authentication token based on context (client vs. server).
 *    - FindPlayer(string GUID) & FindPlayerByIdentity(PlayerIdentity identity): Helper functions to locate player objects in the game based on GUID or identity.
 *    - RequestAuthToken(bool first = false): Method used by clients to request a token via RPC.
 *    - AddPlayerAuth() and GetPlayerAuth(): Enable the caching of player authentication tokens and retrieval on demand.
 *
 * 4. RPC Communication:
 *    - RPCUFrameworkConfig: Processes RPC calls to update the framework configuration and authentication tokens.
 *    - RPCRequestAuthToken & RPCRequestRetry: Handle the token renewal process and client-server communication for authentication.
 *    - SendAuthToken: Dispatches a newly received authentication token to the corresponding client.
 *
 * 5. Callbacks and Async Task Management:
 *    - RegisterCall(), ClearCallback(), and IsCallCanceled(): Mechanisms for managing asynchronous REST calls,
 *      assigning unique call identifiers, and cleaning up completed or canceled calls.
 *    - CheckAndRenewQRandom() and GetQRandomNumbers(): Functions for monitoring and renewing the pool of random numbers used
 *      in various operations, with integration into the Cron manager for periodic execution.
 *
 * 6. Utility and Error Handling:
 *    - ErrorToString(int ErrorCode): Converts REST error codes to human-readable string messages.
 *    - DiscordNotification Methods:
 *         - DiscordMessage() and DiscordObject(): Provide simple static interfaces to send messages or objects to Discord channels via webhooks.
 *
 * 7. Global Initialization:
 *    - The static U() function ensures that there is a singleton instance of UFramework and initializes global settings and RPC listeners.
 *
 * Additional Notes:
 *    - Designed to work in both client and server contexts with conditional behavior based on the execution environment.
 *    - Integrates with external services including RESTful web services and Discord for broader functionality.
 *    - Incorporates robust error handling and retry mechanisms, especially in the context of authentication and web service connectivity.
 *    - The class destructor ensures cleanup of RPC queues and proper memory management for the authentication token.
 *
 * Overall, UFramework centralizes the operations required by the Universal Framework, facilitating communication,
 * configuration, and periodic task execution, making it a fundamental component in the DayZ Universal API infrastructure.
 */
class UFramework extends Managed {
		
	/**
	 * Getter function for the Database Endpoint.
	 *
	 * Function: db(int collection = OBJECT_DB)
	 * - Returns a database endpoint, which is based on the provided collection type.
	 * - When collection equals OBJECT_DB:
	 *    - Returns an object endpoint, creating a new one if m_ObjectEndPoint has not been initialized.
	 * - When collection equals PLAYER_DB:
	 *    - Returns a player endpoint, creating a new one if m_PlayerEndPoint has not been initialized.
	 * - Returns NULL if the collection type is neither OBJECT_DB nor PLAYER_DB.
	 *
	 * Getter function for the Discord Endpoint.
	 *
	 * Function: ds()
	 * - Returns the Discord endpoint.
	 * - If m_UniversalDSEndpoint is not yet initialized, a new instance is created.
	 *
	 * Getter function for the Globals Endpoint.
	 *
	 * Function: globals()
	 * - Returns the globals endpoint.
	 * - If m_UDBGlobalEndpoint is not yet created, a new one is instantiated.
	 *
	 * Getter function for the API Endpoint.
	 *
	 * Function: api()
	 * - Returns the API endpoint.
	 * - If m_UApiEndpoint is not yet instantiated, it's created.
	 *
	 * Getter function for the Cron Manager.
	 *
	 * Function: Cron()
	 * - Returns the cron manager instance.
	 * - If m_UCronManager has not been initialized, a new instance is created and its Init() function is called.
	 */
	//Getter function for the Database Endpoint using either OBJECT_DB or PLAYER_DB
	// "PLAYER_DB" is only accessable on client for the player info being requested 
	// "OBJECT_DB" all clients can access all data.
	UDBEndpoint db(int collection = OBJECT_DB){
		if (collection == OBJECT_DB){
			if (!m_ObjectEndPoint){
				m_ObjectEndPoint = new UDBEndpoint("Object");
			}
			return m_ObjectEndPoint;
		} 
		if (collection == PLAYER_DB){
			if (!m_PlayerEndPoint){
				m_PlayerEndPoint = new UDBEndpoint("Player");
			}
			return m_PlayerEndPoint;
		}
		return NULL;
	}
	
	//Getter function for the Discord Endpoint
	UniversalDSEndpoint ds(){
		if (!m_UniversalDSEndpoint){
			m_UniversalDSEndpoint = new UniversalDSEndpoint;
		}
		return m_UniversalDSEndpoint;
	}
	
	//Getter function for the Globals Endpoint
	UDBGlobalEndpoint globals(){
		if (!m_UDBGlobalEndpoint){
			m_UDBGlobalEndpoint = new UDBGlobalEndpoint;
		}
		return m_UDBGlobalEndpoint;
	}
	
	//Getter function for the API Endpoint
	UApiEndpoint api(){
		if (!m_UApiEndpoint){
			m_UApiEndpoint = new UApiEndpoint;
		}
		return m_UApiEndpoint;
	}
	
	UCronManager Cron(){
		if (!m_UCronManager){
			m_UCronManager = new UCronManager;
			m_UCronManager.Init();
		}
		return m_UCronManager;
	}

	UFMsgEndpoint Msg(){
		if (m_UFMsgEndpoint){
			m_UFMsgEndpoint = new UFMsgEndpoint;
		}
		return m_UFMsgEndpoint;
	}
	
	UFAIChatEndpoint AI(){
		if (m_UFAIChatEndpoint){
			m_UFAIChatEndpoint = new UFAIChatEndpoint;
		}
		return m_UFAIChatEndpoint;
	}
	/**
	 * RequestCallCancel
	 * -----------------
	 * Requests the cancellation of an active call by inserting the provided call ID (cid) into a cancellation list.
	 *
	 * @param cid The call identifier to be canceled.
	 */

	/**
	 * Post (overload 1)
	 * -----------------
	 * Sends a POST HTTP request to the specified URL using default parameters.
	 * The content type is set to "application/json", and a silent callback is used.
	 *
	 * @param url The endpoint URL for the POST request.
	 * @return An integer status value (typically 0).
	 */

	/**
	 * Post (overload 2)
	 * -----------------
	 * Sends a POST HTTP request to the specified URL with a provided JSON string.
	 * If no custom RestCallback is provided, a default silent callback is used.
	 *
	 * @param url The endpoint URL for the POST request.
	 * @param jsonString The JSON formatted string to be sent in the request body.
	 * @param UCBX (Optional) A custom RestCallback to handle the response.
	 * @param contentType (Optional) The MIME type for the content header; defaults to "application/json".
	 * @return An integer status value (typically 0).
	 */

	/**
	 * Post (overload 3)
	 * -----------------
	 * Sends a POST HTTP request to the specified URL with a provided JSON string.
	 * A user-defined callback derived from UFCallbackBase is registered to process the response.
	 *
	 * @param url The endpoint URL for the POST request.
	 * @param jsonString The JSON formatted string to be sent in the request body.
	 * @param cb A user-defined callback of type UFCallbackBase used to handle the response.
	 * @param contentType (Optional) The MIME type for the content header; defaults to "application/json".
	 * @return An integer call identifier if the callback is provided; otherwise, -1.
	 */

	/**
	 * Get (overload 1)
	 * ----------------
	 * Sends a GET HTTP request to the specified URL using a default silent callback.
	 *
	 * @param url The endpoint URL for the GET request.
	 * @return An integer status value (typically 0).
	 */

	/**
	 * Get (overload 2)
	 * ----------------
	 * Sends a GET HTTP request to the specified URL with a provided RestCallback.
	 *
	 * @param url The endpoint URL for the GET request.
	 * @param UCBX The RestCallback to handle the response.
	 * @return An integer status value (typically 0).
	 */

	/**
	 * Get (overload 3)
	 * ----------------
	 * Sends a GET HTTP request to the specified URL with a user-defined callback derived from UFCallbackBase.
	 *
	 * @param url The endpoint URL for the GET request.
	 * @param cb A user-defined callback of type UFCallbackBase used to handle the response.
	 * @return An integer call identifier if the callback is provided; otherwise, -1.
	 */

	/**
	 * IsDiscordEnabled
	 * ----------------
	 * Checks whether the Discord endpoint is configured.
	 *
	 * @return True if the Discord endpoint is enabled; otherwise, false.
	 */

	/**
	 * IsOnline
	 * --------
	 * Determines if the mod has successfully completed its operational status check.
	 *
	 * @return True if the mod is online and operational; otherwise, false.
	 */

	/**
	 * VersionOffset
	 * -------------
	 * Returns the version offset to indicate compatibility:
	 *  - 0: Versions match exactly.
	 *  - ±1: Off by a patch version; minor issues.
	 *  - ±2: Off by a minor version; some endpoints or features might be missing.
	 *  - ±3: Off by a major version; mod functionality may be severely impacted.
	 *
	 * @return An integer representing the version offset.
	 */

	/**
	 * CheckAndRenewQRandom
	 * --------------------
	 * Monitors the remaining pool of QRandom numbers and triggers a renewal process if the available
	 * random numbers fall below 2000.
	 */

	/**
	 * GetVersion
	 * ----------
	 * Retrieves the current version of the mod.
	 *
	 * @return A string representing the mod's version as defined by UF_VERSION.
	 */

	/**
	 * FindPlayer
	 * ----------
	 * Searches for a player on the server by their Global Unique Identifier (GUID).
	 *
	 * @param GUID The unique identifier of the player.
	 * @return A DayZPlayer object if a matching player is found; otherwise, null.
	 */

	/**
	 * FindPlayerByIdentity
	 * --------------------
	 * Searches for a player using their PlayerIdentity object.
	 *
	 * @param identity The PlayerIdentity object associated with the player.
	 * @return A DayZPlayer object if a matching player is found; otherwise, null.
	 */

	/**
	 * Global Initialization Control (m_isInit, isGlobalInit, setGlobalInit)
	 * ---------------------------------------------------------------------
	 * m_isInit: A protected static boolean flag that indicates whether the framework has been globally initialized.
	 *
	 * isGlobalInit:
	 * - A static function that returns the current global initialization state.
	 *
	 * setGlobalInit:
	 * - A static function that sets the global initialization flag to true.
	 */
	//Request a call to be canceled
	void RequestCallCancel(int cid){
		m_CanceledCalls.Insert(cid);
	}
	
	//A super simple Post Interface to help people
	static int Post(string url)
	{
		RestContext ctx = RestCore().GetRestContext(url);
		ctx.SetHeader("application/json");
		ctx.POST(new USilentCallBack, "", "{}");
		return 0;
	}
	
	//A super simple Post Interface to help people
	static int Post(string url, string jsonString, RestCallback UCBX = NULL, string contentType = "application/json")
	{
		autoptr RestCallback vUCBX = UCBX;
		if (!vUCBX){
			vUCBX = new USilentCallBack;
		}
		RestContext ctx = RestCore().GetRestContext(url);
		ctx.SetHeader(contentType);
		ctx.POST(vUCBX, "", jsonString);
		return 0;
	}
	
	//A super simple Post Interface to help people
	static int Post(string url, string jsonString, UFCallbackBase cb, string contentType = "application/json")
	{
		int cid = -1;
		if (cb){
			RestContext ctx = RestCore().GetRestContext(url);
			ctx.SetHeader(contentType);
			ctx.POST(U().RegisterCall(new UNestedCallBack(cb),cid), "", jsonString);
			return cid;
		}
		return -1;
	}
	
	//A super simple Get Interface to help people
	static int Get(string url)
	{
		RestContext ctx =  RestCore().GetRestContext(url);
		ctx.GET(new USilentCallBack, "");
		return 0;
	}
	//A super simple Get Interface to help people
	static int Get(string url, RestCallback UCBX)
	{
		autoptr RestCallback vUCBX = UCBX;
		if (!vUCBX){
			vUCBX = new USilentCallBack;
		}
		RestContext ctx =  RestCore().GetRestContext(url);
		ctx.GET(vUCBX , "");
		return 0;
	}
	//A super simple Get Interface to help people
	static int Get(string url, UFCallbackBase cb)
	{
		int cid = -1;
		if (cb){
			RestContext ctx =  RestCore().GetRestContext(url);
			ctx.GET(U().RegisterCall(new UNestedCallBack(cb), cid), "");
			return cid;
		}
		return -1;
	}
	
	//Will return true if the discord endpoint is configured (this doesn't mean its configured correctly though :p)
	bool IsDiscordEnabled(){
		return m_UDiscordEnabled;
	}
	
	//Returns True if the status check has come back and everything is okay
	bool IsOnline(){
		return m_UFOnline;
	}
	
	//Returns current Version Offset 0 Version Matches exactly
	// -1 or 1 off by a patch this is not a problem and won't cause any major issues
	// -2 or 2 off by a Minor Version this may cause some endpoints to not work or features to be missing
	// -3 or 3 off by a Major Version most likely the mod will not work at all!
	int VersionOffset(){
		return m_UFVersionOffset;
	}
	
	//Checks to see if the Random Numbers are below half and add's more
	void CheckAndRenewQRandom(){
		if (Math.QRandomRemaining()<= 2000){
			GetQRandomNumbers();
		}
	}
	
	//Returns Current Version of the Mod
	static string GetVersion(){
		return UF_VERSION;
	}
	
	//Simple function for finding a player based on their GUID
	static DayZPlayer FindPlayer(string GUID){
		if (GetGame().IsServer()){
			autoptr array<Man> players = new array<Man>;
			GetGame().GetPlayers( players );
			for (int i = 0; i < players.Count(); i++){
				DayZPlayer player = DayZPlayer.Cast(players.Get(i));
				if (player.GetIdentity() && player.GetIdentity().GetId() == GUID ){
					return player;
				}
			}
		}
		return NULL;
	}
	
	//Simple function for finding a player based on their identity
	static DayZPlayer FindPlayerByIdentity(PlayerIdentity identity) {
		if (!identity)
			return NULL;

		int highBits;
		int lowBits;
		GetGame().GetPlayerNetworkIDByIdentityID(identity.GetPlayerId(), lowBits, highBits);
		return DayZPlayer.Cast(GetGame().GetObjectByNetworkId(lowBits, highBits));
	}
	
	
	
	
	protected static bool m_isInit = false;
	
	static bool isGlobalInit(){
		return m_isInit;
	}
	static void setGlobalInit(){
		m_isInit = true;
	}
	
	//Stuff that you don't need to worry about :P
	
	protected bool m_IsServer = false;
	
	protected int m_CallId = 0;
	protected int m_AuthRetries = 0;
	
	protected bool m_UFOnline = false;
	protected int m_UFVersionOffset = 0;
	protected bool m_UDiscordEnabled = false;
	protected bool m_UOpenAIEnabled = false;
	
	protected bool UF_Init = false;
	protected autoptr ApiAuthToken m_UFauthToken;
	
	protected autoptr UniversalRest m_UniversalRest;
	
	protected autoptr UniversalDSEndpoint m_UniversalDSEndpoint;
	protected autoptr UDBGlobalEndpoint m_UDBGlobalEndpoint;
	
	protected autoptr UCronManager m_UCronManager;
	
	protected autoptr UFMsgEndpoint m_UFMsgEndpoint;
	
	protected autoptr UFAIChatEndpoint m_UFAIChatEndpoint;
	
	protected autoptr UDiscordUser dsUser;
		
	protected autoptr map<string, string> PlayerAuths = new map<string, string>;
	
	protected autoptr UDBEndpoint m_PlayerEndPoint;
	
	protected autoptr UDBEndpoint m_ObjectEndPoint;
	//Can't Do Globals due to how globals work
	
	protected autoptr UApiEndpoint m_UApiEndpoint;
	
	protected autoptr TIntSet m_CanceledCalls = new TIntSet;
	
	protected int LastRandomNumberRequestCall = -1;
	
	autoptr map<int,UFRestCallBackBase> m_UCallBacks = new map<int,UFRestCallBackBase>;
		
	/**
	 * Returns the static RestApi instance. If it does not exist, the method creates a new one and sets its
	 * read operation option to 15.
	 *
	 * @return RestApi instance used for API interactions.
	 */

	/**
	 * Retrieves the authentication token. For client instances, if a valid m_UFauthToken exists, returns its token;
	 * for server instances, returns the token from the server configuration (UFConfig().ServerAuth).
	 * If neither condition is met, returns the string "null".
	 *
	 * @return String representing the authentication token.
	 */

	/**
	 * Validates the current authentication token by ensuring it is neither "null", "error", "ERROR", nor an empty string.
	 *
	 * @return Boolean value indicating whether the authentication token is valid.
	 */

	/**
	 * Returns the UniversalRest instance to handle REST callback endpoints (legacy system). Lazily initializes 
	 * the instance if it is not already available.
	 *
	 * @return UniversalRest instance for processing REST callbacks.
	 */

	/**
	 * Destructor of the UFramework class. On server instances where the framework is initialized, this method
	 * removes scheduled callbacks from the game's system call queue and deallocates the m_UFauthToken to free resources.
	 */

	/**
	 * Initializes the UFramework by detecting the server environment, setting global initialization flags,
	 * and registering necessary RPC endpoints for subsequent REST API calls and authentication processes.
	 * For servers, it also schedules periodic status checks and renewals for Q Random numbers.
	 */

	/**
	 * RPC handler for receiving UFramework configuration and authentication token from the server.
	 * Upon receiving valid data, this method resets authentication retry counts and schedules further processing,
	 * such as updating service statuses and retrieving Discord user info.
	 *
	 * @param type CallType of the RPC.
	 * @param ctx Context for reading parameters, expected to carry ApiAuthToken and UFrameworkConfig.
	 * @param sender PlayerIdentity that sent the RPC call.
	 * @param target The RPC call target object.
	 */

	/**
	 * Callback function invoked after the authentication token has been received and processed.
	 * It logs the receipt of the token, triggers a status update on the API, initiates Discord user info retrieval,
	 * and notifies the mission object that the framework is ready.
	 */

	/**
	 * RPC handler to manage retry requests for fetching an authentication token. If running on a client,
	 * it increments a retry counter and schedules another token request after an increasing delay, up to 20 times.
	 *
	 * @param type CallType of the RPC.
	 * @param ctx Context for reading the retry flag.
	 * @param sender PlayerIdentity requesting the retry.
	 * @param target The RPC call target object.
	 */

	/**
	 * Requests an authentication token by sending the appropriate RPC call to the server.
	 * This method is only executed on client instances.
	 *
	 * @param first Boolean flag indicating whether this is the initial request.
	 */

	/**
	 * Initiates a REST-based authentication process for a given player by GUID.
	 *
	 * @param guid Unique identifier for the player.
	 */

	/**
	 * Caches the player's authentication token in a map structure. If the player is currently connected,
	 * the token is directly sent to the player's identity.
	 *
	 * @param guid Unique identifier for the player.
	 * @param auth Authentication token to be cached and possibly delivered immediately.
	 */

	/**
	 * Retrieves a cached authentication token for a player.
	 * If the token is found in the cache, it is returned via the out parameter and the method returns true.
	 * Otherwise, an error is logged and the method returns false.
	 *
	 * @param guid Unique identifier for the player.
	 * @param auth Out parameter to hold the retrieved authentication token.
	 * @return Boolean indicating whether a valid token was found.
	 */

	/**
	 * RPC handler for client requests to fetch an authentication token.
	 * Verifies server configuration and either sends a cached token, triggers a token renewal, or requests a retry.
	 *
	 * @param type CallType of the RPC.
	 * @param ctx Context for reading the retry flag.
	 * @param sender PlayerIdentity requesting the authentication token.
	 * @param target The RPC call target object.
	 */

	/**
	 * Sends the authentication token and partial configuration to a specified player's identity via RPC.
	 * Ensures that the target identity and token are valid before transmission.
	 *
	 * @param idenitity PlayerIdentity receiving the token.
	 * @param auth The authentication token string to be sent.
	 */

	/**
	 * Handles authentication errors for a given player GUID.
	 * Depending on the online status and whether the error occurs on the server or client,
	 * schedules a retry for obtaining the authentication token after a predefined delay.
	 *
	 * @param guid Unique identifier for the player with the authentication error.
	 */

	/**
	 * Sends a Discord message by constructing a Discord object with content, bot name, and avatar URL,
	 * then posting it to the specified webhook URL.
	 *
	 * @param webhookUrl The Discord webhook URL to post the message.
	 * @param message The message content.
	 * @param botName Optional parameter for the bot's name.
	 * @param botAvatarUrl Optional parameter for the bot's avatar URL.
	 */

	/**
	 * Sends a Discord object by converting it to JSON format and posting it to the specified Discord webhook URL.
	 *
	 * @param webhookUrl The Discord webhook URL.
	 * @param discordObject The constructed UDiscordObject containing message details.
	 */

	/**
	 * Converts a REST error code into its corresponding human-readable string description.
	 *
	 * @param ErrorCode An integer representing the REST error code.
	 * @return String description corresponding to the error code.
	 */

	/**
	 * Generates a unique call identifier by incrementing an internal counter.
	 *
	 * @return Integer representing a new unique call ID.
	 */

	/**
	 * Registers a REST callback by assigning it a unique call ID and inserting it into an internal map.
	 * The method returns the registered callback instance.
	 *
	 * @param cb Instance of UFRestCallBackBase representing the callback.
	 * @param cid Output parameter to store the unique call ID.
	 * @return UFRestCallBackBase instance that has been registered.
	 */

	/**
	 * Clears a previously registered callback based on its call ID.
	 * If the callback is found, it is deleted and removed from the internal callbacks map.
	 * Logs an error if the callback cannot be found.
	 *
	 * @param cid Unique call ID of the callback to clear.
	 * @param traceDebug Additional debugging information for logging purposes.
	 */

	/**
	 * Checks whether a given call (identified by its call ID) has been marked as cancelled.
	 *
	 * @param cid The unique call ID to check.
	 * @return Boolean indicating whether the call has been cancelled.
	 */

	/**
	 * Initiates a REST API call to fetch new random numbers (Q Random Numbers) for internal use.
	 * Ensures that a previous request is not still in progress before making a new one.
	 */

	/**
	 * Callback function handling the response from the Q Random Numbers request.
	 * On success, updates the internal Q Random Numbers and reseeds the vanilla random number generator.
	 * Logs an error if the retrieval fails.
	 *
	 * @param cid Unique call ID associated with the random number request.
	 * @param status REST API status code for the request.
	 * @param oid Operation identifier.
	 * @param data Response data containing the new random numbers.
	 */

	/**
	 * Callback function for processing the web service status check response.
	 * Evaluates the service version, authentication status, Discord service availability, and general error conditions.
	 * Depending on the received data, it sets internal status flags, logs warnings or errors,
	 * and handles service version mismatches.
	 *
	 * @param cid Unique call ID for the status check request.
	 * @param status REST API status code indicating success, error, or timeout.
	 * @param oid Operation identifier.
	 * @param data UFStatus object containing service details and potential errors.
	 */
	protected static RestApi RestCore()
	{
		RestApi clCore = GetRestApi();
		if (!clCore) {
			clCore = CreateRestApi();
			clCore.SetOption(ERestOption.ERESTOPTION_READOPERATION, 15);
		}
		return clCore;
	}
	
	string GetAuthToken(){
		if (m_UFauthToken && !GetGame().IsServer()){
			if (m_UFauthToken.IsExpired()) RequestAuthToken(false); //Shouldn't ever be expired but just encase
			return m_UFauthToken.GetAuthToken();
		} else if (GetGame().IsServer() && UFConfig().ServerAuth != ""){
			return UFConfig().ServerAuth;
		}
		return "null";
	}
	
	bool HasValidAuth(){
		return (!m_UFauthToken.IsExpired() && GetAuthToken() != "null" && GetAuthToken() != "error" && GetAuthToken() != "ERROR" && GetAuthToken() != "" );
	}
	
	
	//OLD RestCallBack Endpoints use if you want to use RestCallBack Classes instead of Function Based
	UniversalRest Rest(){
		if (!m_UniversalRest){
			m_UniversalRest = new UniversalRest;
		}
		return m_UniversalRest;
	}	
	
	
	void ~UFramework(){
		if (m_IsServer && UF_Init && GetGame()){
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(this.CheckAndRenewQRandom);
		}
		delete m_UFauthToken;
	}
	
	void Init(){
		#ifdef NO_GUI
			Print("[UF] Detected Server");
			m_IsServer = true;
		#endif
		if (!UF_Init){
			setGlobalInit();
			Print("[UF] First Init");
			UF_Init = true;
			GetRPCManager().AddRPC( "UF", "RPCUFrameworkConfig", this, SingeplayerExecutionType.Both );
			GetRPCManager().AddRPC( "UF", "RPCRequestAuthToken", this, SingeplayerExecutionType.Both );
			GetRPCManager().AddRPC( "UF", "RPCRequestRetry", this, SingeplayerExecutionType.Both );
			if (m_IsServer){
				U().api().Status(this, "CBStatusCheck");
				CheckAndRenewQRandom();
				GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.CheckAndRenewQRandom, 10 * 60 * 1000, true);
			}
		}
	}
	
	protected void RPCUFrameworkConfig( CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target )
	{
		Print("[UF] Received UF Config");
		Param2<ApiAuthToken, UFrameworkConfig> data; 
		if ( !ctx.Read( data ) ) return;
		m_AuthRetries = 0;
		m_UFauthToken = data.param1;
		m_UFrameworkConfig = data.param2;
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Call(this.OnTokenReceived);
	}
	
	protected void OnTokenReceived(){
		Print("[UF] [UAPI] Token received from server, initialize services");
		U().api().Status(this, "CBStatusCheck");
		U().ds().GetUser(GetDayZGame().GetSteamId(), GetDayZGame(), "CBCacheDiscordInfo");
		GetGame().GameScript.CallFunction(GetGame().GetMission(), "UFrameworkReadyTokenReceived", NULL, NULL);
		CheckAndRenewQRandom();
		Print("[UF] OnTokenReceived Proccessed");
	}
	
	
	void RPCRequestRetry( CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		if (GetGame().IsClient() && ++m_AuthRetries <= 20){
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.RequestAuthToken, m_AuthRetries * 2200, false, true);
		}
	}
	
	protected int m_LastRequestAuthRetry = 0;
	
	void RequestAuthToken(bool first = false){
		if (!m_IsServer){
			if (m_LastRequestAuthRetry < (UUtil.GetUTCUnixInt() - 60)){ //Ratelimit to 1 per 60 Seconds if api is down for extended periods of time this could cause infient loops etc.
				m_LastRequestAuthRetry = UUtil.GetUTCUnixInt();
				GetRPCManager().SendRPC("UF", "RPCRequestAuthToken", new Param1<bool>(first), true);
			}
		}
	}
	
	void PreparePlayerAuth(string guid){
		this.Rest().GetAuth(guid);
	}
	
	void AddPlayerAuth(string guid, string auth){
		if (!PlayerAuths){PlayerAuths = new map<string, string>;}
		//Print("[UF] Adding PlayerAuth for " + guid + " to cache");
		PlayerAuths.Set(guid,auth); //Set Auth incase a request comes in.
		
		DayZPlayer player; //If renewing or if player is availbe send to player
		if (Class.CastTo(player, FindPlayer(guid)) && player.GetIdentity() ){
			SendAuthToken(player.GetIdentity(), auth);
		}
	}
	
	bool GetPlayerAuth(string guid, out string auth){
		if (PlayerAuths && PlayerAuths.Contains(guid)){
			auth = PlayerAuths.Get(guid);
			return true;
		}
		Print("[UF] Failed to find Player Auth for " + guid);
		return false;
	}		
		
	protected void RPCRequestAuthToken( CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target )
	{
		Param1<bool> data; 
		if ( !ctx.Read( data ) ) return;
		PlayerIdentity identity = PlayerIdentity.Cast(sender);
		if (m_IsServer && identity){
			UFConfig();
			string authtoken = "";
			if (UFConfig().ServerAuth != "" && UFConfig().ServerAuth != "null" ){
				if (data.param1 && GetPlayerAuth(identity.GetId(), authtoken)){
					Print("[UF] RPCRequestAuthToken Sending Cached Token ");
					SendAuthToken(identity, authtoken);
				} else if (FindPlayer(identity.GetId())){
					Print("[UF] RPCRequestAuthToken  Renewing Auth Token" );
					PreparePlayerAuth(identity.GetId());
				}  else {
					Print("[UF] RPCRequestAuthToken Requesting client retry." );
					GetRPCManager().SendRPC("UF", "RPCRequestRetry", new Param1<bool>(true), true, identity);
				}
			} else if (UFConfig().ServerAuth && UFConfig().ServerAuth != "" && UFConfig().ServerAuth != "null") {
				Error("[UF] Server Auth is empty or null");
			}
		}
	}
	
	protected void SendAuthToken(PlayerIdentity idenitity, string auth){
		if (idenitity && auth != ""){
			Print("[UF] Sending PlayerAuth Token to " + idenitity.GetId());
			autoptr UFrameworkConfig cClientConfig = new UFrameworkConfig;
			cClientConfig.ConfigVersion = UFConfig().ConfigVersion;
			cClientConfig.ServerURL = UFConfig().ServerURL;
			cClientConfig.ServerID = UFConfig().ServerID;
			cClientConfig.ServerAuth = "null";
			cClientConfig.EnableBuiltinLogging = UFConfig().EnableBuiltinLogging;
			cClientConfig.PromptDiscordOnConnect = UFConfig().PromptDiscordOnConnect;
			GetRPCManager().SendRPC("UF", "RPCUFrameworkConfig", new Param2<ApiAuthToken, UFrameworkConfig>(new ApiAuthToken(idenitity.GetId(), auth), cClientConfig), true, idenitity);
		} else {
			Print("[UF] [UAuthCallBack] ERROR ");
			if (idenitity){
				U().AuthError(idenitity.GetId());
			}
		}
	}
	
	void AuthError(string guid){
		Print("[UF] Auth Error for " + guid);
		//If Auth Token Failed just try again in 3 minutes 
		if (guid != "" && IsOnline()){
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(Rest().GetAuth, 180 * 1000, false, guid);
		} 
		if (!m_IsServer && !IsOnline()){
			U().api().Status(this, "CBStatusCheck");
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.AuthError, 300 * 1000, false, guid);
		}
	}
	
	static void DiscordMessage(string webhookUrl, string message, string botName = "", string botAvatarUrl = ""){
		UDiscordObject DiscordMessage = new UDiscordObject;
		DiscordMessage.content = message;
		DiscordMessage.username = botName;
		DiscordMessage.avatar_url = botAvatarUrl;
		Post(webhookUrl, DiscordMessage.ToJson());
	}
	
	static void DiscordObject(string webhookUrl, UDiscordObject discordObject){
		Post(webhookUrl, discordObject.ToJson());
	} 
	
	
	static string ErrorToString(int ErrorCode){
		switch ( ErrorCode )
		{
			case ERestResultState.EREST_EMPTY:
				return "EREST_EMPTY";
			case ERestResultState.EREST_PENDING:
				return "EREST_PENDING";
			case ERestResultState.EREST_FEEDING:
				return "EREST_FEEDING";
			case ERestResultState.EREST_SUCCESS:
				return "EREST_SUCCESS";
			case ERestResultState.EREST_ERROR:
				return "EREST_ERROR";
			case ERestResultState.EREST_ERROR_CLIENTERROR:
				return "EREST_ERROR_CLIENTERROR";
			case ERestResultState.EREST_ERROR_SERVERERROR:
				return "EREST_ERROR_SERVERERROR";
			case ERestResultState.EREST_ERROR_APPERROR:
				return "EREST_ERROR_APPERROR";
			case ERestResultState.EREST_ERROR_TIMEOUT:
				return "EREST_ERROR_TIMEOUT";
			case ERestResultState.EREST_ERROR_NOTIMPLEMENTED:
				return "EREST_ERROR_NOTIMPLEMENTED";
			case ERestResultState.EREST_ERROR_UNKNOWN:
				return "EREST_ERROR_UNKNOWN";
		}
		return "UNDEFINED_ERROR";
	}
	
	int CallId(){
		return ++m_CallId;
	}
	
	
	RestCallback RegisterCall(UFRestCallBackBase cb, out int cid){
		if (!cb) return null;
		cid = this.CallId();
		cb.SetId(cid);
		m_UCallBacks.Insert(cid, UFRestCallBackBase.Cast(cb));
		return UFRestCallBackBase.Cast(cb);
	}
			
	void ClearCallback(int cid, string traceDebug){
		if (!m_UCallBacks) return;
		if (cid == -1) return;
		autoptr UFRestCallBackBase cb;
		if (m_UCallBacks.Find(cid, cb)){
			delete cb;
			m_UCallBacks.Remove(cid);
		} else {
			Error2("[UF] Error couldn't find call back", "CallId: " + cid + "\n--------\n " + traceDebug + "\n--------\n");
		}
	}
	
	bool IsCallCanceled(int cid){
		return (m_CanceledCalls.Find(cid) != -1);
	}
	
	protected void GetQRandomNumbers(){
		if (LastRandomNumberRequestCall != -1){
			return;
		}
		LastRandomNumberRequestCall = api().RandomNumbers(-1, this, "CBRandomNumber");
	}
	
	protected void CBRandomNumber(int cid, int status, string oid, URandomNumberResponse data){
		LastRandomNumberRequestCall = -1;
		if (status == UF_SUCCESS && data){
			Math.AddQRandomNumber(data.Numbers);
			Math.Randomize(Math.QRandom()); //Randomize the Vanilla Randomization a bit more.
			return;
		}
		Print("[UF] Failed to update the Q Random Numbers");
	}
	
	protected void CBStatusCheck(int cid, int status, string oid, UFStatus data){
		if (status == UF_SUCCESS && data){
			if (data.Error == "noerror"){
				m_UFOnline = true;
				Print("[UF] WebService Online Version: " + data.Version + " Mod Version: " + UF_VERSION);
			}
			if (data.Error == "noauth"){
				m_UFOnline = false;
				Print("[UF] Auth Key is not vaild");
			}
			if (data.Error == "noerror" && data.Discord == "Enabled"){
				m_UDiscordEnabled = true;
			}
			if (data.Discord == "Online"){
				m_UDiscordEnabled = true;
			}
			if (data.OpenAI == "Online"){
				m_UOpenAIEnabled = true;
			}
			m_UFVersionOffset = data.CheckVersion(UF_VERSION);
			if (m_UFVersionOffset > 2){
				Error2("Universal Framework WebService Needs Update", "[UF] Webservice is outdated and should be updated right away | WebService Version: " + data.Version + " Mod Version: " + UF_VERSION);
				return;
			}
			if (m_UFVersionOffset > 1){
				Error("[UF] Webservice is outdated and should be updated right away");
				return;
			}
			if (m_UFVersionOffset > 0){
				Print("[UF] You may want to check for new versions of the Universal Framework WebService");
				return;
			}
			if (m_UFVersionOffset < -2){
				Error2("Universal Framework Mod Needs Update", "[UF] Universal Framework Mod is outdated and should be updated right away | WebService Version: " + data.Version + " Mod Version: " + UF_VERSION);
				return;
			}
			if (m_UFVersionOffset < -1){
				Print("[UF] Universal Framework Mod maybe outdated and should be updated right away");
				return;
			}					
			return;
		} else if (status == UF_ERROR){
			Error2("UnviersalApi", "[UF] Something went wrong communicating with the webservice check to make sure it is installed correctly and the mongodb service is running correctly! URL: " + UFConfig().GetBaseURL());
			m_UFOnline = false;
		}  else if (status == UF_TIMEOUT){
			Error2("UnviersalApi", "[UF] Webservice is offline or unreachable! URL: " + UFConfig().GetBaseURL());
			m_UFOnline = false;
		} else {
			Error2("UnviersalApi", "[UF] Error with WebService! Status: " + status + " URL: " + UFConfig().GetBaseURL());
			m_UFOnline = false;
		}
	}
	
};

static ref UFramework g_UFramework;

static UFramework U()
{
	if ( !g_UFramework )
	{
		g_UFramework = new UFramework;
		g_UFramework.Init();
	}

	return g_UFramework;
};
