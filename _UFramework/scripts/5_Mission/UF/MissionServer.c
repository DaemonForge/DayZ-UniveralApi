modded class MissionServer extends MissionBase
{
	void MissionServer()
	{
		U();
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).Call(this.UFrameworkReady);
	}	
	
	override void OnClientPrepareEvent(PlayerIdentity identity, out bool useDB, out vector pos, out float yaw, out int preloadTimeout)
	{
		if (identity){
			//Print("[UF] On Prepare - GUID: " + identity.GetId() );
			// Force fresh token generation on every connection attempt
			U().PreparePlayerAuth(identity.GetId());
		}
		super.OnClientPrepareEvent(identity, useDB, pos, yaw, preloadTimeout);
	}
	
	override void InvokeOnConnect(PlayerBase player, PlayerIdentity identity)
	{
		super.InvokeOnConnect(player, identity);
		
		// Backup: Ensure player has auth token after full connection
		// This catches cases where OnClientPrepareEvent timing wasn't sufficient
		if (identity){
			string authtoken = "";
			if (U().GetPlayerAuth(identity.GetId(), authtoken)){
				// Player auth exists in cache, send it again to ensure delivery
				U().SendAuthToken(identity, authtoken);
			} else {
				// No auth in cache, prepare new one (shouldn't happen but safety check)
				Print("[UF] Warning: No auth token found for " + identity.GetId() + " in InvokeOnConnect, preparing new one");
				U().PreparePlayerAuth(identity.GetId());
			}
		}
	}
	
	override void InvokeOnDisconnect(PlayerBase player)
	{
		super.InvokeOnDisconnect(player);
		
		// Clear cached auth token on disconnect to ensure fresh token on next connection
		// This is especially important for MapLink transfers between servers
		if (player && player.GetIdentity()){
			string guid = player.GetIdentity().GetId();
			Print("[UF] Player disconnected: " + guid + ", clearing cached auth token");
			U().ClearPlayerAuth(guid);
		}
	}
	
	override void UFrameworkReady(){
		//You requests for after the AuthToken Is received for server side code
		super.UFrameworkReady();
	}
}