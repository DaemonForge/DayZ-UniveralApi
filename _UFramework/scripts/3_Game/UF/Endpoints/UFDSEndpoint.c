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
	
	/**
	 * Returns the base URL for Discord API endpoints
	 * @return string The base URL with "Discord/" appended
	 */
	override protected string EndpointBaseUrl(){
		UFrameworkConfig ucfg = UFrameworkConfig.Cast(UFConfig());
		if (!ucfg){
			UFLog.Err("[UFDSEndpoint] EndpointBaseUrl called but UFConfig() is null - RPC not received yet?");
			return "";
		}
		return ucfg.GetBaseURL() + "Discord/";
	}
	
	/**
	 * Generates a linking URL for players to connect their Discord account
	 * @param PlainId Player's Steam ID (uses local player's ID if empty and on client)
	 * @return string The Discord linking URL
	 */
	string Link(string PlainId = ""){
		if (PlainId == "" && g_Game.IsClient()){
			return EndpointBaseUrl() + GetDayZGame().GetSteamId();
		}
		return EndpointBaseUrl() + PlainId;
	}
		
	/**
	 * Adds a Discord role to a user's connected account
	 * @param GUID Player's unique identifier
	 * @param RoleId Discord role ID to add
	 * @param cbInstance Optional callback instance
	 * @param cbFunction Optional callback function name
	 * @param ReturnString If true, returns raw JSON string in callback
	 * @return int Call ID, or -1 on error
	 */
	int AddRole(string GUID, string RoleId, Class cbInstance = NULL, string cbFunction = "", bool ReturnString = false) {
		if (GUID == "" || RoleId == ""){
			Error2("[UF] Error Adding Role from User","GUID and RoleId must be valid strings");
			return -1;
		}
		if (!g_UFramework){
			UFLog.Err("[UF] g_UFramework is NULL - framework not ready");
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
		
		autoptr RestCallback rcb = RestCallback.Cast(g_UFramework.RegisterCall(DBCBX, cid));
		Post(url, roleReq.ToJson(), rcb);
		
		return cid;
	}
	
	/**
	 * Removes a Discord role from a user's connected account
	 * @param GUID Player's unique identifier
	 * @param RoleId Discord role ID to remove
	 * @param cbInstance Optional callback instance
	 * @param cbFunction Optional callback function name
	 * @param ReturnString If true, returns raw JSON string in callback
	 * @return int Call ID, or -1 on error
	 */
	int RemoveRole(string GUID, string RoleId, Class cbInstance = NULL, string cbFunction = "", bool ReturnString = false) {
		if (GUID == "" || RoleId == ""){
			Error2("[UF] Error Removing Role from User","GUID and RoleId must be valid strings");
			return -1;
		}
		if (!g_UFramework){
			UFLog.Err("[UF] g_UFramework is NULL - framework not ready");
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
		
		autoptr RestCallback rcb = RestCallback.Cast(g_UFramework.RegisterCall(DBCBX, cid));
		Post(url, roleReq.ToJson(), rcb);
		
		return cid;
	}
	
	/**
	 * Sends a direct message (DM) to a user's Discord account
	 * @param GUID Player's unique identifier
	 * @param message The message text to send
	 * @param cbInstance Optional callback instance
	 * @param cbFunction Optional callback function name
	 * @param ReturnString If true, returns raw JSON string in callback
	 * @return int Call ID, or -1 on error
	 */
	int UserSend(string GUID, string message,  Class cbInstance = NULL, string cbFunction = "", bool ReturnString = false){
		if (GUID == "" || message == ""){
			Error2("[UF] Error Sending DM to User","GUID must be valid string");
			return -1;
		}
		if (!g_UFramework){
			UFLog.Err("[UF] g_UFramework is NULL - framework not ready");
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
		autoptr RestCallback rcb = RestCallback.Cast(g_UFramework.RegisterCall(DBCBX, cid));
		Post(url, obj.ToJson(), rcb);	
		return cid;	
	}

	/**
	 * Retrieves a user's Discord account information
	 * @param GUID Player's unique identifier
	 * @param cbInstance Callback instance (required)
	 * @param cbFunction Callback function name (required)
	 * @param ReturnString If true, returns raw JSON string in callback
	 * @return int Call ID, or -1 on error
	 */
	int GetUser(string GUID, Class cbInstance, string cbFunction, bool ReturnString = false) {
		if (GUID == ""){
			Error2("[UF] Error Getting Users Object","GUID must be valid string");
			return -1;
		}
		if (!g_UFramework){
			UFLog.Err("[UF] g_UFramework is NULL - framework not ready");
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
		
		autoptr RestCallback rcb = RestCallback.Cast(g_UFramework.RegisterCall(DBCBX, cid));
		Post(url, "{}", rcb);
		return cid;
	}	
	
	/**
	 * Retrieves the Discord voice/text channel a user is currently in
	 * @param GUID Player's unique identifier
	 * @param cbInstance Callback instance (required)
	 * @param cbFunction Callback function name (required)
	 * @param ReturnString If true, returns raw JSON string in callback
	 * @return int Call ID, or -1 on error
	 */
	int GetUsersChannel(string GUID, Class cbInstance, string cbFunction, bool ReturnString = false) {
		if (GUID == ""){
			Error2("[UF] Error Getting Users Channel","GUID must be valid string");
			return -1;
		}
		if (!g_UFramework){
			UFLog.Err("[UF] g_UFramework is NULL - framework not ready");
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
		
		autoptr RestCallback rcb = RestCallback.Cast(g_UFramework.RegisterCall(DBCBX, cid));
		Post(url, "{}", rcb);
		return cid;
	}	
	
	/**
	 * Moves a user to a different Discord voice channel
	 * @param GUID Player's unique identifier
	 * @param ChannelId Target Discord channel ID
	 * @param cbInstance Optional callback instance
	 * @param cbFunction Optional callback function name
	 * @param ReturnString If true, returns raw JSON string in callback
	 * @return int Call ID, or -1 on error
	 */
	int MoveTo(string GUID, string ChannelId, Class cbInstance = NULL , string cbFunction = "", bool ReturnString = false) {
		if (GUID == "" || ChannelId == ""){
			Error2("[UF] Error moving user","GUID and ChannelId must be valid strings");
			return -1;
		}
		if (!g_UFramework){
			UFLog.Err("[UF] g_UFramework is NULL - framework not ready");
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
		
		autoptr RestCallback rcb = RestCallback.Cast(g_UFramework.RegisterCall(DBCBX, cid));
		Post(url, "{}", rcb);
		return cid;
	}
	
	/**
	 * Kicks a user from Discord voice channel
	 * @param GUID Player's unique identifier
	 * @param Reason Optional reason for the kick
	 * @param cbInstance Optional callback instance
	 * @param cbFunction Optional callback function name
	 * @param ReturnString If true, returns raw JSON string in callback
	 * @return int Call ID, or -1 on error
	 */
	int KickUser(string GUID, string Reason = "", Class cbInstance = NULL , string cbFunction = "", bool ReturnString = false) {
		if (GUID == ""){
			Error2("[UF] Error kicking user","GUID must be valid string");
			return -1;
		}
		if (!g_UFramework){
			UFLog.Err("[UF] g_UFramework is NULL - framework not ready");
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
		
		autoptr RestCallback rcb = RestCallback.Cast(g_UFramework.RegisterCall(DBCBX, cid));
		Post(url, txtObj.ToJson(), rcb);
		return cid;
	}
	
	/**
	 * Mutes or unmutes a user in Discord voice channel
	 * @param GUID Player's unique identifier
	 * @param ToMute True to mute, false to unmute
	 * @param cbInstance Optional callback instance
	 * @param cbFunction Optional callback function name
	 * @param ReturnString If true, returns raw JSON string in callback
	 * @return int Call ID, or -1 on error
	 */
	int MuteUser(string GUID, bool ToMute, Class cbInstance = NULL, string cbFunction = "", bool ReturnString = false) {
		if (GUID == ""){
			Error2("[UF] Error Muteing user","GUID must be valid string");
			return -1;
		}
		if (!g_UFramework){
			UFLog.Err("[UF] g_UFramework is NULL - framework not ready");
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
		
		autoptr RestCallback rcb = RestCallback.Cast(g_UFramework.RegisterCall(DBCBX, cid));
		Post(url, muteObject.ToJson(), rcb);
		
		return cid;
	}		
	
	/**
	 * Sets a user's Discord server nickname
	 * @param GUID Player's unique identifier
	 * @param Nickname The new nickname to set
	 * @param cbInstance Optional callback instance
	 * @param cbFunction Optional callback function name
	 * @param ReturnString If true, returns raw JSON string in callback
	 * @return int Call ID, or -1 on error
	 */
	int SetNickname(string GUID, string Nickname, Class cbInstance = NULL, string cbFunction = "", bool ReturnString = false) {
		if (GUID == "" || Nickname == ""){
			Error2("[UF] Error Setting Nickname","GUID and Nickname must be valid strings");
			return -1;
		}
		if (!g_UFramework){
			UFLog.Err("[UF] g_UFramework is NULL - framework not ready");
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
		
		autoptr RestCallback rcb = RestCallback.Cast(g_UFramework.RegisterCall(DBCBX, cid));
		Post(url, nickObject.ToJson(), rcb);
		
		return cid;
	}	
	
	/**
	 * Creates a new Discord channel
	 * @param Name The name of the channel to create
	 * @param Options Optional channel creation options (type, parent, permissions, etc.)
	 * @param cbInstance Optional callback instance
	 * @param cbFunction Optional callback function name
	 * @param ReturnString If true, returns raw JSON string in callback
	 * @return int Call ID, or -1 on error
	 */
	int ChannelCreate(string Name, UChannelOptions Options = NULL, Class cbInstance = NULL, string cbFunction = "", bool ReturnString = false) {
		if ( Name == "" ){
			Error2("[UF] Error Creating channel","Channel ID must be valid string");
			return -1;
		}
		if (!g_UFramework){
			UFLog.Err("[UF] g_UFramework is NULL - framework not ready");
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
			
		autoptr RestCallback rcb = RestCallback.Cast(g_UFramework.RegisterCall(DBCBX, cid));
		Post(url, obj.ToJson(), rcb);	
		
		return cid;	
	}
	
	
	/**
	 * Deletes an existing Discord channel
	 * @param id The Discord channel ID to delete
	 * @param reason Reason for deletion (required for audit log)
	 * @param cbInstance Optional callback instance
	 * @param cbFunction Optional callback function name
	 * @param ReturnString If true, returns raw JSON string in callback
	 * @return int Call ID, or -1 on error
	 */
	int ChannelDelete(string id, string reason, Class cbInstance = NULL, string cbFunction = "", bool ReturnString = false){
		if (id == "" || reason == ""){
			Error2("[UF] Error Deleting channel","Both Channel ID and reason must be valid strings");
			return -1;
		}
		if (!g_UFramework){
			UFLog.Err("[UF] g_UFramework is NULL - framework not ready");
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
		
		autoptr RestCallback rcb = RestCallback.Cast(g_UFramework.RegisterCall(DBCBX, cid));
		Post(url, obj.ToJson(), rcb);
		return cid;	
	}
	
	/**
	 * Edits an existing Discord channel's properties
	 * @param id The Discord channel ID to edit
	 * @param reason Reason for the edit (for audit log)
	 * @param options Update options (name, topic, position, permissions, etc.)
	 * @param cbInstance Optional callback instance
	 * @param cbFunction Optional callback function name
	 * @param ReturnString If true, returns raw JSON string in callback
	 * @return int Call ID, or -1 on error
	 */
	int ChannelEdit(string id, string reason, UChannelUpdateOptions options, Class cbInstance = NULL, string cbFunction = "", bool ReturnString = false){
		if (id == "" || reason == ""){
			Error2("[UF] Error Editing channel","Both Channel ID and reason must be valid strings");
			return -1;
		}
		if (!g_UFramework){
			UFLog.Err("[UF] g_UFramework is NULL - framework not ready");
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
			
		autoptr RestCallback rcb = RestCallback.Cast(g_UFramework.RegisterCall(DBCBX, cid));
		Post(url, obj.ToJson(), rcb);	
		return cid;		
	}
	
	/**
	 * Sends a text message to a Discord channel
	 * @param id The Discord channel ID
	 * @param message The message text to send
	 * @param cbInstance Optional callback instance
	 * @param cbFunction Optional callback function name
	 * @param ReturnString If true, returns raw JSON string in callback
	 * @return int Call ID, or -1 on error
	 */
	int ChannelSend(string id, string message, Class cbInstance = NULL, string cbFunction = "", bool ReturnString = false){
		if (id == "" || message == ""){
			Error2("[UF] Error Sending message to channel","Both Channel ID and message must be valid strings");
			return -1;
		}
		if (!g_UFramework){
			UFLog.Err("[UF] g_UFramework is NULL - framework not ready");
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
			
		autoptr RestCallback rcb = RestCallback.Cast(g_UFramework.RegisterCall(DBCBX, cid));
		Post(url, obj.ToJson(), rcb);		
		return cid;	
	}
	
	
	/**
	 * Sends an embedded message (rich content) to a Discord channel
	 * @param id The Discord channel ID
	 * @param message The embed object containing title, description, fields, colors, etc.
	 * @param cbInstance Optional callback instance
	 * @param cbFunction Optional callback function name
	 * @param ReturnString If true, returns raw JSON string in callback
	 * @return int Call ID, or -1 on error
	 */
	int ChannelSendEmbed(string id, UDiscordEmbed message, Class cbInstance = NULL, string cbFunction = "", bool ReturnString = false){
		if (id == "" || message == NULL){
			Error2("[UF] Error Sending Embed to channel","Both Channel ID and message must be valid");
			return -1;
		}
		if (!g_UFramework){
			UFLog.Err("[UF] g_UFramework is NULL - framework not ready");
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
			
		autoptr RestCallback rcb = RestCallback.Cast(g_UFramework.RegisterCall(DBCBX, cid));
		Post(url, message.ToJson(), rcb);	
		return cid;	
	}
	
	
	/**
	 * Retrieves messages from a Discord channel
	 * @param id The Discord channel ID
	 * @param cbInstance Callback instance (required)
	 * @param cbFunction Callback function name (required)
	 * @param filter Optional filter for message retrieval (limit, before, after, around)
	 * @param ReturnString If true, returns raw JSON string in callback
	 * @return int Call ID, or -1 on error
	 */
	int ChannelMessages(string id,  Class cbInstance, string cbFunction, UDiscordChannelFilter filter = NULL, bool ReturnString = false){
		if (id == ""){
			Error2("[UF] Error Getting messages from channel","Channel ID must be valid");
			return -1;
		}
		if (!g_UFramework){
			UFLog.Err("[UF] g_UFramework is NULL - framework not ready");
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
		
		autoptr RestCallback rcb = RestCallback.Cast(g_UFramework.RegisterCall(DBCBX, cid));
		Post(url, vFilter.ToJson(), rcb);	
		return cid;	
	}
	
	/**
	 * Checks if a player's Discord account is linked AND has a specific role
	 * Useful for server authentication before allowing connection
	 * @param PlainId Player's Steam ID (plain, not GUID)
	 * @param RoleId The Discord role ID to check for
	 * @param cbInstance Callback instance (required)
	 * @param cbFunction Callback function name (required)
	 * @param baseUrl Optional base URL override (uses config default if empty)
	 * @param ReturnString If true, returns raw JSON string in callback
	 * @return int Call ID
	 */
	int CheckRoleDiscord(string PlainId, string RoleId, Class cbInstance, string cbFunction,  string baseUrl = "", bool ReturnString = false){		
		if (!g_UFramework){
			UFLog.Err("[UF] g_UFramework is NULL - framework not ready");
			return -1;
		}
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
		
		autoptr RestCallback rcb = RestCallback.Cast(g_UFramework.RegisterCall(DBCBX, cid));
		g_UFramework.Post(url, "{}", rcb);
		
		return cid;
	}
	
	/**
	 * Checks if a player's Discord account is properly linked
	 * Useful for server authentication validation
	 * @param PlainId Player's Steam ID (plain, not GUID)
	 * @param cbInstance Callback instance (required)
	 * @param cbFunction Callback function name (required)
	 * @param baseUrl Optional base URL override (uses config default if empty)
	 * @param ReturnString If true, returns raw JSON string in callback
	 * @return int Call ID
	 */
	int CheckDiscord(string PlainId, Class cbInstance, string cbFunction,  string baseUrl = "", bool ReturnString = false){		
		if (!g_UFramework){
			UFLog.Err("[UF] g_UFramework is NULL - framework not ready");
			return -1;
		}
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
		
		autoptr RestCallback rcb = RestCallback.Cast(g_UFramework.RegisterCall(DBCBX, cid));
		g_UFramework.Post(url, "{}", rcb);
		
		return cid;
	}
	
	
	/**
	 * Downloads a user's Discord avatar image
	 * @param guid Player's GUID
	 * @param filename Base filename to save as (without extension, default: "discordme")
	 * @return int Call ID, or -1 on error
	 */
	int DownloadAvatar(string guid, string filename = "discordme"){
		if (guid == "" || filename == ""){
			Error2("[UF] DownloadAvatar", "guid or filename is null guid: " + guid + " filename: " + filename);
			return -1;
		}
		if (!g_UFramework){
			UFLog.Err("[UF] g_UFramework is NULL - framework not ready");
			return -1;
		}
		int cid = -1;		
		string url = UFConfig().GetBaseURL() + "Images/Discord/" + guid;
		
		autoptr UFRestCallBackBase ncb = new UFDLDiscordAvatarCallback(filename);
		autoptr RestCallback rcb = RestCallback.Cast(g_UFramework.RegisterCall(ncb, cid));
		g_UFramework.Post(url, "{}", rcb);
	
		return cid;
	}
}