/**
 * ApiAuthToken Class
 *
 * Stores player authentication tokens received from the UFServerService.
 * Each token is a JWT (JSON Web Token) with a 15-minute expiration time.
 *
 * The token lifecycle:
 * 1. Client requests token via RPC to server
 * 2. Server requests token from UFServerService API
 * 3. Service generates JWT token (900 second expiry)
 * 4. Server receives token and sends to client via RPC
 * 5. Client stores token in this object and uses it for all API requests
 * 6. Token auto-refreshes when expired or expiring soon
 *
 * Token Structure:
 * - GUID: Player's unique identifier (Steam GUID)
 * - AUTH: The JWT token string
 * - Expiry: Unix timestamp when token expires (UTC)
 *
 * Important Notes:
 * - Expiry must be public for RPC serialization to work
 * - Constructor is NOT called during RPC deserialization
 * - Always call ResetExpiry() after receiving token via RPC
 *
 * @see UFramework.RequestAuthToken() for token request flow
 * @see UFramework.OnAuthFailure() for token renewal
 */
class ApiAuthToken extends Managed{
	string GUID = "";
	string AUTH = "";
	
	// NOTE: Expiry must be public for RPC serialization workaround
	// When object is sent via RPC, only public members are serialized
	// The constructor is NOT called on deserialization, so we set a default
	// that indicates "not initialized" and call ResetExpiry() after receiving
	int Expiry = -1;
	
	/**
	 * Constructor
	 *
	 * Creates a new authentication token with expiry set to 900 seconds (15 minutes).
	 *
	 * @param guid Player's unique identifier (Steam GUID)
	 * @param auth The JWT token string from UFServerService
	 */
	void ApiAuthToken(string guid, string auth){
		GUID = guid;
		AUTH = auth;
		ResetExpiry();
		UFLog.Debug("[Auth] Token constructed - GUID: " + guid + " Suffix: ..." + GetTokenSuffix() + " ExpiresIn: 900s");
	}
	
	/**
	 * GetAuthToken
	 *
	 * Returns the JWT authentication token string.
	 *
	 * @return The AUTH token string
	 */
	string GetAuthToken(){
		return AUTH;
	}
	
	/**
	 * GetTokenSuffix
	 *
	 * Returns the last 8 characters of the token for safe logging.
	 * Useful for debugging which token is in use without exposing the full token.
	 *
	 * @return Last 8 characters of AUTH, or full AUTH if shorter than 8 chars
	 */
	string GetTokenSuffix(){
		if (AUTH.Length() < 8) return AUTH;
		return AUTH.Substring(AUTH.Length() - 8, 8);
	}
	
	/**
	 * ResetExpiry
	 *
	 * Sets token expiry to 900 seconds (15 minutes) from current UTC time.
	 * MUST be called after receiving token via RPC since constructor isn't called.
	 * Also call this when token is refreshed/renewed.
	 */
	void ResetExpiry(){
		Expiry = UUtil.GetUTCUnixInt() + 900;
	}
	
	/**
	 * IsExpired
	 *
	 * Checks if the token has expired based on current UTC time.
	 *
	 * @return True if token is expired, false otherwise
	 */
	bool IsExpired(){
		return Expiry < UUtil.GetUTCUnixInt();
	}
	
	/**
	 * IsExpiringSoon
	 *
	 * Checks if the token will expire within the specified buffer time.
	 * Used to trigger proactive token renewal before actual expiry.
	 *
	 * @param bufferSeconds Number of seconds before expiry to return true (default: 120)
	 * @return True if token expires within buffer time, false otherwise
	 */
	bool IsExpiringSoon(int bufferSeconds = 120){
		return (Expiry - bufferSeconds) < UUtil.GetUTCUnixInt();
	}
	
	/**
	 * GetSecondsUntilExpiry
	 *
	 * Calculates how many seconds until the token expires.
	 *
	 * @return Seconds until expiry (negative if already expired)
	 */
	int GetSecondsUntilExpiry(){
		return Expiry - UUtil.GetUTCUnixInt();
	}
	
	/**
	 * DoDebug
	 *
	 * Logs detailed token information for debugging purposes.
	 * Includes GUID, token suffix, expiry time, time remaining, and expiration status.
	 */
	void DoDebug(){
		UFLog.Debug("[Auth] Token Debug - GUID: " + GUID + " Suffix: ..." + GetTokenSuffix() + " Expiry: " + Expiry + " UTC: " + UUtil.GetUTCUnixInt() + " SecsLeft: " + GetSecondsUntilExpiry() + " IsExpired: " + IsExpired());
	}
}