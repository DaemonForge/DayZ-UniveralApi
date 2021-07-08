class UApiQnAMakerServerAnswers extends Managed
{
	protected static string ConfigDIR = "$profile:UApi";
	protected static string ConfigPATH = ConfigDIR + "\\QnAMakerServerAnswers.json";
	protected bool UseNotifcationsMod = false;
	string BotName = "Bot";
	autoptr array<autoptr QnAMakerServerAnswer> ServerSpecificAnswers = new array<autoptr QnAMakerServerAnswer>;
	
	void Load(){
		if (GetGame().IsServer()){
			if (FileExist(ConfigPATH)){ //If config exist load File
			    JsonFileLoader<UApiQnAMakerServerAnswers>.JsonLoadFile(ConfigPATH, this);
			}else{ //File does not exist create FileMode.
				ServerSpecificAnswers.Insert(new QnAMakerServerAnswer("#SERVERNAME#", "US1"));
				Save();
			}
		}
	}
	
	void ProcessAnswer(string answer){
		string response = answer;
		for (int i = 0; i < ServerSpecificAnswers.Count(); i++){
			response.Replace(ServerSpecificAnswers.Get(i).ResponseCode, ServerSpecificAnswers.Get(i).Response); 
		}
		SendRespone(response);
	}
	
	void Save(){
			JsonFileLoader<UApiQnAMakerServerAnswers>.JsonSaveFile(ConfigPATH, this);
	}
	
	void SendRespone(string text){
		if (!UseNotifcationsMod){
			GetGame().Chat(BotName + ": " + text, "colorImportant");
		} else {
			
			#ifdef NOTIFICATIONS 
				float nTime = 5;
				int strlen = text.Length();
				if (strlen > 640){
					nTime = 70;
				} else if (strlen > 400){
					nTime = 50;
				} else if (strlen > 240){
					nTime = 35;
				} else if (strlen > 120){
					nTime = 25;
				} else if (strlen > 60){
					nTime = 15;
				}
				NotificationSystem.SimpleNoticiation(text, BotName, "_UniversalApi/images/Bot.edds", ARGB(230, 142, 180, 230), nTime, NULL);
			#endif
		}
	}
	
}


class QnAMakerServerAnswer {
	string ResponseCode = "";
	string Response = "";
	void QnAMakerServerAnswer( string code, string response){
		ResponseCode = code;
		Response = response;
	}
}


ref UApiQnAMakerServerAnswers m_QnAMakerServerAnswers;

//Helper function to return Config
static UApiQnAMakerServerAnswers UApiQnAMaker()
{
	if ( GetGame().IsServer()){
		if (!m_QnAMakerServerAnswers)
		{
			m_QnAMakerServerAnswers = new UApiQnAMakerServerAnswers;
			m_QnAMakerServerAnswers.Load();
		}
	}
	return m_QnAMakerServerAnswers;
};