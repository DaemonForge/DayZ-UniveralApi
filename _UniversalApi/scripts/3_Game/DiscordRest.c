class UniversalDiscordRest
{	
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
	
	
	static void Post(string url, string jsonString = "{}", RestCallback UCBX = NULL)
	{
		if (!UCBX){
			UCBX = new UApiSilentCallBack;
		}
		RestContext ctx =  Api().GetRestContext(url);
		ctx.SetHeader("application/json");
		ctx.POST(UCBX , "", jsonString);
	}
	
	static void Get(string url, RestCallback UCBX = NULL)
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
	
	
	// !!!!!WARNING!!!!! 
	
	// ALL OF THE FOLLOWING FUCNTIONS ARE THREAD BLOCKING ONLY RUN in secondary Thread!
	
	
	
	
	
	
	
	
	
	
	// !!!!!WARNING!!!!!
	// THE FOLLOWING FUCNTION IS THREAD BLOCKING ONLY RUN in secondary Thread!
	static ref UApiDiscordUser GetUserNow(string GUID, bool ReturnError = false, string auth = "") {
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
	static ref UApiDiscordUser GetUserWithPlainIdNow(string plainId, bool ReturnError = false, string auth = "") {
		if (auth == "" ){
			auth = UApi().GetAuthToken();
		}
		
		string url = BaseUrl() + "Discord/GetWithPlainId/" + plainId + "/" + auth;
		
		string Result = PostNow(url,"{}");
		Print(Result);
		JsonSerializer js = new JsonSerializer();
		string error;
		
		ref UApiDiscordUser user;
		
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

