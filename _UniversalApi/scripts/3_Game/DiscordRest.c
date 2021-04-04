class UniversalDiscordRest extends Managed {	
	static protected string m_BaseUrl = "";
	
	static void SetBaseUrl(string new_BaseUrl){
		m_BaseUrl = new_BaseUrl;
	}
	
	static RestApi Api()
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
	
	static void Post(string url, string jsonString = "{}", ref RestCallback UCBX = NULL)
	{
		if (!UCBX){
			UCBX = new UApiSilentCallBack;
		}
		RestContext ctx =  Api().GetRestContext(url);
		ctx.SetHeader("application/json");
		ctx.POST(UCBX , "", jsonString);
	}
	
	static void Get(string url, ref RestCallback UCBX = NULL)
	{
		if (!UCBX){
			UCBX = new UApiSilentCallBack;
		}
		RestContext ctx =  Api().GetRestContext(url);
		ctx.GET(UCBX , "");
	}
	
	static string PostNow(string url, string jsonString = "{}")
	{
		RestContext ctx =  Api().GetRestContext(url);
		ctx.SetHeader("application/json");
		return ctx.POST_now("", jsonString);
	}

	
	static string BaseUrl(){
		if (m_BaseUrl != ""){
			return m_BaseUrl;
		}
		return UApiConfig().ServerURL;
	}
	
	static void AddRole(string GUID, string RoleId, ref RestCallback UCBX = NULL, string auth = "") {
		
		if (!UCBX){
			UCBX = new UApiSilentCallBack;
		}
		
		if (auth == "" ){
			auth = UApi().GetAuthToken();
		}
		
		string url = BaseUrl() + "Discord/AddRole/" + GUID + "/" + auth;
		
		autoptr UApiDiscordRoleReq roleReq = new UApiDiscordRoleReq(RoleId);
		
		string jsonString = roleReq.ToJson();
		if (jsonString){
			Post(url,jsonString,UCBX);
		} else {
			Print("[UPAI] [Discord] Error Adding Role (" + RoleId + ") To " + GUID);
		}
	}
	
	static void RemoveRole(string GUID, string RoleId, ref RestCallback UCBX = NULL, string auth = "") {
		
		if (!UCBX){
			UCBX = new UApiSilentCallBack;
		}
		
		if (auth == "" ){
			auth = UApi().GetAuthToken();
		}
		
		string url = BaseUrl() + "Discord/RemoveRole/" + GUID + "/" + auth;
		
		autoptr UApiDiscordRoleReq roleReq = new UApiDiscordRoleReq(RoleId);
		
		string jsonString = roleReq.ToJson();
		
		if (jsonString){
			Post(url,jsonString,UCBX);
		} else {
			Print("[UPAI] [Discord] Error Removing Role (" + RoleId + ") To " + GUID);
		}
	}
	
	
	static void GetUser(string GUID, ref RestCallback UCBX, string auth = "") {
		if (auth == "" ){
			auth = UApi().GetAuthToken();
		}
		
		string url = BaseUrl() + "Discord/Get/" + GUID + "/" + auth;
		
		Post(url,"{}",UCBX);
	}
	
	static void GetUserWithPlainId(string plainId, ref RestCallback UCBX, string auth = "") {
		if (auth == "" ){
			auth = UApi().GetAuthToken();
		}
		
		string url = BaseUrl() + "Discord/GetWithPlainId/" + plainId + "/" + auth;
		
		Post(url,"{}",UCBX);
	}
	
	
	static void CheckDiscord(string PlainId, ref RestCallback UCBX,  string baseUrl = ""){		
		if (baseUrl == ""){
			baseUrl = BaseUrl();
		}
		string url = baseUrl + "Discord/Check/" + PlainId;
		
		Post(url,"{}",UCBX);
	}
	
	
	
	
	static void ChannelCreate(string Name, ref RestCallback UCBX, UApiChannelOptions Options = NULL, string auth = "") {
		
		if (auth == "" ) {
			auth = UApi().GetAuthToken();
		}
		
		UApiCreateChannelObject obj = new UApiCreateChannelObject(Name, Options);
		
		if (obj){
			string url = BaseUrl() + "Discord/Channel/Create/" + auth;
			
			Post(url,obj.ToJson(),UCBX);		
		}
	}
	
	static void ChannelDelete(string id, string reason,  ref RestCallback UCBX = NULL, string auth = ""){
		if (auth == "" ){
			auth = UApi().GetAuthToken();
		}
		
		if (!UCBX){
			UCBX = new UApiSilentCallBack;
		}
		
		UApiUpdateChannelObject obj = new UApiUpdateChannelObject(reason, NULL);
		
		if (obj){
			string url = BaseUrl() + "Discord/Channel/Delete/" + id + "/" + auth;
			
			Post(url,obj.ToJson(),UCBX);		
		}
	}
	
	
	static void ChannelEdit(string id, string reason, ref UApiChannelUpdateOptions options, ref RestCallback UCBX = NULL, string auth = ""){
		if (auth == "" ){
			auth = UApi().GetAuthToken();
		}
		
		if (!UCBX){
			UCBX = new UApiSilentCallBack;
		}
		
		UApiUpdateChannelObject obj = new UApiUpdateChannelObject(reason, UApiChannelUpdateOptions.Cast(options));
		
		if (obj){
			string url = BaseUrl() + "Discord/Channel/Edit/" + id + "/" + auth;
			
			Post(url,obj.ToJson(),UCBX);		
		}
	}
	
	
	static void ChannelSend(string id, string message, ref RestCallback UCBX = NULL, string auth = ""){
		if (auth == "" ){
			auth = UApi().GetAuthToken();
		}
		
		if (!UCBX){
			UCBX = new UApiSilentCallBack;
		}
		
		UApiDiscordBasicMessage obj = new UApiDiscordBasicMessage(message);
		
		if (obj){
			string url = BaseUrl() + "Discord/Channel/Send/" + id + "/" + auth;
			
			Post(url,obj.ToJson(),UCBX);		
		}
	}
	
	
	static void ChannelSendEmbed(string id, UApiDiscordEmbed message, ref RestCallback UCBX = NULL, string auth = ""){
		if (auth == "" ){
			auth = UApi().GetAuthToken();
		}
		
		if (!UCBX){
			UCBX = new UApiSilentCallBack;
		}
				
		if (message){
			string url = BaseUrl() + "Discord/Channel/Send/" + id + "/" + auth;
			
			Post(url,message.ToJson(),UCBX);		
		}
	}
	
	// !!!!!WARNING!!!!! 
	
	// ALL OF THE FOLLOWING FUCNTIONS ARE THREAD BLOCKING ONLY RUN in Secondary Thread!
	
	
	
	
	
	
	
	
	
	
	// !!!!!WARNING!!!!!
	// THE FOLLOWING FUCNTION IS THREAD BLOCKING ONLY RUN in secondary Thread!
	static UApiDiscordUser GetUserNow(string GUID, bool ReturnError = false, string auth = "") {
		if (auth == "" ){
			auth = UApi().GetAuthToken();
		}
		
		string url = BaseUrl() + "Discord/Get/" + GUID + "/" + auth;
		
		string Result = PostNow(url,"{}");
		
		
		JsonSerializer js = new JsonSerializer();
		string error;
		
		UApiDiscordUser user;
		
		js.ReadFromString(user, Result, error);
		
		if (error != ""){
			Print("[UPAI] [GetUserNow] Error: " + error);
		}
		if (user && (user.Status == "Success" || ReturnError)){
			return user;
		} else if (!user && ReturnError){
			user = new UApiDiscordUser;
			user.Status = "Error";
			user.Error = "Error Fetching Data";
			return user;
		}
		
		return NULL;
		
	}
	
	
	// !!!!!WARNING!!!!!
	// THE FOLLOWING FUCNTION IS THREAD BLOCKING ONLY RUN in secondary Thread!
	static UApiDiscordUser GetUserWithPlainIdNow(string plainId, bool ReturnError = false, string auth = "") {
		if (auth == "" ){
			auth = UApi().GetAuthToken();
		}
		
		string url = BaseUrl() + "Discord/GetWithPlainId/" + plainId + "/" + auth;
		
		string Result = PostNow(url,"{}");
		Print(Result);
		JsonSerializer js = new JsonSerializer();
		string error;
		
		UApiDiscordUser user;
		
		Print("[UPAI] [GetUserWithPlainIdNow] Read from Sting");
		js.ReadFromString(user, Result, error);
		
		if (error != ""){
			Print("[UPAI] [GetUserWithPlainIdNow] Error: " + error);
		}
		if (user && (user.Status == "Success" || ReturnError)){
			Print("[UPAI] [GetUserWithPlainIdNow] Returning User");
			return user;
		} else if (!user && ReturnError){
			Print("[UPAI] [GetUserWithPlainIdNow] Returning Error");
			user = new UApiDiscordUser;
			user.Status = "Error";
			user.Error = "Error Fetching Data";
			return user;
		}
		Print("[UPAI] [GetUserWithPlainIdNow] Returning NULL");
		
		return NULL;		
	}
	
	
	
	
	
}

