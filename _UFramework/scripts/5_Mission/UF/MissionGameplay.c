modded class MissionGameplay extends MissionBase
{
	protected bool m_UFFirstRequest = true;
	protected int m_StartupAuthRetries = 0;
	
	// Variables used to track the state and hold time of the hotkey.
    private bool m_DiscordKeyDown = false;
    private float m_DiscordKeyDownTime = 0;
    private const float m_DiscordHoldThreshold = 650.0;
    private bool m_HoldActionTriggered = false;
	
	override void OnMissionStart(){
		UFLog.Info("MissionGameplay OnMissionStart");
		super.OnMissionStart();
		m_UF_Initialized = false;
		
		// Request initial token
		UFLog.Debug("[Auth] Sending initial auth token request");
		U().RequestAuthToken(m_UFFirstRequest);
		m_UFFirstRequest = false;
		
		// Token renewal strategy (3 layers of protection):
		// 1. CRON: Primary renewal every 10 min (600s) - token expires at 15 min, giving 5 min buffer
		// 2. PROACTIVE: GetAuthToken() checks if <4 min remaining, triggers renewal (catches cron failures)
		// 3. FAILURE RECOVERY: OnAuthFailure() triggers renewal when API returns auth errors
		// All renewal requests are rate-limited to 30s to prevent spam
		U().Cron().runEndless(600, this, "RequestNewAuthToken", NULL);
		
		// DEBUG: Log token status every 60 seconds to watch token age
		U().Cron().runEndless(60, this, "LogTokenStatus", NULL);
		
		// Aggressive startup retry: Check at 5s, 15s, 30s, 60s if no token received
		// Initial connection can have packet loss or timing issues with server
		m_StartupAuthRetries = 0;
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.StartupAuthRetry, 5000, false);  // 5s
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.StartupAuthRetry, 15000, false); // 15s
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.StartupAuthRetry, 30000, false); // 30s
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.StartupAuthRetry, 60000, false); // 60s
		
		#ifndef NO_GUI
		m_UFVideoPlayer = new UFVideoPlayer();
        m_DiscordLoggedInWidget = new DiscordLoggedInWidget();
		#endif
	}
	
	override void OnMissionFinish(){
		UFLog.Info("MissionGameplay OnMissionFinish");
		super.OnMissionFinish();
		U().Cron().Remove(this, "RequestNewAuthToken");
		U().Cron().Remove(this, "LogTokenStatus");
		if (m_UFVideoPlayer){
			delete m_UFVideoPlayer;
		}
	}

	
	override void UFrameworkReady(){
		super.UFrameworkReady();
		UFLog.Info("MissionGameplay UFrameworkReady");
		// Dump cron jobs after framework is ready so we can see initial state
		U().Cron().DebugDump();
	}
	
	void RequestNewAuthToken(){
		if (!g_Game.IsServer()){
			// Log current token status before renewal attempt
			UFLog.Debug("[Cron] ========================================");
			UFLog.Debug("[Cron] RequestNewAuthToken CRON JOB FIRED");
			UFLog.Debug("[Cron] ========================================");
			if (U().HasValidAuth()){
				UFLog.Debug("[Cron] Current token: Valid, SecsLeft=" + U().GetTokenSecondsRemaining() + ", Suffix=..." + U().GetTokenSuffix());
			} else {
				UFLog.Debug("[Cron] Current token: INVALID or missing");
			}
			U().RequestAuthToken(false);
		}
	}
	
	// DEBUG: Log token status every 60 seconds to monitor token age over time
	void LogTokenStatus(){
		if (!g_Game.IsServer()){
			if (U().HasValidAuth()){
				int secsLeft = U().GetTokenSecondsRemaining();
				string status = "OK";
				if (secsLeft < 60) status = "CRITICAL";
				else if (secsLeft < 240) status = "EXPIRING_SOON";
				else if (secsLeft < 300) status = "LOW";
				UFLog.Debug("[Auth] [STATUS] " + status + " SecsLeft=" + secsLeft + " Suffix=..." + U().GetTokenSuffix());
			} else {
				UFLog.Debug("[Auth] [STATUS] NO_VALID_TOKEN");
			}
		}
	}
	
	void StartupAuthRetry(){
		if (!g_Game.IsServer()){
			m_StartupAuthRetries++;
			
			if (U().HasValidAuth()){
				UFLog.Debug("[Auth] StartupAuthRetry #" + m_StartupAuthRetries + " - Token is valid, no retry needed. Suffix: ..." + U().GetTokenSuffix());
				return;
			}
			
			UFLog.Debug("[Auth] StartupAuthRetry #" + m_StartupAuthRetries + " - No valid token, requesting...");
			U().RequestAuthToken(true);
		}
	}
    
    override void OnUpdate(float timeslice)
    {
        super.OnUpdate(timeslice);
        
        // Check whether both the CTRL key and the L key are pressed.
        // KeyCode.KC_LCONTROL represents CTRL (left or right if you wish, adjust if necessary).
        // KeyCode.KC_L represents the L key.
        bool isCtrlDown = (KeyState(KeyCode.KC_LCONTROL) > 0);
        bool isLDown = (KeyState(KeyCode.KC_L) > 0);
        bool isDiscordKeyPressed = isCtrlDown && isLDown;
        
        // When the key combination is pressed for the first time...
        if (!m_DiscordKeyDown && isDiscordKeyPressed)
        {
            m_DiscordKeyDown = true;
            m_DiscordKeyDownTime = 0;
        }
        
        // If the key remains pressed, accumulate the held time.
        if (m_DiscordKeyDown && isDiscordKeyPressed)
        {
            // timeslice is in seconds; convert to milliseconds.
            m_DiscordKeyDownTime += timeslice * 1000;
            
            // If the hold duration exceeds the threshold, hide the avatar.
            if (!m_HoldActionTriggered && m_DiscordKeyDownTime >= m_DiscordHoldThreshold && U().IsDiscordEnabled())
            {
                if (GetDiscordLoggedInWidget())
                    GetDiscordLoggedInWidget().ToggleAvatar();
                m_HoldActionTriggered = true;
            }
        }
        
        // Detect key release: if we were tracking a key down but the combination is no longer pressed.
        if (m_DiscordKeyDown && !isDiscordKeyPressed)
        {
            // On release, if the hold time was less than the threshold, treat it as a tap.
            if (m_DiscordKeyDownTime < m_DiscordHoldThreshold && !GetDiscordLoggedInWidget().IsSet())
            {
                // Open the URL. This will not execute if the hotkey was held.
                g_Game.OpenURL(U().ds().Link());
				U().Cron().runEndCount(30, 20, this, "ReCheckDiscord", NULL);
            }
            // Reset the key state tracking.
            m_DiscordKeyDown = false;
            m_DiscordKeyDownTime = 0;
            m_HoldActionTriggered = false;
        }
    }
	
	void ReCheckDiscord(){
		if (GetDayZGame().DiscordUser()) return;
		U().ds().GetUser(GetDayZGame().GetSteamId(), GetDayZGame(), "CBCacheDiscordInfo");
	}
	
}