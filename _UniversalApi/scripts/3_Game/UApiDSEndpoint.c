class UniversalDSEndpoint extends Managed
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
	
	
	
	
		
	
	int CheckDiscord(string PlainId, Class instance, string function,  string baseUrl = ""){		
		int cid = UApi().CallId();
		if (baseUrl == ""){
			baseUrl = BaseUrl();
		}
		
		ref RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDBCallBack(instance, function, cid, PlainId);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		string url = baseUrl + "Discord/Check/" + PlainId;
		
		UApi().Rest().Post(url,"{}",DBCBX);
		
		return cid;
	}
	
	int CheckDiscordObj(string PlainId, Class instance, string function,  string baseUrl = ""){		
		int cid = UApi().CallId();
		if (baseUrl == ""){
			baseUrl = BaseUrl();
		}
		
		ref RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDBCallBack(instance, function, cid, PlainId);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		string url = baseUrl + "Discord/Check/" + PlainId;
		
		UApi().Rest().Post(url,"{}",DBCBX);
		
		return cid;
	}
	
	
	
	
	int ChannelCreate(string Name, UApiChannelOptions Options = NULL, Class instance = NULL, string function = "") {
		int cid = UApi().CallId();
		
		
		ref RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDBCallBack(instance, function, cid, Name);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		UApiCreateChannelObject obj = new UApiCreateChannelObject(Name, Options);
		
		if (obj){
			string url = "Discord/Channel/Create";
			
			Post(url,obj.ToJson(),DBCBX);	
		
			return cid;	
		}
		
		return -1;
	}
	
	
	int ChannelCreateObj(string Name, UApiChannelOptions Options = NULL, Class instance = NULL, string function = "") {
		int cid = UApi().CallId();
		
		
		ref RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDiscordStatusCallBack(instance, function, cid, Name);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		UApiCreateChannelObject obj = new UApiCreateChannelObject(Name, Options);
		
		if (obj){
			string url = "Discord/Channel/Create";
			
			Post(url,obj.ToJson(),DBCBX);	
		
			return cid;	
		}
		
		return -1;
	}
	
	
	int ChannelDelete(string id, string reason, Class instance = NULL, string function = ""){
		int cid = UApi().CallId();
		
		
		ref RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDBCallBack(instance, function, cid, id);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		UApiUpdateChannelObject obj = new UApiUpdateChannelObject(reason, NULL);
		
		if (obj){
			string url = "Discord/Channel/Delete/" + id;
			
			Post(url,obj.ToJson(),DBCBX);
			return cid;			
		}
		return -1;
	}
	
	int ChannelDeleteObj(string id, string reason, Class instance = NULL, string function = ""){
		int cid = UApi().CallId();
		
		
		ref RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDiscordStatusCallBack(instance, function, cid, id);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		UApiUpdateChannelObject obj = new UApiUpdateChannelObject(reason, NULL);
		
		if (obj){
			string url = "Discord/Channel/Delete/" + id;
			
			Post(url,obj.ToJson(),DBCBX);
			return cid;			
		}
		return -1;
	}
	
	int ChannelEdit(string id, string reason, ref UApiChannelUpdateOptions options, Class instance = NULL, string function = ""){
		int cid = UApi().CallId();
		
		
		ref RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDBCallBack(instance, function, cid, id);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		UApiUpdateChannelObject obj = new UApiUpdateChannelObject(reason, UApiChannelUpdateOptions.Cast(options));
		
		if (obj){
			string url = "Discord/Channel/Edit/" + id;
			
			Post(url,obj.ToJson(),DBCBX);	
			return cid;		
		}
		return -1;
	}
	
	int ChannelEditObj(string id, string reason, ref UApiChannelUpdateOptions options, Class instance = NULL, string function = ""){
		int cid = UApi().CallId();
		
		
		ref RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDiscordStatusCallBack(instance, function, cid, id);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		UApiUpdateChannelObject obj = new UApiUpdateChannelObject(reason, UApiChannelUpdateOptions.Cast(options));
		
		if (obj){
			string url = "Discord/Channel/Edit/" + id;
			
			Post(url,obj.ToJson(),DBCBX);	
			return cid;		
		}
		return -1;
	}
	
	int ChannelSend(string id, string message, Class instance = NULL, string function = ""){
		int cid = UApi().CallId();
		
		
		ref RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDBCallBack(instance, function, cid, id);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		UApiDiscordBasicMessage obj = new UApiDiscordBasicMessage(message);
		
		if (obj){
			string url = "Discord/Channel/Send/" + id;
			
			Post(url,obj.ToJson(),DBCBX);		
			return cid;	
		}
		return -1;
	}
	
	int ChannelSendObj(string id, string message, Class instance = NULL, string function = ""){
		int cid = UApi().CallId();
		
		
		ref RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDiscordStatusCallBack(instance, function, cid, id);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		UApiDiscordBasicMessage obj = new UApiDiscordBasicMessage(message);
		
		if (obj){
			string url = "Discord/Channel/Send/" + id;
			
			Post(url,obj.ToJson(),DBCBX);		
			return cid;	
		}
		return -1;
	}
	
	
	int ChannelSendEmbed(string id, UApiDiscordEmbed message, Class instance = NULL, string function = ""){
		int cid = UApi().CallId();
		
		
		ref RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDBCallBack(instance, function, cid, id);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
				
		if (message){
			string url = "Discord/Channel/Send/" + id;
			
			Post(url,message.ToJson(),DBCBX);	
			return cid;		
		}
		return -1;
	}
	
	
	int ChannelSendEmbedObj(string id, UApiDiscordEmbed message, Class instance = NULL, string function = ""){
		int cid = UApi().CallId();
		
		
		ref RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDiscordStatusCallBack(instance, function, cid, id);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
				
		if (message){
			string url = "Discord/Channel/Send/" + id;
			
			Post(url,message.ToJson(),DBCBX);	
			return cid;		
		}
		return -1;
	}
	
	
	
	int ChannelMessages(string id,  Class instance, string function, ref UApiDiscordChannelFilter filter = NULL,  string auth = ""){
		int cid = UApi().CallId();
		
		if (!filter){
			filter = new UApiDiscordChannelFilter();
		}
		
		ref RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDBCallBack(instance, function, cid, id);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		string url = BaseUrl() + "Discord/Channel/Messages/" + id;
		
		if (filter && DBCBX){
			Post(url,filter.ToJson(),DBCBX);	
			return cid;		
		}
		return -1;
	}
	
	int ChannelMessagesObj(string id,  Class instance, string function, ref UApiDiscordChannelFilter filter = NULL,  string auth = ""){
		int cid = UApi().CallId();
		
		if (!filter){
			filter = new UApiDiscordChannelFilter();
		}
		
		ref RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDiscordMessagesCallBack(instance, function, cid, id);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		string url = "Discord/Channel/Messages/" + id;
		
		if (filter && DBCBX){
			Post(url,filter.ToJson(),DBCBX);	
			return cid;		
		}
		return -1;
	}
	
}