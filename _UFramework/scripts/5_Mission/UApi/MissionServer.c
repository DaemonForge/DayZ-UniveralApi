modded class MissionServer extends MissionBase
{
	void MissionServer()
	{
		U();
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Call(this.UFrameworkReady);
	}	
	
	override void OnClientPrepareEvent(PlayerIdentity identity, out bool useDB, out vector pos, out float yaw, out int preloadTimeout)
	{
		if (identity){
			//Print("[UF] On Prepare - GUID: " + identity.GetId() );
			U().PreparePlayerAuth(identity.GetId());
		}
		super.OnClientPrepareEvent(identity, useDB, pos, yaw, preloadTimeout);
	}
	
	override void UFrameworkReady(){
		//You requests for after the AuthToken Is received for server side code
		super.UFrameworkReady();
	}
}