class UniversalApi extends Managed {
		
	//Getter function for the Database Endpoint using either OBJECT_DB or PLAYER_DB
	// "PLAYER_DB" is only accessable on client for the player info being requested 
	// "OBJECT_DB" all clients can access all data.
	UApiDBEndpoint db(int collection = OBJECT_DB){
		if (collection == OBJECT_DB){
			if (!m_ObjectEndPoint){
				m_ObjectEndPoint = new UApiDBEndpoint("Object");
			}
			return m_ObjectEndPoint;
		} 
		if (collection == PLAYER_DB){
			if (!m_PlayerEndPoint){
				m_PlayerEndPoint = new UApiDBEndpoint("Player");
			}
			return m_PlayerEndPoint;
		}
		return NULL;
	}
	
	//Getter function for the Discord Endpoint
	UniversalDSEndpoint ds(){
		if (!m_UniversalDSEndpoint){
			m_UniversalDSEndpoint = new UniversalDSEndpoint;
		}
		return m_UniversalDSEndpoint;
	}
	
	//Getter function for the Globals Endpoint
	UApiDBGlobalEndpoint globals(){
		if (!m_UApiDBGlobalEndpoint){
			m_UApiDBGlobalEndpoint = new UApiDBGlobalEndpoint;
		}
		return m_UApiDBGlobalEndpoint;
	}
	
	//Getter function for the API Endpoint
	UApiAPIEndpoint api(){
		if (!m_UApiAPIEndpoint){
			m_UApiAPIEndpoint = new UApiAPIEndpoint;
		}
		return m_UApiAPIEndpoint;
	}
	
	//Request a call to be canceled
	void RequestCallCancel(int cid){
		m_CanceledCalls.Insert(cid);
	}
	
	//A super simple Post Interface to help people
	static int Post(string url)
	{
		RestContext ctx = RestCore().GetRestContext(url);
		ctx.SetHeader("application/json");
		ctx.POST(new UApiSilentCallBack, "", "{}");
		return 0;
	}
	
	//A super simple Post Interface to help people
	static int Post(string url, string jsonString, autoptr RestCallback UCBX = NULL, string contentType = "application/json")
	{
		if (!UCBX){
			UCBX = new UApiSilentCallBack;
		}
		RestContext ctx = RestCore().GetRestContext(url);
		ctx.SetHeader(contentType);
		ctx.POST(UCBX, "", jsonString);
		return 0;
	}
	
	//A super simple Post Interface to help people
	static int Post(string url, string jsonString, autoptr UApiCallbackBase cb, string contentType = "application/json")
	{
		int cid = UApi().CallId();
		if (cb){
			RestContext ctx = RestCore().GetRestContext(url);
			ctx.SetHeader(contentType);
			ctx.POST(new UApiDBNestedCallBack(cb,cid), "", jsonString);
			return cid;
		}
		return -1;
	}
	
	//A super simple Get Interface to help people
	static int Get(string url)
	{
		RestContext ctx =  RestCore().GetRestContext(url);
		ctx.GET(new UApiSilentCallBack, "");
		return 0;
	}
	//A super simple Get Interface to help people
	static int Get(string url, autoptr RestCallback UCBX)
	{
		if (!UCBX){
			UCBX = new UApiSilentCallBack;
		}
		RestContext ctx =  RestCore().GetRestContext(url);
		ctx.GET(UCBX , "");
		return 0;
	}
	//A super simple Get Interface to help people
	static int Get(string url,autoptr UApiCallbackBase cb)
	{
		int cid = UApi().CallId();
		if (cb){
			RestContext ctx =  RestCore().GetRestContext(url);
			ctx.GET(new UApiDBNestedCallBack(cb,cid), "");
			return cid;
		}
		return -1;
	}
	
	//Will return true if the discord endpoint is configured (this doesn't mean its configured correctly though :p)
	bool IsDiscordEnabled(){
		return m_UApiDiscordEnabled;
	}
	
	//Will return true if the Translate Endpoint is configured
	bool IsTranslateEnabled(){
		return m_UApiTranslateEnabled;
	}
	
	//Returns True if the status check has come back and everything is okay
	bool IsOnline(){
		return m_UApiOnline;
	}
	
	//Returns current Version Offset 0 Version Matches exactly
	// -1 or 1 off by a patch this is not a problem and won't cause any major issues
	// -2 or 2 off by a Minor Version this may cause some endpoints to not work or features to be missing
	// -3 or 3 off by a Major Version most likely the mod will not work at all!
	int VersionOffset(){
		return m_UApiVersionOffset;
	}
	
	//Checks to see if the Random Numbers are below half and add's more
	void CheckAndRenewQRandom(){
		if (Math.QRandomRemaining()<= 2000){
			GetQRandomNumbers();
		}
	}
	
	//Returns Current Version of the Mod
	static string GetVersion(){
		return UAPI_VERSION;
	}
	
	//Simple function for finding a player based on their GUID
	static DayZPlayer FindPlayer(string GUID){
		if (GetGame().IsServer()){
			autoptr array<Man> players = new array<Man>;
			GetGame().GetPlayers( players );
			for (int i = 0; i < players.Count(); i++){
				DayZPlayer player = DayZPlayer.Cast(players.Get(i));
				if (player.GetIdentity() && player.GetIdentity().GetId() == GUID ){
					return player;
				}
			}
		}
		return NULL;
	}
	
	//Simple function for finding a player based on their identity
	static DayZPlayer FindPlayerByIdentity(PlayerIdentity identity) {
		if (!identity)
			return NULL;

		int highBits;
		int lowBits;
		GetGame().GetPlayerNetworkIDByIdentityID(identity.GetPlayerId(), lowBits, highBits);
		return DayZPlayer.Cast(GetGame().GetObjectByNetworkId(lowBits, highBits));
	}
	
	
	
	
	
	
	//Stuff that you don't need to worry about :P
	
	protected bool m_IsServer = false;
	
	protected int m_CallId = 0;
	protected int m_AuthRetries = 0;
	
	protected bool m_UApiOnline = false;
	protected int m_UApiVersionOffset = 0;
	protected bool m_UApiDiscordEnabled = false;
	protected bool m_UApiTranslateEnabled = false;
	
	protected bool UAPI_Init = false;
	protected autoptr ApiAuthToken m_authToken;
	
	protected autoptr UniversalRest m_UniversalRest;
	
	protected autoptr UniversalDiscordRest m_UniversalDiscordRest;
	protected autoptr UniversalDSEndpoint m_UniversalDSEndpoint;
	protected autoptr UApiDBGlobalEndpoint m_UApiDBGlobalEndpoint;
	
	protected autoptr UApiDiscordUser dsUser;
		
	protected autoptr map<string, string> PlayerAuths = new map<string, string>;
	
	protected autoptr UApiDBEndpoint m_PlayerEndPoint;
	
	protected autoptr UApiDBEndpoint m_ObjectEndPoint;
	//Can't Do Globals due to how globals work
	
	protected autoptr UApiAPIEndpoint m_UApiAPIEndpoint;
	
	protected autoptr TIntSet m_CanceledCalls = new TIntSet;
	
	protected int LastRandomNumberRequestCall = -1;
	
		
	protected static RestApi RestCore()
	{
		RestApi clCore = GetRestApi();
		if (!clCore) {
			clCore = CreateRestApi();
			clCore.SetOption(ERestOption.ERESTOPTION_READOPERATION, 15);
		}
		return clCore;
	}
	
	string GetAuthToken(){
		if (m_authToken && !GetGame().IsServer()){
			return m_authToken.GetAuthToken();
		} else if (GetGame().IsServer() && UApiConfig().ServerAuth != ""){
			return UApiConfig().ServerAuth;
		}
		return "null";
	}
	
	bool HasValidAuth(){
		return (GetAuthToken() != "null" && GetAuthToken() != "error" && GetAuthToken() != "ERROR" && GetAuthToken() != "" );
	}
	
	
	//OLD RestCallBack Endpoints use if you want to use RestCallBack Classes instead of Function Based
	UniversalRest Rest(){
		if (!m_UniversalRest){
			m_UniversalRest = new UniversalRest;
		}
		return m_UniversalRest;
	}

	UniversalDiscordRest Discord(){
		if (!m_UniversalDiscordRest){
			m_UniversalDiscordRest = new UniversalDiscordRest;
		}
		return m_UniversalDiscordRest;
	}
	
	
	
	void ~UniversalApi(){
		if (m_IsServer && UAPI_Init && GetGame()){
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(this.CheckAndRenewQRandom);
		}
	}
	
	void Init(){
		#ifdef NO_GUI
			Print("[UAPI] Detected Server");
			m_IsServer = true;
		#endif
		if (!UAPI_Init){
			Print("[UAPI] First Init");
			UAPI_Init = true;
			GetRPCManager().AddRPC( "UAPI", "RPCUniversalApiConfig", this, SingeplayerExecutionType.Both );
			GetRPCManager().AddRPC( "UAPI", "RPCRequestQnAConfig", this, SingeplayerExecutionType.Both );
			GetRPCManager().AddRPC( "UAPI", "RPCRequestAuthToken", this, SingeplayerExecutionType.Both );
			GetRPCManager().AddRPC( "UAPI", "RPCRequestRetry", this, SingeplayerExecutionType.Both );
			if (m_IsServer){
				UApi().api().Status(this, "CBStatusCheck");
				CheckAndRenewQRandom();
				GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.CheckAndRenewQRandom, 10 * 60 * 1000, true);
			}
		}
	}
	
	protected void RPCUniversalApiConfig( CallType type, ParamsReadContext ctx, PlayerIdentity sender, ref Object target )
	{
		Print("[UAPI] Received UApi Config");
		Param2<ApiAuthToken, UniversalApiConfig> data; 
		if ( !ctx.Read( data ) ) return;
		m_AuthRetries = 0;
		m_authToken = data.param1;
		m_UniversalApiConfig = data.param2;
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Call(this.OnTokenReceived);
	}
	
	protected void OnTokenReceived(){
		UApi().api().Status(this, "CBStatusCheck");
		if (m_UniversalApiConfig.QnAEnabled){
			GetRPCManager().SendRPC("UAPI", "RPCRequestQnAConfig", new Param1<UApiQnAMakerServerAnswers>(NULL), true);
		}
		UApi().ds().GetUser(GetDayZGame().GetSteamId(), GetDayZGame(), "CBCacheDiscordInfo");
		GetGame().GameScript.CallFunction(GetGame().GetMission(), "UniversalApiReadyTokenReceived", NULL, NULL);
		CheckAndRenewQRandom();
		Print("[UAPI] OnTokenReceived Proccessed");
	}
	
	
	void RPCRequestRetry( CallType type, ParamsReadContext ctx, PlayerIdentity sender, ref Object target ) {
		if (GetGame().IsClient() && ++m_AuthRetries <= 20){
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.RequestAuthToken, m_AuthRetries * 2200, false, true);
		}
	}
	
	void RequestAuthToken(bool first = false){
		if (!m_IsServer){
			GetRPCManager().SendRPC("UAPI", "RPCRequestAuthToken", new Param1<bool>(first), true);
		}
	}
	
	void PreparePlayerAuth(string guid){
		this.Rest().GetAuth(guid);
	}
	
	void AddPlayerAuth(string guid, string auth){
		if (!PlayerAuths){PlayerAuths = new map<string, string>;}
		//Print("[UAPI] Adding PlayerAuth for " + guid + " to cache");
		PlayerAuths.Set(guid,auth); //Set Auth incase a request comes in.
		
		DayZPlayer player; //If renewing or if player is availbe send to player
		if (Class.CastTo(player, FindPlayer(guid)) && player.GetIdentity() ){
			SendAuthToken(player.GetIdentity(), auth);
		}
	}
	
	bool GetPlayerAuth(string guid, out string auth){
		if (PlayerAuths && PlayerAuths.Contains(guid)){
			auth = PlayerAuths.Get(guid);
			return true;
		}
		Print("[UAPI] Failed to find Player Auth for " + guid);
		return false;
	}		
	
	protected void RPCRequestQnAConfig( CallType type, ParamsReadContext ctx, PlayerIdentity sender, ref Object target )
	{
		Param1<UApiQnAMakerServerAnswers> data; 
		if ( !ctx.Read( data ) ) return;
		if (!GetGame().IsServer() && !PlayerIdentity.Cast(sender) && data.param1){
			m_QnAMakerServerAnswers = data.param1;
		} else if (GetGame().IsServer() && PlayerIdentity.Cast(sender) && data.param1 == NULL){
			GetRPCManager().SendRPC("UAPI", "RPCRequestQnAConfig", new Param1<UApiQnAMakerServerAnswers>(m_QnAMakerServerAnswers), true, sender);
		}
	}
	
	protected void RPCRequestAuthToken( CallType type, ParamsReadContext ctx, PlayerIdentity sender, ref Object target )
	{
		Param1<bool> data; 
		if ( !ctx.Read( data ) ) return;
		PlayerIdentity identity = PlayerIdentity.Cast(sender);
		if (m_IsServer && identity){
			UApiConfig();
			string authtoken = "";
			if (UApiConfig().ServerAuth != "" && UApiConfig().ServerAuth != "null" ){
				if (data.param1 && GetPlayerAuth(identity.GetId(), authtoken)){
					//Print("[UAPI] RPCRequestAuthToken Sending Cached Token ");
					SendAuthToken(identity, authtoken);
				} else if (FindPlayer(identity.GetId())){
					//Print("[UAPI] RPCRequestAuthToken  Renewing Auth Token" );
					PreparePlayerAuth(identity.GetId());
				}  else {
					Print("[UAPI] RPCRequestAuthToken Requesting client retry." );
					GetRPCManager().SendRPC("UAPI", "RPCRequestRetry", new Param1<bool>(true), true, identity);
				}
			} else if (UApiConfig().ServerAuth && UApiConfig().ServerAuth != "" && UApiConfig().ServerAuth != "null") {
				Error("[UAPI] Server Auth is empty or null");
			}
		}
	}
	
	protected void SendAuthToken(PlayerIdentity idenitity, string auth){
		if (idenitity && auth != ""){
			Print("[UAPI] Sending PlayerAuth Token to " + idenitity.GetId());
			autoptr UniversalApiConfig m_ClientConfig = new UniversalApiConfig;
			m_ClientConfig.ConfigVersion = UApiConfig().ConfigVersion;
			m_ClientConfig.ServerURL = UApiConfig().ServerURL;
			m_ClientConfig.ServerID = UApiConfig().ServerID;
			m_ClientConfig.ServerAuth = "null";
			m_ClientConfig.QnAEnabled = UApiConfig().QnAEnabled;
			m_ClientConfig.EnableBuiltinLogging = UApiConfig().EnableBuiltinLogging;
			m_ClientConfig.PromptDiscordOnConnect = UApiConfig().PromptDiscordOnConnect;
			autoptr ApiAuthToken m_authToken = new ApiAuthToken;
			m_authToken.GUID = idenitity.GetId();
			m_authToken.AUTH = auth;
			GetRPCManager().SendRPC("UAPI", "RPCUniversalApiConfig", new Param2<ApiAuthToken, UniversalApiConfig>(m_authToken, m_ClientConfig), true, idenitity);
		} else {
			Print("[UAPI] [UApiAuthCallBack] ERROR ");
			if (idenitity){
				UApi().AuthError(idenitity.GetId());
			}
		}
	}
	
	void AuthError(string guid){
		Print("[UAPI] Auth Error for " + guid);
		//If Auth Token Failed just try again in 3 minutes 
		if (guid != "" && IsOnline()){
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(Rest().GetAuth, 180 * 1000, false, guid);
		} 
		if (!m_IsServer && !IsOnline()){
			UApi().api().Status(this, "CBStatusCheck");
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.AuthError, 300 * 1000, false, guid);
		}
	}
	
	static void DiscordMessage(string webhookUrl, string message, string botName = "", string botAvatarUrl = ""){
		UApiDiscordObject DiscordMessage = new UApiDiscordObject;
		DiscordMessage.content = message;
		DiscordMessage.username = botName;
		DiscordMessage.avatar_url = botAvatarUrl;
		Post(webhookUrl, DiscordMessage.ToJson());
	}
	
	static void DiscordObject(string webhookUrl, UApiDiscordObject discordObject){
		Post(webhookUrl, discordObject.ToJson());
	} 
	
	
	static string ErrorToString(int ErrorCode){
		switch ( ErrorCode )
		{
			case ERestResultState.EREST_EMPTY:
				return "EREST_EMPTY";
			case ERestResultState.EREST_PENDING:
				return "EREST_PENDING";
			case ERestResultState.EREST_FEEDING:
				return "EREST_FEEDING";
			case ERestResultState.EREST_SUCCESS:
				return "EREST_SUCCESS";
			case ERestResultState.EREST_ERROR:
				return "EREST_ERROR";
			case ERestResultState.EREST_ERROR_CLIENTERROR:
				return "EREST_ERROR_CLIENTERROR";
			case ERestResultState.EREST_ERROR_SERVERERROR:
				return "EREST_ERROR_SERVERERROR";
			case ERestResultState.EREST_ERROR_APPERROR:
				return "EREST_ERROR_APPERROR";
			case ERestResultState.EREST_ERROR_TIMEOUT:
				return "EREST_ERROR_TIMEOUT";
			case ERestResultState.EREST_ERROR_NOTIMPLEMENTED:
				return "EREST_ERROR_NOTIMPLEMENTED";
			case ERestResultState.EREST_ERROR_UNKNOWN:
				return "EREST_ERROR_UNKNOWN";
		}
		return "UNDEFINED_ERROR";
	}
	
	int CallId(){
		return ++m_CallId;
	}
	
	
	bool IsCallCanceled(int cid){
		return (m_CanceledCalls.Find(cid) != -1);
	}
	
	protected void GetQRandomNumbers(){
		if (LastRandomNumberRequestCall != -1){
			return;
		}
		LastRandomNumberRequestCall = api().RandomNumbersFull(-1, this, "CBRandomNumber");
	}
	
	protected void CBRandomNumber(int cid, int status, string oid, UApiRandomNumberResponse data){
		LastRandomNumberRequestCall = -1;
		if (status == UAPI_SUCCESS && data){
			Math.AddQRandomNumber(data.Numbers);
			Math.Randomize(Math.QRandom()); //Randomize the Vanilla Randomization a bit more.
			return;
		}
		Print("[UAPI] Failed to update the Q Random Numbers");
	}
	
	protected void CBStatusCheck(int cid, int status, string oid, UApiStatus data){
		if (status == UAPI_SUCCESS && data){
			if (data.Error == "noerror"){
				m_UApiOnline = true;
				Print("[UAPI] WebService Online Version: " + data.Version + " Mod Version: " + UAPI_VERSION);
			}
			if (data.Error == "noauth"){
				m_UApiOnline = false;
				Print("[UAPI] Auth Key is not vaild");
			}
			if (data.Error == "noerror" && data.Discord == "Enabled"){
				m_UApiDiscordEnabled = true;
			}
			if (data.Error == "noerror" && data.Translate == "Enabled"){
				m_UApiTranslateEnabled = true;
			}
			m_UApiVersionOffset = data.CheckVersion(UAPI_VERSION);
			if (m_UApiVersionOffset > 2){
				Error2("Universal API WebService Needs Update", "[UAPI] Webservice is outdated and should be updated right away | WebService Version: " + data.Version + " Mod Version: " + UAPI_VERSION);
				return;
			}
			if (m_UApiVersionOffset > 1){
				Error("[UAPI] Webservice is outdated and should be updated right away");
				return;
			}
			if (m_UApiVersionOffset > 0){
				Print("[UAPI] You may want to check for new versions of the Universal API WebService");
				return;
			}
			if (m_UApiVersionOffset < -2){
				Error2("Universal API Mod Needs Update", "[UAPI] Universal Api Mod is outdated and should be updated right away | WebService Version: " + data.Version + " Mod Version: " + UAPI_VERSION);
				return;
			}
			if (m_UApiVersionOffset < -1){
				Print("[UAPI] Universal Api Mod maybe outdated and should be updated right away");
				return;
			}					
			return;
		} else if (status == UAPI_ERROR){
			Error2("UnviersalApi", "[UAPI] Something went wrong communicating with the webservice check to make sure it is installed correctly and the mongodb service is running correctly! URL: " + UApiConfig().GetBaseURL());
			m_UApiOnline = false;
		}  else if (status == UAPI_TIMEOUT){
			Error2("UnviersalApi", "[UAPI] Webservice is offline or unreachable! URL: " + UApiConfig().GetBaseURL());
			m_UApiOnline = false;
		} else {
			Error2("UnviersalApi", "[UAPI] Error with WebService! Status: " + status + " URL: " + UApiConfig().GetBaseURL());
			m_UApiOnline = false;
		}
	}
	
};

static ref UniversalApi g_UniversalApi;

static UniversalApi UApi()
{
	if ( !g_UniversalApi )
	{
		g_UniversalApi = new UniversalApi;
		g_UniversalApi.Init();
	}

	return g_UniversalApi;
};
