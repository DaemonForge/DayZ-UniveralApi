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

// Native RPC IDs for UFramework authentication system
// Using unique IDs to avoid conflicts with other mods
const int UF_RPC_CONFIG        = 237983606; // Server -> Client: Send auth token + config
const int UF_RPC_REQUEST_AUTH  = 237983607; // Client -> Server: Request auth token
const int UF_RPC_REQUEST_RETRY = 237983608; // Server -> Client: Request client to retry

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
	/**
	 * Gets the database endpoint for object or player data operations.
	 * 
	 * @param collection OBJECT_DB (default) for shared data, PLAYER_DB for player-specific data
	 * @return UDBEndpoint instance for database operations, NULL if invalid collection type
	 * 
	 * @usage Object Database (accessible to all):
	 * U().db().Save("MyMod", "config", jsonData, this, "OnSaved");
	 * U().db().Load("MyMod", "playerData_" + guid, this, "OnLoaded");
	 * 
	 * @usage Player Database (client-only, own data):
	 * U().db(PLAYER_DB).Save("MyMod", "settings", jsonData, this, "OnSaved");
	 */
	UDBEndpoint db(int collection = OBJECT_DB){
		if (collection == OBJECT_DB){
			if (!m_ObjectEndPoint){
				m_ObjectEndPoint = new UDBEndpoint("Object");
				if (!m_ObjectEndPoint){
					Error2("[UF] db()", "CRITICAL: Failed to create Object endpoint!");
				}
			}
			return m_ObjectEndPoint;
		} 
		if (collection == PLAYER_DB){
			if (!m_PlayerEndPoint){
				m_PlayerEndPoint = new UDBEndpoint("Player");
				if (!m_PlayerEndPoint){
					Error2("[UF] db()", "CRITICAL: Failed to create Player endpoint!");
				}
			}
			return m_PlayerEndPoint;
		}
		Error2("[UF] db()", "Invalid collection type: " + collection + " - use OBJECT_DB or PLAYER_DB");
		return NULL;
	}
	
	/**
	 * Gets the Discord integration endpoint.
	 * 
	 * @return UniversalDSEndpoint for Discord operations (roles, DMs, channels, voice)
	 * 
	 * @usage
	 * U().ds().AddRole(playerGUID, "RoleID", this, "OnRoleDone");
	 * U().ds().UserSend(playerGUID, "Welcome message!", this, "OnSent");
	 * string linkUrl = U().ds().Link();  // Get Discord OAuth link
	 */
	UniversalDSEndpoint ds(){
		if (!m_UniversalDSEndpoint){
			m_UniversalDSEndpoint = new UniversalDSEndpoint;
		}
		return m_UniversalDSEndpoint;
	}
	
	/**
	 * Gets the global database endpoint for mod-wide shared data.
	 * 
	 * @return UDBGlobalEndpoint for global mod data (no object ID, shared across all instances)
	 * 
	 * @usage
	 * U().globals().Save("MyMod", jsonData, this, "OnSaved");
	 * U().globals().Increment("MyMod", "playerCount", 1);  // Atomic increment
	 * U().globals().Update("MyMod", "serverStatus", "\"online\"");  // Must quote strings
	 */
	UDBGlobalEndpoint globals(){
		if (!m_UDBGlobalEndpoint){
			m_UDBGlobalEndpoint = new UDBGlobalEndpoint;
		}
		return m_UDBGlobalEndpoint;
	}
	
	/**
	 * Gets the API utilities endpoint for server queries, crypto prices, random numbers, etc.
	 * 
	 * @return UApiEndpoint for utility operations
	 * 
	 * @usage
	 * U().api().Status(this, "OnStatusCheck");  // Check API status
	 * U().api().RandomNumbers(1000, this, "OnRandoms");  // Get random numbers
	 * U().api().SteamQuery(ip, port, this, "OnServerStatus");  // Query game server
	 */
	UApiEndpoint api(){
		if (!m_UApiEndpoint){
			m_UApiEndpoint = new UApiEndpoint;
		}
		return m_UApiEndpoint;
	}
	
	/**
	 * Gets the CRON manager for scheduling recurring tasks.
	 * 
	 * @return UCronManager for task scheduling
	 * 
	 * @usage
	 * U().Cron().runEndless(60, this, "OnEveryMinute", NULL);  // Every 60 seconds
	 * U().Cron().runEndCount(10, 5, this, "OnFiveTimes", NULL);  // 5 times, 10s apart
	 * U().Cron().Remove(this, "OnEveryMinute");  // Cancel scheduled task
	 */
	UCronManager Cron(){
		if (!m_UCronManager){
			m_UCronManager = new UCronManager;
			m_UCronManager.Init();
		}
		return m_UCronManager;
	}

	/**
	 * Gets the message queue endpoint for async messaging between players/server.
	 * 
	 * @return UFMsgEndpoint for message queue operations
	 * 
	 * @usage
	 * U().Msg().Write("MyMod", "notifications", msgObject, callback);
	 * U().Msg().Read("MyMod", "notifications", callback);  // Read all unread
	 * U().Msg().ReadLatest("MyMod", "notifications", 10, callback);  // Latest 10
	 */
	UFMsgEndpoint Msg(){
		if (!m_UFMsgEndpoint){
			m_UFMsgEndpoint = new UFMsgEndpoint;
			if (!m_UFMsgEndpoint){
				Error2("[UF] Msg()", "CRITICAL: Failed to create Message endpoint!");
			}
		}
		return m_UFMsgEndpoint;
	}
	
	/**
	 * Gets the AI Chat endpoint for OpenAI integration.
	 * 
	 * @return UFAIChatEndpoint for AI chat session operations
	 * 
	 * @usage
	 * U().AI().Create(systemPrompt, "string", "", "", -1, callback);
	 * U().AI().Send(chatId, userMessage, callback);
	 * U().AI().History(chatId, callback);  // Get chat history
	 * 
	 * @note Check U().IsOpenAIEnabled() before using
	 */
	UFAIChatEndpoint AI(){
		if (!m_UFAIChatEndpoint){
			m_UFAIChatEndpoint = new UFAIChatEndpoint;
		}
		return m_UFAIChatEndpoint;
	}
	
	/**
	 * Gets the Mod Settings endpoint for registering custom HTML settings pages.
	 * 
	 * Server operators can view and interact with registered settings pages in the
	 * Electron UI under Options → Mod Settings.
	 * 
	 * @return UFModSettingsEndpoint for settings page registration
	 * 
	 * @usage
	 * U().Settings().Register("my-mod", htmlTemplate);
	 * U().Settings().Register("my-mod", "My Mod", htmlTemplate);
	 */
	UFModSettingsEndpoint Settings(){
		if (!m_UFModSettingsEndpoint){
			m_UFModSettingsEndpoint = new UFModSettingsEndpoint;
		}
		return m_UFModSettingsEndpoint;
	}
	
	/**
	 * Requests cancellation of an active REST callback by its call ID.
	 * 
	 * @param cid Call identifier to cancel
	 */
	void RequestCallCancel(int cid){
		m_CanceledCalls.Insert(cid);
	}
	
	/**
	 * Sends a POST request with empty body (fire-and-forget).
	 * 
	 * @param url Target endpoint URL
	 * @return Always returns 0
	 * 
	 * @usage U().Post("https://api.example.com/log");
	 */
	static int Post(string url)
	{
		RestContext ctx = RestCore().GetRestContext(url);
		ctx.SetHeader("application/json");
		ctx.POST(new USilentCallBack, "", "{}");
		return 0;
	}
	
	/**
	 * Sends a POST request with JSON body and optional callback.
	 * 
	 * @param url Target endpoint URL
	 * @param jsonString JSON data to send
	 * @param UCBX RestCallback to handle response (uses silent callback if NULL)
	 * @param contentType MIME type for Content-Type header (default: "application/json")
	 * @return Always returns 0
	 * 
	 * @usage U().Post("https://api.example.com/data", jsonString, new MyCallback());
	 */
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
	
	/**
	 * Sends a POST request with UFCallbackBase callback for response handling.
	 * 
	 * @param url Target endpoint URL
	 * @param jsonString JSON data to send
	 * @param cb UFCallbackBase-derived callback (required)
	 * @param contentType MIME type for Content-Type header (default: "application/json")
	 * @return Call ID for tracking/cancellation, or -1 if cb is NULL
	 * 
	 * @usage
	 * int callId = U().Post("https://api.example.com/save", jsonData, new MySaveCallback());
	 * // Later: U().RequestCallCancel(callId);
	 */
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
	
	/**
	 * Sends a GET request (fire-and-forget).
	 * 
	 * @param url Target endpoint URL
	 * @return Always returns 0
	 * 
	 * @usage U().Get("https://api.example.com/status");
	 */
	static int Get(string url)
	{
		RestContext ctx =  RestCore().GetRestContext(url);
		ctx.GET(new USilentCallBack, "");
		return 0;
	}
	/**
	 * Sends a GET request with RestCallback for response handling.
	 * 
	 * @param url Target endpoint URL
	 * @param UCBX RestCallback to handle response (uses silent callback if NULL)
	 * @return Always returns 0
	 * 
	 * @usage U().Get("https://api.example.com/data", new MyDataCallback());
	 */
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
	/**
	 * Sends a GET request with UFCallbackBase callback for response handling.
	 * 
	 * @param url Target endpoint URL
	 * @param cb UFCallbackBase-derived callback (required)
	 * @return Call ID for tracking/cancellation, or -1 if cb is NULL
	 * 
	 * @usage
	 * int callId = U().Get("https://api.example.com/config", new MyConfigCallback());
	 */
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
	
	/**
	 * Checks if Discord integration is configured.
	 * 
	 * @return True if Discord endpoint exists in config (not necessarily valid)
	 * 
	 * @note This only checks config presence, not if credentials are correct
	 */
	bool IsDiscordEnabled(){
		return m_UDiscordEnabled;
	}

	/**
	 * Checks if OpenAI integration is available and operational.
	 * 
	 * @return True if OpenAI service is online and configured
	 */
	bool IsOpenAIEnabled(){
		return m_UOpenAIEnabled;
	}
	
	/**
	 * Checks if the framework has completed initial status check successfully.
	 * 
	 * @return True if mod is fully initialized and service is operational
	 * 
	 * @usage if (!U().IsOnline()) { Print("API not ready yet"); return; }
	 */
	bool IsOnline(){
		return m_UFOnline;
	}
	
	/**
	 * Gets the server ID from configuration.
	 * 
	 * @return Server identifier string (available on both client and server after auth)
	 */
	string GetServerID(){
		if (UFConfig()){
			return UFConfig().GetServerID();
		}
		return "";
	}
	
	/**
	 * Gets version compatibility offset between mod and service.
	 * 
	 * @return Version offset:
	 *  - 0: Perfect match
	 *  - ±1: Patch difference (minor issues possible)
	 *  - ±2: Minor version difference (some features may be missing)
	 *  - ±3: Major version difference (mod likely broken)
	 * 
	 * @usage if (U().VersionOffset() >= 2) { Error("Version mismatch!"); }
	 */
	int VersionOffset(){
		return m_UFVersionOffset;
	}
	
	/**
	 * Checks QRandom pool and requests more numbers if below threshold.
	 * 
	 * @note Automatically triggered when random pool drops below 2000 numbers
	 */
	void CheckAndRenewQRandom(){
		if (Math.QRandomRemaining() <= 2000){
			GetQRandomNumbers();
		}
	}
	
	/**
	 * Gets the current framework version.
	 * 
	 * @return Version string (e.g., "2.0.0")
	 */
	static string GetVersion(){
		return UF_VERSION;
	}
	
	/**
	 * Finds a player by their GUID (server-side only).
	 * 
	 * @param GUID Player's unique identifier
	 * @return DayZPlayer if found, NULL otherwise
	 * 
	 * @note Only works on server (returns NULL on client)
	 * @usage DayZPlayer player = U().FindPlayer(identity.GetId());
	 */
	static DayZPlayer FindPlayer(string GUID){
		if (g_Game.IsServer()){
			autoptr array<Man> players = new array<Man>;
			g_Game.GetPlayers( players );
			for (int i = 0; i < players.Count(); i++){
				DayZPlayer player = DayZPlayer.Cast(players.Get(i));
				if (player.GetIdentity() && player.GetIdentity().GetId() == GUID ){
					return player;
				}
			}
		}
		return NULL;
	}
	
	/**
	 * Finds a player by their PlayerIdentity (server-side only).
	 * 
	 * @param identity PlayerIdentity object
	 * @return DayZPlayer if found, NULL if identity is NULL or player not found
	 * 
	 * @note Uses network ID lookup for faster retrieval than FindPlayer()
	 */
	static DayZPlayer FindPlayerByIdentity(PlayerIdentity identity) {
		if (!identity)
			return NULL;

		int highBits;
		int lowBits;
		g_Game.GetPlayerNetworkIDByIdentityID(identity.GetPlayerId(), lowBits, highBits);
		return DayZPlayer.Cast(g_Game.GetObjectByNetworkId(lowBits, highBits));
	}
	
	protected static bool m_isInit = false;
	
	/**
	 * Checks if framework has been globally initialized.
	 * 
	 * @return True if setGlobalInit() has been called
	 */
	static bool isGlobalInit(){
		return m_isInit;
	}
	/**
	 * Marks framework as globally initialized.
	 * 
	 * @note Internal use only - called during framework startup
	 */
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
	
	protected autoptr UFModSettingsEndpoint m_UFModSettingsEndpoint;
	
	protected autoptr UDiscordUser dsUser;
		
	protected autoptr map<string, string> PlayerAuths = new map<string, string>;
	
	protected autoptr UDBEndpoint m_PlayerEndPoint;
	
	protected autoptr UDBEndpoint m_ObjectEndPoint;
	//Can't Do Globals due to how globals work
	
	protected autoptr UApiEndpoint m_UApiEndpoint;
	
	protected autoptr TIntSet m_CanceledCalls = new TIntSet;
	
	// Track pending auth requests to prevent duplicate requests
	protected autoptr set<string> m_PendingAuthRequests = new set<string>;
	
	protected int LastRandomNumberRequestCall = -1;
	
	autoptr map<int,UFRestCallBackBase> m_UCallBacks = new map<int,UFRestCallBackBase>;
	
	string m_BaseURL = "";
		
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
	
	/**
	 * Gets the authentication token for API requests.
	 * 
	 * @return Auth token string. On client: JWT token (or "" if expired). On server: ServerAuth from config.
	 * 
	 * @note Client automatically renews tokens when expiring (<4 min left)
	 * @note Returns "" if client token is expired to prevent using stale credentials
	 */
	string GetAuthToken(){
		if (m_UFauthToken && !g_Game.IsServer()){
			if (m_UFauthToken.IsExpired()) {
				// Token is expired - trigger renewal and return empty to prevent using stale token
				UFLog.Info("[Auth] Token expired when GetAuthToken called, triggering renewal");
				RequestAuthToken(true); // Force immediate renewal
				return ""; // Return empty so request will fail gracefully instead of using expired token
			}
			// Proactive renewal: if token expires in less than 4 minutes, trigger background renewal
			// This is a last-resort fallback (Layer 3) - watchdog at 60s (Layer 1.5) should catch first
			// Only log once per renewal cycle to avoid log spam that was previously causing stack overflow
			if (m_UFauthToken.IsExpiringSoon(240)) {
				RequestAuthToken(false); // Non-blocking renewal (rate limited to 30s internally)
			}
			return m_UFauthToken.GetAuthToken();
		} else if (g_Game.IsServer() && UFConfig().ServerAuth != ""){
			return UFConfig().ServerAuth;
		}
		return "null";
	}
	
	/**
	 * Checks if framework has a valid, non-expired auth token.
	 * 
	 * @return True if token exists and is valid (not expired, not error state)
	 * 
	 * @usage if (!U().HasValidAuth()) { Error("Cannot make API call - no auth"); return; }
	 */
	bool HasValidAuth(){
		if (!m_UFauthToken) return false;
		if (m_UFauthToken.IsExpired()) return false;
		// Don't call GetAuthToken() here - it has side effects (logging, renewal)
		// that cause infinite recursion: HasValidAuth -> GetAuthToken -> UFLog.Info -> SendToApi -> HasValidAuth
		string token;
		if (g_Game.IsServer()) {
			if (!UFConfig()) return false;
			token = UFConfig().ServerAuth;
		} else {
			token = m_UFauthToken.GetAuthToken();
		}
		return (token != "null" && token != "error" && token != "ERROR" && token != "");
	}
	
	/**
	 * Checks if auth token will expire soon.
	 * 
	 * @param bufferSeconds Time threshold in seconds (default: 120 = 2 minutes)
	 * @return True if token expires within bufferSeconds (client only, always false on server)
	 */
	bool IsTokenExpiringSoon(int bufferSeconds = 120){
		if (!m_UFauthToken || g_Game.IsServer()) return false;
		return m_UFauthToken.IsExpiringSoon(bufferSeconds);
	}
	
	/**
	 * Gets seconds remaining until token expiration.
	 * 
	 * @return Seconds until expiry, or -1 if no token exists
	 */
	int GetTokenSecondsRemaining(){
		if (!m_UFauthToken) return -1;
		return m_UFauthToken.GetSecondsUntilExpiry();
	}
	
	/**
	 * Gets last 8 characters of token for safe logging.
	 * 
	 * @return Token suffix or "NO_TOKEN" if no token exists
	 * @note Safe to log - doesn't expose full token
	 */
	string GetTokenSuffix(){
		if (!m_UFauthToken) return "NO_TOKEN";
		return m_UFauthToken.GetTokenSuffix();
	}
	
	/**
	 * Outputs detailed token state to debug logs.
	 * 
	 * @note For debugging auth issues - logs token status, expiry, etc.
	 */
	void DebugTokenState(){
		if (!m_UFauthToken){
			UFLog.Debug("[Auth] DebugTokenState: No token exists");
			return;
		}
		m_UFauthToken.DoDebug();
	}
	
	/**
	 * Gets UniversalRest endpoint (legacy RestCallback interface).
	 * 
	 * @return UniversalRest for callback-based REST operations
	 * @deprecated Use db(), ds(), api(), etc. with UFCallbackBase instead
	 */
	UniversalRest Rest(){
		if (!m_UniversalRest){
			m_UniversalRest = new UniversalRest;
		}
		return m_UniversalRest;
	}	
	
	
	void ~UFramework(){
		if (m_IsServer && UF_Init && g_Game){
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(this.CheckAndRenewQRandom);
		}
		delete m_UFauthToken;
	}
	
	void Init(){
		#ifdef NO_GUI
			UFLog.Info("Detected Server (NO_GUI defined)");
			m_IsServer = true;
		#endif
		if (!UF_Init){
			setGlobalInit();
			UFLog.Info("First Init");
			UF_Init = true;
			
			// Register centralized RPC handler with DayZGame.Event_OnRPC
			// This eliminates the need for duplicate OnRPC overrides in PlayerBase/MissionBase
			UFRPCHandler.Register();
			
			if (m_IsServer){
				UFrameworkConfig cfg = UFConfig();
				if (!cfg){
					UFLog.Err("[Init] CRITICAL: UFConfig() returned NULL on server! Cannot make Status call.");
				} else {
					string baseUrl = cfg.GetBaseURL();
					if (baseUrl == "" || baseUrl == "null"){
						UFLog.Err("[Init] CRITICAL: BaseURL is empty or null! Check UFramework.json config file.");
					} else {
						U().api().Status(this, "CBStatusCheck");
					}
				}
				CheckAndRenewQRandom();
			}
		}
	}
	
	protected bool m_InitialTokenReceived = false;
	
	/**
	 * Native RPC handler for receiving auth token and config from server.
	 * Called via OnRPC in PlayerBase or MissionBase.
	 */
	void OnRPC_UFrameworkConfig(ParamsReadContext ctx, PlayerIdentity sender)
	{
		UFLog.Debug("[Auth] RPCUFrameworkConfig received - processing...");
		Param2<ApiAuthToken, UFrameworkConfig> data; 
		if ( !ctx.Read( data ) ){
			UFLog.Err("Failed to read RPC data in RPCUFrameworkConfig - data may be corrupted");
			return;
		}
		
		if (!data.param1 || !data.param2){
			UFLog.Err("RPC data params are null - param1: " + data.param1 + " param2: " + data.param2);
			return;
		}
		
		m_AuthRetries = 0;
		
		// Log old token info before replacing (if exists)
		if (m_UFauthToken){
			UFLog.Debug("[Auth] Replacing old token - OldSuffix: ..." + m_UFauthToken.GetTokenSuffix() + " SecsLeft: " + m_UFauthToken.GetSecondsUntilExpiry());
		} else {
			UFLog.Debug("[Auth] No existing token - this is the first token");
		}
		
		// Replace with new token
		Class.CastTo(m_UFauthToken, data.param1);
		Class.CastTo(m_UFrameworkConfig, data.param2);
		
		// CRITICAL: RPC deserialization doesn't call constructors, so Expiry is not set!
		// We must manually reset the expiry after receiving the token
		if (m_UFauthToken){
			m_UFauthToken.ResetExpiry();
			UFLog.Debug("[Auth] New token active - Suffix: ..." + m_UFauthToken.GetTokenSuffix() + " ExpiresIn: " + m_UFauthToken.GetSecondsUntilExpiry() + "s");
		} else {
			UFLog.Err("Failed to cast token from RPC data");
		}
		
		if (m_UFrameworkConfig && m_UFrameworkConfig.ServerURL != ""){
			m_BaseURL = m_UFrameworkConfig.ServerURL;
		} else {
			UFLog.Info("Received Config and Auth Token but Config or Server URL are Null");
		}
		
		// Apply log level from server config to client logger
		if (m_UFrameworkConfig){
			int logLevel = LOG_INFO;
			if (m_UFrameworkConfig.DebugLevel == "DEBUG"){
				logLevel = LOG_DEBUG;
			}
			UFLog.SetLogLevels(logLevel);
			UFLog.Debug("[Auth] Client log level set to: " + m_UFrameworkConfig.DebugLevel);
		}
		
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).Call(this.OnTokenReceived);
	}
	
	protected void OnTokenReceived(){
		// Always check and renew random numbers to prevent starvation during long sessions
		CheckAndRenewQRandom();
		
		// Only run full initialization on first token
		if (!m_InitialTokenReceived){
			m_InitialTokenReceived = true;
			UFLog.Info("[UAPI] Initial token received, initializing services");
			U().api().Status(this, "CBStatusCheck");
			U().ds().GetUser(GetDayZGame().GetSteamId(), GetDayZGame(), "CBCacheDiscordInfo");
			// NOTE: UFrameworkReadyTokenReceived is now fired from CBStatusCheck after we know the OpenAI/Discord status
		} else {
			UFLog.Info("[UAPI] Token renewed successfully");
		}
	}
	
	/**
	 * Called by callbacks when they receive an auth failure (401/204 unauthorized).
	 * Triggers a token renewal with 30s rate limit to prevent spam.
	 */
	void OnAuthFailure(){
		if (!g_Game.IsServer()){
			UFLog.Info("[Auth] Auth failure detected, requesting token renewal (30s rate limited)");
			RequestAuthToken(false); // Use rate-limited renewal, not forced
		}
	}
	
	
	/**
	 * Native RPC handler for retry request from server.
	 * Called via OnRPC in PlayerBase or MissionBase.
	 */
	void OnRPC_RequestRetry(ParamsReadContext ctx, PlayerIdentity sender) {
		if (g_Game.IsClient() && ++m_AuthRetries <= 20){
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.RequestAuthToken, m_AuthRetries * 2200, false, true);
		}
	}
	
	protected int m_LastRequestAuthRetry = 0;
	protected int m_LastForcedAuthRequest = 0;
	
	/**
	 * Request a new auth token from the server.
	 * 
	 * Token lifecycle:
	 *   - Token expires: 15 minutes (900s)
	 *   - Cron renewal: every 10 minutes (600s) - primary renewal mechanism
	 *   - Proactive check: triggers at 4 minutes remaining (240s) - fallback if cron fails
	 *   - Rate limits: 30s between any renewal requests to prevent spam
	 * 
	 * @param first - If true, this is a forced/initial request (only for actual expiry, not proactive)
	 */
	void RequestAuthToken(bool first = false){
		if (!m_IsServer){
			int currentTime = UUtil.GetUTCUnixInt();
			
			// Log current token state before making request
			if (m_UFauthToken){
				UFLog.Debug("[Auth] RequestAuthToken called - first=" + first + " TokenExpired=" + m_UFauthToken.IsExpired() + " SecsLeft=" + m_UFauthToken.GetSecondsUntilExpiry() + " Suffix=..." + m_UFauthToken.GetTokenSuffix());
			} else {
				UFLog.Debug("[Auth] RequestAuthToken called - first=" + first + " NO TOKEN EXISTS");
			}
			
			// Forced requests (first=true) have a 30 second rate limit
			// This prevents spam when token is fully expired and multiple requests fail
			if (first){
				if ((currentTime - m_LastForcedAuthRequest) < 30){
					UFLog.Debug("[Auth] Forced auth request rate limited (30s cooldown), last=" + (currentTime - m_LastForcedAuthRequest) + "s ago");
					return;
				}
				m_LastForcedAuthRequest = currentTime;
				m_LastRequestAuthRetry = currentTime;
				UFLog.Debug("[Auth] Sending FORCED auth token RPC to server (UF_RPC_REQUEST_AUTH=" + UF_RPC_REQUEST_AUTH + ")");
				// Native RPC: Client -> Server (use NULL target for server)
				GetGame().RPCSingleParam(NULL, UF_RPC_REQUEST_AUTH, new Param1<bool>(true), true);
				return;
			}
			
			// Normal/proactive requests have a 30 second rate limit
			if ((currentTime - m_LastRequestAuthRetry) >= 30){
				m_LastRequestAuthRetry = currentTime;
				UFLog.Debug("[Auth] Sending SCHEDULED auth token RPC to server (UF_RPC_REQUEST_AUTH=" + UF_RPC_REQUEST_AUTH + ")");
				// Native RPC: Client -> Server (use NULL target for server)
				GetGame().RPCSingleParam(NULL, UF_RPC_REQUEST_AUTH, new Param1<bool>(false), true);
			} else {
				UFLog.Debug("[Auth] Scheduled auth request rate limited (" + (30 - (currentTime - m_LastRequestAuthRetry)) + "s until next allowed)");
			}
		}
	}
	/**
	 * Requests a fresh auth token from web service for a player (server-side only).
	 * 
	 * @param guid Player GUID
	 * @note Prevents duplicate requests if one is already pending
	 * @note Internal use - called by OnRPC_RequestAuthToken
	 */
	void PreparePlayerAuth(string guid){
		// Check if request is already pending to avoid duplicates
		if (m_PendingAuthRequests && m_PendingAuthRequests.Find(guid) != -1){
			UFLog.Debug("Auth request already pending for " + guid + ", skipping duplicate request");
			return;
		}
		// Mark as pending
		if (!m_PendingAuthRequests){
			m_PendingAuthRequests = new set<string>;
		}
		m_PendingAuthRequests.Insert(guid);
		UFLog.Debug("Preparing auth token for " + guid);
		this.Rest().GetAuth(guid);
	}
	
	/**
	 * Caches player auth token and sends it to the player (server-side only).
	 * 
	 * @param guid Player GUID
	 * @param auth JWT auth token
	 * @note Automatically sends token via RPC if player is connected
	 */
	void AddPlayerAuth(string guid, string auth){
		if (!PlayerAuths){PlayerAuths = new map<string, string>;}
		
		// Clear pending status
		if (m_PendingAuthRequests){
			m_PendingAuthRequests.RemoveItem(guid);
		}
		
		UFLog.Debug("Adding PlayerAuth for " + guid + " to cache");
		PlayerAuths.Set(guid, auth); //Set Auth in case a request comes in.
		
		// Send token to player if they are connected
		DayZPlayer player;
		if (Class.CastTo(player, FindPlayer(guid)) && player.GetIdentity()){
			SendAuthToken(player.GetIdentity(), auth);
		}
	}
	
	/**
	 * Retrieves cached auth token for a player (server-side only).
	 * 
	 * @param guid Player GUID
	 * @param auth Out parameter to receive auth token
	 * @return True if token found in cache, false otherwise
	 */
	bool GetPlayerAuth(string guid, out string auth){
		if (PlayerAuths && PlayerAuths.Contains(guid)){
			auth = PlayerAuths.Get(guid);
			return true;
		}
		UFLog.Debug("Failed to find Player Auth for " + guid);
		return false;
	}
	
	void ClearPlayerAuth(string guid){
		if (PlayerAuths && PlayerAuths.Contains(guid)){
			UFLog.Debug("Clearing cached auth token for " + guid);
			PlayerAuths.Remove(guid);
		}
		// Also clear any pending request status
		if (m_PendingAuthRequests){
			m_PendingAuthRequests.RemoveItem(guid);
		}
	}		
		
	/**
	 * Native RPC handler for auth token requests from client.
	 * Called via OnRPC in PlayerBase or MissionBase.
	 * 
	 * IMPORTANT: We ALWAYS request fresh tokens from the web service because:
	 * - JWT tokens have a fixed expiry (15 min from creation)
	 * - Cached tokens will eventually expire and become useless
	 * - Each renewal request should get a NEW token with fresh 15-min expiry
	 */
	void OnRPC_RequestAuthToken(ParamsReadContext ctx, PlayerIdentity sender)
	{
		string senderInfo = "null";
		if (sender){
			senderInfo = sender.GetId();
		}
		UFLog.Debug("[Auth] OnRPC_RequestAuthToken ENTRY - sender=" + senderInfo);
		Param1<bool> data; 
		if ( !ctx.Read( data ) ){
			UFLog.Err("[Auth] OnRPC_RequestAuthToken ERROR: Failed to read RPC data");
			return;
		}
		UFLog.Debug("[Auth] OnRPC_RequestAuthToken - isInitial=" + data.param1);
		PlayerIdentity identity = sender;
		if (m_IsServer && identity){
			UFConfig();
			if (UFConfig().ServerAuth != "" && UFConfig().ServerAuth != "null" ){
				string guid = identity.GetId();
				
				// Always request a fresh token from the web service
				// This ensures the client always gets a token with full 15-min expiry
				// The old approach of sending cached tokens was wrong because:
				// - Cached token has same expiry as when first created
				// - After 10 min, cached token only has 5 min left
				// - Client needs a FRESH token with 15 min expiry
				if (FindPlayer(guid)){
					if (data.param1){
						UFLog.Debug("[Auth] [SERVER] Initial connection - requesting fresh token for " + guid);
					} else {
						UFLog.Debug("[Auth] [SERVER] Renewal request - requesting NEW fresh token for " + guid);
					}
					// Clear old cached token and request fresh one
					ClearPlayerAuth(guid);
					PreparePlayerAuth(guid);
				} else {
					UFLog.Debug("[Auth] [SERVER] Player not found for " + guid + ", requesting client retry");
					// Try to find any player object to send retry RPC
					DayZPlayer player = FindPlayer(guid);
					if (player){
						GetGame().RPCSingleParam(player, UF_RPC_REQUEST_RETRY, new Param1<bool>(true), true, identity);
					}
				}
			} else {
				UFLog.Err("[Auth] [SERVER] ServerAuth is empty or null - cannot process auth request");
			}
		} else {
			UFLog.Debug("[Auth] OnRPC_RequestAuthToken - SKIPPED: m_IsServer=" + m_IsServer + " identity=" + (identity != null));
		}
	}
	
	void SendAuthToken(PlayerIdentity identity, string auth){
		if (identity && auth != ""){
			string authSuffix = auth.Substring(Math.Max(0, auth.Length() - 10), 10);
			UFLog.Debug("[Auth] [SERVER] SendAuthToken to " + identity.GetId() + " - AuthSuffix: ..." + authSuffix);
			autoptr UFrameworkConfig cClientConfig = new UFrameworkConfig;
			cClientConfig.ConfigVersion = UFConfig().ConfigVersion;
			cClientConfig.ServerURL = UFConfig().ServerURL;
			cClientConfig.ServerID = UFConfig().ServerID;
			cClientConfig.ServerAuth = "null";
			cClientConfig.EnableBuiltinLogging = UFConfig().EnableBuiltinLogging;
			cClientConfig.PromptDiscordOnConnect = UFConfig().PromptDiscordOnConnect;
			cClientConfig.DebugLevel = UFConfig().DebugLevel;
			cClientConfig.LogToSeperateFile = 0; // Client always uses Print, never file
			
			// Native RPC: Server -> specific Client
			// Find the player object to use as RPC target
			DayZPlayer player = FindPlayer(identity.GetId());
			if (player){
				UFLog.Debug("[Auth] [SERVER] Sending UF_RPC_CONFIG (" + UF_RPC_CONFIG + ") to player " + player.GetType());
				GetGame().RPCSingleParam(player, UF_RPC_CONFIG, new Param2<ApiAuthToken, UFrameworkConfig>(new ApiAuthToken(identity.GetId(), auth), cClientConfig), true, identity);
				UFLog.Debug("[Auth] [SERVER] RPC sent successfully");
			} else {
				UFLog.Err("[Auth] [SERVER] Cannot find player object for " + identity.GetId() + " to send auth token");
			}
		} else {
			UFLog.Err("[Auth] [SERVER] SendAuthToken ERROR - identity=" + (identity != null) + " auth.len=" + auth.Length());
			if (identity){
				U().AuthError(identity.GetId());
			}
		}
	}
	
	void AuthError(string guid){
		UFLog.Err("Auth Error for " + guid);
		//If Auth Token Failed just try again in 3 minutes 
		if (guid != "" && IsOnline()){
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(Rest().GetAuth, 180 * 1000, false, guid);
		} 
		if (!m_IsServer && !IsOnline()){
			U().api().Status(this, "CBStatusCheck");
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.AuthError, 300 * 1000, false, guid);
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
	
	/**
	 * Converts REST API error code to human-readable string.
	 * 
	 * @param ErrorCode ERestResultState error code
	 * @return Error description string
	 */
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
	
	/**
	 * Generates unique call ID for REST callbacks.
	 * 
	 * @return Unique integer call identifier
	 * @note Internal use - auto-incremented counter
	 */
	int CallId(){
		return ++m_CallId;
	}
	
	/**
	 * Registers a UFCallbackBase-derived callback for a REST call.
	 * 
	 * @param cb Callback instance to register
	 * @param cid Out parameter to receive assigned call ID
	 * @return RestCallback wrapper for the registered callback
	 * 
	 * @note Internal use - called by endpoint methods
	 * @note Callbacks are auto-cleared when they execute
	 */
	RestCallback RegisterCall(UFRestCallBackBase cb, out int cid){
		if (!cb) {
			UFLog.Err("[UFramework] RegisterCall - callback is null!");
			return null;
		}
		cid = this.CallId();
		cb.SetId(cid);
		m_UCallBacks.Insert(cid, UFRestCallBackBase.Cast(cb));
		return UFRestCallBackBase.Cast(cb);
	}
			
	/**
	 * Clears a registered callback by its call ID.
	 * 
	 * @param cid Call ID to clear
	 * @param traceDebug Debug trace string for error logging
	 * 
	 * @note Logs error if callback not found
	 * @note Internal use - called after callback execution or cancellation
	 */
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
	
	/**
	 * Checks if a call has been canceled via RequestCallCancel().
	 * 
	 * @param cid Call ID to check
	 * @return True if call was canceled
	 * 
	 * @usage Used internally by callbacks to skip execution if canceled
	 */
	bool IsCallCanceled(int cid){
		return (m_CanceledCalls.Find(cid) != -1);
	}
	
	protected void GetQRandomNumbers(){
		if ( LastRandomNumberRequestCall != -1 )
		{
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
		UFLog.Err("Failed to update the Q Random Numbers");
	}
	
	protected void CBStatusCheck(int cid, int status, string oid, UFStatus data){
		if (status == UF_SUCCESS && data){
			if (data.Error == "noerror"){
				m_UFOnline = true;
				UFLog.Info("WebService Online Version: " + data.Version + " Mod Version: " + UF_VERSION);
			}
			if (data.Error == "noauth"){
				m_UFOnline = false;
				UFLog.Err("Auth Key is not vaild");
				if (!m_IsServer){
					this.RequestAuthToken(false);
				}
			}
			if (data.Error == "noerror" && data.Discord == "Enabled"){
				m_UDiscordEnabled = true;
			}
			if (data.Discord == "Online"){
				m_UDiscordEnabled = true;
			}
			if (data.OpenAI == "Online"){
				m_UOpenAIEnabled = true;
				UFLog.Info("[UAPI] OpenAI is enabled");
			}
			
			// Fire the ready event AFTER we've set the OpenAI/Discord status flags
			// This ensures mods can use AI Chat immediately after receiving this event
			if (!m_IsServer && m_InitialTokenReceived){
				UFLog.Debug("[UAPI] Firing UFrameworkReadyTokenReceived event (OpenAI=" + m_UOpenAIEnabled + ", Discord=" + m_UDiscordEnabled + ")");
				g_Game.GameScript.CallFunction(g_Game.GetMission(), "UFrameworkReadyTokenReceived", NULL, NULL);
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
				UFLog.Info("You may want to check for new versions of the Universal Framework WebService");
				return;
			}
			if (m_UFVersionOffset < -2){
				Error2("Universal Framework Mod Needs Update", "[UF] Universal Framework Mod is outdated and should be updated right away | WebService Version: " + data.Version + " Mod Version: " + UF_VERSION);
				return;
			}
			if (m_UFVersionOffset < -1){
				UFLog.Info("Universal Framework Mod maybe outdated and should be updated right away");
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
		
		// Fire ready event even on failure so mods aren't left waiting forever
		// They can check U().IsOpenAIEnabled() to see if AI is available
		if (!m_IsServer && m_InitialTokenReceived){
			UFLog.Debug("[UAPI] Firing UFrameworkReadyTokenReceived event (status check failed, OpenAI=" + m_UOpenAIEnabled + ")");
			g_Game.GameScript.CallFunction(g_Game.GetMission(), "UFrameworkReadyTokenReceived", NULL, NULL);
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
