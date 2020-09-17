class UApiAuthCallBack : RestCallback
{
	
	override void OnError(int errorCode) {
		Print("[GameApi] Auth of a Player Failed errorCode: " + errorCode);		
	};
	override void OnTimeout() {
		Print("[GameApi] Auth of a Player Failed errorCode: Timeout");
		
	};
	override void OnSuccess(string data, int dataSize) {
		
		ref ApiAuthToken authToken;
		
		JsonSerializer js = new JsonSerializer();
		string error;
		js.ReadFromString(authToken, data, error);
		
		if (authToken.GUID){
			DayZPlayer player = DayZPlayer.Cast(GameApiFindPlayer(GUID));
			if (player && player.GetIdentity()){
				new UniversalApiConfig m_ClientConfig = new UniversalApiConfig;
				m_ClientConfig.ServerURL = UApiConfig().ServerURL;
				m_ClientConfig.ServerID = UApiConfig().ServerID;
				m_ClientConfig.ServerAuth = "null";
				m_ClientConfig.ReadAuth = UApiConfig().ReadAuth;
				m_ClientConfig.ReadAllAuth = UApiConfig().ReadAllAuth;
				GetRPCManager().SendRPC("UAPI", "RPCUniversalApiConfig", new Param2<ApiAuthToken, DayZGameApiConfig>(authToken, m_ClientConfig), true, identity);
			}
		}
	};
};

