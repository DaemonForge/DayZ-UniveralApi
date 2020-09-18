class UniversalRest
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
	
	static string BaseUrl(){
		if (m_BaseUrl != ""){
			return m_BaseUrl;
		}
		return UApiConfig().ServerURL;
	}
	
	static void PlayerSave(string mod, string guid, string jsonString, ref RestCallback UCBX = NULL, string auth = "")
	{		
		if (auth == "" ){
			auth = UApi().GetAuthToken();
		}
		
		string url = BaseUrl() + "/Player/Save/" + guid + "/" + mod  + "/" + auth;
		if (!UCBX){
			UCBX = new ref UApiSilentCallBack;
		}
		
		if (jsonString){
			RestContext ctx = Api().GetRestContext(url);
			ctx.SetHeader("application/json");
			ctx.POST(UCBX, "", jsonString);
		} else {
			Print("[UPAI] [Api] Error Saving Player Data for " + mod);
		}
	}
	
	static void PlayerLoad(string mod, string guid,  ref RestCallback UCBX, string jsonString = "{}",string auth = "")
	{
		
		if (auth == "" ){
			auth = UApi().GetAuthToken();
		}
		
		string url = BaseUrl() + "/Player/Load/" + guid + "/" + mod  + "/" + auth;
		
		if (UCBX){
			RestContext ctx = Api().GetRestContext(url);
			ctx.SetHeader("application/json");
			ctx.POST(UCBX, "", jsonString);
		} else {
			Print("[UPAI] [Api] Error Loading Player Data for " + mod);
		}
	}
	
	static void GetAuth( string guid,  string auth  = "")
	{
		
		if (auth == "" ){
			auth = UApi().GetAuthToken();
		}
		string url = BaseUrl() + "/GetAuth/" + guid + "/" + auth;
		
		UApiAuthCallBack UCBX = new UApiAuthCallBack;
		
		RestContext ctx = Api().GetRestContext(url);
		ctx.SetHeader("application/json");
		ctx.POST(UCBX, "", "{}");
	}


	static void GlobalsSave(string mod, string jsonString, ref RestCallback UCBX = NULL, string auth = "")
	{
		
		if (auth == "" ){
			auth = UApi().GetAuthToken();
		}
		string url = BaseUrl() + "/Gobals/Save/" + mod  + "/" + auth;
		
		if (!UCBX){
			UCBX = new ref UApiSilentCallBack;
		}
		
		if (jsonString){
			RestContext ctx = Api().GetRestContext(url);
			ctx.SetHeader("application/json");
			ctx.POST(UCBX, "", jsonString);
		} else {
			Print("[UPAI] [Api] Error Saving Globals Data for " + mod);
		}
	}
	
	static void GlobalsLoad(string mod, ref RestCallback UCBX, string jsonString = "{}", string auth = ""){
		
		if (auth == "" ){
			auth = UApi().GetAuthToken();
		}
		string url = BaseUrl() + "/Gobals/Load/" + mod  + "/" + auth;

		if (UCBX){
			RestContext ctx = Api().GetRestContext(url);
			ctx.SetHeader("application/json");
			ctx.POST(UCBX, "", jsonString);
		} else {
			Print("[UPAI] [Api] Error Loading Globals Data for " + mod);
		}
	}
	
	static void ItemSave(string mod, string itemId, string jsonString, ref RestCallback UCBX = NULL, string auth = ""){
		
		if (!UCBX){
			UCBX = new ref UApiSilentCallBack;
		}
		
		if (auth == "" ){
			auth = UApi().GetAuthToken();
		}
		string url = BaseUrl() + "/Item/Save/" + itemId + "/" +  mod + "/" + auth;
		
		if (jsonString){
			RestContext ctx = Api().GetRestContext(url);
			ctx.SetHeader("application/json");
			ctx.POST(UCBX, "", jsonString);
		} else {
			Print("[UPAI] [Api] Error Saving Globals Data for " + mod);
		}
	}
	
	static void ItemLoad(string mod, string itemId, ref RestCallback UCBX, string jsonString = "{}", string auth = ""){
		
		if (auth == "" ){
			auth = UApi().GetAuthToken();
		}
		
		string url = BaseUrl() + "/Item/Load/" +  itemId + "/" + mod  + "/" + auth;
		
		if (UCBX){
			RestContext ctx = Api().GetRestContext(url);
			ctx.SetHeader("application/json");
			ctx.POST(UCBX, "", jsonString);
		} else {
			Print("[UPAI] [Api] Error Loading Globals Data for " + mod);
		}
	}
};