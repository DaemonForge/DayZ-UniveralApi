class UniversalRest
{	
	static RestApi Api()
	{
		RestApi clCore = GetRestApi();
		if (!clCore)
		{
			clCore = CreateRestApi();
		}
		return clCore;
	}
	
	static void PlayerSave(string mod, string guid, ref ConfigBase data_out, ref RestCallback UCBX = NULL, string auth = "")
	{		
		string url = UApiConfig().ServerURL + "/Player/Save/" + guid + "/" + mod  + "/" + auth;
		
		if (auth == "" ){
			auth = UApi().GetAuthToken();
		}
		
		if (!UCBX){
			UCBX = new ref UApiSilentCallBack;
		}
		
		string jsonString = "{}";
		bool ok = false;
		if (data_out){
			JsonSerializer js = new JsonSerializer();
			ok = js.WriteToString(data_out, false, jsonString);
		}
		if (ok){
			RestContext ctx = Api().GetRestContext(url);
			ctx.SetHeader("application/json");
			ctx.POST(UCBX, "", jsonString);
		} else {
			Print("[UPAI] [Api] Error Saving Player Data for " + mod);
		}
	}
	
	static void PlayerLoad(string mod, string guid, ref ConfigBase data_out = NULL, ref RestCallback UCBX = NULL, string auth = "")
	{
		string url = UApiConfig().ServerURL + "/Player/Load/" + guid + "/" + mod  + "/" + auth;
		
		if (auth == "" ){
			auth = UApi().GetAuthToken();
		}
		
		string jsonString = "{}";
		bool ok = true;
		
		if (!data_out && !UCBX){
			UCBX = new ref RestCallback;
		}
		
		if (data_out){
			if (!UCBX){
				ref UApiCallBack UAPICBX = new ref UApiCallBack;
				UAPICBX.SetConfig(data_out);
				UCBX = Class.Cast(UAPICBX);
			}
			JsonSerializer js = new JsonSerializer();
			ok = js.WriteToString(data_out, false, jsonString);
		}
		
		if (ok){
			RestContext ctx = Api().GetRestContext(url);
			ctx.SetHeader("application/json");
			ctx.POST(UCBX, "", jsonString);
		} else {
			Print("[UPAI] [Api] Error Loading Player Data for " + mod);
		}
	}
	
	static void GetAuth( string guid,  string auth  = "")
	{
		string url = UApiConfig().ServerURL + "/Player/GetAuth/" + guid + "/" + auth;
		
		if (auth == "" ){
			auth = UApi().GetAuthToken();
		}
		
		UApiAuthCallBack UCBX = new UApiAuthCallBack;
		
		RestContext ctx = Api().GetRestContext(url);
		ctx.SetHeader("application/json");
		ctx.POST(UCBX, "", "{}");
	}


	static void GlobalsSave(string mod, ref ConfigBase data_out, ref RestCallback UCBX = NULL, string auth = "")
	{
		string url = UApiConfig().ServerURL + "/Gobals/Save/" + mod  + "/" + auth;
		
		if (auth == "" ){
			auth = UApi().GetAuthToken();
		}
		
		if (!UCBX){
			UCBX = new ref UApiSilentCallBack;
		}
		
		string jsonString = "{}";
		bool ok = false;
		if (data_out){
			JsonSerializer js = new JsonSerializer();
			ok = js.WriteToString(data_out, false, jsonString);
		}
		if (ok){
			RestContext ctx = Api().GetRestContext(url);
			ctx.SetHeader("application/json");
			ctx.POST(UCBX, "", jsonString);
		} else {
			Print("[UPAI] [Api] Error Saving Globals Data for " + mod);
		}
	}
	
	static void GlobalsLoad(string mod, ref RestCallback UCBX = NULL, ref ConfigBase data_out = NULL, string auth = "")
	{
		string url = UApiConfig().ServerURL + "/Gobals/Load/" + mod  + "/" + auth;
		
		if (auth == "" ){
			auth = UApi().GetAuthToken();
		}
		
		string jsonString = "{}";
		bool ok = true;
		if (data_out){
			JsonSerializer js = new JsonSerializer();
			ok = js.WriteToString(data_out, false, jsonString);
		}
		
		if (ok){
			RestContext ctx = Api().GetRestContext(url);
			ctx.SetHeader("application/json");
			ctx.POST(UCBX, "", jsonString);
		} else {
			Print("[UPAI] [Api] Error Loading Globals Data for " + mod);
		}
	}
	
	static void ItemSave(string mod, string itemId, ref ConfigBase data_out, ref RestCallback UCBX = NULL, string auth = "")
	{
		string url = UApiConfig().ServerURL + "/Item/Save/" + itemId + "/" +  mod + "/" + auth;
		
		if (!UCBX){
			UCBX = new ref UApiSilentCallBack;
		}
		
		if (auth == "" ){
			auth = UApi().GetAuthToken();
		}
		
		string jsonString = "{}";
		bool ok = false;
		if (data_out){
			JsonSerializer js = new JsonSerializer();
			ok = js.WriteToString(data_out, false, jsonString);
		}
		if (ok){
			RestContext ctx = Api().GetRestContext(url);
			ctx.SetHeader("application/json");
			ctx.POST(UCBX, "", jsonString);
		} else {
			Print("[UPAI] [Api] Error Saving Globals Data for " + mod);
		}
	}
	
	static void ItemLoad(string mod, string itemId, ref RestCallback UCBX = NULL, ref ConfigBase data_out = NULL, string auth = "")
	{
		string url = UApiConfig().ServerURL + "/Item/Load/" +  itemId + "/" + mod  + "/" + auth;
		
		if (auth == "" ){
			auth = UApi().GetAuthToken();
		}
		
		
		string jsonString = "{}";
		bool ok = true;
		if (data_out){
			JsonSerializer js = new JsonSerializer();
			ok = js.WriteToString(data_out, false, jsonString);
		}
		
		if (ok){
			RestContext ctx = Api().GetRestContext(url);
			ctx.SetHeader("application/json");
			ctx.POST(UCBX, "", jsonString);
		} else {
			Print("[UPAI] [Api] Error Loading Globals Data for " + mod);
		}
	}
};