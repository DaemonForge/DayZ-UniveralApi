modded class MissionBase extends MissionBaseWorld
{
	void MissionBase()
	{
		UApi();
	}
	
	override void UniversalApiReady(){
		super.UniversalApiReady();
		//A Safe Place to start pulling datadown from the WebServer 
		//(This will be called on init for the server, and after should be after the AuthToken Is received)
		
		//Don't forget to Super As
		//super.UniversalApiReady();
		
	}
}