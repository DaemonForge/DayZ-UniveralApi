/**
 * Modded MissionServer providing robust player authentication with automatic retry.
 * 
 * Implements a 3-layer failsafe system for player auth token delivery:
 * 1. Initial request during OnClientPrepareEvent
 * 2. Retry delivery on InvokeOnConnect
 * 3. Delayed retry checks with exponential backoff (up to 3 retries)
 * 
 * Prevents authentication failures due to network timing issues or packet loss.
 */
modded class MissionServer extends MissionBase
{
	// Track players waiting for auth delivery (for delayed retry)
	protected autoptr map<string, int> m_AuthRetryCount = new map<string, int>;
	protected const int MAX_AUTH_RETRIES = 3;
	protected const int AUTH_RETRY_DELAY_MS = 2000;
	
	/**
	 * Constructor - initializes framework and schedules UFrameworkReady.
	 */
	void MissionServer()
	{
		UF();
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).Call(this.UFrameworkReady);
	}
	
	/**
	 * Called when a client is preparing to connect to the server.
	 * Initiates player auth token request BEFORE player spawns.
	 * 
	 * @param identity Player's identity (contains GUID)
	 * @param useDB Whether to use database for character load
	 * @param pos Output - spawn position
	 * @param yaw Output - spawn orientation
	 * @param preloadTimeout Output - timeout for preload
	 * 
	 * @note Resets retry counter for this player
	 * @note Requests fresh token via UF().PreparePlayerAuth()
	 */
	override void OnClientPrepareEvent(PlayerIdentity identity, out bool useDB, out vector pos, out float yaw, out int preloadTimeout)
	{
		if (identity){
			string guid = identity.GetId();
			UFLog.Debug("OnClientPrepareEvent - Preparing auth for: " + guid);
			// Reset retry counter for new connection
			m_AuthRetryCount.Set(guid, 0);
			// Request fresh token - PreparePlayerAuth now tracks pending requests internally
			UF().PreparePlayerAuth(guid);
		}
		super.OnClientPrepareEvent(identity, useDB, pos, yaw, preloadTimeout);
	}
	
	/**
	 * Called when player finishes connecting and spawns into the world.
	 * Schedules a delayed check to ensure auth token was delivered successfully.
	 * 
	 * @param player Player that connected
	 * @param identity Player's identity
	 * 
	 * @note Acts as a failsafe if initial token send during PreparePlayerAuth failed
	 * @note Schedules EnsureAuthDelivered after AUTH_RETRY_DELAY_MS (2000ms)
	 */
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
	
	/**
	 * Failsafe mechanism that ensures auth token was delivered to client.
	 * Retries sending if token is cached but delivery failed.
	 * Uses exponential backoff and max retry limit.
	 * 
	 * @param guid Player GUID to check
	 * 
	 * @note Called automatically by InvokeOnConnect after delay
	 * @note Retries up to MAX_AUTH_RETRIES (3) times
	 * @note Retry delay increases with each attempt (2s, 4s, 6s)
	 * @note Also re-requests auth from API if cached token not available
	 * @note Gives up after max retries and logs warning
	 */
	protected void EnsureAuthDelivered(string guid){
		// Get current retry count
		int retryCount = 0;
		if (m_AuthRetryCount.Contains(guid)){
			retryCount = m_AuthRetryCount.Get(guid);
		}
		
		// Find the player by GUID
		DayZPlayer player = UF().FindPlayer(guid);
		if (!player || !player.GetIdentity()){
			UFLog.Debug("EnsureAuthDelivered - Player " + guid + " no longer connected, skipping");
			m_AuthRetryCount.Remove(guid);
			return;
		}
		
		string authtoken = "";
		if (UF().GetPlayerAuth(guid, authtoken)){
			// Auth is cached, send it to player
			UFLog.Debug("EnsureAuthDelivered - Sending auth token to " + guid + " (retry #" + retryCount + ")");
			UF().SendAuthToken(player.GetIdentity(), authtoken);
			m_AuthRetryCount.Remove(guid);
		} else if (retryCount < MAX_AUTH_RETRIES){
			// Auth not ready yet, schedule another check
			retryCount++;
			m_AuthRetryCount.Set(guid, retryCount);
			UFLog.Debug("EnsureAuthDelivered - Auth not ready for " + guid + ", scheduling retry #" + retryCount);
			// Also try to request auth again in case the first request failed
			UF().PreparePlayerAuth(guid);
			g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.EnsureAuthDelivered, AUTH_RETRY_DELAY_MS * retryCount, false, guid);
		} else {
			UFLog.Info("EnsureAuthDelivered - Max retries reached for " + guid + ", giving up");
			m_AuthRetryCount.Remove(guid);
		}
	}
	
	/**
	 * Called when player disconnects from server.
	 * Clears cached auth token to ensure fresh token on next connection.
	 * 
	 * @param player Player that disconnected
	 * 
	 * @note Critical for MapLink transfers between servers
	 * @note Removes player from retry tracking map
	 * @note Forces token refresh on reconnect
	 */
	override void InvokeOnDisconnect(PlayerBase player)
	{
		super.InvokeOnDisconnect(player);
		
		// Clear cached auth token on disconnect to ensure fresh token on next connection
		// This is especially important for MapLink transfers between servers
		if (player && player.GetIdentity()){
			string guid = player.GetIdentity().GetId();
			UFLog.Debug("Player disconnected: " + guid + ", clearing cached auth token");
			UF().ClearPlayerAuth(guid);
			m_AuthRetryCount.Remove(guid);
		}
	}
	
	/**
	 * Called when framework is fully initialized on server.
	 * Override to perform server-side initialization requiring API access.
	 * 
	 * @usage
	 * override void UFrameworkReady() {
	 *     super.UFrameworkReady();
	 *     // Load server config from database
	 *     UF().db().Load("ServerMod", "config", this, "OnServerConfigLoaded");
	 * }
	 * 
	 * @note ALWAYS call super.UFrameworkReady() first
	 */
	override void UFrameworkReady(){
		//Your requests for after the AuthToken Is received for server side code
		super.UFrameworkReady();
	}
}