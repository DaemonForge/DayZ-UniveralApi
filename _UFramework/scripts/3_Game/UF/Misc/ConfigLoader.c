class UFrameworkConfig extends Managed {
	
	protected static string ConfigDIR = "$profile:UF";
	protected static string ConfigPATH = ConfigDIR + "\\UFramework.json";
	string ConfigVersion = "1";
	string ServerURL = "";
	string ServerID = "";
    string ServerAuth = "";
	int EnableBuiltinLogging = 0;
	int PromptDiscordOnConnect = 0;
	
	void Load(){
		if (g_Game.IsServer()){
			if (FileExist(ConfigPATH)){ //If config exist load File
			    JsonFileLoader<UFrameworkConfig>.JsonLoadFile(ConfigPATH, this);
				if (ServerURL != ""){
					int lastIndex = ServerURL.Length() - 1;
					if ( ServerURL.Substring(lastIndex,1) != "/"){ //correct URL
						ServerURL = ServerURL + "/";
						Save();
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
		if (g_Game.IsServer()){
			return ServerAuth;
		}
		return "ERROR";
	}
	
	void Save(){
		JsonFileLoader<UFrameworkConfig>.JsonSaveFile(ConfigPATH, this);
	}
	
	
}

ref UFrameworkConfig m_UFrameworkConfig;

//Helper function to return Config
static UFrameworkConfig UFConfig()
{
	if ( g_Game.IsServer()){
		if (!m_UFrameworkConfig)
		{
			m_UFrameworkConfig = new UFrameworkConfig;
			m_UFrameworkConfig.Load();
		}
	} else if (!m_UFrameworkConfig){
		Print("[UF] [WARN] UFramework Config is null on client");
	}
	return m_UFrameworkConfig;
};