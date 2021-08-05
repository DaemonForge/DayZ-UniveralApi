class UniversalApiConfig extends Managed {
	
	protected static string ConfigDIR = "$profile:UApi";
	protected static string ConfigPATH = ConfigDIR + "\\UniversalApi.json";
	string ConfigVersion = "1";
	string ServerURL = "";
	string ServerID = "";
    string ServerAuth = "";
	bool QnAEnabled = false;
	int EnableBuiltinLogging = 0;
	int PromptDiscordOnConnect = 0;
	
	void Load(){
		if (GetGame().IsServer()){
			if (FileExist(ConfigPATH)){ //If config exist load File
			    JsonFileLoader<UniversalApiConfig>.JsonLoadFile(ConfigPATH, this);
				if (ServerURL != ""){
					int lastIndex = ServerURL.Length() - 1;
					if ( ServerURL.Substring(lastIndex,1) != "/"){ //correct URL
						ServerURL = ServerURL + "/";
						Save();
					}
					if (QnAEnabled){
						UApiQnAMaker();
					}
				}
				if (ConfigVersion != "1"){
					ConfigVersion = "1";
					PromptDiscordOnConnect = 0;
					Save();
				}
			} else { //File does not exist create file	
				MakeDirectory(ConfigDIR);
				Save();
			}
		}
	}
	
	string GetBaseURL(){
		return ServerURL;
	}
	
	string GetAuth(){
		if (GetGame().IsServer()){
			return ServerAuth;
		}
		return "ERROR";
	}
	
	void Save(){
		JsonFileLoader<UniversalApiConfig>.JsonSaveFile(ConfigPATH, this);
	}
	
	
}

ref UniversalApiConfig m_UniversalApiConfig;

//Helper function to return Config
static UniversalApiConfig UApiConfig()
{
	if ( GetGame().IsServer()){
		if (!m_UniversalApiConfig)
		{
			m_UniversalApiConfig = new UniversalApiConfig;
			m_UniversalApiConfig.Load();
		}
	}
	return m_UniversalApiConfig;
};