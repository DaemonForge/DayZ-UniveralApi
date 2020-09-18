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
		ref ConfigBase data_in;
		js.ReadFromString(data_in, data, error);
		if (error == "" && data_in){
			data_in.OnDataReceive();
		} else {
			Print("[UPAI] [GameApi]CallBack Failed errorCode: Invalid Data");
		}*/
		
	};
};
