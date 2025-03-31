modded class MissionGameplay extends MissionBase
{
	protected bool m_UFFirstRequest = true;	
	// Variables used to track the state and hold time of the hotkey.
    private bool m_DiscordKeyDown = false;
    private float m_DiscordKeyDownTime = 0;
    private const float m_DiscordHoldThreshold = 650.0;
    private bool m_HoldActionTriggered = false;
	
	override void OnMissionStart(){
		super.OnMissionStart();
		m_UF_Initialized = false;
    	//Token expires in 22 minutes, tokens renew every 10 Minutes ensuring that if the API is down at the time of the renewal token will work till next retry
		int TokenRefreshRate = 600; 
		U().RequestAuthToken(true);
		U().Cron().runEndless(TokenRefreshRate, this, "RequestNewAuthToken", NULL);
		#ifndef NO_GUI
		m_UFVideoPlayer = new UFVideoPlayer(); //extra save to ensure that you can't play audio on server, this is due to the way that video works as if you try it will crash server
        m_DiscordLoggedInWidget = new DiscordLoggedInWidget();
		#endif
	}
	
	override void OnMissionFinish(){
		super.OnMissionFinish();
		U().Cron().Remove(this,"RequestNewAuthToken");
		if (m_UFVideoPlayer){
			delete m_UFVideoPlayer;
		}
	}

	
	override void UFrameworkReady(){
		//You requests for after the AuthToken Is received
		super.UFrameworkReady();
	}
	
	
	void RequestNewAuthToken(){
		if (!GetGame().IsServer()){
			U().RequestAuthToken(false);
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
                GetGame().OpenURL(U().ds().Link());
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