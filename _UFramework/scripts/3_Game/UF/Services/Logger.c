modded class ULoggerBaseInstance extends Managed {
	override protected void SendToApi(string jsonString){
		U().Rest().Log(jsonString);
	}
}