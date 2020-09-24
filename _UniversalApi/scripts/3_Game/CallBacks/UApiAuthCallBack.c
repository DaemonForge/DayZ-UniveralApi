class UApiAuthCallBack : RestCallback
{
	protected int m_TryCount = 0;
	protected string m_GUID = "";
	
	void UApiAuthCallBack(string guid = ""){
		m_GUID = guid;
	}
	
	override void OnError(int errorCode) {
		Print("[UPAI] [UApiAuthCallBack] Auth of a Player Failed errorCode: " + errorCode);
		if (m_GUID != ""){
			UApi().AuthError(m_GUID);
		}
	};
	override void OnTimeout() {
		Print("[UPAI] [UApiAuthCallBack] Auth of a Player Failed errorCode: Timeout");
		if (m_GUID != ""){
			UApi().AuthError(m_GUID);
		}
	};
	override void OnSuccess(string data, int dataSize) {
		
		//Print("[UPAI] [UApiAuthCallBack] Auth of a Player Success data: " + data);
		ref ApiAuthToken authToken;
		
		JsonSerializer js = new JsonSerializer();
		string error;
		js.ReadFromString(authToken, data, error);
		if (error != ""){
			Print("[UPAI] [UApiAuthCallBack] Error: " + error);
		}
		if (authToken.GUID && authToken.AUTH != "ERROR"){
			Print("[UPAI] [UApiAuthCallBack] Auth of a Player Success data: GUID " + authToken.GUID);
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Call(this.DeferredLoad, authToken);
		} else {
			if (m_GUID != ""){
				UApi().AuthError(m_GUID);
			}
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
			m_ClientConfig.QnAEnabled = UApiConfig().QnAEnabled;
			GetRPCManager().SendRPC("UAPI", "RPCUniversalApiConfig", new Param2<ApiAuthToken, UniversalApiConfig>(authToken, m_ClientConfig), true, player);
			GetRPCManager().SendRPC("UAPI", "RPCUniversalApiReady", new Param1<string>(authToken.GUID), true, player);
			//Removing the Player from the Que About 3 Second Later to allow for other mods the possiblty to search the que as well.
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(UApi().RemoveFromQueue, 3000, false, player);
		} else if (m_TryCount <= 8){
			m_TryCount++;
			Print("[UPAI] [UApiAuthCallBack] Auth of a Player failed to Find Player retry: " + m_TryCount);
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.DeferredLoad, m_TryCount * 250, false, authToken);
		} else {
			Print("[UPAI] [UApiAuthCallBack] Couldn't Find Player on server");
			if (m_GUID != ""){
				UApi().AuthError(m_GUID);
			}
		}
	}
};

