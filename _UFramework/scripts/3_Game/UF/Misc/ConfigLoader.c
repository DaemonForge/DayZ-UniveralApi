/**
 * UFrameworkConfig Class
 *
 * Manages the Universal Framework configuration file stored at $profile:UF/UFramework.json.
 * This configuration is only loaded and managed on the server side. Client configurations
 * are sent via RPC from the server.
 *
 * Configuration Fields:
 *   - ConfigVersion: Version identifier for the config format (current: "1")
 *   - ServerURL: Base URL for the UFServerService API endpoint
 *   - ServerID: Unique identifier for this DayZ server instance
 *   - ServerAuth: Authentication token for server-to-service communication
 *   - EnableBuiltinLogging: Flag to enable/disable built-in logging features (0/1)
 *   - PromptDiscordOnConnect: Flag to prompt players about Discord linking on connect (0/1)
 *
 * Methods:
 *   - Load(): Loads configuration from file or creates default if missing
 *   - Save(): Saves current configuration to file
 *   - GetBaseURL(): Returns the ServerURL (base API endpoint)
 *   - GetServerID(): Returns the unique server identifier
 *   - GetAuth(): Returns the server authentication token (server-side only)
 *
 * Usage:
 *   UFrameworkConfig cfg = UFConfig();  // Global singleton accessor
 *   string url = cfg.GetBaseURL();
 */
class UFrameworkConfig extends Managed {
	
	protected static string ConfigDIR = "$profile:UF";
	protected static string ConfigPATH = ConfigDIR + "\\UFramework.json";
	string ConfigVersion = "1";
	string ServerURL = "";
	string ServerID = "";
    string ServerAuth = "";
	int EnableBuiltinLogging = 0;
	int PromptDiscordOnConnect = 0;
	
	/**
	 * Load
	 *
	 * Loads the configuration from UFramework.json if it exists, or creates a new
	 * default configuration file if it doesn't. Only operates on the server side.
	 *
	 * Actions performed:
	 * - Checks if config file exists, loads it if found
	 * - Validates and corrects ServerURL format (ensures trailing slash)
	 * - Migrates config version if needed
	 * - Creates default config file if none exists
	 */
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
	
	/**
	 * GetBaseURL
	 *
	 * Returns the base URL for the UFServerService REST API endpoint.
	 *
	 * @return The ServerURL from configuration (e.g., "https://yourserver.com/")
	 */
	string GetBaseURL(){
		return ServerURL;
	}
	
	/**
	 * GetServerID
	 *
	 * Returns the unique identifier for this DayZ server instance.
	 * Used to identify which server is making API requests.
	 *
	 * @return The ServerID from configuration
	 */
	string GetServerID(){
		return ServerID;
	}
	
	/**
	 * GetAuth
	 *
	 * Returns the server authentication token used for server-to-service communication.
	 * Only accessible on the server side.
	 *
	 * @return The ServerAuth token if on server, otherwise "ERROR"
	 */
	string GetAuth(){
		if (g_Game.IsServer()){
			return ServerAuth;
		}
		return "ERROR";
	}
	
	/**
	 * Save
	 *
	 * Saves the current configuration to the UFramework.json file.
	 * Creates the directory if it doesn't exist.
	 */
	void Save(){
		JsonFileLoader<UFrameworkConfig>.JsonSaveFile(ConfigPATH, this);
	}
	
	
}

ref UFrameworkConfig m_UFrameworkConfig;

/**
 * UFConfig
 *
 * Global singleton accessor for the Universal Framework configuration.
 * On the server, this creates and loads the configuration from file.
 * On the client, the configuration is populated via RPC from the server.
 *
 * The function uses both g_Game.IsServer() and the NO_GUI preprocessor define
 * to correctly detect server context during early initialization.
 *
 * @return The global UFrameworkConfig instance
 */
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