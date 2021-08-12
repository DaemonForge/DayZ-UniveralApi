modded class MissionServer extends MissionBase
{
	void MissionServer()
	{
		UApi();
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Call(this.UniversalApiReady);
	}	
	
	override void OnClientPrepareEvent(PlayerIdentity identity, out bool useDB, out vector pos, out float yaw, out int preloadTimeout)
	{
		if (identity){
			//Print("[UAPI] On Prepare - GUID: " + identity.GetId() );
			UApi().PreparePlayerAuth(identity.GetId());
		}
		super.OnClientPrepareEvent(identity, useDB, pos, yaw, preloadTimeout);
	}
	
	override void UniversalApiReady(){
		//You requests for after the AuthToken Is received for server side code
		super.UniversalApiReady();
	}
}