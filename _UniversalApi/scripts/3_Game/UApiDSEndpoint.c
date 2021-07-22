class UniversalDSEndpoint extends UApiBaseEndpoint
{	
	

	override protected string EndpointBaseUrl(){
		return UApiConfig().GetBaseURL() + "Discord/";
	}
	
	string Link(string PlainId = ""){
		if (PlainId == "" && GetGame().IsClient()){
			DayZPlayer player = DayZPlayer.Cast(GetGame().GetPlayer());
			if ( player && player.GetIdentity() ){
				return EndpointBaseUrl() + player.GetIdentity().GetPlainId();
			}
			if (GetGame().GetUserManager() && GetGame().GetUserManager().GetTitleInitiator()){
				return EndpointBaseUrl() + GetGame().GetUserManager().GetTitleInitiator().GetUid();
			}
		}
		return EndpointBaseUrl() + PlainId;
	}
		
	int AddRole(string GUID, string RoleId, Class instance = NULL, string function = "") {
		int cid = UApi().CallId();
		autoptr RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDBCallBack(instance, function, cid, GUID);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
			
		string url = "AddRole/" + GUID;
		
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
		autoptr RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDBCallBack(instance, function, cid, GUID);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		string url = "RemoveRole/" + GUID;
		
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
		autoptr RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDBCallBack(instance, function, cid, GUID);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		string url = "Get/" + GUID;
		
		Post(url,"{}",DBCBX);
		return cid;
	}
	
	int GetUserWithPlainId(string plainId, Class instance, string function) {
		int cid = UApi().CallId();
		autoptr RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDBCallBack(instance, function, cid, plainId);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		string url = "GetWithPlainId/" + plainId;
		if (plainId && plainId != ""){
			Post(url,"{}",DBCBX);
		}
		return cid;
	}
	
	int GetUserObj(string GUID, Class instance, string function) {
		int cid = UApi().CallId();
		autoptr RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDSCallBack(instance, function, cid, GUID);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		string url = "Get/" + GUID;
		
		Post(url,"{}",DBCBX);
		return cid;
	}
	
	int GetUserObjWithPlainId(string plainId, Class instance, string function) {
		int cid = UApi().CallId();
		autoptr RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDSCallBack(instance, function, cid, plainId);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		string url = "GetWithPlainId/" + plainId;
		if (plainId && plainId != ""){
			Post(url,"{}",DBCBX);
		}
		return cid;
	}
	
	
	int CheckDiscord(string PlainId, Class instance, string function,  string baseUrl = ""){		
		int cid = UApi().CallId();
		if (baseUrl == ""){
			baseUrl = UApiConfig().GetBaseURL();
		}
		
		autoptr RestCallback DBCBX;
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
			baseUrl = UApiConfig().GetBaseURL();
		}
		
		autoptr RestCallback DBCBX;
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
		
		
		autoptr RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDBCallBack(instance, function, cid, Name);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		autoptr UApiCreateChannelObject obj = new UApiCreateChannelObject(Name, UApiChannelCreateOptions.Cast(Options));
		
		if (obj){
			string url = "Channel/Create";
			
			Post(url,obj.ToJson(),DBCBX);	
		
			return cid;	
		}
		
		return -1;
	}
	
	
	int ChannelCreateObj(string Name, UApiChannelOptions Options = NULL, Class instance = NULL, string function = "") {
		int cid = UApi().CallId();
		
		
		autoptr RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDiscordStatusCallBack(instance, function, cid, Name);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		autoptr UApiCreateChannelObject obj = new UApiCreateChannelObject(Name, UApiChannelCreateOptions.Cast(Options));
		
		if (obj){
			string url = "Channel/Create";
			
			Post(url,obj.ToJson(),DBCBX);	
		
			return cid;	
		}
		
		return -1;
	}
	
	
	int ChannelDelete(string id, string reason, Class instance = NULL, string function = ""){
		int cid = UApi().CallId();
		
		
		autoptr RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDBCallBack(instance, function, cid, id);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		autoptr UApiUpdateChannelObject obj = new UApiUpdateChannelObject(reason, NULL);
		
		if (obj){
			string url = "Channel/Delete/" + id;
			
			Post(url,obj.ToJson(),DBCBX);
			return cid;			
		}
		return -1;
	}
	
	int ChannelDeleteObj(string id, string reason, Class instance = NULL, string function = ""){
		int cid = UApi().CallId();
		
		
		autoptr RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDiscordStatusCallBack(instance, function, cid, id);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		autoptr UApiUpdateChannelObject obj = new UApiUpdateChannelObject(reason, NULL);
		
		if (obj){
			string url = "Channel/Delete/" + id;
			
			Post(url,obj.ToJson(),DBCBX);
			return cid;			
		}
		return -1;
	}
	
	int ChannelEdit(string id, string reason, autoptr UApiChannelUpdateOptions options, Class instance = NULL, string function = ""){
		int cid = UApi().CallId();
		
		
		autoptr RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDBCallBack(instance, function, cid, id);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		autoptr UApiUpdateChannelObject obj = new UApiUpdateChannelObject(reason, UApiChannelUpdateOptions.Cast(options));
		
		if (obj){
			string url = "Channel/Edit/" + id;
			
			Post(url,obj.ToJson(),DBCBX);	
			return cid;		
		}
		return -1;
	}
	
	int ChannelEditObj(string id, string reason, autoptr UApiChannelUpdateOptions options, Class instance = NULL, string function = ""){
		int cid = UApi().CallId();
		
		
		autoptr RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDiscordStatusCallBack(instance, function, cid, id);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		autoptr UApiUpdateChannelObject obj = new UApiUpdateChannelObject(reason, UApiChannelUpdateOptions.Cast(options));
		
		if (obj){
			string url = "Channel/Edit/" + id;
			
			Post(url,obj.ToJson(),DBCBX);	
			return cid;		
		}
		return -1;
	}
	
	int ChannelSend(string id, string message, Class instance = NULL, string function = ""){
		int cid = UApi().CallId();
		
		
		autoptr RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDBCallBack(instance, function, cid, id);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		autoptr UApiDiscordBasicMessage obj = new UApiDiscordBasicMessage(message);
		
		if (obj){
			string url = "Channel/Send/" + id;
			
			Post(url,obj.ToJson(),DBCBX);		
			return cid;	
		}
		return -1;
	}
	
	int ChannelSendObj(string id, string message, Class instance = NULL, string function = ""){
		int cid = UApi().CallId();
		
		
		autoptr RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDiscordStatusCallBack(instance, function, cid, id);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		autoptr UApiDiscordBasicMessage obj = new UApiDiscordBasicMessage(message);
		
		if (obj){
			string url = "Channel/Send/" + id;
			
			Post(url,obj.ToJson(),DBCBX);		
			return cid;	
		}
		return -1;
	}
	
	
	int ChannelSendEmbed(string id, UApiDiscordEmbed message, Class instance = NULL, string function = ""){
		int cid = UApi().CallId();
		
		
		autoptr RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDBCallBack(instance, function, cid, id);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
				
		if (message){
			string url = "Channel/Send/" + id;
			
			Post(url,message.ToJson(),DBCBX);	
			return cid;		
		}
		return -1;
	}
	
	
	int ChannelSendEmbedObj(string id, UApiDiscordEmbed message, Class instance = NULL, string function = ""){
		int cid = UApi().CallId();
		
		
		autoptr RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDiscordStatusCallBack(instance, function, cid, id);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
				
		if (message){
			string url = "Channel/Send/" + id;
			
			Post(url,message.ToJson(),DBCBX);	
			return cid;		
		}
		return -1;
	}
	
	
	int UserSend(string guid, string message,  Class instance = NULL, string function = ""){
		int cid = UApi().CallId();
		
		autoptr RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDBCallBack(instance, function, cid, guid);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		string url = "Send/" + guid;
		
		if (message != "" && DBCBX){
			autoptr UApiDiscordBasicMessage obj = new UApiDiscordBasicMessage(message);
			Post(url,obj.ToJson(),DBCBX);	
			return cid;		
		}
		return -1;
	}
	
	
	int ChannelMessages(string id,  Class instance, string function, autoptr UApiDiscordChannelFilter filter = NULL){
		int cid = UApi().CallId();
		
		if (!filter){
			filter = new UApiDiscordChannelFilter();
		}
		
		autoptr RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDBCallBack(instance, function, cid, id);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		string url = "Channel/Messages/" + id;
		
		if (filter && DBCBX){
			Post(url,filter.ToJson(),DBCBX);	
			return cid;		
		}
		return -1;
	}
	
	int ChannelMessagesObj(string id,  Class instance, string function, autoptr UApiDiscordChannelFilter filter = NULL){
		int cid = UApi().CallId();
		
		if (!filter){
			filter = new UApiDiscordChannelFilter();
		}
		
		autoptr RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDiscordMessagesCallBack(instance, function, cid, id);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		string url = "Channel/Messages/" + id;
		
		if (filter && DBCBX){
			Post(url,filter.ToJson(),DBCBX);	
			return cid;		
		}
		return -1;
	}
	
}