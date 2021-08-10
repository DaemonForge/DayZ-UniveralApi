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
	int AddRole(string GUID, string RoleId, Class cbInstance = NULL, string cbFunction = "", bool ReturnString = false) {
		if (GUID == ""){
			return -1;
		}
		int cid = UApi().CallId();
		autoptr RestCallback DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UApiDBCallBack(cbInstance, cbFunction, cid, GUID);
		}  else if (cbInstance && cbFunction != ""){
			DBCBX = new UApiDBNestedCallBack(new UApiCallback<UApiDiscordUser>(cbInstance, cbFunction, GUID), cid);
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
	int RemoveRole(string GUID, string RoleId, Class cbInstance = NULL, string cbFunction = "", bool ReturnString = false) {
		if (GUID == ""){
			return -1;
		}
		int cid = UApi().CallId();
		autoptr RestCallback DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UApiDBCallBack(cbInstance, cbFunction, cid, GUID);
		}  else if (cbInstance && cbFunction != ""){
			DBCBX = new UApiDBNestedCallBack(new UApiCallback<UApiDiscordUser>(cbInstance, cbFunction, GUID), cid);
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
	int UserSend(string GUID, string message,  Class cbInstance = NULL, string cbFunction = "", bool ReturnString = false){
		if (GUID == ""){
			return -1;
		}
		int cid = UApi().CallId();
		
		autoptr RestCallback DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UApiDBCallBack(cbInstance, cbFunction, cid, GUID);
		}  else if (cbInstance && cbFunction != ""){
			DBCBX = new UApiDBNestedCallBack(new UApiCallback<UApiDiscordStatusObject>(cbInstance, cbFunction, GUID), cid);
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
	int GetUser(string GUID, Class cbInstance, string cbFunction, bool ReturnString = false) {
		if (GUID == ""){
			return -1;
		}
		int cid = UApi().CallId();
		autoptr RestCallback DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UApiDBCallBack(cbInstance, cbFunction, cid, GUID);
		} else if (cbInstance && cbFunction != ""){
			DBCBX = new UApiDBNestedCallBack(new UApiCallback<UApiDiscordUser>(cbInstance, cbFunction, GUID), cid);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		string url = "Get/" + GUID;
		
		Post(url,"{}",DBCBX);
		return cid;
	}	
	
	//Return's a User's currently connected channel `UApiDiscordStatusObject` Object 
	int GetUsersChannel(string GUID, Class cbInstance, string cbFunction, bool ReturnString = false) {
		if (GUID == ""){
			return -1;
		}
		int cid = UApi().CallId();
		autoptr RestCallback DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UApiDBCallBack(cbInstance, cbFunction, cid, GUID);
		} else if (cbInstance && cbFunction != ""){
			DBCBX = new UApiDBNestedCallBack(new UApiCallback<UApiDiscordStatusObject>(cbInstance, cbFunction, GUID), cid);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		string url = "GetChannel/" + GUID;
		
		Post(url,"{}",DBCBX);
		return cid;
	}	
	
	int MoveTo(string GUID, string ChannelId, Class cbInstance = NULL , string cbFunction = "", bool ReturnString = false) {
		if (GUID == "" || ChannelId == ""){
			return -1;
		}
		int cid = UApi().CallId();
		autoptr RestCallback DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UApiDBCallBack(cbInstance, cbFunction, cid, GUID);
		} else if (cbInstance && cbFunction != ""){
			DBCBX = new UApiDBNestedCallBack(new UApiCallback<StatusObject>(cbInstance, cbFunction, GUID), cid);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		string url = "Move/" + GUID + "/" + ChannelId;
		
		Post(url, "{}", DBCBX);
		return cid;
	}
	
	int KickUser(string GUID, string Reason = "", Class cbInstance = NULL , string cbFunction = "", bool ReturnString = false) {
		if (GUID == ""){
			return -1;
		}
		int cid = UApi().CallId();
		autoptr RestCallback DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UApiDBCallBack(cbInstance, cbFunction, cid, GUID);
		} else if (cbInstance && cbFunction != ""){
			DBCBX = new UApiDBNestedCallBack(new UApiCallback<StatusObject>(cbInstance, cbFunction, GUID), cid);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		string url = "Kick/" + GUID;
		autoptr UApiTextObject txtObj = new UApiTextObject(Reason);
		
		Post(url, txtObj.ToJson(), DBCBX);
		return cid;
	}
	
	int MuteUser(string GUID, bool ToMute, Class cbInstance, string cbFunction, bool ReturnString = false) {
		if (GUID == ""){
			return -1;
		}
		int cid = UApi().CallId();
		autoptr RestCallback DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UApiDBCallBack(cbInstance, cbFunction, cid, GUID);
		} else if (cbInstance && cbFunction != ""){
			DBCBX = new UApiDBNestedCallBack(new UApiCallback<StatusObject>(cbInstance, cbFunction, GUID), cid);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		string url = "Mute/" + GUID;
		
		autoptr UApiDiscordMute muteObject = new UApiDiscordMute(ToMute);
		
		Post(url, muteObject.ToJson(), DBCBX);
		
		return cid;
	}		
	
	int SetNickname(string GUID, string Nickname, Class cbInstance = NULL, string cbFunction = "", bool ReturnString = false) {
		if (GUID == "" || Nickname == ""){
			return -1;
		}
		int cid = UApi().CallId();
		autoptr RestCallback DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UApiDBCallBack(cbInstance, cbFunction, cid, GUID);
		} else if (cbInstance && cbFunction != ""){
			DBCBX = new UApiDBNestedCallBack(new UApiCallback<StatusObject>(cbInstance, cbFunction, GUID), cid);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		string url = "SetNickname/" + GUID;
		
		autoptr UApiDiscordNickname nickObject = new UApiDiscordNickname(Nickname);
		
		Post(url, nickObject.ToJson(), DBCBX);
		
		return cid;
	}	
	
	int ChannelCreate(string Name, UApiChannelOptions Options = NULL, Class cbInstance = NULL, string cbFunction = "", bool ReturnString = false) {
		int cid = UApi().CallId();
		
		autoptr RestCallback DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UApiDBCallBack(cbInstance, cbFunction, cid, Name);
		} else if (cbInstance && cbFunction != "") {
			DBCBX = new UApiDBNestedCallBack(new UApiCallback<UApiDiscordStatusObject>(cbInstance, cbFunction, Name), cid);
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
	
	
	int ChannelDelete(string id, string reason, Class cbInstance = NULL, string cbFunction = "", bool ReturnString = false){
		int cid = UApi().CallId();
		
		
		autoptr RestCallback DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UApiDBCallBack(cbInstance, cbFunction, cid, id);
		} else if (cbInstance && cbFunction != "") {
			DBCBX = new UApiDBNestedCallBack(new UApiCallback<UApiDiscordStatusObject>(cbInstance, cbFunction, id), cid);
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
	
	int ChannelEdit(string id, string reason, autoptr UApiChannelUpdateOptions options, Class cbInstance = NULL, string cbFunction = "", bool ReturnString = false){
		int cid = UApi().CallId();
		
		
		autoptr RestCallback DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UApiDBCallBack(cbInstance, cbFunction, cid, id);
		} else if (cbInstance && cbFunction != "") {
			DBCBX = new UApiDBNestedCallBack(new UApiCallback<UApiDiscordStatusObject>(cbInstance, cbFunction, id), cid);
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
	
	int ChannelSend(string id, string message, Class cbInstance = NULL, string cbFunction = "", bool ReturnString = false){
		int cid = UApi().CallId();
		
		
		autoptr RestCallback DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UApiDBCallBack(cbInstance, cbFunction, cid, id);
		} else if (cbInstance && cbFunction != "") {
			DBCBX = new UApiDBNestedCallBack(new UApiCallback<UApiDiscordStatusObject>(cbInstance, cbFunction, id), cid);
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
	
	
	int ChannelSendEmbed(string id, UApiDiscordEmbed message, Class cbInstance = NULL, string cbFunction = "", bool ReturnString = false){
		int cid = UApi().CallId();
		
		
		autoptr RestCallback DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UApiDBCallBack(cbInstance, cbFunction, cid, id);
		} else if (cbInstance && cbFunction != "") {
			DBCBX = new UApiDBNestedCallBack(new UApiCallback<UApiDiscordStatusObject>(cbInstance, cbFunction, id), cid);
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
	
	
	int ChannelMessages(string id,  Class cbInstance, string cbFunction, autoptr UApiDiscordChannelFilter filter = NULL, bool ReturnString = false){
		int cid = UApi().CallId();
		
		if (!filter){
			filter = new UApiDiscordChannelFilter();
		}
		
		autoptr RestCallback DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UApiDBCallBack(cbInstance, cbFunction, cid, id);
		} else if (cbInstance && cbFunction != "") {
			DBCBX = new UApiDBNestedCallBack(new UApiCallback<UApiDiscordMessagesResponse>(cbInstance, cbFunction, id), cid);
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
	int CheckDiscord(string PlainId, Class cbInstance, string cbFunction,  string baseUrl = "", bool ReturnString = false){		
		int cid = UApi().CallId();
		if (baseUrl == ""){
			baseUrl = UApiConfig().GetBaseURL();
		}
		
		autoptr RestCallback DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UApiDBCallBack(cbInstance, cbFunction, cid, PlainId);
		} else if (cbInstance && cbFunction != "") {
			DBCBX = new UApiDBNestedCallBack(new UApiCallback<StatusObject>(cbInstance, cbFunction, PlainId), cid);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		string url = baseUrl + "Discord/Check/" + PlainId;
		
		UApi().Post(url,"{}",DBCBX);
		
		return cid;
	}
	
	
	
	//Obsolete GetUser accepts both plainid or GUID
	/*int GetUserWithPlainId(string plainId, Class cbInstance, string cbFunction) {
		int cid = UApi().CallId();
		autoptr RestCallback DBCBX;
		if (cbInstance && cbFunction != ""){
			DBCBX = new UApiDBCallBack(cbInstance, cbFunction, cid, plainId);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		string url = "GetWithPlainId/" + plainId;
		if (plainId && plainId != ""){
			Post(url,"{}",DBCBX);
		}
		return cid;
	}*/
	
	//Obsolete GetUserObj accepts both plainid or GUID
	/*int GetUserObjWithPlainId(string plainId, Class cbInstance, string cbFunction) {
		int cid = UApi().CallId();
		autoptr RestCallback DBCBX;
		if (cbInstance && cbFunction != ""){
			DBCBX = new UApiDBNestedCallBack(new UApiCallback<UApiDiscordUser>(cbInstance, cbFunction, plainId), cid);
		} else {
			DBCBX = new UApiSilentCallBack();
		}
		
		string url = "GetWithPlainId/" + plainId;
		if (plainId && plainId != ""){
			Post(url,"{}",DBCBX);
		}
		return cid;
	}*/
	
	
}