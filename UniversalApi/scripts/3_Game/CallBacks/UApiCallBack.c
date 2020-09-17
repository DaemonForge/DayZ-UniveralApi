class UApiCallBack : RestCallback
{
	ref ConfigBase data_out;
	
	void SetConfig( ref ConfigBase data_in){
		data_out = data_in;
	}
	
	override void OnError(int errorCode) {
		Print("[UPAI] [GameApi] CallBack Failed errorCode: " + errorCode);		
	};
	
	override void OnTimeout() {
		Print("[UPAI] [GameApi]CallBack Failed errorCode: Timeout");
		
	};
	
	override void OnSuccess(string data, int dataSize) {
		JsonSerializer js = new JsonSerializer();
		string error;
		ref ConfigBase data_in;
		js.ReadFromString(data_in, data, error);
		if (Class.CastTo(data_out, data_in)){
			
		} else {
			Print("[UPAI] [GameApi]CallBack Failed errorCode: Invalid Data");
		}
		
	};
};
