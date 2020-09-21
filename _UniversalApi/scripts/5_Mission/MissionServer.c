modded class MissionServer extends MissionBase
{
	void MissionServer()
	{
		UApi();
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Call(this.UniversalApiReady);
	}	
	
	override void UniversalApiReady(){
		//You requests for after the AuthToken Is received for server side code
		super.UniversalApiReady();
		
	}
}