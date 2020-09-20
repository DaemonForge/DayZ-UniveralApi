class UApiSilentCallBack : RestCallback
{
	override void OnError(int errorCode) {};
	override void OnTimeout() {};
	override void OnSuccess(string data, int dataSize) {};
};