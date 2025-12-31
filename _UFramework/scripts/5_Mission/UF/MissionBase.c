modded class MissionBase extends MissionBaseWorld
{
	void MissionBase()
	{
		U();
	}
	
	override void UFrameworkReady(){
		super.UFrameworkReady();
			UFLog.Info("MissionBase UFrameworkReady");
		//A Safe Place to start pulling datadown from the WebServer 
		//(This will be called on init for the server, and after should be after the AuthToken Is received)
		
		//Don't forget to Super As
		//super.UFrameworkReady();
		
	}
}