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

