/**
 * UFBaseEndpoint Class
 *
 * Base class for all Universal Framework API endpoints. Provides common functionality
 * for making REST API calls to the UFServerService, including authentication token
 * management and REST context setup.
 *
 * This class is extended by specific endpoint implementations:
 * - UDBEndpoint: Database operations
 * - UniversalDSEndpoint: Discord operations
 * - UFAIChatEndpoint: AI Chat operations
 * - UApiEndpoint: Generic API forwarding
 * - UFMsgEndpoint: Messaging operations
 * - UDBGlobalEndpoint: Global state management
 *
 * Methods:
 *   - EndpointBaseUrl(): Returns the base URL for API endpoints from configuration
 *   - AuthToken(): Returns the current authentication token
 *   - Api(): Creates and configures a RestContext for API calls
 *   - Post(route, jsonString, callback): Sends a POST request to the specified route
 *   - UpdateAuthToken(): Updates the authentication token on the current context
 *
 * Derived classes should override EndpointBaseUrl() to specify their endpoint path.
 */
class UFBaseEndpoint extends Managed {
	
	protected RestContext m_Context;
	
	/**
	 * EndpointBaseUrl
	 *
	 * Returns the base URL for this endpoint from the framework configuration.
	 * Derived classes can override this to specify their specific endpoint path.
	 *
	 * @return The base URL from UFConfig().GetBaseURL()
	 */
	protected string EndpointBaseUrl(){
		UFrameworkConfig cfg = UFConfig();
		if (!cfg){
			UFLog.Err("[UFBaseEndpoint] EndpointBaseUrl called but UFConfig() is null!");
			return "";
		}
		return cfg.GetBaseURL();
	}

	/**
	 * AuthToken
	 *
	 * Returns the current authentication token for API requests.
	 * On the server, returns the ServerAuth token from configuration.
	 * On the client, returns the player's authentication token received via RPC.
	 *
	 * @return The authentication token string
	 */
	protected string AuthToken(){
		UFramework uf = U();
		if (!uf){
			UFLog.Err("[UFBaseEndpoint] AuthToken called but U() is null!");
			return "";
		}
		return uf.GetAuthToken();
	}
	
	
	/**
	 * Api
	 *
	 * Creates and configures a REST API context for making HTTP requests.
	 * - Creates a RestApi instance if one doesn't exist
	 * - Sets read operation timeout to 30 seconds
	 * - Gets the endpoint base URL from configuration
	 * - Sets the authorization header with the current auth token
	 *
	 * @return A configured RestContext ready for POST requests, or null on error
	 */
	protected RestContext Api()
	{
		RestApi clCore = GetRestApi();
		if (!clCore)
		{
			clCore = CreateRestApi();
			if (!clCore){
				Error2("[UF] UFBaseEndpoint::Api()", "CRITICAL: Failed to create RestApi!");
				return null;
			}
			clCore.SetOption(ERestOption.ERESTOPTION_READOPERATION, 30);
			clCore.EnableDebug(false);
		}
		
		string baseUrl = EndpointBaseUrl();
		if (baseUrl == "" || baseUrl == "null"){
			Error2("[UF] UFBaseEndpoint::Api()", "BaseURL is empty or null - config not loaded?");
			return null;
		}
		
		m_Context = clCore.GetRestContext(baseUrl);
		if (!m_Context){
			Error2("[UF] UFBaseEndpoint::Api()", "GetRestContext returned null for URL: " + baseUrl);
			return null;
		}
		
		// Get fresh token and set header
		string token = AuthToken();
		if (token == "" || token == "null"){
			UFLog.Info("[WARN] Api() called with invalid token: '" + token + "' - request will likely fail");
		}
		m_Context.SetHeader(token);
		return m_Context;
	}
	
	/**
	 * Post
	 *
	 * Sends a POST request to the specified route with JSON data.
	 *
	 * @param route The API route to call (relative to endpoint base URL)
	 * @param jsonString The JSON payload to send in the request body
	 * @param UCBX The callback to handle the response
	 */
	protected void Post(string route, string jsonString, RestCallback UCBX)
	{
		RestContext ctx = Api();
		if (!ctx){
			Error2("[UF] UFBaseEndpoint::Post()", "Api() returned null - cannot make request to: " + route);
			return;
		}
		ctx.POST(UCBX, route, jsonString);
	}
	
	/**
	 * UpdateAuthToken
	 *
	 * Updates the authentication token on the current REST context.
	 * Called when the auth token has been renewed or changed.
	 */
	void UpdateAuthToken(){
		if (m_Context){
			m_Context.SetHeader(AuthToken());
		}
	}
	
}
