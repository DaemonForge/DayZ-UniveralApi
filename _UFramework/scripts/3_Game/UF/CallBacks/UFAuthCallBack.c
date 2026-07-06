/**
 * UAuthCallBack Class
 *
 * Callback handler for player authentication requests to the UFServerService.
 * Handles the response from authentication endpoint and manages player auth tokens.
 *
 * This callback is used internally by the framework when:
 * - A player connects and needs an authentication token
 * - The server requests authentication for a specific player GUID
 *
 * On Success:
 * - Receives an ApiAuthToken containing GUID and AUTH token
 * - Stores the token via UF().AddPlayerAuth() for future API requests
 *
 * On Error/Timeout:
 * - Triggers UF().AuthError() which schedules a retry attempt
 * - Logs error details for debugging
 *
 * @see ApiAuthToken for token structure
 * @see UFramework.AddPlayerAuth() for token storage
 */
class UAuthCallBack : UFRestCallBackBase
{
	protected int m_TryCount = 0;
	protected string m_GUID = "";
	
	/**
	 * Constructor
	 *
	 * @param guid The player's unique identifier (Steam GUID)
	 */
	void UAuthCallBack(string guid = ""){
		m_GUID = guid;
	}
	
	/**
	 * OnError
	 *
	 * Called when the authentication request fails with an error.
	 * Triggers a retry mechanism via UF().AuthError().
	 *
	 * @param errorCode The REST error code (see UF_* constants)
	 */
	override void OnError(int errorCode) {
		UFLog.Err("[UAuthCallBack] Auth of a Player Failed errorCode: " + UF().ErrorToString(errorCode));
		if (m_GUID != ""){
			UF().AuthError(m_GUID);
		}
		super.OnError(errorCode);
	};
	/**
	 * OnTimeout
	 *
	 * Called when the authentication request times out.
	 * Triggers a retry mechanism via UF().AuthError().
	 */
	override void OnTimeout() {
		UFLog.Err("[UAuthCallBack] Auth of a Player Failed errorCode: Timeout");
		if (m_GUID != ""){
			UF().AuthError(m_GUID);
		}
		super.OnTimeout();
	};
	
	/**
	 * OnSuccess
	 *
	 * Called when the authentication request succeeds.
	 * Parses the ApiAuthToken from JSON and stores it for the player.
	 *
	 * @param data JSON response containing ApiAuthToken
	 * @param dataSize Size of the response data
	 */
	override void OnSuccess(string data, int dataSize) {
		
		//UFLog.Debug("[UAuthCallBack] Auth of a Player Success data: " + data);
		autoptr ApiAuthToken authToken;

		JsonSerializer js = new JsonSerializer();
		string error;
		bool parsed = js.ReadFromString(authToken, data, error);
		if (!parsed || !authToken){
			//Malformed response (e.g. proxy error page) - retry instead of crashing on authToken.GUID
			UFLog.Err("[UAuthCallBack] Failed to parse auth response: " + error);
			if (m_GUID != ""){
				UF().AuthError(m_GUID);
			}
			super.OnSuccess(data, dataSize); //still queue callback cleanup
			return;
		}
		if (authToken.GUID == m_GUID && authToken.AUTH != "ERROR"){
			UFLog.Debug("[UAuthCallBack] Auth of a Player Success data: GUID " + authToken.GUID);
			UF().AddPlayerAuth(authToken.GUID, authToken.AUTH);
		} else {
			if (m_GUID != ""){
				UF().AuthError(m_GUID);
			}
		}
		super.OnSuccess(data,dataSize);
	};
	
};
