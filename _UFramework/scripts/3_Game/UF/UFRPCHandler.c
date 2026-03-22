/**
 * UFRPCHandler - Centralized RPC handler for UFramework
 * 
 * Uses DayZGame.Event_OnRPC to register a single static handler
 * instead of having duplicate OnRPC overrides in PlayerBase and MissionGameplay.
 * 
 * Pattern follows vanilla SyncEvents.c implementation.
 */
class UFRPCHandler
{
	protected static bool m_Registered = false;
	
	/**
	 * Register the UFramework RPC handler with DayZGame's Event_OnRPC.
	 * Call this once during game initialization.
	 * Safe to call multiple times - will only register once.
	 */
	static void Register()
	{
		if (m_Registered)
			return;
		
		DayZGame dzGame = DayZGame.Cast(g_Game);
		if (!dzGame)
		{
			UFLog.Err("[RPC] Cannot register RPC handler - DayZGame not available");
			return;
		}
		
		dzGame.Event_OnRPC.Insert(OnRPC);
		m_Registered = true;
		UFLog.Info("[RPC] Registered centralized RPC handler with DayZGame.Event_OnRPC");
	}
	
	/**
	 * Unregister the handler (optional, for cleanup).
	 */
	static void Unregister()
	{
		if (!m_Registered)
			return;
		
		DayZGame dzGame = DayZGame.Cast(g_Game);
		if (dzGame)
		{
			dzGame.Event_OnRPC.Remove(OnRPC);
		}
		
		m_Registered = false;
		UFLog.Info("[RPC] Unregistered centralized RPC handler");
	}
	
	/**
	 * Central RPC handler - receives ALL RPCs through DayZGame.Event_OnRPC.
	 * Filters and handles only UFramework RPC IDs.
	 * 
	 * @param sender - PlayerIdentity of the sender (null for server broadcasts)
	 * @param target - Target Object (may be null for broadcast RPCs)
	 * @param rpc_type - The RPC type/ID integer
	 * @param ctx - ParamsReadContext containing RPC data
	 */
	static void OnRPC(PlayerIdentity sender, Object target, int rpc_type, ParamsReadContext ctx)
	{
		// Only handle UFramework RPCs (range 237983606+)
		if (rpc_type < UF_RPC_CONFIG || rpc_type > UF_RPC_REQUEST_RETRY)
			return;
		
		// Log all UF RPC traffic for debugging
		string senderInfo = "null";
		if (sender)
			senderInfo = sender.GetId();
		string targetInfo = "null";
		if (target)
			targetInfo = target.GetType();
		string rpcContext = "CLIENT";
		if (g_Game.IsServer())
			rpcContext = "SERVER";
		UFLog.Debug("[RPC] [" + rpcContext + "] Received RPC " + rpc_type + " from sender=" + senderInfo + " target=" + targetInfo);
		
		// Get UFramework singleton - if not available, we can't handle RPCs
		UFramework uf = UF();
		if (!uf)
		{
			UFLog.Info("[RPC] UFramework not initialized, cannot handle RPC " + rpc_type);
			return;
		}
		
		switch (rpc_type)
		{
			case UF_RPC_CONFIG:
				// Server -> Client: Auth token + config received
				if (g_Game.IsClient() || !g_Game.IsMultiplayer())
				{
					UFLog.Debug("[RPC] [CLIENT] Processing UF_RPC_CONFIG (auth token from server)");
					uf.OnRPC_UFrameworkConfig(ctx, sender);
				}
				break;
				
			case UF_RPC_REQUEST_AUTH:
				// Client -> Server: Auth token request
				if (g_Game.IsServer())
				{
					UFLog.Debug("[RPC] [SERVER] Processing UF_RPC_REQUEST_AUTH from " + senderInfo);
					uf.OnRPC_RequestAuthToken(ctx, sender);
				}
				break;
				
			case UF_RPC_REQUEST_RETRY:
				// Server -> Client: Retry request
				if (g_Game.IsClient() || !g_Game.IsMultiplayer())
				{
					UFLog.Debug("[RPC] [CLIENT] Processing UF_RPC_REQUEST_RETRY");
					uf.OnRPC_RequestRetry(ctx, sender);
				}
				break;
		}
	}
}
