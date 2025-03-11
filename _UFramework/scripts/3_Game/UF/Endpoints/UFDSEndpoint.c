class UniversalDSEndpoint extends UFBaseEndpoint
{	
	

	override protected string EndpointBaseUrl(){
		return UFConfig().GetBaseURL() + "Discord/";
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
		if (GUID == "" || RoleId == ""){
			Error2("[UF] Error Adding Role from User","GUID and RoleId must be valid strings");
			return -1;
		}
		int cid = U().CallId();
		
		autoptr RestCallback DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, GUID);
		}  else if (cbInstance && cbFunction != ""){
			DBCBX = new UDBNestedCallBack(new UFCallback<UDiscordUser>(cbInstance, cbFunction, GUID), cid);
		} else {
			DBCBX = new USilentCallBack();
		}
			
		string url = "AddRole/" + GUID;
		
		autoptr UDiscordRoleReq roleReq = new UDiscordRoleReq(RoleId);
		
		Post(url,roleReq.ToJson(),DBCBX);
		
		return cid;
	}
	
	//Removes a role from a user's connected discord
	int RemoveRole(string GUID, string RoleId, Class cbInstance = NULL, string cbFunction = "", bool ReturnString = false) {
		if (GUID == "" || RoleId == ""){
			Error2("[UF] Error Removing Role from User","GUID and RoleId must be valid strings");
			return -1;
		}
		int cid = U().CallId();
		autoptr RestCallback DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, GUID);
		}  else if (cbInstance && cbFunction != ""){
			DBCBX = new UDBNestedCallBack(new UFCallback<UDiscordUser>(cbInstance, cbFunction, GUID), cid);
		} else {
			DBCBX = new USilentCallBack();
		}
		
		string url = "RemoveRole/" + GUID;
		
		autoptr UDiscordRoleReq roleReq = new UDiscordRoleReq(RoleId);
		
		Post(url,roleReq.ToJson(),DBCBX);
		
		return cid;
	}
	
	//Sends a DM to a user's discord retuns `StatusObject`
	int UserSend(string GUID, string message,  Class cbInstance = NULL, string cbFunction = "", bool ReturnString = false){
		if (GUID == "" || message == ""){
			Error2("[UF] Error Sending DM to User","GUID must be valid string");
			return -1;
		}
		int cid = U().CallId();
		
		autoptr RestCallback DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, GUID);
		}  else if (cbInstance && cbFunction != ""){
			DBCBX = new UDBNestedCallBack(new UFCallback<UDiscordStatusObject>(cbInstance, cbFunction, GUID), cid);
		} else {
			DBCBX = new USilentCallBack();
		}
		
		string url = "Send/" + GUID;
		
		autoptr UDiscordBasicMessage obj = new UDiscordBasicMessage(message);
		Post(url,obj.ToJson(),DBCBX);	
		return cid;	
	}

	//Return's a User's `UDiscordUser` Object 
	int GetUser(string GUID, Class cbInstance, string cbFunction, bool ReturnString = false) {
		if (GUID == ""){
			Error2("[UF] Error Getting Users Object","GUID must be valid string");
			return -1;
		}
		int cid = U().CallId();
		autoptr RestCallback DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, GUID);
		} else if (cbInstance && cbFunction != ""){
			DBCBX = new UDBNestedCallBack(new UFCallback<UDiscordUser>(cbInstance, cbFunction, GUID), cid);
		} else {
			DBCBX = new USilentCallBack();
		}
		
		string url = "Get/" + GUID;
		
		Post(url,"{}",DBCBX);
		return cid;
	}	
	
	//Return's a User's currently connected channel `UDiscordStatusObject` Object 
	int GetUsersChannel(string GUID, Class cbInstance, string cbFunction, bool ReturnString = false) {
		if (GUID == ""){
			Error2("[UF] Error Getting Users Channel","GUID must be valid string");
			return -1;
		}
		int cid = U().CallId();
		autoptr RestCallback DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, GUID);
		} else if (cbInstance && cbFunction != ""){
			DBCBX = new UDBNestedCallBack(new UFCallback<UDiscordStatusObject>(cbInstance, cbFunction, GUID), cid);
		} else {
			DBCBX = new USilentCallBack();
		}
		
		string url = "GetChannel/" + GUID;
		
		Post(url,"{}",DBCBX);
		return cid;
	}	
	
	int MoveTo(string GUID, string ChannelId, Class cbInstance = NULL , string cbFunction = "", bool ReturnString = false) {
		if (GUID == "" || ChannelId == ""){
			Error2("[UF] Error moving user","GUID and ChannelId must be valid strings");
			return -1;
		}
		int cid = U().CallId();
		autoptr RestCallback DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, GUID);
		} else if (cbInstance && cbFunction != ""){
			DBCBX = new UDBNestedCallBack(new UFCallback<StatusObject>(cbInstance, cbFunction, GUID), cid);
		} else {
			DBCBX = new USilentCallBack();
		}
		
		string url = "Move/" + GUID + "/" + ChannelId;
		
		Post(url, "{}", DBCBX);
		return cid;
	}
	
	int KickUser(string GUID, string Reason = "", Class cbInstance = NULL , string cbFunction = "", bool ReturnString = false) {
		if (GUID == ""){
			Error2("[UF] Error kicking user","GUID must be valid string");
			return -1;
		}
		int cid = U().CallId();
		autoptr RestCallback DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, GUID);
		} else if (cbInstance && cbFunction != ""){
			DBCBX = new UDBNestedCallBack(new UFCallback<StatusObject>(cbInstance, cbFunction, GUID), cid);
		} else {
			DBCBX = new USilentCallBack();
		}
		
		string url = "Kick/" + GUID;
		autoptr UTextObject txtObj = new UTextObject(Reason);
		
		Post(url, txtObj.ToJson(), DBCBX);
		return cid;
	}
	
	int MuteUser(string GUID, bool ToMute, Class cbInstance = NULL, string cbFunction = "", bool ReturnString = false) {
		if (GUID == ""){
			Error2("[UF] Error Muteing user","GUID must be valid string");
			return -1;
		}
		int cid = U().CallId();
		autoptr RestCallback DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, GUID);
		} else if (cbInstance && cbFunction != ""){
			DBCBX = new UDBNestedCallBack(new UFCallback<StatusObject>(cbInstance, cbFunction, GUID), cid);
		} else {
			DBCBX = new USilentCallBack();
		}
		
		string url = "Mute/" + GUID;
		
		autoptr UDiscordMute muteObject = new UDiscordMute(ToMute);
		
		Post(url, muteObject.ToJson(), DBCBX);
		
		return cid;
	}		
	
	int SetNickname(string GUID, string Nickname, Class cbInstance = NULL, string cbFunction = "", bool ReturnString = false) {
		if (GUID == "" || Nickname == ""){
			Error2("[UF] Error Setting Nickname","GUID and Nickname must be valid strings");
			return -1;
		}
		int cid = U().CallId();
		autoptr RestCallback DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, GUID);
		} else if (cbInstance && cbFunction != ""){
			DBCBX = new UDBNestedCallBack(new UFCallback<StatusObject>(cbInstance, cbFunction, GUID), cid);
		} else {
			DBCBX = new USilentCallBack();
		}
		
		string url = "SetNickname/" + GUID;
		
		autoptr UDiscordNickname nickObject = new UDiscordNickname(Nickname);
		
		Post(url, nickObject.ToJson(), DBCBX);
		
		return cid;
	}	
	
	int ChannelCreate(string Name, UChannelOptions Options = NULL, Class cbInstance = NULL, string cbFunction = "", bool ReturnString = false) {
		if ( Name == "" ){
			Error2("[UF] Error Creating channel","Channel ID must be valid string");
			return -1;
		}
		int cid = U().CallId();
		
		autoptr RestCallback DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, Name);
		} else if (cbInstance && cbFunction != "") {
			DBCBX = new UDBNestedCallBack(new UFCallback<UDiscordStatusObject>(cbInstance, cbFunction, Name), cid);
		} else {
			DBCBX = new USilentCallBack();
		}
		
		autoptr UCreateChannelObject obj = new UCreateChannelObject(Name, UChannelCreateOptions.Cast(Options));
		
		string url = "Channel/Create";
			
		Post(url,obj.ToJson(),DBCBX);	
		
		return cid;	
	}
	
	
	int ChannelDelete(string id, string reason, Class cbInstance = NULL, string cbFunction = "", bool ReturnString = false){
		if (id == "" || reason == ""){
			Error2("[UF] Error Deleting channel","Both Channel ID and reason must be valid strings");
			return -1;
		}
		int cid = U().CallId();
		
		
		autoptr RestCallback DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, id);
		} else if (cbInstance && cbFunction != "") {
			DBCBX = new UDBNestedCallBack(new UFCallback<UDiscordStatusObject>(cbInstance, cbFunction, id), cid);
		} else {
			DBCBX = new USilentCallBack();
		}
		
		autoptr UUpdateChannelObject obj = new UUpdateChannelObject(reason, NULL);
		
		string url = "Channel/Delete/" + id;
		
		Post(url,obj.ToJson(),DBCBX);
		return cid;	
	}
	
	int ChannelEdit(string id, string reason, UChannelUpdateOptions options, Class cbInstance = NULL, string cbFunction = "", bool ReturnString = false){
		if (id == "" || reason == ""){
			Error2("[UF] Error Editing channel","Both Channel ID and reason must be valid strings");
			return -1;
		}
		int cid = U().CallId();
		
		
		autoptr RestCallback DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, id);
		} else if (cbInstance && cbFunction != "") {
			DBCBX = new UDBNestedCallBack(new UFCallback<UDiscordStatusObject>(cbInstance, cbFunction, id), cid);
		} else {
			DBCBX = new USilentCallBack();
		}
		
		autoptr UUpdateChannelObject obj = new UUpdateChannelObject(reason, UChannelUpdateOptions.Cast(options));
		
		string url = "Channel/Edit/" + id;
			
		Post(url,obj.ToJson(),DBCBX);	
		return cid;		
	}
	
	int ChannelSend(string id, string message, Class cbInstance = NULL, string cbFunction = "", bool ReturnString = false){
		if (id == "" || message == ""){
			Error2("[UF] Error Sending message to channel","Both Channel ID and message must be valid strings");
			return -1;
		}
		int cid = U().CallId();
		
		
		autoptr RestCallback DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, id);
		} else if (cbInstance && cbFunction != "") {
			DBCBX = new UDBNestedCallBack(new UFCallback<UDiscordStatusObject>(cbInstance, cbFunction, id), cid);
		} else {
			DBCBX = new USilentCallBack();
		}
		
		autoptr UDiscordBasicMessage obj = new UDiscordBasicMessage(message);
		
		string url = "Channel/Send/" + id;
			
		Post(url,obj.ToJson(),DBCBX);		
		return cid;	
	}
	
	
	int ChannelSendEmbed(string id, UDiscordEmbed message, Class cbInstance = NULL, string cbFunction = "", bool ReturnString = false){
		if (id == "" || message != NULL){
			Error2("[UF] Error Sending Embed to channel","Both Channel ID and message must be valid");
			return -1;
		}
		int cid = U().CallId();
		
		
		autoptr RestCallback DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, id);
		} else if (cbInstance && cbFunction != "") {
			DBCBX = new UDBNestedCallBack(new UFCallback<UDiscordStatusObject>(cbInstance, cbFunction, id), cid);
		} else {
			DBCBX = new USilentCallBack();
		}
		
		string url = "Channel/Send/" + id;
			
		Post(url,message.ToJson(),DBCBX);	
		return cid;	
	}
	
	
	int ChannelMessages(string id,  Class cbInstance, string cbFunction, UDiscordChannelFilter filter = NULL, bool ReturnString = false){
		if (id == ""){
			Error2("[UF] Error Getting messages from channel","Channel ID must be valid");
			return -1;
		}
		int cid = U().CallId();
		
		autoptr UDiscordChannelFilter vFilter = filter;
		if (!vFilter){
			vFilter = new UDiscordChannelFilter();
		}
		
		autoptr RestCallback DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, id);
		} else if (cbInstance && cbFunction != "") {
			DBCBX = new UDBNestedCallBack(new UFCallback<UDiscordMessagesResponse>(cbInstance, cbFunction, id), cid);
		} else {
			DBCBX = new USilentCallBack();
		}
		
		string url = "Channel/Messages/" + id;
		
		Post(url,vFilter.ToJson(),DBCBX);	
		return cid;	
	}
	
	//A way to check if a player's discord is set up before they connect to the server and get an authkey
	int CheckRoleDiscord(string PlainId, string RoleId, Class cbInstance, string cbFunction,  string baseUrl = "", bool ReturnString = false){		
		int cid = U().CallId();
		if (baseUrl == ""){
			baseUrl = UFConfig().GetBaseURL();
		}
		
		autoptr RestCallback DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, PlainId);
		} else if (cbInstance && cbFunction != "") {
			DBCBX = new UDBNestedCallBack(new UFCallback<StatusObject>(cbInstance, cbFunction, PlainId), cid);
		} else {
			DBCBX = new USilentCallBack();
		}
		
		string url = baseUrl + "Discord/CheckRole/" + PlainId + "/" + RoleId;
		
		U().Post(url,"{}",DBCBX);
		
		return cid;
	}
	
	//A way to check if a player's discord is set up before they connect to the server and get an authkey
	int CheckDiscord(string PlainId, Class cbInstance, string cbFunction,  string baseUrl = "", bool ReturnString = false){		
		int cid = U().CallId();
		if (baseUrl == ""){
			baseUrl = UFConfig().GetBaseURL();
		}
		
		autoptr RestCallback DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, PlainId);
		} else if (cbInstance && cbFunction != "") {
			DBCBX = new UDBNestedCallBack(new UFCallback<StatusObject>(cbInstance, cbFunction, PlainId), cid);
		} else {
			DBCBX = new USilentCallBack();
		}
		
		string url = baseUrl + "Discord/Check/" + PlainId;
		
		U().Post(url,"{}",DBCBX);
		
		return cid;
	}
	
	
	
	//Obsolete GetUser accepts both plainid or GUID
	/*int GetUserWithPlainId(string plainId, Class cbInstance, string cbFunction) {
		int cid = U().CallId();
		autoptr RestCallback DBCBX;
		if (cbInstance && cbFunction != ""){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, plainId);
		} else {
			DBCBX = new USilentCallBack();
		}
		
		string url = "GetWithPlainId/" + plainId;
		if (plainId && plainId != ""){
			Post(url,"{}",DBCBX);
		}
		return cid;
	}*/
	
	//Obsolete GetUserObj accepts both plainid or GUID
	/*int GetUserObjWithPlainId(string plainId, Class cbInstance, string cbFunction) {
		int cid = U().CallId();
		autoptr RestCallback DBCBX;
		if (cbInstance && cbFunction != ""){
			DBCBX = new UDBNestedCallBack(new UFCallback<UDiscordUser>(cbInstance, cbFunction, plainId), cid);
		} else {
			DBCBX = new USilentCallBack();
		}
		
		string url = "GetWithPlainId/" + plainId;
		if (plainId && plainId != ""){
			Post(url,"{}",DBCBX);
		}
		return cid;
	}*/
	
	
}