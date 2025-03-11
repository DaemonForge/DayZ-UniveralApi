class UFramework extends Managed {
		
	//Getter function for the Database Endpoint using either OBJECT_DB or PLAYER_DB
	// "PLAYER_DB" is only accessable on client for the player info being requested 
	// "OBJECT_DB" all clients can access all data.
	UDBEndpoint db(int collection = OBJECT_DB){
		if (collection == OBJECT_DB){
			if (!m_ObjectEndPoint){
				m_ObjectEndPoint = new UDBEndpoint("Object");
			}
			return m_ObjectEndPoint;
		} 
		if (collection == PLAYER_DB){
			if (!m_PlayerEndPoint){
				m_PlayerEndPoint = new UDBEndpoint("Player");
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
	UDBGlobalEndpoint globals(){
		if (!m_UDBGlobalEndpoint){
			m_UDBGlobalEndpoint = new UDBGlobalEndpoint;
		}
		return m_UDBGlobalEndpoint;
	}
	
	//Getter function for the API Endpoint
	UApiEndpoint api(){
		if (!m_UApiEndpoint){
			m_UApiEndpoint = new UApiEndpoint;
		}
		return m_UApiEndpoint;
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
		ctx.POST(new USilentCallBack, "", "{}");
		return 0;
	}
	
	//A super simple Post Interface to help people
	static int Post(string url, string jsonString, RestCallback UCBX = NULL, string contentType = "application/json")
	{
		autoptr RestCallback vUCBX = UCBX;
		if (!vUCBX){
			vUCBX = new USilentCallBack;
		}
		RestContext ctx = RestCore().GetRestContext(url);
		ctx.SetHeader(contentType);
		ctx.POST(vUCBX, "", jsonString);
		return 0;
	}
	
	//A super simple Post Interface to help people
	static int Post(string url, string jsonString, UFCallbackBase cb, string contentType = "application/json")
	{
		int cid = U().CallId();
		if (cb){
			RestContext ctx = RestCore().GetRestContext(url);
			ctx.SetHeader(contentType);
			ctx.POST(new UDBNestedCallBack(cb,cid), "", jsonString);
			return cid;
		}
		return -1;
	}
	
	//A super simple Get Interface to help people
	static int Get(string url)
	{
		RestContext ctx =  RestCore().GetRestContext(url);
		ctx.GET(new USilentCallBack, "");
		return 0;
	}
	//A super simple Get Interface to help people
	static int Get(string url, RestCallback UCBX)
	{
		autoptr RestCallback vUCBX = UCBX;
		if (!vUCBX){
			vUCBX = new USilentCallBack;
		}
		RestContext ctx =  RestCore().GetRestContext(url);
		ctx.GET(vUCBX , "");
		return 0;
	}
	//A super simple Get Interface to help people
	static int Get(string url, UFCallbackBase cb)
	{
		int cid = U().CallId();
		if (cb){
			RestContext ctx =  RestCore().GetRestContext(url);
			ctx.GET(new UDBNestedCallBack(cb,cid), "");
			return cid;
		}
		return -1;
	}
	
	//Will return true if the discord endpoint is configured (this doesn't mean its configured correctly though :p)
	bool IsDiscordEnabled(){
		return m_UDiscordEnabled;
	}
	
	//Returns True if the status check has come back and everything is okay
	bool IsOnline(){
		return m_UFOnline;
	}
	
	//Returns current Version Offset 0 Version Matches exactly
	// -1 or 1 off by a patch this is not a problem and won't cause any major issues
	// -2 or 2 off by a Minor Version this may cause some endpoints to not work or features to be missing
	// -3 or 3 off by a Major Version most likely the mod will not work at all!
	int VersionOffset(){
		return m_UFVersionOffset;
	}
	
	//Checks to see if the Random Numbers are below half and add's more
	void CheckAndRenewQRandom(){
		if (Math.QRandomRemaining()<= 2000){
			GetQRandomNumbers();
		}
	}
	
	//Returns Current Version of the Mod
	static string GetVersion(){
		return UF_VERSION;
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
	
	protected bool m_UFOnline = false;
	protected int m_UFVersionOffset = 0;
	protected bool m_UDiscordEnabled = false;
	
	protected bool UF_Init = false;
	protected autoptr ApiAuthToken m_authToken;
	
	protected autoptr UniversalRest m_UniversalRest;
	
	protected autoptr UniversalDiscordRest m_UniversalDiscordRest;
	protected autoptr UniversalDSEndpoint m_UniversalDSEndpoint;
	protected autoptr UDBGlobalEndpoint m_UDBGlobalEndpoint;
	
	protected autoptr UDiscordUser dsUser;
		
	protected autoptr map<string, string> PlayerAuths = new map<string, string>;
	
	protected autoptr UDBEndpoint m_PlayerEndPoint;
	
	protected autoptr UDBEndpoint m_ObjectEndPoint;
	//Can't Do Globals due to how globals work
	
	protected autoptr UApiEndpoint m_UApiEndpoint;
	
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
		} else if (GetGame().IsServer() && UFConfig().ServerAuth != ""){
			return UFConfig().ServerAuth;
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
	
	
	
	void ~UFramework(){
		if (m_IsServer && UF_Init && GetGame()){
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(this.CheckAndRenewQRandom);
		}
	}
	
	void Init(){
		#ifdef NO_GUI
			Print("[UF] Detected Server");
			m_IsServer = true;
		#endif
		if (!UF_Init){
			Print("[UF] First Init");
			UF_Init = true;
			GetRPCManager().AddRPC( "UF", "RPCUFrameworkConfig", this, SingeplayerExecutionType.Both );
			GetRPCManager().AddRPC( "UF", "RPCRequestAuthToken", this, SingeplayerExecutionType.Both );
			GetRPCManager().AddRPC( "UF", "RPCRequestRetry", this, SingeplayerExecutionType.Both );
			if (m_IsServer){
				U().api().Status(this, "CBStatusCheck");
				CheckAndRenewQRandom();
				GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.CheckAndRenewQRandom, 10 * 60 * 1000, true);
			}
		}
	}
	
	protected void RPCUFrameworkConfig( CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target )
	{
		Print("[UF] Received UF Config");
		Param2<ApiAuthToken, UFrameworkConfig> data; 
		if ( !ctx.Read( data ) ) return;
		m_AuthRetries = 0;
		m_authToken = data.param1;
		m_UFrameworkConfig = data.param2;
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Call(this.OnTokenReceived);
	}
	
	protected void OnTokenReceived(){
		U().api().Status(this, "CBStatusCheck");
		U().ds().GetUser(GetDayZGame().GetSteamId(), GetDayZGame(), "CBCacheDiscordInfo");
		GetGame().GameScript.CallFunction(GetGame().GetMission(), "UFrameworkReadyTokenReceived", NULL, NULL);
		CheckAndRenewQRandom();
		Print("[UF] OnTokenReceived Proccessed");
	}
	
	
	void RPCRequestRetry( CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target ) {
		if (GetGame().IsClient() && ++m_AuthRetries <= 20){
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.RequestAuthToken, m_AuthRetries * 2200, false, true);
		}
	}
	
	void RequestAuthToken(bool first = false){
		if (!m_IsServer){
			GetRPCManager().SendRPC("UF", "RPCRequestAuthToken", new Param1<bool>(first), true);
		}
	}
	
	void PreparePlayerAuth(string guid){
		this.Rest().GetAuth(guid);
	}
	
	void AddPlayerAuth(string guid, string auth){
		if (!PlayerAuths){PlayerAuths = new map<string, string>;}
		//Print("[UF] Adding PlayerAuth for " + guid + " to cache");
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
		Print("[UF] Failed to find Player Auth for " + guid);
		return false;
	}		
		
	protected void RPCRequestAuthToken( CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target )
	{
		Param1<bool> data; 
		if ( !ctx.Read( data ) ) return;
		PlayerIdentity identity = PlayerIdentity.Cast(sender);
		if (m_IsServer && identity){
			UFConfig();
			string authtoken = "";
			if (UFConfig().ServerAuth != "" && UFConfig().ServerAuth != "null" ){
				if (data.param1 && GetPlayerAuth(identity.GetId(), authtoken)){
					//Print("[UF] RPCRequestAuthToken Sending Cached Token ");
					SendAuthToken(identity, authtoken);
				} else if (FindPlayer(identity.GetId())){
					//Print("[UF] RPCRequestAuthToken  Renewing Auth Token" );
					PreparePlayerAuth(identity.GetId());
				}  else {
					Print("[UF] RPCRequestAuthToken Requesting client retry." );
					GetRPCManager().SendRPC("UF", "RPCRequestRetry", new Param1<bool>(true), true, identity);
				}
			} else if (UFConfig().ServerAuth && UFConfig().ServerAuth != "" && UFConfig().ServerAuth != "null") {
				Error("[UF] Server Auth is empty or null");
			}
		}
	}
	
	protected void SendAuthToken(PlayerIdentity idenitity, string auth){
		if (idenitity && auth != ""){
			Print("[UF] Sending PlayerAuth Token to " + idenitity.GetId());
			autoptr UFrameworkConfig m_ClientConfig = new UFrameworkConfig;
			m_ClientConfig.ConfigVersion = UFConfig().ConfigVersion;
			m_ClientConfig.ServerURL = UFConfig().ServerURL;
			m_ClientConfig.ServerID = UFConfig().ServerID;
			m_ClientConfig.ServerAuth = "null";
			m_ClientConfig.EnableBuiltinLogging = UFConfig().EnableBuiltinLogging;
			m_ClientConfig.PromptDiscordOnConnect = UFConfig().PromptDiscordOnConnect;
			autoptr ApiAuthToken m_authToken = new ApiAuthToken;
			m_authToken.GUID = idenitity.GetId();
			m_authToken.AUTH = auth;
			GetRPCManager().SendRPC("UF", "RPCUFrameworkConfig", new Param2<ApiAuthToken, UFrameworkConfig>(m_authToken, m_ClientConfig), true, idenitity);
		} else {
			Print("[UF] [UAuthCallBack] ERROR ");
			if (idenitity){
				U().AuthError(idenitity.GetId());
			}
		}
	}
	
	void AuthError(string guid){
		Print("[UF] Auth Error for " + guid);
		//If Auth Token Failed just try again in 3 minutes 
		if (guid != "" && IsOnline()){
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(Rest().GetAuth, 180 * 1000, false, guid);
		} 
		if (!m_IsServer && !IsOnline()){
			U().api().Status(this, "CBStatusCheck");
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.AuthError, 300 * 1000, false, guid);
		}
	}
	
	static void DiscordMessage(string webhookUrl, string message, string botName = "", string botAvatarUrl = ""){
		UDiscordObject DiscordMessage = new UDiscordObject;
		DiscordMessage.content = message;
		DiscordMessage.username = botName;
		DiscordMessage.avatar_url = botAvatarUrl;
		Post(webhookUrl, DiscordMessage.ToJson());
	}
	
	static void DiscordObject(string webhookUrl, UDiscordObject discordObject){
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
		LastRandomNumberRequestCall = api().RandomNumbers(-1, this, "CBRandomNumber");
	}
	
	protected void CBRandomNumber(int cid, int status, string oid, URandomNumberResponse data){
		LastRandomNumberRequestCall = -1;
		if (status == UF_SUCCESS && data){
			Math.AddQRandomNumber(data.Numbers);
			Math.Randomize(Math.QRandom()); //Randomize the Vanilla Randomization a bit more.
			return;
		}
		Print("[UF] Failed to update the Q Random Numbers");
	}
	
	protected void CBStatusCheck(int cid, int status, string oid, UFStatus data){
		if (status == UF_SUCCESS && data){
			if (data.Error == "noerror"){
				m_UFOnline = true;
				Print("[UF] WebService Online Version: " + data.Version + " Mod Version: " + UF_VERSION);
			}
			if (data.Error == "noauth"){
				m_UFOnline = false;
				Print("[UF] Auth Key is not vaild");
			}
			if (data.Error == "noerror" && data.Discord == "Enabled"){
				m_UDiscordEnabled = true;
			}
			m_UFVersionOffset = data.CheckVersion(UF_VERSION);
			if (m_UFVersionOffset > 2){
				Error2("Universal Framework WebService Needs Update", "[UF] Webservice is outdated and should be updated right away | WebService Version: " + data.Version + " Mod Version: " + UF_VERSION);
				return;
			}
			if (m_UFVersionOffset > 1){
				Error("[UF] Webservice is outdated and should be updated right away");
				return;
			}
			if (m_UFVersionOffset > 0){
				Print("[UF] You may want to check for new versions of the Universal Framework WebService");
				return;
			}
			if (m_UFVersionOffset < -2){
				Error2("Universal Framework Mod Needs Update", "[UF] Universal Framework Mod is outdated and should be updated right away | WebService Version: " + data.Version + " Mod Version: " + UF_VERSION);
				return;
			}
			if (m_UFVersionOffset < -1){
				Print("[UF] Universal Framework Mod maybe outdated and should be updated right away");
				return;
			}					
			return;
		} else if (status == UF_ERROR){
			Error2("UnviersalApi", "[UF] Something went wrong communicating with the webservice check to make sure it is installed correctly and the mongodb service is running correctly! URL: " + UFConfig().GetBaseURL());
			m_UFOnline = false;
		}  else if (status == UF_TIMEOUT){
			Error2("UnviersalApi", "[UF] Webservice is offline or unreachable! URL: " + UFConfig().GetBaseURL());
			m_UFOnline = false;
		} else {
			Error2("UnviersalApi", "[UF] Error with WebService! Status: " + status + " URL: " + UFConfig().GetBaseURL());
			m_UFOnline = false;
		}
	}
	
};

static ref UFramework g_UFramework;

static UFramework U()
{
	if ( !g_UFramework )
	{
		g_UFramework = new UFramework;
		g_UFramework.Init();
	}

	return g_UFramework;
};
