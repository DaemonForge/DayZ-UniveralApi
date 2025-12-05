modded class MissionGameplay extends MissionBase
{
	protected bool m_UFFirstRequest = true;
	
	// Variables used to track the state and hold time of the hotkey.
    private bool m_DiscordKeyDown = false;
    private float m_DiscordKeyDownTime = 0;
    private const float m_DiscordHoldThreshold = 650.0;
    private bool m_HoldActionTriggered = false;
	
	override void OnMissionStart(){
		Print("[UF] MissionGameplay OnMissionStart");
		super.OnMissionStart();
		m_UF_Initialized = false;
		
		// Request initial token
		U().RequestAuthToken(m_UFFirstRequest);
		m_UFFirstRequest = false;
		
		// Token expires in 22 minutes, renew every 10 minutes
		// Simple cron job - the bug was in CronManager not initializing m_endCall properly
		U().Cron().runEndless(600, this, "RequestNewAuthToken", NULL);
		
		// Simple startup failsafe: Check in 45 seconds if we have auth. 
		// If the initial request failed (packet loss/server busy), this catches it early instead of waiting 10 mins.
		g_Game.GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.StartupAuthCheck, 45000, false);
		
		#ifndef NO_GUI
		m_UFVideoPlayer = new UFVideoPlayer();
        m_DiscordLoggedInWidget = new DiscordLoggedInWidget();
		#endif
	}
	
	override void OnMissionFinish(){
		Print("[UF] MissionGameplay OnMissionFinish");
		super.OnMissionFinish();
		U().Cron().Remove(this, "RequestNewAuthToken");
		if (m_UFVideoPlayer){
			delete m_UFVideoPlayer;
		}
	}

	
	override void UFrameworkReady(){
		super.UFrameworkReady();
		Print("[UF] MissionGameplay UFrameworkReady");
	}
	
	void RequestNewAuthToken(){
		if (!g_Game.IsServer()){
			Print("[UF] MissionGameplay RequestNewAuthToken");
			U().RequestAuthToken(false);
		}
	}
	
	void StartupAuthCheck(){
		if (!g_Game.IsServer() && !U().HasValidAuth()){
			Print("[UF] StartupAuthCheck - No auth received after 45s, retrying...");
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