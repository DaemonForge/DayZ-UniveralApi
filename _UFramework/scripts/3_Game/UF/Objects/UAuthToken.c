class ApiAuthToken extends Managed{
	string GUID = "";
	string AUTH = "";
	
	// NOTE: Expiry must be public for RPC serialization workaround
	// When object is sent via RPC, only public members are serialized
	// The constructor is NOT called on deserialization, so we set a default
	// that indicates "not initialized" and call ResetExpiry() after receiving
	int Expiry = -1;
	
	void ApiAuthToken(string guid, string auth){
		GUID = guid;
		AUTH = auth;
		ResetExpiry();
		UFLog.Debug("[Auth] Token constructed - GUID: " + guid + " Suffix: ..." + GetTokenSuffix() + " ExpiresIn: 900s");
	}
	
	string GetAuthToken(){
		return AUTH;
	}
	
	// Get last 8 characters of token for logging (safe to log, helps identify which token is in use)
	string GetTokenSuffix(){
		if (AUTH.Length() < 8) return AUTH;
		return AUTH.Substring(AUTH.Length() - 8, 8);
	}
	
	// Token expiry is set to 900 seconds (15 minutes) to match server-side JWT expiry
	// Server creates tokens with expiresIn: 900, so client must match
	void ResetExpiry(){
		Expiry = UUtil.GetUTCUnixInt() + 900;
	}
	
	bool IsExpired(){
		return Expiry < UUtil.GetUTCUnixInt();
	}
	
	// Check if token will expire within the given buffer seconds
	bool IsExpiringSoon(int bufferSeconds = 120){
		return (Expiry - bufferSeconds) < UUtil.GetUTCUnixInt();
	}
	
	// Get seconds until expiry (negative if expired)
	int GetSecondsUntilExpiry(){
		return Expiry - UUtil.GetUTCUnixInt();
	}
	
	void DoDebug(){
		UFLog.Debug("[Auth] Token Debug - GUID: " + GUID + " Suffix: ..." + GetTokenSuffix() + " Expiry: " + Expiry + " UTC: " + UUtil.GetUTCUnixInt() + " SecsLeft: " + GetSecondsUntilExpiry() + " IsExpired: " + IsExpired());
	}
}