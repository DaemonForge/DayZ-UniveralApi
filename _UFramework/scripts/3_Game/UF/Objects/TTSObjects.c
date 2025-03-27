class UTTSMessage extends UFObject_Base {
    string Message = "";
	float StaticNoise = 0.0;
	string Instructions = UTTSPersonality.RAGED_SURVIVOR;
	string Visual = UTTSVisual.LINE;
	
	void UTTSMessage(string msg, string instruc = UTTSPersonality.RAGED_SURVIVOR, float staticNoise = 0.0, string visual = UTTSVisual.LINE ){
		Message = msg;
		StaticNoise = staticNoise;
		Instructions = instruc;
		Visual = visual;
	}
	
	
	override string ToJson(){
		return JsonFileLoader<UTTSMessage>.JsonMakeData(this);
	}
 
}

class UTTSStatus extends StatusObject{
	string TTSId = "";
}
