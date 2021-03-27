class UniversalDSEndpoint
{	
	
	static protected string m_BaseUrl = "";
	
	protected RestContext m_Context;
	
	protected string m_Collection = "Discord";
	
	
	void UniversalDiscordEndpoint(string collection){
		m_Collection = collection;
	}
	
	static void SetBaseUrl(string new_BaseUrl){
		m_BaseUrl = new_BaseUrl;
	}
	
	static string BaseUrl(){
		if (m_BaseUrl != ""){
			return m_BaseUrl;
		}
		return UApiConfig().ServerURL;
	}

	RestContext Api()
	{
		if (!m_Context){
			RestApi clCore = GetRestApi();
			if (!clCore)
			{
				clCore = CreateRestApi();
				clCore.SetOption(ERestOption.ERESTOPTION_READOPERATION, 15);
			}
			string url = BaseUrl() + m_Collection;
			m_Context =  clCore.GetRestContext(url);
			m_Context.SetHeader("application/json");
		}
		return m_Context;
	}
	
	void Post(string endpoint, string jsonString, RestCallback UCBX)
	{
		string route = endpoint + "/" + UApi().GetAuthToken();
		Api().POST(UCBX, route, jsonString);
	}
	
	static string Link(string PlainId = ""){
		if (PlainId == "" && GetGame().IsClient()){
			DayZPlayer player = DayZPlayer.Cast(GetGame().GetPlayer());
			if ( player && player.GetIdentity() ){
				return BaseUrl() + "Discord/" + player.GetIdentity().GetPlainId();
			}
			if (GetGame().GetUserManager() && GetGame().GetUserManager().GetTitleInitiator()){
				return BaseUrl() + "Discord/" + GetGame().GetUserManager().GetTitleInitiator().GetUid();
			}
		}
		return BaseUrl() + "Discord/" + PlainId;
	}
		
	int AddRole(string GUID, string RoleId, Class instance = NULL, string function = "") {
		int cid = UApi().CallId();
		ref RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDBCallBack(instance, function, cid, GUID);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
				
		string url = "/AddRole/" + GUID;
		
		autoptr UApiDiscordRoleReq roleReq = new UApiDiscordRoleReq(RoleId);
		
		string jsonString = roleReq.ToJson();
		if (jsonString){
			Post(url,jsonString,DBCBX);
		} else {
			Print("[UPAI] [Discord] Error Adding Role (" + RoleId + ") To " + GUID);
			cid = -1;
		}
		return cid;
	}
	
	int RemoveRole(string GUID, string RoleId, Class instance = NULL, string function = "") {
		int cid = UApi().CallId();
		ref RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDBCallBack(instance, function, cid, GUID);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		string url = "/RemoveRole/" + GUID;
		
		autoptr UApiDiscordRoleReq roleReq = new UApiDiscordRoleReq(RoleId);
		
		string jsonString = roleReq.ToJson();
		
		if (jsonString){
			Post(url,jsonString,DBCBX);
		} else {
			Print("[UPAI] [Discord] Error Removing Role (" + RoleId + ") To " + GUID);
			cid = -1;
		}
		return cid;
	}
	
	
	int GetUser(string GUID, Class instance, string function) {
		int cid = UApi().CallId();
		ref RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDBCallBack(instance, function, cid, GUID);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		string url = "/Get/" + GUID;
		
		Post(url,"{}",DBCBX);
		return cid;
	}
	
	int GetUserWithPlainId(string plainId, Class instance, string function) {
		int cid = UApi().CallId();
		ref RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDBCallBack(instance, function, cid, plainId);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		string url = "/GetWithPlainId/" + plainId;
		if (plainId && plainId != ""){
			Post(url,"{}",DBCBX);
		}
		return cid;
	}
	
	int GetUserObj(string GUID, Class instance, string function) {
		int cid = UApi().CallId();
		ref RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDSCallBack(instance, function, cid, GUID);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		string url = "/Get/" + GUID;
		
		Post(url,"{}",DBCBX);
		return cid;
	}
	
	int GetUserObjWithPlainId(string plainId, Class instance, string function) {
		int cid = UApi().CallId();
		ref RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDSCallBack(instance, function, cid, plainId);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		string url = "/GetWithPlainId/" + plainId;
		if (plainId && plainId != ""){
			Post(url,"{}",DBCBX);
		}
		return cid;
	}
}