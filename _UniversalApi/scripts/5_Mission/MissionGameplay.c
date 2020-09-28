modded class MissionGameplay extends MissionBase
{
	protected bool m_UApi_Initialized = false;
	void MissionGameplay(){
		GetRPCManager().AddRPC( "UAPI", "RPCUniversalApiReady", this, SingeplayerExecutionType.Both );
		if (!GetGame().IsServer()){
			Print("[UPAI] Requesting First API TOKEN");
			GetRPCManager().SendRPC("UAPI", "RPCRequestAuthToken", NULL, true);
			int TokenRefreshRate = Math.RandomInt(1200,1325); //So that way on server starts it less likley to get a ton of requests at once 
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.RequestNewAuthToken, TokenRefreshRate * 1000, true);
		}
	}
		
	void RPCUniversalApiReady( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
		Param1< string > data  //Player ID, Icon
		if ( !ctx.Read( data ) ) return;
		if (!m_UApi_Initialized){
			m_UApi_Initialized = true;
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.UniversalApiReady, 200, false); //Waiting a bit just to make sure that the AuthToken isn't delayed
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
		}
	}
	
}