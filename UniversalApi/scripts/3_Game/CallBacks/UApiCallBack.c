class UApiCallBack : RestCallback
{
	
	override void OnError(int errorCode) {
		Print("[UPAI] [GameApi] CallBack Failed errorCode: " + errorCode);		
	};
	
	override void OnTimeout() {
		Print("[UPAI] [GameApi]CallBack Failed errorCode: Timeout");
		
	};
	
	override void OnSuccess(string data, int dataSize) {
		/*JsonSerializer js = new JsonSerializer();
		string error;
		CONFIGCALSS out_data;
		js.ReadFromString(out_data, data, error);
		if (Class.CastTo(m_ConfigVar, out_data)){
			
		} else {
			Print("[UPAI] [GameApi]CallBack Failed errorCode: Invalid Data");
		}*/
		
	};
};
