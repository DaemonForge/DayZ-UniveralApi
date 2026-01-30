/**
 * Modded MissionGameplay providing client-side authentication and Discord integration.
 * 
 * Implements:
 * - Multi-layer JWT token renewal system (CRON, proactive, failure recovery)
 * - Aggressive startup retry for initial connection issues
 * - Discord link detection and avatar display
 * - CTRL+L hotkey for Discord integration (tap=link, hold=toggle avatar)
 */
modded class MissionGameplay extends MissionBase
{
	protected bool m_UFFirstRequest = true;  // Tracks if this is first auth request
	protected int m_StartupAuthRetries = 0;  // Counter for startup retry attempts
	
	// Variables used to track the state and hold time of the hotkey.
    private bool m_DiscordKeyDown = false;
    private float m_DiscordKeyDownTime = 0;
    private const float m_DiscordHoldThreshold = 650.0;  // ms to hold before triggering hide
    private bool m_HoldActionTriggered = false;
	
	/**
	 * Called when mission starts on client.
	 * Initializes 3-layer token renewal system and sets up Discord integration.
	 * 
	 * Token Renewal Strategy:
	 * 1. CRON: Primary renewal every 10 min (600s) - token expires at 15 min, giving 5 min buffer
	 * 2. PROACTIVE: GetAuthToken() checks if <4 min remaining, triggers renewal (catches cron failures)
	 * 3. FAILURE RECOVERY: OnAuthFailure() triggers renewal when API returns auth errors
	 * All renewal requests are rate-limited to 30s to prevent spam.
	 * 
	 * Startup Retries:
	 * - 5s, 15s, 30s, 60s retry schedule
	 * - Handles initial connection packet loss or timing issues
	 * 
	 * @note Initializes UFVideoPlayer and DiscordLoggedInWidget (client only)
	 */
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
	
	/**
	 * Called when mission ends on client.
	 * Cleans up CRON jobs and video player.
	 */
	override void OnMissionFinish(){
		UFLog.Info("MissionGameplay OnMissionFinish");
		super.OnMissionFinish();
		U().Cron().Remove(this, "RequestNewAuthToken");
		U().Cron().Remove(this, "LogTokenStatus");
		if (m_UFVideoPlayer){
			delete m_UFVideoPlayer;
		}
	}

	
	/**
	 * Called when framework is ready on client (after receiving auth token).
	 * Safe point to make authenticated API calls.
	 * 
	 * @note Dumps CRON jobs for debugging
	 */
	override void UFrameworkReady(){
		super.UFrameworkReady();
		UFLog.Info("MissionGameplay UFrameworkReady");
		// Dump cron jobs after framework is ready so we can see initial state
		U().Cron().DebugDump();
	}
	
	/**
	 * CRON job - requests new auth token every 10 minutes.
	 * Primary token renewal mechanism (Layer 1 of 3).
	 * 
	 * @note Only runs on client (!IsServer)
	 * @note Logs current token status before renewal for debugging
	 * @note Called automatically by CRON system
	 */
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
	
	/**
	 * DEBUG CRON job - logs token status every 60 seconds.
	 * Helps monitor token age and detect issues early.
	 * 
	 * Status levels:
	 * - OK: >300 seconds remaining
	 * - LOW: 240-300 seconds remaining
	 * - EXPIRING_SOON: 60-240 seconds remaining
	 * - CRITICAL: <60 seconds remaining
	 * - NO_VALID_TOKEN: No token or expired
	 */
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
	
	/**
	 * Startup failsafe - retries auth request if no valid token received.
	 * Scheduled at 5s, 15s, 30s, and 60s after mission start.
	 * 
	 * @note Handles initial connection packet loss or server timing issues
	 * @note Skips retry if valid token already received
	 * @note Uses forceFresh=true to bypass rate limiting during startup
	 */
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
    
	/**
	 * Called every frame to handle Discord integration hotkey (CTRL+L).
	 * 
	 * Hotkey Behavior:
	 * - TAP (press < 650ms): Opens Discord link URL for account linking
	 * - HOLD (press >= 650ms): Toggles Discord avatar visibility in UI
	 * 
	 * @param timeslice Time in seconds since last update
	 * 
	 * @note Only functions if U().IsDiscordEnabled() returns true
	 * @note After link tap, schedules ReCheckDiscord CRON job (20 iterations, 30s interval)
	 */
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
	
	/**
	 * CRON job - rechecks if player has linked their Discord account.
	 * Called periodically after player opens Discord link.
	 * 
	 * @note Scheduled by OnUpdate after CTRL+L tap
	 * @note Runs 20 times at 30-second intervals (total 10 minutes)
	 * @note Auto-stops if Discord user is detected
	 * @note Queries API for updated Discord info via U().ds().GetUser()
	 */
	void ReCheckDiscord(){
		if (GetDayZGame().DiscordUser()) return;
		U().ds().GetUser(GetDayZGame().GetSteamId(), GetDayZGame(), "CBCacheDiscordInfo");
	}
	
}