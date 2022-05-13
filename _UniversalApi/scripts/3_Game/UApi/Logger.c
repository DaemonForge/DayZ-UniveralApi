modded class ULoggerBaseInstance extends Managed {
	override protected void SendToApi(string jsonString){
		UApi().Rest().Log(jsonString);
	}
}