class UApiAuthCallBack : RestCallback
{
	protected int m_TryCount = 0;
	override void OnError(int errorCode) {
		Print("[UPAI] [UApiAuthCallBack] Auth of a Player Failed errorCode: " + errorCode);
		UApi().AuthError();
	};
	override void OnTimeout() {
		Print("[UPAI] [UApiAuthCallBack] Auth of a Player Failed errorCode: Timeout");
		UApi().AuthError();
	};
	override void OnSuccess(string data, int dataSize) {
		
		Print("[UPAI] [UApiAuthCallBack] Auth of a Player Success data: " + data);
		ref ApiAuthToken authToken;
		
		JsonSerializer js = new JsonSerializer();
		string error;
		js.ReadFromString(authToken, data, error);
		if (error != ""){
			Print("[UPAI] [UApiAuthCallBack] Error: " + error);
		}
		if (authToken.GUID){
			Print("[UPAI] [UApiAuthCallBack] Auth of a Player Success data: GUID " + authToken.GUID);
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Call(this.DeferredLoad, authToken);
		}
	};
	
	void DeferredLoad(ref ApiAuthToken authToken){
		PlayerIdentity player = PlayerIdentity.Cast(UApi().SearchQueue(authToken.GUID));
		if (player){
			Print("[UPAI] [UApiAuthCallBack] Auth of a Player Success PlayerFound");
			UniversalApiConfig m_ClientConfig = new UniversalApiConfig;
			m_ClientConfig.ServerURL = UApiConfig().ServerURL;
			m_ClientConfig.ServerID = UApiConfig().ServerID;
			m_ClientConfig.ServerAuth = "null";
			m_ClientConfig.ReadAuth = UApiConfig().ReadAuth;
			m_ClientConfig.ReadAllAuth = UApiConfig().ReadAllAuth;
			GetRPCManager().SendRPC("UAPI", "RPCUniversalApiConfig", new Param2<ApiAuthToken, UniversalApiConfig>(authToken, m_ClientConfig), true, player);
			UApi().RemoveFromQueue(player);
		} else if (m_TryCount <= 16){
			m_TryCount++;
			Print("[UPAI] [UApiAuthCallBack] Auth of a Player failed to Find Player retry: " + m_TryCount);
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.DeferredLoad, m_TryCount * 250, false, authToken);
		} else {
			Print("[UPAI] [UApiAuthCallBack] Couldn't Find Player on server");
		}
	}
};

