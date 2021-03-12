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
	
	
	static void Post(string url, string jsonString = "{}", ref RestCallback UCBX = NULL)
	{
		if (!UCBX){
			UCBX = new ref UApiSilentCallBack;
		}
		RestContext ctx =  Api().GetRestContext(url);
		ctx.SetHeader("application/json");
		ctx.POST(UCBX , "", jsonString);
	}
	
	static void Get(string url, ref RestCallback UCBX = NULL)
	{
		if (!UCBX){
			UCBX = new ref UApiSilentCallBack;
		}
		RestContext ctx =  Api().GetRestContext(url);
		ctx.GET(UCBX , "");
	}
	
	
	static string BaseUrl(){
		if (m_BaseUrl != ""){
			return m_BaseUrl;
		}
		return UApiConfig().ServerURL;
	}
	
	static void AddRole(string GUID, string RoleId, ref RestCallback UCBX = NULL, string auth = "") {
		
		if (!UCBX){
			UCBX = new ref UApiSilentCallBack;
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
			UCBX = new ref UApiSilentCallBack;
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
	
	
	static void GetRoles(string GUID, ref RestCallback UCBX, string auth = "") {
		if (auth == "" ){
			auth = UApi().GetAuthToken();
		}
		
		string url = BaseUrl() + "Discord/GetRoles/" + GUID + "/" + auth;
		

		Post(url,"{}",UCBX);
		
	}
	
	
}