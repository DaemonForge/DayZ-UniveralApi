/**
 * UniversalDiscordRest Class
 * 
 * This class provides static methods for interacting with a Discord REST API.
 * It encapsulates functions for sending various API requests to a backend server,
 * which includes operations such as adding/removing roles to Discord users, handling
 * Discord channels (create, delete, edit, send messages and embeds), and retrieving 
 * Discord user information. Some operations are synchronous and blocking, so they 
 * should only be run in secondary threads.
 *
 * Methods:
 * -----------------------------------------------------------------------------
 *
 * protected static RestApi Api()
 *   - Retrieves the global RestApi instance. If the instance does not exist, it creates one,
 *     setting specific options (e.g., read operation timeout).
 *
 * static string Link(string PlainId = "")
 *   - Generates a URL link for Discord. When a PlainId is not provided and the game is in client mode,
 *     it appends the Steam ID from GetDayZGame() to the URL; otherwise, it uses the given PlainId.
 *
 * protected static void Post(string url, string jsonString = "{}", RestCallback UCBX = NULL)
 *   - Executes an asynchronous HTTP POST request to the specified URL with the given JSON string.
 *     It uses a callback (USilentCallBack by default) to handle response data.
 *
 * protected static void Get(string url, RestCallback UCBX = NULL)
 *   - Executes an asynchronous HTTP GET request to the specified URL, using a callback (USilentCallBack by default)
 *     to process the response.
 *
 * protected static string PostNow(string url, string jsonString = "{}")
 *   - Executes a synchronous (blocking) HTTP POST request to the specified URL with the provided JSON string.
 *     Returns the resulting response as a string.
 *
 * protected static string BaseUrl()
 *   - Retrieves the base URL for the server from the configuration (UFConfig().ServerURL).
 *
 * static void AddRole(string GUID, string RoleId, RestCallback UCBX = NULL)
 *   - Sends a request to add a specific Discord role (given by RoleId) to a user identified by GUID.
 *     It packages the role information into a JSON formatted string before sending via a POST request.
 *
 * static void RemoveRole(string GUID, string RoleId, RestCallback UCBX = NULL)
 *   - Sends a request to remove a specific Discord role (given by RoleId) from a user identified by GUID.
 *     Similar to AddRole, it creates a JSON payload and performs a POST request.
 *
 * static void GetUser(string GUID, RestCallback UCBX)
 *   - Retrieves Discord user data for the user identified by GUID by sending a POST request with an empty payload.
 *
 * static void GetUserWithPlainId(string plainId, RestCallback UCBX)
 *   - Retrieves Discord user data based on a plain identifier (plainId) using a POST request with an empty JSON payload.
 *
 * static void CheckDiscord(string PlainId, RestCallback UCBX, string baseUrl = "")
 *   - Checks the status or validity of a Discord user by their plain identifier.
 *     If no baseUrl is provided, it uses the default BaseUrl().
 *
 * static void ChannelCreate(string Name, RestCallback UCBX, UChannelOptions Options = NULL)
 *   - Creates a Discord channel with the specified name and optional channel creation options.
 *     It sends a POST request with the channel creation details in JSON format.
 *
 * static void ChannelDelete(string id, string reason, RestCallback UCBX = NULL)
 *   - Deletes a Discord channel identified by id. It uses the provided reason for deletion
 *     and sends this information as a JSON payload with a POST request.
 *
 * static void ChannelEdit(string id, string reason, UChannelUpdateOptions options, RestCallback UCBX = NULL)
 *   - Edits an existing Discord channel identified by id by applying modifications specified in options.
 *     A reason for the edit is included, and a POST request is sent with the updated channel details in JSON.
 *
 * static void ChannelSend(string id, string message, RestCallback UCBX = NULL)
 *   - Sends a plain text message to a Discord channel identified by id by packaging the message
 *     into a JSON payload and posting it.
 *
 * static void ChannelSendEmbed(string id, UDiscordEmbed message, RestCallback UCBX = NULL)
 *   - Sends an embed message to a Discord channel identified by id. The embed data is serialized to JSON
 *     and sent via a POST request.
 *
 * static void ChannelMessages(string id, RestCallback UCBX, UDiscordChannelFilter filter = NULL, string auth = "")
 *   - Retrieves messages from a Discord channel identified by id. 
 *     It sends a POST request including optional filtering options. If no filter is provided,
 *     an empty filter object is used.
 *
 * // Thread Blocking Functions (should be run in a secondary thread)
 *
 * static UDiscordUser GetUserNow(string GUID, bool ReturnError = false)
 *   - Synchronously retrieves Discord user data using a blocking POST request.
 *     Uses a JSON deserializer to parse the returned string into a UDiscordUser object.
 *     If deserialization fails or data is erroneous, returns an error object if ReturnError is true,
 *     otherwise returns NULL.
 *
 * static UDiscordUser GetUserWithPlainIdNow(string plainId, bool ReturnError = false)
 *   - Synchronously retrieves Discord user data by a plain identifier using a blocking POST request.
 *     Processes the returned JSON string to generate a UDiscordUser object.
 *     If errors occur during the JSON parsing, it optionally returns an error object based on ReturnError.
 */
class UniversalDiscordRest extends Managed {	

	protected static RestApi Api()
	{
		RestApi clCore = GetRestApi();
		if (!clCore)
		{
			clCore = CreateRestApi();
			clCore.SetOption(ERestOption.ERESTOPTION_READOPERATION, 15);
		}
		return clCore;
	}
	
	static string Link(string PlainId = ""){
		if (PlainId == "" && GetGame().IsClient()){
			return BaseUrl() + "Discord/" +  GetDayZGame().GetSteamId();
		}
		return BaseUrl() + "Discord/" + PlainId;
	}
	
	protected static void Post(string url, string jsonString = "{}", RestCallback UCBX = NULL)
	{
		autoptr RestCallback vUCBX = UCBX;
		if (!vUCBX){
			vUCBX = new USilentCallBack;
		}
		RestContext ctx =  Api().GetRestContext(url);
		ctx.SetHeader(U().GetAuthToken());
		ctx.POST(vUCBX , "", jsonString);
	}
	
	protected static void Get(string url, RestCallback UCBX = NULL)
	{
		autoptr RestCallback vUCBX = UCBX;
		if (!vUCBX){
			vUCBX = new USilentCallBack;
		}
		RestContext ctx =  Api().GetRestContext(url);
		ctx.GET(vUCBX , "");
	}
	
	protected static string PostNow(string url, string jsonString = "{}")
	{
		RestContext ctx =  Api().GetRestContext(url);
		ctx.SetHeader(U().GetAuthToken());
		return ctx.POST_now("", jsonString);
	}

	
	protected static string BaseUrl(){
		return UFConfig().ServerURL;
	}
	
	static void AddRole(string GUID, string RoleId, RestCallback UCBX = NULL) {
		
		autoptr RestCallback vUCBX = UCBX;
		if (!vUCBX){
			vUCBX = new USilentCallBack;
		}

		string url = BaseUrl() + "Discord/AddRole/" + GUID;
		
		autoptr UDiscordRoleReq roleReq = new UDiscordRoleReq(RoleId);
		
		string jsonString = roleReq.ToJson();
		if (jsonString){
			Post(url,jsonString,vUCBX);
		} else {
			Print("[UF] [Discord] Error Adding Role (" + RoleId + ") To " + GUID);
		}
	}
	
	static void RemoveRole(string GUID, string RoleId, RestCallback UCBX = NULL) {
		
		autoptr RestCallback vUCBX = UCBX;
		if (!vUCBX){
			vUCBX = new USilentCallBack;
		}
		
		string url = BaseUrl() + "Discord/RemoveRole/" + GUID;
		
		autoptr UDiscordRoleReq roleReq = new UDiscordRoleReq(RoleId);
		
		string jsonString = roleReq.ToJson();
		
		if (jsonString){
			Post(url,jsonString,vUCBX);
		} else {
			Print("[UF] [Discord] Error Removing Role (" + RoleId + ") To " + GUID);
		}
	}
	
	
	static void GetUser(string GUID, RestCallback UCBX) {
		string url = BaseUrl() + "Discord/Get/" + GUID;
		
		Post(url,"{}",UCBX);
	}
	
	static void GetUserWithPlainId(string plainId, RestCallback UCBX) {
		string url = BaseUrl() + "Discord/GetWithPlainId/" + plainId;
		
		Post(url,"{}",UCBX);
	}
	
	
	static void CheckDiscord(string PlainId, RestCallback UCBX,  string baseUrl = ""){		
		if (baseUrl == ""){
			baseUrl = BaseUrl();
		}
		string url = baseUrl + "Discord/Check/" + PlainId;
		
		Post(url,"{}",UCBX);
	}
	
	
	
	
	static void ChannelCreate(string Name, RestCallback UCBX, UChannelOptions Options = NULL) {
		
		UCreateChannelObject obj = new UCreateChannelObject(Name, UChannelCreateOptions.Cast(Options));
		
		if (obj){
			string url = BaseUrl() + "Discord/Channel/Create";
			
			Post(url,obj.ToJson(),UCBX);		
		}
	}
	
	static void ChannelDelete(string id, string reason,  RestCallback UCBX = NULL){

		autoptr RestCallback vUCBX = UCBX;
		if (!vUCBX){
			vUCBX = new USilentCallBack;
		}
		
		UUpdateChannelObject obj = new UUpdateChannelObject(reason, NULL);
		
		if (obj){
			string url = BaseUrl() + "Discord/Channel/Delete/" + id;
			
			Post(url,obj.ToJson(),vUCBX);		
		}
	}
	
	
	static void ChannelEdit(string id, string reason, UChannelUpdateOptions options, RestCallback UCBX = NULL){

		autoptr RestCallback vUCBX = UCBX;
		if (!vUCBX){
			vUCBX = new USilentCallBack;
		}
		
		UUpdateChannelObject obj = new UUpdateChannelObject(reason, UChannelUpdateOptions.Cast(options));
		
		if (obj){
			string url = BaseUrl() + "Discord/Channel/Edit/" + id;
			
			Post(url,obj.ToJson(),vUCBX);		
		}
	}
	
	
	static void ChannelSend(string id, string message, RestCallback UCBX = NULL){

		autoptr RestCallback vUCBX = UCBX;
		if (!vUCBX){
			vUCBX = new USilentCallBack;
		}
		
		UDiscordBasicMessage obj = new UDiscordBasicMessage(message);
		
		if (obj){
			string url = BaseUrl() + "Discord/Channel/Send/" + id;
			
			Post(url,obj.ToJson(),vUCBX);		
		}
	}
	
	
	static void ChannelSendEmbed(string id, UDiscordEmbed message, RestCallback UCBX = NULL){

		autoptr RestCallback vUCBX = UCBX;
		if (!vUCBX){
			vUCBX = new USilentCallBack;
		}
				
		if (message){
			string url = BaseUrl() + "Discord/Channel/Send/" + id;
			
			Post(url,message.ToJson(),vUCBX);		
		}
	}
	
	
	
	
	static void ChannelMessages(string id,  RestCallback UCBX, UDiscordChannelFilter filter = NULL,  string auth = ""){
		
		autoptr UDiscordChannelFilter vFilter = filter;
		if (!vFilter){
			vFilter = new UDiscordChannelFilter();
		}
		
		string url = BaseUrl() + "Discord/Channel/Messages/" + id;
		
		if (vFilter && UCBX){
			Post(url,vFilter.ToJson(),UCBX);	
		} else if (UCBX) {
			Post(url, "{}",UCBX);
		}
	}
	
	// !!!!!WARNING!!!!! 
	
	// ALL OF THE FOLLOWING FUCNTIONS ARE THREAD BLOCKING ONLY RUN in Secondary Thread!
	
	
	
	
	
	
	
	
	
	
	// !!!!!WARNING!!!!!
	// THE FOLLOWING FUCNTION IS THREAD BLOCKING ONLY RUN in secondary Thread!
	static UDiscordUser GetUserNow(string GUID, bool ReturnError = false) {

		string url = BaseUrl() + "Discord/Get/" + GUID;
		
		string Result = PostNow(url,"{}");
		
		
		JsonSerializer js = new JsonSerializer();
		string error;
		
		UDiscordUser user;
		
		js.ReadFromString(user, Result, error);
		
		if (error != ""){
			Print("[UF] [GetUserNow] Error: " + error);
		}
		if (user && (user.Status == "Success" || ReturnError)){
			return user;
		} else if (!user && ReturnError){
			user = new UDiscordUser;
			user.Status = "Error";
			user.Error = "Error Fetching Data";
			return user;
		}
		
		return NULL;
		
	}
	
	
	// !!!!!WARNING!!!!!
	// THE FOLLOWING FUCNTION IS THREAD BLOCKING ONLY RUN in secondary Thread!
	static UDiscordUser GetUserWithPlainIdNow(string plainId, bool ReturnError = false) {

		string url = BaseUrl() + "Discord/GetWithPlainId/" + plainId;
		
		string Result = PostNow(url,"{}");
		Print(Result);
		JsonSerializer js = new JsonSerializer();
		string error;
		
		UDiscordUser user;
		
		Print("[UF] [GetUserWithPlainIdNow] Read from Sting");
		js.ReadFromString(user, Result, error);
		
		if (error != ""){
			Print("[UF] [GetUserWithPlainIdNow] Error: " + error);
		}
		if (user && (user.Status == "Success" || ReturnError)){
			Print("[UF] [GetUserWithPlainIdNow] Returning User");
			return user;
		} else if (!user && ReturnError){
			Print("[UF] [GetUserWithPlainIdNow] Returning Error");
			user = new UDiscordUser;
			user.Status = "Error";
			user.Error = "Error Fetching Data";
			return user;
		}
		Print("[UF] [GetUserWithPlainIdNow] Returning NULL");
		
		return NULL;		
	}	
	
}

