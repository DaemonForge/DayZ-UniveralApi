class UniversalDSEndpoint extends UApiBaseEndpoint
{	
	

	override protected string EndpointBaseUrl(){
		return UApiConfig().GetBaseURL() + "Discord/";
	}
	
	//Returns a link for the player based on the players steam id so they can connect there discord to there steam account
	string Link(string PlainId = ""){
		if (PlainId == "" && GetGame().IsClient()){
			return EndpointBaseUrl() + GetDayZGame().GetSteamId();
		}
		return EndpointBaseUrl() + PlainId;
	}
		
	//Add's a role to a user's connected discord
	int AddRole(string GUID, string RoleId, Class instance = NULL, string function = "") {
		if (GUID == ""){
			return -1;
		}
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
	
	//Removes a role from a user's connected discord
	int RemoveRole(string GUID, string RoleId, Class instance = NULL, string function = "") {
		if (GUID == ""){
			return -1;
		}
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
	
	//Sends a DM to a user's discord retuns `StatusObject`
	int UserSend(string GUID, string message,  Class instance = NULL, string function = ""){
		if (GUID == ""){
			return -1;
		}
		int cid = UApi().CallId();
		
		autoptr RestCallback DBCBX;
		if (instance && function != ""){
			DBCBX = new UApiDBCallBack(instance, function, cid, GUID);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		string url = "Send/" + GUID;
		
		if (message != "" && DBCBX){
			autoptr UApiDiscordBasicMessage obj = new UApiDiscordBasicMessage(message);
			Post(url,obj.ToJson(),DBCBX);	
			return cid;		
		}
		return -1;
	}

	//Return's a User's `UApiDiscordUser` Object 
	int GetUser(string GUID, Class instance, string function) {
		if (GUID == ""){
			return -1;
		}
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
	
	//A way to check if a player's discord is set up before they connect to the server and get an authkey
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
		
		UApi().Post(url,"{}",DBCBX);
		
		return cid;
	}
	
	//A way to check if a player's discord is set up before they connect to the server and get an authkey
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
		
		UApi().Post(url,"{}",DBCBX);
		
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
	
	
}