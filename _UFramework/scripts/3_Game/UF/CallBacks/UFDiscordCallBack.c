/**
 * UDiscordCallBack Class
 *
 * Base callback class for Discord user lookup operations.
 * Provides virtual methods that can be overridden to handle different Discord user states.
 *
 * This class is designed to be extended by modders who want to check if a player
 * has linked their Discord account and handle the various states:
 * - User found and linked (OnDiscordUserReceived)
 * - User not found or not linked (OnDiscordUserNotFound)
 * - Error during lookup (OnDiscordUserError)
 *
 * Usage Example:
 *   class MyDiscordCheck extends UDiscordCallBack {
 *       override void OnDiscordUserReceived(UDiscordUser user) {
 *           // Player has Discord linked, do something
 *       }
 *       override void OnDiscordUserNotFound(UDiscordUser user) {
 *           // Player needs to link Discord
 *       }
 *   }
 *
 * @see UF().ds().GetUser() for Discord user lookup
 */
class UDiscordCallBack: RestCallback
{
	
	/**
	 * OnError
	 *
	 * Called when the Discord API request fails with an error.
	 *
	 * @param errorCode The REST error code
	 */
	override void OnError(int errorCode) {
		UFLog.Err("[UDiscordCallBack] Failed errorCode: " + errorCode);
	};
	
	/**
	 * OnTimeout
	 *
	 * Called when the Discord API request times out.
	 */
	override void OnTimeout() {
		UFLog.Err("[UDiscordCallBack] Failed errorCode: Timeout");
	};
	
	/**
	 * OnSuccess
	 *
	 * Called when the Discord API request succeeds.
	 * Parses the UDiscordUser response and routes to appropriate handler method.
	 *
	 * @param data JSON response containing UDiscordUser
	 * @param dataSize Size of the response data
	 */
	override void OnSuccess(string data, int dataSize) {
		UDiscordUser user;
		
		JsonSerializer js = new JsonSerializer();
		string error;
		js.ReadFromString(user, data, error);
		if (error != ""){
			UFLog.Err("[UDiscordCallBack] Error: " + error);
		}
		if (user.Status && user.Status == "Success" && user.id && user.id != "0"){
			OnDiscordUserReceived(UDiscordUser.Cast(user));
		} else if (user.Status && (user.Status == "NotFound" || user.Status ==  "NotSetup")){
			OnDiscordUserNotFound(UDiscordUser.Cast(user));
			
		} else {
			OnDiscordUserError(UDiscordUser.Cast(user));
		}
	};
	
	/**
	 * OnDiscordUserReceived
	 *
	 * Virtual method called when the Discord user is found and linked.
	 * Override this method to handle the successful Discord user lookup.
	 *
	 * @param user The Discord user object containing id, username, roles, etc.
	 */
	void OnDiscordUserReceived(UDiscordUser user){
		//Do Stuff Here
		UFLog.Info("[UDiscordCallBack] Success: " + user.id);
		
	}
	
	/**
	 * OnDiscordUserNotFound
	 *
	 * Virtual method called when the player has not linked their Discord account.
	 * Override this method to handle the "not linked" state (e.g., show linking instructions).
	 *
	 * @param user The Discord user object (will have Status = "NotFound" or "NotSetup")
	 */
	void OnDiscordUserNotFound(UDiscordUser user){
		//Do Stuff Here
		UFLog.Info("[UDiscordCallBack] User not found");
	}
	
	/**
	 * OnDiscordUserError
	 *
	 * Virtual method called when an error occurs during Discord user lookup.
	 * Override this method to handle error states.
	 *
	 * @param user The Discord user object (will have Error message set)
	 */
	void OnDiscordUserError(UDiscordUser user){
		//Do Stuff Here
		UFLog.Err("[UDiscordCallBack] Error: " + user.Error);
		
	}
}
