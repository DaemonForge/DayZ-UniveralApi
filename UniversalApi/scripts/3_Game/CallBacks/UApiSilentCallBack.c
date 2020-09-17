class UApiSilentCallBack : UApiCallBack
{
	override void OnError(int errorCode) {};
	override void OnTimeout() {};
	override void OnSuccess(string data, int dataSize) {};
};