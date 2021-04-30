modded class MissionGameplay extends MissionBase
{

	void MissionGameplay(){
		GetRPCManager().AddRPC( "UAPI", "RPCUniversalApiReady", this, SingeplayerExecutionType.Both );
	}
	
	override void OnMissionStart(){
		super.OnMissionStart();
		m_UApi_Initialized = false;
		Print("[UPAI] Requesting First API TOKEN");
		GetRPCManager().SendRPC("UAPI", "RPCRequestAuthToken", NULL, true);
		int TokenRefreshRate = Math.RandomInt(1200,1325); //So that way on server starts it less likley to get a ton of requests at once 
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.RequestNewAuthToken, TokenRefreshRate * 1000, false);
	}
	
	override void OnMissionFinish(){
		super.OnMissionFinish();
		Print("[UPAI] Removing Token Refresh from Call Que");
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(this.RequestNewAuthToken);
	}
		
	void RPCUniversalApiReady( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
		Param1< string > data  //Player ID, Icon
		if ( !ctx.Read( data ) ) return;
		if (!m_UApi_Initialized){
			m_UApi_Initialized = true;
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.UniversalApiReady, 300, false); //Waiting a bit just to make sure that the AuthToken isn't delayed
		}
	}
	
	
	
	override void UniversalApiReady(){
		//You requests for after the AuthToken Is received
		super.UniversalApiReady();
	}
	
	void RequestNewAuthToken(){
		if (!GetGame().IsServer()){
			Print("[UPAI] Requesting Renewed API TOKEN");
			GetRPCManager().SendRPC("UAPI", "RPCRequestAuthToken", NULL, true);
			int TokenRefreshRate = Math.RandomInt(1200,1325); 
			//Prevents the call que from failing after being active for a long time.
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.RequestNewAuthToken, TokenRefreshRate * 1000, false);
		}
	}
	
}