modded class LoggerBaseInstance extends Managed {
	override protected SendToApi(string jsonString){
		UApi().Rest().Log(jsonString);
	}
}