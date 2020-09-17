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
	
	static void PlayerSave(string mod, string guid, string auth, ref ConfigBase data_out, ref RestCallback UCBX = NULL)
	{		
		string url = UApiConfig().ServerURL + "/Player/Save/" + guid + "/" + auth + "/" + mod;
		
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
			Print("[UPAI] [GameApi] Error Saving Player Data for " + mod);
		}
	}
	
	static void PlayerLoad(string mod, string guid, string auth,ref RestCallback UCBX, ref ConfigBase data_out = NULL)
	{
		string url = UApiConfig().ServerURL + "/Player/Load/" + guid + "/" + auth + "/" + mod;
		
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
			Print("[UPAI] [GameApi] Error Loading Player Data for " + mod);
		}
	}
	
	static void GetAuth( string auth, string guid )
	{
		string url = UApiConfig().ServerURL + "/Player/GetAuth/" + guid + "/" + auth;
		
		UApiAuthCallBack UCBX = new UApiAuthCallBack;
		
		RestContext ctx = Api().GetRestContext(url);
		ctx.SetHeader("application/json");
		ctx.POST(UCBX, "", "{}");
	}


	static void GlobalsSave(string mod, string auth, ref ConfigBase data_out, ref RestCallback UCBX = NULL)
	{
		string url = UApiConfig().ServerURL + "/Gobals/Save/" + auth + "/" + mod;
		
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
			Print("[UPAI] [GameApi] Error Saving Globals Data for " + mod);
		}
	}
	
	static void GlobalsLoad(string mod, string auth, ref RestCallback UCBX, ref ConfigBase data_out = NULL)
	{
		string url = UApiConfig().ServerURL + "/Gobals/Load/" + auth + "/" + mod;
		
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
			Print("[UPAI] [GameApi] Error Loading Globals Data for " + mod);
		}
	}
	
	static void ItemSave(string mod, string serverId, string auth, ref ConfigBase data_out, ref RestCallback UCBX = NULL)
	{
		string url = UApiConfig().ServerURL + "/Item/Save/" + serverId + "/" + auth + "/" + mod;
		
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
			Print("[UPAI] [GameApi] Error Saving Globals Data for " + mod);
		}
	}
	
	static void ItemLoad(string mod, string serverId, string auth, ref RestCallback UCBX, ref ConfigBase data_out = NULL)
	{
		string url = UApiConfig().ServerURL + "/Item/Load/" + serverId + "/" + auth + "/" + mod;
		
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
			Print("[UPAI] [GameApi] Error Loading Globals Data for " + mod);
		}
	}
};