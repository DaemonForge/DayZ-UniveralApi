class UniversalApi{

	protected bool UAPI_Init = false;
	protected ref ApiAuthToken m_authToken;
	
	protected string m_ReadAuthToken;
	
	protected ref UniversalRest m_UniversalRest;
	
	protected ref array<ref PlayerIdentity> QueuedPlayers = new ref array<ref PlayerIdentity>;
	
	string GetAuthToken(){
		if (m_authToken && !GetGame().IsServer()){
			return m_authToken.GetAuthToken();
		} else if (GetGame().IsServer() && UApiConfig().ServerAuth != ""){
			return UApiConfig().ServerAuth;
		}
		return "null";
	}
	
	string GetReadAuth(){
		return m_ReadAuthToken;
	}
	
	void SetReadAuth(string readAuthToken){
		m_ReadAuthToken  = readAuthToken;
	}
	
	ref UniversalRest Request(){
		if (!m_UniversalRest){
			m_UniversalRest = new ref UniversalRest;
		}
		return m_UniversalRest;
	}
	
	void Init(){
		if (!UAPI_Init){
			UAPI_Init = true;
			Print("[UPAI] UAPIRPCRegistrations");
			GetRPCManager().AddRPC( "UAPI", "RPCUniversalApiConfig", this, SingeplayerExecutionType.Both );
			GetRPCManager().AddRPC( "UAPI", "RPCRequestAuthToken", this, SingeplayerExecutionType.Both );
		}
	}
	
	void RPCUniversalApiConfig( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
		Param2<ApiAuthToken, UniversalApiConfig> data;  //Player ID, Icon
		if ( !ctx.Read( data ) ) return;
		m_authToken = data.param1;
		Print("[UPAI] Received Auth Token: GUID" + m_authToken.GUID + " AUTH" + m_authToken.AUTH);
		m_UniversalApiConfig = data.param2;
		SetReadAuth(m_UniversalApiConfig.ReadAuth);
	}
	
	void RPCRequestAuthToken( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
		PlayerIdentity idenitity = PlayerIdentity.Cast(sender);
		if (GetGame().IsServer() && idenitity){
			AddToQueue(idenitity);
			UApiConfig();
			if (UApiConfig().ServerAuth != "" && UApiConfig().ServerAuth != "null"){
				Request().GetAuth( UApiConfig().ServerAuth, idenitity.GetId() );
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
					Print("[UPAI] GUID Match: " + GUID + " to " + player.GetIdentity().GetId());
					return player;
				}
			}
		}
		return NULL;
	}
	
	ref PlayerIdentity SearchQueue(string GUID){
		if (GetGame().IsServer()){
			for (int i = 0; i < QueuedPlayers.Count(); i++){
				PlayerIdentity indentity = PlayerIdentity.Cast(QueuedPlayers.Get(i));
				if (indentity && indentity.GetId() == GUID ){
					Print("[UPAI] GUID Match: " + GUID + " to " + indentity.GetId());
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
	
	void AuthError(){
		Print("[UPAI] Auth Error");
	}
	
};

static ref UniversalApi g_UniversalApi;

static ref UniversalApi UApi()
{
	if ( !g_UniversalApi )
	{
		Print("[UPAI] Init");
		g_UniversalApi = new ref UniversalApi;
		g_UniversalApi.Init();
	}

	return g_UniversalApi;
};
