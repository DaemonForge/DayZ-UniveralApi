modded class ULoggerBaseInstance extends Managed {
	override protected void SendToApi(string jsonString){
		if (!U().IsOnline()) {
			Print("[UF] API Offline trying to send logs to UF Service");
		}
		U().Rest().Log(jsonString);
	}
}