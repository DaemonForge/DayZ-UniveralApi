modded class ULoggerBaseInstance extends Managed {
	// Re-entrancy guard to prevent infinite loop:
	// UFLog -> DoLog -> SendToApi -> UniversalRest.Log -> Post -> UFLog -> DoLog -> SendToApi...
	protected static bool m_SendingToApi = false;
	
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