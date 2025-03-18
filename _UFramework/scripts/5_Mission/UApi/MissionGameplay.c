modded class MissionGameplay extends MissionBase
{
	protected bool m_UFFirstRequest = true;	
	
	override void OnMissionStart(){
		super.OnMissionStart();
		m_UF_Initialized = false;
    	//Token expires in 46.5 minutes, tokens renew every 21-23 Minutes ensuring that if the API is down at the time of the renewal token will work till next retry
		int TokenRefreshRate = Math.RandomInt(1260,1380); //Uses a 2 minutes random to prevent lots of renewals happening at exact same time after server restarts
		U().Cron().runEndless(TokenRefreshRate, this, "RequestNewAuthToken", NULL);
	}
	
	override void OnMissionFinish(){
		super.OnMissionFinish();
		U().Cron().Remove(this,"RequestNewAuthToken");
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