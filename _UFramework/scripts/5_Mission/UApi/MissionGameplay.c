modded class MissionGameplay extends MissionBase
{
	protected bool m_UFFirstRequest = true;	
	
	override void OnMissionStart(){
		super.OnMissionStart();
		m_UF_Initialized = false;
    	//Token expires in 22 minutes, tokens renew every 10 Minutes ensuring that if the API is down at the time of the renewal token will work till next retry
		int TokenRefreshRate = 600; 
		U().Cron().runEndless(TokenRefreshRate, this, "RequestNewAuthToken", NULL);
		#ifndef NO_GUI
		m_UFVideoPlayer = new UFVideoPlayer(); //extra save to ensure that you can't play audio on server, this is due to the way that video works as if you try it will crash server
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
			U().RequestAuthToken(m_UFFirstRequest);
			m_UFFirstRequest = false;
		}
	}
	
	
}