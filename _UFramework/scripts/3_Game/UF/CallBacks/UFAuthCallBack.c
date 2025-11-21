class UAuthCallBack : UFRestCallBackBase
{
	protected int m_TryCount = 0;
	protected string m_GUID = "";
	
	void UAuthCallBack(string guid = ""){
		m_GUID = guid;
	}
	
	override void OnError(int errorCode) {
		Print("[UF] [UAuthCallBack] Auth of a Player Failed errorCode: " + U().ErrorToString(errorCode));
		if (m_GUID != ""){
			U().AuthError(m_GUID);
		}
		super.OnError(errorCode);
	};
	override void OnTimeout() {
		Print("[UF] [UAuthCallBack] Auth of a Player Failed errorCode: Timeout");
		if (m_GUID != ""){
			U().AuthError(m_GUID);
		}
		super.OnTimeout();
	};
	
	override void OnSuccess(string data, int dataSize) {
		
		//Print("[UF] [UAuthCallBack] Auth of a Player Success data: " + data);
		autoptr ApiAuthToken authToken;
		
		JsonSerializer js = new JsonSerializer();
		string error;
		js.ReadFromString(authToken, data, error);
		if (error != ""){
			Print("[UF] [UAuthCallBack] Error: " + error);
		}
		if (authToken.GUID == m_GUID && authToken.AUTH != "ERROR"){
			Print("[UF] [UAuthCallBack] Auth of a Player Success data: GUID " + authToken.GUID);
			U().AddPlayerAuth(authToken.GUID, authToken.AUTH);
		} else {
			if (m_GUID != ""){
				U().AuthError(m_GUID);
			}
		}
		super.OnSuccess(data,dataSize);
	};
	
};
