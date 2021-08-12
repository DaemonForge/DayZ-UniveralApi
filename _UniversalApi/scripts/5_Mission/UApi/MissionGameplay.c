modded class MissionGameplay extends MissionBase
{
	
	override void OnMissionStart(){
		super.OnMissionStart();
		m_UApi_Initialized = false;
		UApi().RequestAuthToken(true);
    	//Token expires in 46.5 minutes, tokens renew every 21-23 Minutes ensuring that if the API is down at the time of the renewal token will work till next retry
		int TokenRefreshRate = Math.RandomInt(1260,1380) * 1000; //Uses a 2 minutes random to prevent lots of renewals happening at exact same time after server restarts
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.RequestNewAuthToken, TokenRefreshRate, false);
	}
	
	override void OnMissionFinish(){
		super.OnMissionFinish();
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(this.RequestNewAuthToken);
	}

	
	override void UniversalApiReady(){
		//You requests for after the AuthToken Is received
		super.UniversalApiReady();
	}
	
	
	void RequestNewAuthToken(){
		if (!GetGame().IsServer()){
			UApi().RequestAuthToken();
			int TokenRefreshRate = Math.QRandomInt(1260,1380) * 1000; 
			//Token expires in 46.5 minutes, tokens renew every 21-23 Minutes ensuring that if the API is down at the time of the renewal token will work till next retry
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.RequestNewAuthToken, TokenRefreshRate, false);
		}
	}
	
	
}