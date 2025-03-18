class UDBTransactionCallBack : UFRestCallBackBase
{
	string Status = "Pending";
	string ID = "";
	float Value;
	string Element;
	
	override void OnError(int errorCode) {
		super.OnError(errorCode);
	
	};
	
	override void OnTimeout() {
		super.OnTimeout();
	
	};
	
	override void OnSuccess(string data, int dataSize) {
		super.OnSuccess(data,dataSize);
	};
};