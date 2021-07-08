class UniversalApi extends Managed {

	protected int m_CallId = 0;
	protected int m_AuthRetries = 0;
	
	protected bool UAPI_Init = false;
	protected autoptr ApiAuthToken m_authToken;
	
	protected autoptr UniversalRest m_UniversalRest;
	
	protected autoptr UniversalDiscordRest m_UniversalDiscordRest;
	protected autoptr UniversalDSEndpoint m_UniversalDSEndpoint;
	protected autoptr UApiDBGlobalEndpoint m_UApiDBGlobalEndpoint;
	
	protected autoptr UApiDiscordUser dsUser;
	
	protected autoptr array<ref PlayerIdentity> QueuedPlayers = new array<ref PlayerIdentity>;
	
	protected autoptr map<string, string> PlayerAuths = new map<string, string>;
	
	protected autoptr UApiDBEndpoint m_PlayerEndPoint;
	
	protected autoptr UApiDBEndpoint m_ObjectEndPoint;
	//Can't Do Globals due to how globals work
	
	protected autoptr UApiAPIEndpoint m_UApiAPIEndpoint;
	
	protected autoptr TIntSet m_CanceledCalls = new TIntSet;
	
	protected autoptr TIntArray m_RandomNumbers = new TIntArray;
	
	protected int LastRandomNumberRequestCall = -1;
	
	
	string GetAuthToken(){
		if (m_authToken && !GetGame().IsServer()){
			return m_authToken.GetAuthToken();
		} else if (GetGame().IsServer() && UApiConfig().ServerAuth != ""){
			return UApiConfig().ServerAuth;
		}
		return "null";
	}
	
	bool HasValidAuth(){
		if (GetAuthToken() != "null" && GetAuthToken() != "error" && GetAuthToken() != "ERROR" && GetAuthToken() != "" ){
			return true;
		}
		return false;
	}
	
	static string GetVersion(){
		return UAPI_VERSION;
	}
	
	UApiDBEndpoint db(int collection = OBJECT_DB){
		if (collection == OBJECT_DB){
			if (!m_ObjectEndPoint){
				m_ObjectEndPoint = new UApiDBEndpoint("Object");
			}
			return m_ObjectEndPoint;
		} else if (collection == PLAYER_DB){
			if (!m_PlayerEndPoint){
				m_PlayerEndPoint = new UApiDBEndpoint("Player");
			}
			return m_PlayerEndPoint;
		}
		return NULL;
	}
		
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
	
	UniversalDSEndpoint DS(){ // will remove
		return ds();
	}
	
	UniversalDSEndpoint ds(){
		if (!m_UniversalDSEndpoint){
			m_UniversalDSEndpoint = new UniversalDSEndpoint;
		}
		return m_UniversalDSEndpoint;
	}
	
	UApiDBGlobalEndpoint globals(){
		if (!m_UApiDBGlobalEndpoint){
			m_UApiDBGlobalEndpoint = new UApiDBGlobalEndpoint;
		}
		return m_UApiDBGlobalEndpoint;
	}
	
	UApiAPIEndpoint api(){
		if (!m_UApiAPIEndpoint){
			m_UApiAPIEndpoint = new UApiAPIEndpoint;
		}
		return m_UApiAPIEndpoint;
	
	}
	
	void Init(){
		if (!UAPI_Init){
			UAPI_Init = true;
			Print("[UPAI] UAPIRPCRegistrations");
			GetRPCManager().AddRPC( "UAPI", "RPCUniversalApiConfig", this, SingeplayerExecutionType.Both );
			GetRPCManager().AddRPC( "UAPI", "RPCRequestQnAConfig", this, SingeplayerExecutionType.Both );
			GetRPCManager().AddRPC( "UAPI", "RPCRequestAuthToken", this, SingeplayerExecutionType.Both );
			GetRPCManager().AddRPC( "UAPI", "RPCRequestRetry", this, SingeplayerExecutionType.Both );
		}
	}
	
	void RPCUniversalApiConfig( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
		Print("[UAPI] Received UApi Config");
		Param2<ApiAuthToken, UniversalApiConfig> data; 
		if ( !ctx.Read( data ) ) return;
		m_AuthRetries = 0;
		m_authToken = data.param1;
		m_UniversalApiConfig = data.param2;
		if (m_UniversalApiConfig.QnAEnabled){
			GetRPCManager().SendRPC("UAPI", "RPCRequestQnAConfig", new Param1<UApiQnAMakerServerAnswers>(NULL), true);
		}
		if (m_UniversalApiConfig.PromptDiscordOnConnect >= 1){
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Call(this.CheckAndPromptDiscordThread);
			//Should run on thread but give access violation not sure why yet
		}
		GetGame().GameScript.CallFunction(GetGame().GetMission(), "UniversalApiReadyTokenReceived", NULL, NULL);
		Print("[UAPI] Proccessed UApi Config");
	}
	
	void RPCRequestRetry( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
		if (GetGame().IsClient() && ++m_AuthRetries <= 20){
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.RequestAuthToken, m_AuthRetries * 2200, false, true);
		}
	}
	
	void RequestAuthToken(bool first = false){
		if (!GetGame().IsServer()){
			GetRPCManager().SendRPC("UAPI", "RPCRequestAuthToken", new Param1<bool>(first), true);
		}
	}
	
	void PreparePlayerAuth(string guid){
		UApi().Rest().GetAuthNew(guid);
	}
	
	void AddPlayerAuth(string guid, string auth){
		if (!PlayerAuths){PlayerAuths = new map<string, string>;}
		Print("[UAPI] Adding PlayerAuth for " + guid + " to cache");
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
	
	
	void CheckAndPromptDiscordThread(){
		thread CheckAndPromptDiscord();
	}
	
	
	protected void CheckAndPromptDiscord(){
		if (GetGame().GetUserManager() && GetGame().GetUserManager().GetTitleInitiator()){
			Print("[UPAI] PromtDiscordOnConnect enbabled Requesting Discord Info");
			dsUser = UApi().Discord().GetUserWithPlainIdNow(GetGame().GetUserManager().GetTitleInitiator().GetUid(), true);
			if (dsUser && dsUser.Status == "Success"){
				Print("[UPAI] PromtDiscordOnConnect Already Connected Continue");
			} else if (dsUser && dsUser.Status == "NotSetup"){
				GetGame().OpenURL(UApi().Discord().Link());
			} else if (dsUser){
				Print("[UPAI] PromtDiscordOnConnect  " + dsUser.Status + " Error Occured - " + dsUser.Error);
			} else {
				Print("[UPAI] PromtDiscordOnConnect dsUser is Null");
			}
		} else {
			Print("[UPAI] PromtDiscordOnConnect enbabled but Player/Id is null");
		}
	}
	
	
	void RPCRequestQnAConfig( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
		Param1<UApiQnAMakerServerAnswers> data; 
		if ( !ctx.Read( data ) ) return;
		if (!GetGame().IsServer() && !PlayerIdentity.Cast(sender) && data.param1){
			m_QnAMakerServerAnswers = data.param1;
		} else if (GetGame().IsServer() && PlayerIdentity.Cast(sender) && data.param1 == NULL){
			GetRPCManager().SendRPC("UAPI", "RPCRequestQnAConfig", new Param1<UApiQnAMakerServerAnswers>(m_QnAMakerServerAnswers), true, sender);
		}
	}
	
	void RPCRequestAuthToken( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
		Param1<bool> data; 
		if ( !ctx.Read( data ) ) return;
		PlayerIdentity identity = PlayerIdentity.Cast(sender);
		if (GetGame().IsServer() && identity){
			UApiConfig();
			string authtoken = "";
			if (UApiConfig().ServerAuth != "" && UApiConfig().ServerAuth != "null" ){
				if (data.param1 && GetPlayerAuth(identity.GetId(), authtoken)){
					Print("[UAPI] RPCRequestAuthToken Sending Cached Token ");
					SendAuthToken(identity, authtoken);
				} else if (FindPlayer(identity.GetId())){
					Print("[UAPI] RPCRequestAuthToken  Renewing Auth Token" );
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
	
	void SendAuthToken(ref PlayerIdentity idenitity, string auth){
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
			Print("[UPAI] [UApiAuthCallBack] ERROR ");
			if (idenitity){
				UApi().AuthError(idenitity.GetId());
			}
		}
	}
	
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
	
	
	//This is so I can get the Identity Quicker as the GetPlayers Method doesn't get the player ID till later
	PlayerIdentity SearchQueue(string GUID){
		if (GetGame().IsServer()){
			for (int i = 0; i < QueuedPlayers.Count(); i++){
				PlayerIdentity indentity = PlayerIdentity.Cast(QueuedPlayers.Get(i));
				if (indentity && indentity.GetId() == GUID ){
					return indentity;
				}
			}
		}
		return NULL;
	}
	
	
	
	//TODO: Rework to not need queue 
	void AddToQueue(ref PlayerIdentity idenitity){
		if (GetGame().IsServer()){
			if (QueuedPlayers.Find(idenitity) == -1){
				QueuedPlayers.Insert(idenitity);
			}
		}
	}
	
	void RemoveFromQueue(ref PlayerIdentity idenitity){
		if (GetGame().IsServer()){
			if (QueuedPlayers.Find(idenitity) != -1){
				QueuedPlayers.RemoveItem(idenitity);
			}
		}
	}
	
	void AuthError(string guid){
		Print("[UPAI] Auth Error for " + guid);
		//If Auth Token Failed just try again in 3 minutes 
		if (guid != ""){
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(Rest().GetAuthNew, 180 * 1000, false, guid);
		}
	}
	
	void DiscordMessage(string webhookUrl, string message, string botName = "", string botAvatarUrl = ""){
		UApiDiscordObject DiscordMessage = new UApiDiscordObject;
		DiscordMessage.content = message;
		DiscordMessage.username = botName;
		DiscordMessage.avatar_url = botAvatarUrl;
		Rest().Post(webhookUrl, DiscordMessage.ToJson());
	}
	
	void DiscordObject(string webhookUrl, UApiDiscordObject discordObject){
		Rest().Post(webhookUrl, discordObject.ToJson());
	} 
	
	
	////This will be removed and moved to the UApi().api().QnA()
	void QnA(string question, bool alwaysAnswer = true, ref RestCallback UCBX = NULL, string jsonString = "{}", string auth = ""){
		
		if (!UCBX && alwaysAnswer){
			UApiQnACallBack QnACBX = new UApiQnACallBack;
			QnACBX.SetAlwaysAnswer();
			UCBX = QnACBX;
		} else if (!UCBX) {
			UCBX = new UApiQnACallBack;
		}
		
		if (auth == "" ){
			auth = UApi().GetAuthToken();
		}
		if (jsonString == "{}" && question != "" ){
			QnAQuestion QuestionObj = new QnAQuestion(question);
			jsonString = QuestionObj.ToJson();
		}
		string url = Rest().BaseUrl() + "QnAMaker/" + auth;
		
		if (jsonString != "{}" ){
			Rest().Post(url,jsonString,UCBX);
		} else {
			Print("[UPAI] [Api] Error Asking Question ");
		}
	}
	
	
	string ErrorToString(int ErrorCode){
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
	
	void RequestCallCancel(int cid){
		m_CanceledCalls.Insert(cid);
	}
	
	bool IsCallCanceled(int cid){
		return (m_CanceledCalls.Find(cid) != -1);
	}
	
	void PrepareTrueRandom(){
		if (!m_RandomNumbers || m_RandomNumbers.Count() <= 1500){
			GetRandomNumbers();
		}
	}
	
	int rndInt(int min = 0, int max = 65535){
		if (!m_RandomNumbers || m_RandomNumbers.Count() <= 0){
			Print("[UAPI] TRUE RANDOM OUT OF NUMBERS USING VANILLA RANDOM");
			GetRandomNumbers();
			return Math.RandomInt(min, max);
		}
		if (m_RandomNumbers.Count() < 1000){
			GetRandomNumbers();
		}
		int idx = m_RandomNumbers.GetRandomIndex();
		int number = m_RandomNumbers.Get(idx);
		m_RandomNumbers.Remove(idx);
		if (min == 0 && max == 65535){
			return number;
		}
		float num = number / 65535;
		int diff = max - min;
		int dnum = diff * num;
		return  Math.Round( dnum + min);
	}
	
	float rndFloat(float min = 0, float max = 1){
		if (!m_RandomNumbers || m_RandomNumbers.Count() <= 0){
			Print("[UAPI] TRUE RANDOM OUT OF NUMBERS USING VANILLA RANDOM");
			GetRandomNumbers();
			return Math.RandomFloat(min, max);
		}
		if (m_RandomNumbers.Count() < 1000){
			GetRandomNumbers();
		}
		int idx = m_RandomNumbers.GetRandomIndex();
		int number = m_RandomNumbers.Get(idx);
		m_RandomNumbers.Remove(idx);
		float num = number / 65535;
		float diff = max - min;
		float dnum = diff * num;
		return  dnum + min;
	}
	
	bool rndFlip(){
		if (!m_RandomNumbers || m_RandomNumbers.Count() <= 0){
			Print("[UAPI] TRUE RANDOM OUT OF NUMBERS USING VANILLA RANDOM");
			GetRandomNumbers();
			return (Math.RandomInt(1, 8) >= 5);
		}
		if (m_RandomNumbers.Count() < 1000){
			GetRandomNumbers();
		}
		int idx = m_RandomNumbers.GetRandomIndex();
		int number = m_RandomNumbers.Get(idx);
		m_RandomNumbers.Remove(idx);
		int reval = number % 2;
		return (reval != 0);
		
	}
	
	protected void GetRandomNumbers(){
		if (LastRandomNumberRequestCall > 0){
			return;
		}
		LastRandomNumberRequestCall = api().RandomNumbers(2048, this, "ReadRandomNumber");
	}
	
	void ReadRandomNumber(int cid, int status, string oid, string data){
		LastRandomNumberRequestCall = -1;
		if (status == UAPI_SUCCESS){
			UApiRandomNumberResponse dataload;
			if (UApiJSONHandler<UApiRandomNumberResponse>.FromString(data, dataload)){
				if (!m_RandomNumbers){
					m_RandomNumbers = new TIntArray;
				}
				m_RandomNumbers.InsertAll(dataload.Numbers);
				return;
			}
		}
	}
	
};

static ref UniversalApi g_UniversalApi;

static UniversalApi UApi()
{
	if ( !g_UniversalApi )
	{
		Print("[UPAI] Init");
		g_UniversalApi = new UniversalApi;
		g_UniversalApi.Init();
	}

	return g_UniversalApi;
};
