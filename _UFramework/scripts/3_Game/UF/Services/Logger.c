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
		m_SendingToApi = true;
		U().Rest().Log(jsonString);
		m_SendingToApi = false;
	}
}