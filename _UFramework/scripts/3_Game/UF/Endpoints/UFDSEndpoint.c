/**
 * Class UniversalDSEndpoint
 *
 * Provides REST API endpoints to interact with Discord for various user and
 * channel management tasks within the application. The methods in this class 
 * handle operations such as linking a Discord account to a user's Steam ID,
 * managing roles, sending direct messages, and performing channel operations 
 * (create, delete, edit, send messages, etc.).
 *
 * Methods:
 *
 *  EndpointBaseUrl()
 *      - Returns the base URL for Discord API endpoints by appending "Discord/" 
 *        to the application's base URL.
 *
 *  Link(string PlainId = "")
 *      - Generates a URL link for a player. If PlainId is empty and the game is 
 *        running on a client, it uses the player's Steam ID, otherwise uses the 
 *        given PlainId.
 *
 *  AddRole(string GUID, string RoleId, Class cbInstance = NULL, string cbFunction = "", bool ReturnString = false)
 *      - Adds a role to the user's connected Discord account.
 *      - Parameters:
 *          GUID      : The unique identifier of the user.
 *          RoleId    : The identifier of the role to be added.
 *          cbInstance: Optional callback instance to process the response.
 *          cbFunction: Optional callback function name.
 *          ReturnString: Optional flag for return value format.
 *      - Returns an integer call ID or -1 on error.
 *
 *  RemoveRole(string GUID, string RoleId, Class cbInstance = NULL, string cbFunction = "", bool ReturnString = false)
 *      - Removes a role from the user's connected Discord account.
 *      - Parameters are similar to AddRole.
 *      - Returns an integer call ID or -1 on error.
 *
 *  UserSend(string GUID, string message, Class cbInstance = NULL, string cbFunction = "", bool ReturnString = false)
 *      - Sends a direct message (DM) to the user’s Discord account.
 *      - Parameters:
 *          GUID   : The unique identifier of the user.
 *          message: The message content to send.
 *          cbInstance, cbFunction, ReturnString: Optional callback parameters.
 *      - Returns an integer call ID or -1 on error.
 *
 *  GetUser(string GUID, Class cbInstance, string cbFunction, bool ReturnString = false)
 *      - Retrieves the Discord user object associated with the provided GUID.
 *      - Returns an integer call ID or -1 on error.
 *
 *  GetUsersChannel(string GUID, Class cbInstance, string cbFunction, bool ReturnString = false)
 *      - Retrieves the current channel information (Discord status) for the 
 *        specified user's Discord account.
 *      - Returns an integer call ID or -1 on error.
 *
 *  MoveTo(string GUID, string ChannelId, Class cbInstance = NULL, string cbFunction = "", bool ReturnString = false)
 *      - Moves the user to a specified Discord channel.
 *      - Parameters:
 *          GUID     : The user's identifier.
 *          ChannelId: The target Discord channel's identifier.
 *      - Returns an integer call ID or -1 on error.
 *
 *  KickUser(string GUID, string Reason = "", Class cbInstance = NULL, string cbFunction = "", bool ReturnString = false)
 *      - Kicks the user from Discord with an optional reason.
 *      - Returns an integer call ID or -1 on error.
 *
 *  MuteUser(string GUID, bool ToMute, Class cbInstance = NULL, string cbFunction = "", bool ReturnString = false)
 *      - Mutes or unmutes a user on Discord based on the ToMute flag.
 *      - Returns an integer call ID or -1 on error.
 *
 *  SetNickname(string GUID, string Nickname, Class cbInstance = NULL, string cbFunction = "", bool ReturnString = false)
 *      - Sets or updates the user's nickname on Discord.
 *      - Returns an integer call ID or -1 on error.
 *
 *  ChannelCreate(string Name, UChannelOptions Options = NULL, Class cbInstance = NULL, string cbFunction = "", bool ReturnString = false)
 *      - Creates a new Discord channel with the specified name and optional 
 *        configuration options.
 *      - Returns an integer call ID or -1 on error.
 *
 *  ChannelDelete(string id, string reason, Class cbInstance = NULL, string cbFunction = "", bool ReturnString = false)
 *      - Deletes an existing Discord channel identified by id with a given reason.
 *      - Returns an integer call ID or -1 on error.
 *
 *  ChannelEdit(string id, string reason, UChannelUpdateOptions options, Class cbInstance = NULL, string cbFunction = "", bool ReturnString = false)
 *      - Edits the properties of an existing Discord channel.
 *      - Returns an integer call ID or -1 on error.
 *
 *  ChannelSend(string id, string message, Class cbInstance = NULL, string cbFunction = "", bool ReturnString = false)
 *      - Sends a text-based message to a specified Discord channel.
 *      - Returns an integer call ID or -1 on error.
 *
 *  ChannelSendEmbed(string id, UDiscordEmbed message, Class cbInstance = NULL, string cbFunction = "", bool ReturnString = false)
 *      - Sends an embedded (rich content) message to a Discord channel.
 *      - Returns an integer call ID or -1 on error.
 *
 *  ChannelMessages(string id, Class cbInstance, string cbFunction, UDiscordChannelFilter filter = NULL, bool ReturnString = false)
 *      - Retrieves messages from a Discord channel, possibly filtered by criteria.
 *      - Returns an integer call ID or -1 on error.
 *
 *  CheckRoleDiscord(string PlainId, string RoleId, Class cbInstance, string cbFunction, string baseUrl = "", bool ReturnString = false)
 *      - Checks if a user's Discord account is set up and verifies if the user 
 *        has a specific role.
 *      - Returns an integer call ID or -1 on error.
 *
 *  CheckDiscord(string PlainId, Class cbInstance, string cbFunction, string baseUrl = "", bool ReturnString = false)
 *      - Checks if a user's Discord account is properly set up before providing an 
 *        authentication key.
 *      - Returns an integer call ID or -1 on error.
 */
class UniversalDSEndpoint extends UFBaseEndpoint
{	
	

	override protected string EndpointBaseUrl(){
		return UFConfig().GetBaseURL() + "Discord/";
	}
	
	//Returns a link for the player based on the players steam id so they can connect there discord to there steam account
	string Link(string PlainId = ""){
		if (PlainId == "" && g_Game.IsClient()){
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
		int cid = -1;
		
		autoptr UFRestCallBackBase DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, GUID);
		}  else if (cbInstance && cbFunction != ""){
			DBCBX = new UNestedCallBack(new UFCallback<UDiscordUser>(cbInstance, cbFunction, GUID));
		} else {
			DBCBX = new USilentCallBack();
		}
			
		string url = "AddRole/" + GUID;
		
		autoptr UDiscordRoleReq roleReq = new UDiscordRoleReq(RoleId);
		
		Post(url,roleReq.ToJson(),U().RegisterCall(DBCBX, cid));
		
		return cid;
	}
	
	//Removes a role from a user's connected discord
	int RemoveRole(string GUID, string RoleId, Class cbInstance = NULL, string cbFunction = "", bool ReturnString = false) {
		if (GUID == "" || RoleId == ""){
			Error2("[UF] Error Removing Role from User","GUID and RoleId must be valid strings");
			return -1;
		}
		int cid = -1;
		autoptr UFRestCallBackBase DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, GUID);
		}  else if (cbInstance && cbFunction != ""){
			DBCBX = new UNestedCallBack(new UFCallback<UDiscordUser>(cbInstance, cbFunction, GUID));
		} else {
			DBCBX = new USilentCallBack();
		}
		
		string url = "RemoveRole/" + GUID;
		
		autoptr UDiscordRoleReq roleReq = new UDiscordRoleReq(RoleId);
		
		Post(url,roleReq.ToJson(),U().RegisterCall(DBCBX, cid));
		
		return cid;
	}
	
	//Sends a DM to a user's discord retuns `StatusObject`
	int UserSend(string GUID, string message,  Class cbInstance = NULL, string cbFunction = "", bool ReturnString = false){
		if (GUID == "" || message == ""){
			Error2("[UF] Error Sending DM to User","GUID must be valid string");
			return -1;
		}
		int cid = -1;
		
		autoptr UFRestCallBackBase DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, GUID);
		}  else if (cbInstance && cbFunction != ""){
			DBCBX = new UNestedCallBack(new UFCallback<UDiscordStatusObject>(cbInstance, cbFunction, GUID));
		} else {
			DBCBX = new USilentCallBack();
		}
		
		string url = "Send/" + GUID;
		
		autoptr UDiscordBasicMessage obj = new UDiscordBasicMessage(message);
		Post(url,obj.ToJson(),U().RegisterCall(DBCBX, cid));	
		return cid;	
	}

	//Return's a User's `UDiscordUser` Object 
	int GetUser(string GUID, Class cbInstance, string cbFunction, bool ReturnString = false) {
		if (GUID == ""){
			Error2("[UF] Error Getting Users Object","GUID must be valid string");
			return -1;
		}
		int cid = -1;
		autoptr UFRestCallBackBase DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, GUID);
		} else if (cbInstance && cbFunction != ""){
			DBCBX = new UNestedCallBack(new UFCallback<UDiscordUser>(cbInstance, cbFunction, GUID));
		} else {
			DBCBX = new USilentCallBack();
		}
		
		string url = "Get/" + GUID;
		
		Post(url,"{}",U().RegisterCall(DBCBX, cid));
		return cid;
	}	
	
	//Return's a User's currently connected channel `UDiscordStatusObject` Object 
	int GetUsersChannel(string GUID, Class cbInstance, string cbFunction, bool ReturnString = false) {
		if (GUID == ""){
			Error2("[UF] Error Getting Users Channel","GUID must be valid string");
			return -1;
		}
		int cid = -1;
		autoptr UFRestCallBackBase DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, GUID);
		} else if (cbInstance && cbFunction != ""){
			DBCBX = new UNestedCallBack(new UFCallback<UDiscordStatusObject>(cbInstance, cbFunction, GUID));
		} else {
			DBCBX = new USilentCallBack();
		}
		
		string url = "GetChannel/" + GUID;
		
		Post(url,"{}",U().RegisterCall(DBCBX, cid));
		return cid;
	}	
	
	int MoveTo(string GUID, string ChannelId, Class cbInstance = NULL , string cbFunction = "", bool ReturnString = false) {
		if (GUID == "" || ChannelId == ""){
			Error2("[UF] Error moving user","GUID and ChannelId must be valid strings");
			return -1;
		}
		int cid = -1;
		autoptr UFRestCallBackBase DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, GUID);
		} else if (cbInstance && cbFunction != ""){
			DBCBX = new UNestedCallBack(new UFCallback<StatusObject>(cbInstance, cbFunction, GUID));
		} else {
			DBCBX = new USilentCallBack();
		}
		
		string url = "Move/" + GUID + "/" + ChannelId;
		
		Post(url, "{}", U().RegisterCall(DBCBX, cid));
		return cid;
	}
	
	int KickUser(string GUID, string Reason = "", Class cbInstance = NULL , string cbFunction = "", bool ReturnString = false) {
		if (GUID == ""){
			Error2("[UF] Error kicking user","GUID must be valid string");
			return -1;
		}
		int cid = -1;
		autoptr UFRestCallBackBase DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, GUID);
		} else if (cbInstance && cbFunction != ""){
			DBCBX = new UNestedCallBack(new UFCallback<StatusObject>(cbInstance, cbFunction, GUID));
		} else {
			DBCBX = new USilentCallBack();
		}
		
		string url = "Kick/" + GUID;
		autoptr UTextObject txtObj = new UTextObject(Reason);
		
		Post(url, txtObj.ToJson(), U().RegisterCall(DBCBX, cid));
		return cid;
	}
	
	int MuteUser(string GUID, bool ToMute, Class cbInstance = NULL, string cbFunction = "", bool ReturnString = false) {
		if (GUID == ""){
			Error2("[UF] Error Muteing user","GUID must be valid string");
			return -1;
		}
		int cid = -1;
		autoptr UFRestCallBackBase DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, GUID);
		} else if (cbInstance && cbFunction != ""){
			DBCBX = new UNestedCallBack(new UFCallback<StatusObject>(cbInstance, cbFunction, GUID));
		} else {
			DBCBX = new USilentCallBack();
		}
		
		string url = "Mute/" + GUID;
		
		autoptr UDiscordMute muteObject = new UDiscordMute(ToMute);
		
		Post(url, muteObject.ToJson(), U().RegisterCall(DBCBX, cid));
		
		return cid;
	}		
	
	int SetNickname(string GUID, string Nickname, Class cbInstance = NULL, string cbFunction = "", bool ReturnString = false) {
		if (GUID == "" || Nickname == ""){
			Error2("[UF] Error Setting Nickname","GUID and Nickname must be valid strings");
			return -1;
		}
		int cid = -1;
		autoptr UFRestCallBackBase DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, GUID);
		} else if (cbInstance && cbFunction != ""){
			DBCBX = new UNestedCallBack(new UFCallback<StatusObject>(cbInstance, cbFunction, GUID));
		} else {
			DBCBX = new USilentCallBack();
		}
		
		string url = "SetNickname/" + GUID;
		
		autoptr UDiscordNickname nickObject = new UDiscordNickname(Nickname);
		
		Post(url, nickObject.ToJson(), U().RegisterCall(DBCBX, cid));
		
		return cid;
	}	
	
	int ChannelCreate(string Name, UChannelOptions Options = NULL, Class cbInstance = NULL, string cbFunction = "", bool ReturnString = false) {
		if ( Name == "" ){
			Error2("[UF] Error Creating channel","Channel ID must be valid string");
			return -1;
		}
		int cid = -1;
		
		autoptr UFRestCallBackBase DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, Name);
		} else if (cbInstance && cbFunction != "") {
			DBCBX = new UNestedCallBack(new UFCallback<UDiscordStatusObject>(cbInstance, cbFunction, Name));
		} else {
			DBCBX = new USilentCallBack();
		}
		
		autoptr UCreateChannelObject obj = new UCreateChannelObject(Name, UChannelCreateOptions.Cast(Options));
		
		string url = "Channel/Create";
			
		Post(url,obj.ToJson(),U().RegisterCall(DBCBX, cid));	
		
		return cid;	
	}
	
	
	int ChannelDelete(string id, string reason, Class cbInstance = NULL, string cbFunction = "", bool ReturnString = false){
		if (id == "" || reason == ""){
			Error2("[UF] Error Deleting channel","Both Channel ID and reason must be valid strings");
			return -1;
		}
		int cid = -1;
		
		
		autoptr UFRestCallBackBase DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, id);
		} else if (cbInstance && cbFunction != "") {
			DBCBX = new UNestedCallBack(new UFCallback<UDiscordStatusObject>(cbInstance, cbFunction, id));
		} else {
			DBCBX = new USilentCallBack();
		}
		
		autoptr UUpdateChannelObject obj = new UUpdateChannelObject(reason, NULL);
		
		string url = "Channel/Delete/" + id;
		
		Post(url,obj.ToJson(),U().RegisterCall(DBCBX, cid));
		return cid;	
	}
	
	int ChannelEdit(string id, string reason, UChannelUpdateOptions options, Class cbInstance = NULL, string cbFunction = "", bool ReturnString = false){
		if (id == "" || reason == ""){
			Error2("[UF] Error Editing channel","Both Channel ID and reason must be valid strings");
			return -1;
		}
		int cid = -1;
		
		
		autoptr UFRestCallBackBase DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, id);
		} else if (cbInstance && cbFunction != "") {
			DBCBX = new UNestedCallBack(new UFCallback<UDiscordStatusObject>(cbInstance, cbFunction, id));
		} else {
			DBCBX = new USilentCallBack();
		}
		
		autoptr UUpdateChannelObject obj = new UUpdateChannelObject(reason, UChannelUpdateOptions.Cast(options));
		
		string url = "Channel/Edit/" + id;
			
		Post(url,obj.ToJson(),U().RegisterCall(DBCBX, cid));	
		return cid;		
	}
	
	int ChannelSend(string id, string message, Class cbInstance = NULL, string cbFunction = "", bool ReturnString = false){
		if (id == "" || message == ""){
			Error2("[UF] Error Sending message to channel","Both Channel ID and message must be valid strings");
			return -1;
		}
		int cid = -1;
		
		
		autoptr UFRestCallBackBase DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, id);
		} else if (cbInstance && cbFunction != "") {
			DBCBX = new UNestedCallBack(new UFCallback<UDiscordStatusObject>(cbInstance, cbFunction, id));
		} else {
			DBCBX = new USilentCallBack();
		}
		
		autoptr UDiscordBasicMessage obj = new UDiscordBasicMessage(message);
		
		string url = "Channel/Send/" + id;
			
		Post(url,obj.ToJson(),U().RegisterCall(DBCBX, cid));		
		return cid;	
	}
	
	
	int ChannelSendEmbed(string id, UDiscordEmbed message, Class cbInstance = NULL, string cbFunction = "", bool ReturnString = false){
		if (id == "" || message == NULL){
			Error2("[UF] Error Sending Embed to channel","Both Channel ID and message must be valid");
			return -1;
		}
		int cid = -1;
		
		
		autoptr UFRestCallBackBase DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, id);
		} else if (cbInstance && cbFunction != "") {
			DBCBX = new UNestedCallBack(new UFCallback<UDiscordStatusObject>(cbInstance, cbFunction, id));
		} else {
			DBCBX = new USilentCallBack();
		}
		
		string url = "Channel/Send/" + id;
			
		Post(url,message.ToJson(),U().RegisterCall(DBCBX, cid));	
		return cid;	
	}
	
	
	int ChannelMessages(string id,  Class cbInstance, string cbFunction, UDiscordChannelFilter filter = NULL, bool ReturnString = false){
		if (id == ""){
			Error2("[UF] Error Getting messages from channel","Channel ID must be valid");
			return -1;
		}
		int cid = -1;
		
		autoptr UDiscordChannelFilter vFilter = filter;
		if (!vFilter){
			vFilter = new UDiscordChannelFilter();
		}
		
		autoptr UFRestCallBackBase DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, id);
		} else if (cbInstance && cbFunction != "") {
			DBCBX = new UNestedCallBack(new UFCallback<UDiscordMessagesResponse>(cbInstance, cbFunction, id));
		} else {
			DBCBX = new USilentCallBack();
		}
		
		string url = "Channel/Messages/" + id;
		
		Post(url,vFilter.ToJson(),U().RegisterCall(DBCBX, cid));	
		return cid;	
	}
	
	//A way to check if a player's discord is set up before they connect to the server and get an authkey
	int CheckRoleDiscord(string PlainId, string RoleId, Class cbInstance, string cbFunction,  string baseUrl = "", bool ReturnString = false){		
		int cid = -1;
		if (baseUrl == ""){
			baseUrl = UFConfig().GetBaseURL();
		}
		
		autoptr UFRestCallBackBase DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, PlainId);
		} else if (cbInstance && cbFunction != "") {
			DBCBX = new UNestedCallBack(new UFCallback<StatusObject>(cbInstance, cbFunction, PlainId));
		} else {
			DBCBX = new USilentCallBack();
		}
		
		string url = baseUrl + "Discord/CheckRole/" + PlainId + "/" + RoleId;
		
		U().Post(url,"{}",U().RegisterCall(DBCBX, cid));
		
		return cid;
	}
	
	//A way to check if a player's discord is set up before they connect to the server and get an authkey
	int CheckDiscord(string PlainId, Class cbInstance, string cbFunction,  string baseUrl = "", bool ReturnString = false){		
		int cid = -1;
		if (baseUrl == ""){
			baseUrl = UFConfig().GetBaseURL();
		}
		
		autoptr UFRestCallBackBase DBCBX;
		if (cbInstance && cbFunction != "" && ReturnString){
			DBCBX = new UDBCallBack(cbInstance, cbFunction, cid, PlainId);
		} else if (cbInstance && cbFunction != "") {
			DBCBX = new UNestedCallBack(new UFCallback<StatusObject>(cbInstance, cbFunction, PlainId));
		} else {
			DBCBX = new USilentCallBack();
		}
		
		string url = baseUrl + "Discord/Check/" + PlainId;
		
		U().Post(url,"{}",U().RegisterCall(DBCBX, cid));
		
		return cid;
	}
	
	
	int DownloadAvatar(string guid, string filename = "discordme"){
		int cid = -1;		
		string url = UFConfig().GetBaseURL() + "Images/Discord/" + guid;
		if (guid == "" || filename == ""){
			Error2("[UF] DownloadAvatar", "guid or filename is null guid: " + guid + " filename: " + filename);
			return -1;
		}
		
		U().Post(url,"{}",U().RegisterCall(new UFDLDiscordAvatarCallback(filename), cid));
	
		return cid;
	}
}