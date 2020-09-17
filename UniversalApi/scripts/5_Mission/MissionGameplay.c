modded class MissionGameplay extends MissionBase
{
	
	override void OnMissionStart()
	{
		if (!GetGame().IsServer()){
			Print("[UPAI] Requesting API TOKEN");
			GetRPCManager().SendRPC("UAPI", "RPCRequestAuthToken", NULL, true);
		}
	}
}