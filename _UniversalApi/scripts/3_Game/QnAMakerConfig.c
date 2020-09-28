class UApiQnAMakerServerAnswers
{
	protected static string ConfigPATH = "$profile:UApi\\QnAMakerServerAnswers.json";
	string BotName = "Bot";
	ref array<ref QnAMakerServerAnswer> ServerSpecificAnswers = new ref array<ref QnAMakerServerAnswer>;
	
	void Load(){
		if (GetGame().IsServer()){
			if (FileExist(ConfigPATH)){ //If config exist load File
			    JsonFileLoader<UApiQnAMakerServerAnswers>.JsonLoadFile(ConfigPATH, this);
			}else{ //File does not exist create FileMode
				ServerSpecificAnswers.Insert(new ref QnAMakerServerAnswer("#SERVERNAME#", "US1"));
				Save();
			}
		}
	}
	
	string ProcessAnswer(string answer){
		string response = answer;
		for (int i = 0; i < ServerSpecificAnswers.Count(); i++){
			response.Replace(ServerSpecificAnswers.Get(i).ResponseCode, ServerSpecificAnswers.Get(i).Response); 
		}
		response = BotName + ": " + response;
		return response;
	}
	
	void Save(){
			JsonFileLoader<UApiQnAMakerServerAnswers>.JsonSaveFile(ConfigPATH, this);
	}
	
}


class QnAMakerServerAnswer{
	string ResponseCode = "";
	string Response = "";
	void QnAMakerServerAnswer( string code, string response){
		ResponseCode = code;
		Response = response;
	}
}


ref UApiQnAMakerServerAnswers m_QnAMakerServerAnswers;

//Helper function to return Config
static ref UApiQnAMakerServerAnswers UApiQnAMaker()
{
	if ( GetGame().IsServer()){
		if (!m_QnAMakerServerAnswers)
		{
			m_QnAMakerServerAnswers = new ref UApiQnAMakerServerAnswers;
			m_QnAMakerServerAnswers.Load();
		}
	}
	return m_QnAMakerServerAnswers;
};