modded class MissionGameplay extends MissionBase
{
	void MissionGameplay(){
		GetRPCManager().AddRPC( "UAPI", "RPCUniversalApiReady", this, SingeplayerExecutionType.Both );
		if (!GetGame().IsServer()){
			Print("[UPAI] Requesting API TOKEN");
			
			GetRPCManager().SendRPC("UAPI", "RPCRequestAuthToken", NULL, true);
		}
	}
	
	override void OnMissionStart()
	{
		super.OnMissionStart();
	}
	
	void RPCUniversalApiReady( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
		Param1< string > data  //Player ID, Icon
		if ( !ctx.Read( data ) ) return;
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.UniversalApiReady, 100, false); //Waiting a bit just to make sure that the AuthToken isn't delayed
	}
	
	override void UniversalApiReady(){
		//You requests for after the AuthToken Is received
		super.UniversalApiReady();
	}
	
	
}