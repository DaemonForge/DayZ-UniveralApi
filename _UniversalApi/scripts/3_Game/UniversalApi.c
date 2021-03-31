class UniversalApi{

	protected int m_CallId = 0;
	
	protected bool UAPI_Init = false;
	protected ref ApiAuthToken m_authToken;
	
	protected ref UniversalRest m_UniversalRest;
	
	protected ref UniversalDiscordRest m_UniversalDiscordRest;
	protected ref UniversalDSEndpoint m_UniversalDSEndpoint;
	
	protected ref UApiDiscordUser dsUser;
	
	protected ref array<ref PlayerIdentity> QueuedPlayers = new array<ref PlayerIdentity>;
	
	protected ref UApiDBEndpoint m_PlayerEndPoint;
	
	protected ref UApiDBEndpoint m_ObjectEndPoint;
	//Can't Do Globals due to how globals work
	
	
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
	
	ref UApiDBEndpoint db(int collection = OBJECT_DB){
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
		
	ref UniversalRest Rest(){
		if (!m_UniversalRest){
			m_UniversalRest = new UniversalRest;
		}
		return m_UniversalRest;
	}
	
	ref UniversalDiscordRest Discord(){
		if (!m_UniversalDiscordRest){
			m_UniversalDiscordRest = new UniversalDiscordRest;
		}
		return m_UniversalDiscordRest;
	}
	
	ref UniversalDSEndpoint DS(){
		if (!m_UniversalDSEndpoint){
			m_UniversalDSEndpoint = new UniversalDSEndpoint;
		}
		return m_UniversalDSEndpoint;
	}
	
	void Init(){
		if (!UAPI_Init){
			UAPI_Init = true;
			Print("[UPAI] UAPIRPCRegistrations");
			GetRPCManager().AddRPC( "UAPI", "RPCUniversalApiConfig", this, SingeplayerExecutionType.Both );
			GetRPCManager().AddRPC( "UAPI", "RPCRequestQnAConfig", this, SingeplayerExecutionType.Both );
			GetRPCManager().AddRPC( "UAPI", "RPCRequestAuthToken", this, SingeplayerExecutionType.Both );
		}
	}
	
	void RPCUniversalApiConfig( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
		Param2<ApiAuthToken, UniversalApiConfig> data; 
		if ( !ctx.Read( data ) ) return;
		m_authToken = data.param1;
		m_UniversalApiConfig = data.param2;
		if (m_UniversalApiConfig.QnAEnabled){
			GetRPCManager().SendRPC("UAPI", "RPCRequestQnAConfig", new Param1<UApiQnAMakerServerAnswers>(NULL), true);
		}
		if (m_UniversalApiConfig.PromptDiscordOnConnect >= 1){
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Call(this.CheckAndPromptDiscordThread);
			//Should run on thread but give access violation not sure why yet
		}
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
		PlayerIdentity idenitity = PlayerIdentity.Cast(sender);
		if (GetGame().IsServer() && idenitity){
			AddToQueue(idenitity);
			UApiConfig();
			if (UApiConfig().ServerAuth != "" && UApiConfig().ServerAuth != "null"){
				Rest().GetAuth(idenitity.GetId());
			}
		}
	}
	
	static DayZPlayer FindPlayer(string GUID){
		if (GetGame().IsServer()){
			ref array<Man> players = new array<Man>;
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
	ref PlayerIdentity SearchQueue(string GUID){
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
		if (PlayerIdentity.Cast(SearchQueue(guid))){ //Check to make sure the GUID is still in the queue to prevent any endless loops
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(Rest().GetAuth, 180 * 1000, false, guid);
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
	
	
	//This is just something I'm playing with maybe helpfull
	void QnA(string question, bool alwaysAnswer = true, ref RestCallback UCBX = NULL, string jsonString = "{}", string auth = ""){
		
		if (!UCBX && alwaysAnswer){
			ref UApiQnACallBack QnACBX = new UApiQnACallBack;
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
};

static ref UniversalApi g_UniversalApi;

static ref UniversalApi UApi()
{
	if ( !g_UniversalApi )
	{
		Print("[UPAI] Init");
		g_UniversalApi = new UniversalApi;
		g_UniversalApi.Init();
	}

	return g_UniversalApi;
};
