modded class MissionServer extends MissionBase
{
	// Track players waiting for auth delivery (for delayed retry)
	protected autoptr map<string, int> m_AuthRetryCount = new map<string, int>;
	protected const int MAX_AUTH_RETRIES = 3;
	protected const int AUTH_RETRY_DELAY_MS = 2000;
	
	void MissionServer()
	{
		U();
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).Call(this.UFrameworkReady);
	}
	
	override void OnClientPrepareEvent(PlayerIdentity identity, out bool useDB, out vector pos, out float yaw, out int preloadTimeout)
	{
		if (identity){
			string guid = identity.GetId();
			UFLog.Debug("OnClientPrepareEvent - Preparing auth for: " + guid);
			// Reset retry counter for new connection
			m_AuthRetryCount.Set(guid, 0);
			// Request fresh token - PreparePlayerAuth now tracks pending requests internally
			U().PreparePlayerAuth(guid);
		}
		super.OnClientPrepareEvent(identity, useDB, pos, yaw, preloadTimeout);
	}
	
	override void InvokeOnConnect(PlayerBase player, PlayerIdentity identity)
	{
		super.InvokeOnConnect(player, identity);
		
		// Schedule a delayed check to ensure auth was delivered
		// This acts as a failsafe if the initial send during REST callback failed
		if (identity){
			string guid = identity.GetId();
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.EnsureAuthDelivered, AUTH_RETRY_DELAY_MS, false, guid);
		}
	}
	
	// Failsafe: Check if auth was delivered, retry if needed
	protected void EnsureAuthDelivered(string guid){
		// Get current retry count
		int retryCount = 0;
		if (m_AuthRetryCount.Contains(guid)){
			retryCount = m_AuthRetryCount.Get(guid);
		}
		
		// Find the player by GUID
		DayZPlayer player = U().FindPlayer(guid);
		if (!player || !player.GetIdentity()){
			UFLog.Debug("EnsureAuthDelivered - Player " + guid + " no longer connected, skipping");
			m_AuthRetryCount.Remove(guid);
			return;
		}
		
		string authtoken = "";
		if (U().GetPlayerAuth(guid, authtoken)){
			// Auth is cached, send it to player
			UFLog.Debug("EnsureAuthDelivered - Sending auth token to " + guid + " (retry #" + retryCount + ")");
			U().SendAuthToken(player.GetIdentity(), authtoken);
			m_AuthRetryCount.Remove(guid);
		} else if (retryCount < MAX_AUTH_RETRIES){
			// Auth not ready yet, schedule another check
			retryCount++;
			m_AuthRetryCount.Set(guid, retryCount);
			UFLog.Debug("EnsureAuthDelivered - Auth not ready for " + guid + ", scheduling retry #" + retryCount);
			// Also try to request auth again in case the first request failed
			U().PreparePlayerAuth(guid);
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.EnsureAuthDelivered, AUTH_RETRY_DELAY_MS * retryCount, false, guid);
		} else {
			UFLog.Info("EnsureAuthDelivered - Max retries reached for " + guid + ", giving up");
			m_AuthRetryCount.Remove(guid);
		}
	}
	
	override void InvokeOnDisconnect(PlayerBase player)
	{
		super.InvokeOnDisconnect(player);
		
		// Clear cached auth token on disconnect to ensure fresh token on next connection
		// This is especially important for MapLink transfers between servers
		if (player && player.GetIdentity()){
			string guid = player.GetIdentity().GetId();
			UFLog.Debug("Player disconnected: " + guid + ", clearing cached auth token");
			U().ClearPlayerAuth(guid);
			m_AuthRetryCount.Remove(guid);
		}
	}
	
	override void UFrameworkReady(){
		//Your requests for after the AuthToken Is received for server side code
		super.UFrameworkReady();
	}
}