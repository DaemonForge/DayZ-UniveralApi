class ApiAuthToken extends Managed{
	string GUID = "";
	string AUTH = "";
	
	protected int Expiry = -1;
	
	void ApiAuthToken(string guid, string auth){
		GUID = guid;
		AUTH = auth;
		ResetExpiry();
	}
	
	string GetAuthToken(){
		return AUTH;
	}
	
	void ResetExpiry(){
		Expiry = UUtil.GetUTCUnixInt() + 840;
	}
	
	bool IsExpired(){
		return Expiry < UUtil.GetUTCUnixInt();
	}
	
	void DoDebug(){
		Print("ApiAuthToken Debug GUID: " + GUID + " Expiry: " + Expiry + " Current UTC Time:" + UUtil.GetUTCUnixInt() + " IsExpired: " + IsExpired());
	}
}