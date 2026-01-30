/**
 * ULoggerBaseInstance (modded)
 *
 * Modded extension of the base logger that adds API logging capability.
 * Sends log messages to the UFServerService API endpoint for centralized logging.
 *
 * This allows server logs to be stored in MongoDB and viewed/analyzed remotely.
 * Client logs are only sent if the client has a valid auth token from the server.
 *
 * Safety Features:
 * - Re-entrancy guard prevents infinite loops (UFLog -> API -> UFLog -> ...)
 * - Only sends logs when framework is initialized and online
 * - Validates configuration before attempting API calls
 * - File logging continues to work even if API logging fails
 *
 * @see UFLog for the logging interface
 * @see UniversalRest.Log() for the API endpoint
 */
modded class ULoggerBaseInstance extends Managed {
	/**
	 * Re-entrancy guard to prevent infinite loop:
	 * UFLog -> DoLog -> SendToApi -> UniversalRest.Log -> Post -> UFLog -> DoLog -> SendToApi...
	 */
	protected static bool m_SendingToApi = false;
	
	/**
	 * SendToApi
	 *
	 * Sends a log entry to the UFServerService API for centralized logging.
	 * This method has extensive safety checks to prevent errors and infinite loops.
	 *
	 * Safety Checks:
	 * 1. Re-entrancy guard (m_SendingToApi)
	 * 2. Framework initialization check
	 * 3. API online status check
	 * 4. Configuration availability check
	 * 5. Auth token validation (server needs ServerAuth, client needs player auth)
	 *
	 * The log is silently dropped if any check fails - this prevents log spam
	 * about logs failing to log, which would be counterproductive.
	 *
	 * @param jsonString The log entry in JSON format
	 */
	override protected void SendToApi(string jsonString){
		// Prevent re-entrancy (infinite loop)
		if (m_SendingToApi){
			return;
		}
		
		// Don't try to log to API if framework isn't initialized or API is offline
		// Silently skip - file logging still works, no need to spam console
		if (!UFramework.isGlobalInit() || !U().IsOnline()) {
			return;
		}
		
		// Check if API configuration is available (ServerURL and ServerAuth)
		// This prevents API calls before config is loaded
		if (!UFConfig()) {
			return;
		}
		
		if (g_Game.IsServer()) {
			// On server, verify ServerAuth is configured
			if (UFConfig().ServerAuth == "" || UFConfig().ServerAuth == "null") {
				return;
			}
			// Verify ServerURL is configured
			if (UFConfig().ServerURL == "" || UFConfig().ServerURL == "null") {
				return;
			}
		} else {
			// On client, verify we have received auth token from server via RPC
			if (!U().HasValidAuth()) {
				return;
			}
		}
		
		m_SendingToApi = true;
		U().Rest().Log(jsonString);
		m_SendingToApi = false;
	}
}