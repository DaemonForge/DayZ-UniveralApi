/**
 * Modded MissionBase providing Universal Framework initialization.
 * Base class for both server and client missions.
 * 
 * Initializes the framework singleton (UF()) and provides UFrameworkReady hook.
 */
modded class MissionBase extends MissionBaseWorld
{
	/**
	 * Constructor - initializes Universal Framework singleton.
	 * Calling UF() ensures framework is ready for use.
	 */
	void MissionBase()
	{
		UF();
	}
	
	/**
	 * Called when the Universal Framework is fully initialized and ready.
	 * Override this in your mod to perform initialization that requires framework services.
	 * 
	 * Server: Called immediately after framework init
	 * Client: Called after auth token is received from server
	 * 
	 * @usage
	 * override void UFrameworkReady() {
	 *     super.UFrameworkReady();
	 *     // Load mod data from database
	 *     UF().db().Load("MyMod", "config", this, "OnConfigLoaded");
	 * }
	 * 
	 * @note ALWAYS call super.UFrameworkReady() first
	 * @note This is the earliest safe point to make API calls requiring authentication
	 */
	override void UFrameworkReady(){
		super.UFrameworkReady();
			UFLog.Info("MissionBase UFrameworkReady");
		//A Safe Place to start pulling datadown from the WebServer 
		//(This will be called on init for the server, and after should be after the AuthToken Is received)
		
		//Don't forget to Super As
		//super.UFrameworkReady();
		
	}
}