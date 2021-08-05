modded class MissionGameplay extends MissionBase
{
	
	override void OnMissionStart(){
		super.OnMissionStart();
		m_UApi_Initialized = false;
		UApi().RequestAuthToken(true);
		int TokenRefreshRate = Math.RandomInt(1200,1500) * 1000; //So that way on server starts it less likley to get a ton of requests at once 
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
			int TokenRefreshRate = Math.QRandomInt(1200,1500) * 1000; 
			//Prevents the call que from failing after being active for a long time.
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.RequestNewAuthToken, TokenRefreshRate, false);
		}
	}
	
	
}