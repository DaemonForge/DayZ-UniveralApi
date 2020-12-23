class UniversalApiConfig
{
	protected static string ConfigDIR = "$profile:UApi";
	protected static string ConfigPATH = ConfigDIR + "\\UniversalApi.json";
	string ServerURL = "";
	string ServerID = "";
    string ServerAuth = "";
	bool QnAEnabled = false;
	bool EnableBuiltinLogging = false;
	
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
			}else{ //File does not exist create file	
				MakeDirectory(ConfigDIR);
				Save();
			}
		}
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
static ref UniversalApiConfig UApiConfig()
{
	if ( GetGame().IsServer()){
		if (!m_UniversalApiConfig)
		{
			m_UniversalApiConfig = new ref UniversalApiConfig;
			m_UniversalApiConfig.Load();
		}
	}
	return m_UniversalApiConfig;
};