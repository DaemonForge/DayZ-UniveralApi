class UApiTransactionCallBack : RestCallback
{
	string Status = "Pending";
	string ID = "";
	float Value;
	string Element;
	
	override void OnError(int errorCode) {
	
	};
	
	override void OnTimeout() {
	
	};
	
	override void OnSuccess(string data, int dataSize) {
	
	};
};