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
	
	string GetServerID(){
		return ServerID;
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
	// Use both g_Game.IsServer() AND the NO_GUI define to detect server
	// During very early init, g_Game.IsServer() may not be accurate yet
	bool isServer = false;
	#ifdef NO_GUI
		isServer = true;
	#endif
	if (!isServer && g_Game){
		isServer = g_Game.IsServer();
	}
	
	if (isServer){
		if (!m_UFrameworkConfig)
		{
			UFLog.Debug("[UFConfig] Creating new config and loading from file...");
			m_UFrameworkConfig = new UFrameworkConfig;
			m_UFrameworkConfig.Load();
			if (m_UFrameworkConfig){
				UFLog.Debug("[UFConfig] Config loaded successfully. BaseURL: " + m_UFrameworkConfig.GetBaseURL());
			} else {
				UFLog.Err("[UFConfig] CRITICAL: Config is still null after Load()!");
			}
		}
	} else if (!m_UFrameworkConfig){
		// Only warn once per session, not spam
		static bool s_WarnedOnce = false;
		if (!s_WarnedOnce){
			UFLog.Info("[WARN] UFramework Config is null on client - waiting for RPC");
			s_WarnedOnce = true;
		}
	}
	return m_UFrameworkConfig;
};