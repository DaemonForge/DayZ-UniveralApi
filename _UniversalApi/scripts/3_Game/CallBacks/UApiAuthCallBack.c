class UApiAuthCallBack : RestCallback
{
	protected int m_TryCount = 0;
	protected string m_GUID = "";
	
	void UApiAuthCallBack(string guid = ""){
		m_GUID = guid;
	}
	
	override void OnError(int errorCode) {
		Print("[UPAI] [UApiAuthCallBack] Auth of a Player Failed errorCode: " + UApi().ErrorToString(errorCode));
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
		autoptr ApiAuthToken authToken;
		
		JsonSerializer js = new JsonSerializer();
		string error;
		js.ReadFromString(authToken, data, error);
		if (error != ""){
			Print("[UPAI] [UApiAuthCallBack] Error: " + error);
		}
		if (authToken.GUID == m_GUID && authToken.AUTH != "ERROR"){
			Print("[UPAI] [UApiAuthCallBack] Auth of a Player Success data: GUID " + authToken.GUID);
			UApi().AddPlayerAuth(authToken.GUID, authToken.AUTH);
		} else {
			if (m_GUID != ""){
				UApi().AuthError(m_GUID);
			}
		}
	};
	
};
