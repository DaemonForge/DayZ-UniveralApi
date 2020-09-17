class BankingModApiCallBack : RestCallback
{
	
	override void OnError(int errorCode) {
		Print("[GameApi] CallBack Failed errorCode: " + errorCode);		
	};
	
	override void OnTimeout() {
		Print("[GameApi]CallBack Failed errorCode: Timeout");
		
	};
	
	override void OnSuccess(string data, int dataSize) {
		JsonSerializer js = new JsonSerializer();
		string error;
		js.ReadFromString(out_data, data, error);
		if (Class.CastTo(m_data,out_data)){
		} else {
			Print("[GameApi]CallBack Failed errorCode: Invalid Data");
		}
		
	};
};
